/*
 * fooyin-plugins - Key Analyzer plugin
 * Copyright © 2026
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

// Headless verifier for the Key Analyzer's analysis & tag-content pipeline.
//
// Unlike the in-plugin worker (which decodes through fooyin's AudioLoader) this
// tool decodes standalone via ffmpeg to mono F32 @ 44100 -- exactly the target
// format the plugin feeds libkeyfinder -- then runs the SAME libkeyfinder call
// and the SAME notation / comment-assembly helpers the plugin uses.
//
//   keyanalyzer_cli analyze <file> [--notation camelot|openkey|standard]
//   keyanalyzer_cli write-init <file> <key>
//   keyanalyzer_cli write-comment <file> --mode overwrite|start|end <key>
//   keyanalyzer_cli check <file>
//
// Build via: cmake -B build -DKEYANALYZER_BUILD_TOOLS=ON (dev only).

#include "keyanalyzercomments.h"
#include "keyanalyzernotations.h"

#include <keyfinder/audiodata.h>
#include <keyfinder/keyfinder.h>

#include <taglib/fileref.h>
#include <taglib/tag.h>
#include <taglib/tpropertymap.h>

#include <QByteArray>
#include <QProcess>
#include <QStringList>

#include <cstdio>
#include <cstring>
#include <exception>
#include <vector>

using namespace Qt::StringLiterals;
using namespace Fooyin::KeyAnalyzer;

namespace {

constexpr int TargetSampleRate = 44100;

bool decodeMonoF32(const QString& path, std::vector<float>& out, QString& error)
{
    const QString ffmpeg = qEnvironmentVariable("KEYANALYZER_FFMPEG", "ffmpeg");

    QProcess proc;
    proc.start(ffmpeg, {"-v", "error", "-i", path,
                        "-f", "f32le", "-ac", "1", "-ar", QString::number(TargetSampleRate),
                        "-"});
    if(!proc.waitForStarted(5000)) {
        error = u"Could not start ffmpeg ("_s + ffmpeg + u")."_s;
        return false;
    }
    proc.waitForFinished(-1);

    const QByteArray stderrText = proc.readAllStandardError();
    if(proc.exitStatus() != QProcess::NormalExit || proc.exitCode() != 0) {
        error = u"ffmpeg failed: %1"_s.arg(QString::fromUtf8(stderrText).trimmed());
        return false;
    }

    const QByteArray raw = proc.readAllStandardOutput();
    if(raw.isEmpty() || raw.size() % static_cast<int>(sizeof(float)) != 0) {
        error = u"ffmpeg produced no usable mono PCM data."_s;
        return false;
    }

    out.resize(static_cast<std::size_t>(raw.size()) / sizeof(float));
    std::memcpy(out.data(), raw.constData(), static_cast<std::size_t>(raw.size()));
    return true;
}

bool analyzePcm(const std::vector<float>& samples, KeyFinder::key_t& keyOut, QString& error)
{
    if(samples.empty()) {
        error = u"No audio decoded."_s;
        return false;
    }

    KeyFinder::AudioData audio;
    audio.setFrameRate(TargetSampleRate);
    audio.setChannels(1);
    audio.addToSampleCount(static_cast<unsigned int>(samples.size()));
    for(std::size_t i = 0; i < samples.size(); ++i)
        audio.setSampleByFrame(static_cast<unsigned int>(i), 0, samples[i]);

    try {
        KeyFinder::KeyFinder keyFinder;
        keyOut = keyFinder.keyOfAudio(audio);
    }
    catch(const std::exception& e) {
        error = u"libkeyfinder threw: %1"_s.arg(QString::fromLocal8Bit(e.what()));
        return false;
    }

    if(keyOut == KeyFinder::SILENCE) {
        error = u"Key detection failed (silence or no tonal content)."_s;
        return false;
    }
    return true;
}

// ---- TagLib helpers ----

bool readTagField(const QString& path, const QString& field, QString& value)
{
    TagLib::FileRef ref(path.toLocal8Bit().constData());
    TagLib::File* file = ref.file();
    if(!file)
        return false;

    const TagLib::PropertyMap map      = file->properties();
    const TagLib::StringList   values  = map[field.toStdString().c_str()];
    value = values.isEmpty() ? QString{} : QString::fromUtf8(values.front().toCString(true));
    return true;
}

bool writeTagField(const QString& path, const QString& field, const QString& value, QString& error)
{
    TagLib::FileRef ref(path.toLocal8Bit().constData());
    TagLib::File* file = ref.file();
    if(!file) {
        error = u"TagLib could not open the file."_s;
        return false;
    }

    TagLib::PropertyMap map = file->properties();
    const QByteArray     utf8 = value.toUtf8();
    map.replace(field.toStdString().c_str(),
                TagLib::StringList{TagLib::String{utf8.constData(), TagLib::String::UTF8}});
    file->setProperties(map);
    return file->save();
}

int printKey(const QString& path, KeyFinder::key_t key, Notation notation)
{
    std::puts("=== Analysis ===");
    std::printf("file:      %s\n", path.toLocal8Bit().constData());
    std::printf("key_t:     %d\n", static_cast<int>(key));
    std::printf("camelot:   %s\n", keyToNotation(key, Notation::Camelot).toLocal8Bit().constData());
    std::printf("open key:  %s\n", keyToNotation(key, Notation::OpenKey).toLocal8Bit().constData());
    std::printf("standard:  %s\n", keyToNotation(key, Notation::Standard).toLocal8Bit().constData());
    std::printf("theory:    %s\n", keyTheoryNote(key).toLocal8Bit().constData());
    std::printf("requested: %s\n",
                keyToNotation(key, notation).toLocal8Bit().constData());
    return 0;
}

int cmdAnalyze(const QStringList& args)
{
    if(args.size() < 2) {
        std::puts("usage: keyanalyzer_cli analyze <file> [--notation camelot|openkey|standard]");
        return 2;
    }

    const QString file     = args.at(1);
    QString        notation = u"camelot"_s;
    if(const int idx = args.indexOf(u"--notation"_s); idx >= 0 && idx + 1 < args.size())
        notation = args.at(idx + 1).toLower();

    std::vector<float> samples;
    QString error;
    if(!decodeMonoF32(file, samples, error)) {
        std::printf("ERROR: %s\n", error.toLocal8Bit().constData());
        return 2;
    }

    KeyFinder::key_t key;
    if(!analyzePcm(samples, key, error)) {
        std::printf("ERROR: %s\n", error.toLocal8Bit().constData());
        return 3;
    }

    const Notation notationValue = notation == u"openkey"_s  ? Notation::OpenKey
                                  : notation == u"standard"_s ? Notation::Standard
                                                              : Notation::Camelot;
    return printKey(file, key, notationValue);
}

int cmdWriteInit(const QStringList& args)
{
    if(args.size() < 3) {
        std::puts("usage: keyanalyzer_cli write-init <file> <key>");
        return 1;
    }
    const QString file  = args.at(1);
    const QString value = args.at(2);

    QString error;
    if(!writeTagField(file, "INITIALKEY", value, error)) {
        std::printf("ERROR: %s\n", error.toLocal8Bit().constData());
        return 2;
    }
    std::printf("wrote INITIALKEY=%s\n", value.toLocal8Bit().constData());

    QString readback;
    readTagField(file, "INITIALKEY", readback);
    std::printf("readback INITIALKEY=[%s]\n", readback.toLocal8Bit().constData());
    std::printf("ok-initialkey: %s\n", readback == value ? "yes" : "NO");
    return readback == value ? 0 : 3;
}

int cmdWriteComment(const QStringList& args)
{
    if(args.size() < 4) {
        std::puts("usage: keyanalyzer_cli write-comment <file> --mode overwrite|start|end <key>");
        return 1;
    }
    const QString file = args.at(1);
    QString        modeString = u"overwrite"_s;
    if(const int idx = args.indexOf(u"--mode"_s); idx >= 0 && idx + 1 < args.size())
        modeString = args.at(idx + 1).toLower();
    const QString key = args.last();

    CommentMode mode;
    if(modeString == u"start"_s)
        mode = CommentMode::AppendStart;
    else if(modeString == u"end"_s)
        mode = CommentMode::AppendEnd;
    else
        mode = CommentMode::Overwrite;

    QString existing;
    readTagField(file, "COMMENT", existing);

    const QString newComment = makeComment(mode, key, existing);

    QString error;
    if(!writeTagField(file, "COMMENT", newComment, error)) {
        std::printf("ERROR: %s\n", error.toLocal8Bit().constData());
        return 2;
    }

    QString readback;
    readTagField(file, "COMMENT", readback);

    std::printf("before:   [%s]\n", existing.toLocal8Bit().constData());
    std::printf("computed: [%s]\n", newComment.toLocal8Bit().constData());
    std::printf("readback: [%s]\n", readback.toLocal8Bit().constData());
    std::printf("ok-comment: %s\n", readback == newComment ? "yes" : "NO");
    return readback == newComment ? 0 : 3;
}

int cmdCheck(const QStringList& args)
{
    if(args.size() < 2) {
        std::puts("usage: keyanalyzer_cli check <file>");
        return 1;
    }
    const QString file = args.at(1);
    QString        value;
    readTagField(file, "INITIALKEY", value);
    std::printf("INITIALKEY: [%s]\n", value.toLocal8Bit().constData());
    readTagField(file, "COMMENT", value);
    std::printf("COMMENT:    [%s]\n", value.toLocal8Bit().constData());
    return 0;
}

} // namespace

int main(const int argc, char** argv)
{
    if(argc < 2) {
        std::puts("usage: keyanalyzer_cli <analyze|write-init|write-comment|check> ...");
        return 1;
    }

    const QStringList args(argv + 1, argv + argc);
    const QString     mode = args.at(0);

    if(mode == u"analyze")
        return cmdAnalyze(args);
    if(mode == u"write-init")
        return cmdWriteInit(args);
    if(mode == u"write-comment")
        return cmdWriteComment(args);
    if(mode == u"check")
        return cmdCheck(args);

    std::fprintf(stderr, "unknown mode: %s\n", mode.toLocal8Bit().constData());
    return 2;
}