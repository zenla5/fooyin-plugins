/*
 * fooyin-plugins - Key Analyzer plugin
 * Copyright © 2026
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#include "keyanalyzerworker.h"

#include "keyanalyzerdefs.h"
#include "keyanalyzernotations.h"

#include <core/engine/audiobuffer.h>
#include <core/engine/audioconverter.h>
#include <core/engine/audioformat.h>
#include <core/engine/audioloader.h>
#include <core/engine/audioinput.h>

#include <keyfinder/audiodata.h>
#include <keyfinder/keyfinder.h>

#include <QObject>
#include <QStringList>

#include <vector>

using namespace Qt::StringLiterals;

namespace Fooyin::KeyAnalyzer {

namespace {

constexpr int TargetSampleRate = 44100;

// Linear-interpolation resampler. libkeyfinder's spectral analysis is tuned for
// 44.1 kHz (detections shift at other rates), so files at any other sample
// rate must be converted to TargetSampleRate before analysis. Audio::convert
// does NOT resample, hence this small self-contained stage.
void resampleToTarget(std::vector<float>& mono, int streamRate)
{
    if(mono.empty() || streamRate <= 0 || streamRate == TargetSampleRate)
        return;

    const double step  = static_cast<double>(streamRate) / TargetSampleRate;
    const auto   nIn   = mono.size();
    const auto   nOut  = static_cast<std::size_t>(std::ceil(static_cast<double>(nIn) * TargetSampleRate / streamRate));

    std::vector<float> out;
    out.reserve(nOut);
    for(std::size_t i = 0; i < nOut; ++i) {
        const double pos  = static_cast<double>(i) * step;
        const auto   i0   = std::min<std::size_t>(static_cast<std::size_t>(pos), nIn - 1);
        const auto   i1   = std::min<std::size_t>(i0 + 1, nIn - 1);
        const double frac = pos - static_cast<double>(i0);
        out.push_back(static_cast<float>(mono[i0] * (1.0 - frac) + mono[i1] * frac));
    }
    mono = std::move(out);
}

} // namespace

// ---------------------------------------------------------------------------
// KeyAnalyzerWorker
// ---------------------------------------------------------------------------

KeyAnalyzerWorker::KeyAnalyzerWorker(std::shared_ptr<AudioLoader> audioLoader)
    : m_audioLoader{std::move(audioLoader)}
{ }

KeyResult KeyAnalyzerWorker::computeKey(const Track& track,
                                        const QAtomicInt& cancelled,
                                        const AnalysisOptions& options) const
{
    // ---- Build result stub ----
    KeyResult result;
    result.track = track;

    const QStringList existingKey = track.extraTag(u"INITIALKEY"_s);
    if(!existingKey.isEmpty())
        result.storedKey = existingKey.first();

    const QStringList comments = track.extraTag(u"COMMENT"_s);
    if(!comments.isEmpty())
        result.existingComment = comments.first();

    if(options.skipExisting && !options.force && !result.storedKey.isEmpty()) {
        result.status = KeyResult::Status::Skipped;
        return result;
    }

    // ---- Open decoder ----
    const auto loaded = m_audioLoader->loadDecoderForTrack(
        track,
        AudioDecoder::NoSeeking | AudioDecoder::NoInfiniteLooping);

    if(!loaded.decoder) {
        result.status      = KeyResult::Status::Error;
        result.errorString = QObject::tr("No decoder available");
        return result;
    }

    // ---- Decode and convert to mono float PCM ----
    //
    // Note: we deliberately do NOT rely on fooyin's Audio::convert to reduce
    // the channel count. For a source with a channel layout, `setChannelCount(1)`
    // gives the mono target a FrontCenter layout position that a stereo source
    // lacks, so the converter's channel map resolves to -1 and every output
    // sample stays zero (silence). Instead we convert to F32 *keeping the source
    // channel count* (same layout -> identity map, like fooyin's own ebur128
    // scanner) and downmix to mono ourselves.
    //
    // Audio::convert also does NOT resample, so we feed libkeyfinder the actual
    // stream sample rate rather than assuming 44.1 kHz.
    loaded.decoder->start();

    std::vector<float> mono;
    int                streamRate = 0;

    constexpr size_t ChunkBytes = 65536;
    while(!cancelled.loadRelaxed()) {
        AudioBuffer buf = loaded.decoder->readBuffer(ChunkBytes);
        if(!buf.isValid() || buf.byteCount() == 0)
            break;

        const AudioFormat sourceFmt = buf.format();
        const int         channels  = sourceFmt.channelCount();
        const int         rate      = sourceFmt.sampleRate();
        if(streamRate == 0)
            streamRate = rate;
        if(channels <= 0)
            continue;

        // Convert sample format to F32, preserving channels & rate & layout.
        AudioFormat target = sourceFmt;
        target.setSampleFormat(SampleFormat::F32);

        const AudioBuffer converted = Audio::convert(buf, target);
        if(!converted.isValid())
            continue;

        const auto        span = converted.constData();
        const float*      samples = reinterpret_cast<const float*>(span.data());
        const std::size_t nFloats = span.size_bytes() / sizeof(float);
        const std::size_t frames  = nFloats / static_cast<std::size_t>(channels);

        const std::size_t base = mono.size();
        mono.resize(base + frames);
        for(std::size_t f = 0; f < frames; ++f) {
            double acc = 0.0;
            for(int c = 0; c < channels; ++c)
                acc += samples[f * static_cast<std::size_t>(channels) + static_cast<std::size_t>(c)];
            mono[base + f] = static_cast<float>(acc / channels);
        }
    }

    loaded.decoder->stop();

    if(cancelled.loadRelaxed()) {
        result.status      = KeyResult::Status::Error;
        result.errorString = QObject::tr("Cancelled");
        return result;
    }

    if(mono.empty()) {
        result.status      = KeyResult::Status::Error;
        result.errorString = QObject::tr("No audio decoded");
        return result;
    }

    // libkeyfinder is tuned for 44.1 kHz; resample any other rate to match.
    resampleToTarget(mono, streamRate);

    // ---- Run libkeyfinder ----
    KeyFinder::AudioData audio;
    audio.setFrameRate(TargetSampleRate);
    audio.setChannels(1);
    audio.addToSampleCount(static_cast<unsigned int>(mono.size()));
    for(std::size_t i = 0; i < mono.size(); ++i)
        audio.setSampleByFrame(static_cast<unsigned int>(i), 0, mono[i]);

    KeyFinder::KeyFinder keyFinder;
    const KeyFinder::key_t key = keyFinder.keyOfAudio(audio);

    if(key == KeyFinder::SILENCE) {
        result.status      = KeyResult::Status::Error;
        result.errorString = QObject::tr("Key detection failed (silence or no tonal content)");
        return result;
    }

    const QString rendered = keyToNotation(key, options.notation);
    if(rendered.isEmpty()) {
        result.status      = KeyResult::Status::Error;
        result.errorString = QObject::tr("Unknown key detected");
        return result;
    }

    result.analyzedKey = rendered;
    result.theoryNote  = keyTheoryNote(key);
    result.status = result.storedKey.isEmpty() ? KeyResult::Status::New
                                               : KeyResult::Status::Updated;
    return result;
}

} // namespace Fooyin::KeyAnalyzer
