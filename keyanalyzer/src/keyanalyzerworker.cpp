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

#include <core/coresettings.h>
#include <core/engine/audiobuffer.h>
#include <core/engine/audioconverter.h>
#include <core/engine/audioformat.h>
#include <core/engine/audioloader.h>
#include <core/engine/audioinput.h>

#include <keyfinder/audiodata.h>
#include <keyfinder/keyfinder.h>

#include <QObject>

#include <algorithm>
#include <vector>

using namespace Qt::StringLiterals;

namespace Fooyin::KeyAnalyzer {

namespace {

constexpr int TargetSampleRate = 44100;

} // namespace

// ---------------------------------------------------------------------------
// KeyAnalyzerWorker
// ---------------------------------------------------------------------------

KeyAnalyzerWorker::KeyAnalyzerWorker(std::shared_ptr<AudioLoader> audioLoader)
    : m_audioLoader{std::move(audioLoader)}
{ }

KeyResult KeyAnalyzerWorker::computeKey(const Track& track,
                                        const QAtomicInt& cancelled) const
{
    // ---- Read settings ----
    FySettings settings;

    const auto notation = static_cast<Notation>(
        settings.value(QLatin1String{SettingNotation}, DefaultNotation).toInt());

    const bool skipExisting =
        settings.value(QLatin1String{SettingSkipExisting}, false).toBool();

    // ---- Build result stub ----
    KeyResult result;
    result.track = track;

    const QStringList existingKey = track.extraTag(u"INITIALKEY"_s);
    if(!existingKey.isEmpty())
        result.storedKey = existingKey.first();

    const QStringList comments = track.extraTag(u"COMMENT"_s);
    if(!comments.isEmpty())
        result.existingComment = comments.first();

    if(skipExisting && !result.storedKey.isEmpty()) {
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

    const AudioFormat fmt = loaded.format.value_or(AudioFormat{});
    if(!fmt.isValid()) {
        result.status      = KeyResult::Status::Error;
        result.errorString = QObject::tr("Could not determine audio format");
        return result;
    }

    // ---- Decode and convert to mono float PCM ----
    loaded.decoder->start();

    std::vector<float> mono;
    mono.reserve(static_cast<size_t>(fmt.sampleRate()) * 30);  // rough pre-alloc

    constexpr size_t ChunkBytes = 65536;
    while(!cancelled.loadRelaxed()) {
        AudioBuffer buf = loaded.decoder->readBuffer(ChunkBytes);
        if(!buf.isValid() || buf.byteCount() == 0)
            break;

        AudioFormat target = buf.format();
        target.setSampleFormat(SampleFormat::F32);
        target.setChannelCount(1);
        target.setSampleRate(TargetSampleRate);

        const AudioBuffer converted = Audio::convert(buf, target);
        if(!converted.isValid())
            continue;

        const auto span = converted.constData();
        const auto *samples = reinterpret_cast<const float *>(span.data());
        const size_t count  = span.size_bytes() / sizeof(float);
        mono.insert(mono.end(), samples, samples + count);
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

    const QString rendered = keyToNotation(key, notation);
    if(rendered.isEmpty()) {
        result.status      = KeyResult::Status::Error;
        result.errorString = QObject::tr("Unknown key detected");
        return result;
    }

    result.analyzedKey = rendered;
    result.status = result.storedKey.isEmpty() ? KeyResult::Status::New
                                               : KeyResult::Status::Updated;
    return result;
}

} // namespace Fooyin::KeyAnalyzer
