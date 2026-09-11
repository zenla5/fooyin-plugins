/*
 * fooyin-plugins - Key Analyzer plugin
 * Copyright © 2026
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#include "keyanalyzerscanner.h"

#include "keyanalyzerdefs.h"
#include "keyanalyzerworker.h"

#include <core/coresettings.h>

#include <QtConcurrent/QtConcurrent>
#include <QThread>

namespace Fooyin::KeyAnalyzer {

KeyAnalyzerScanner::KeyAnalyzerScanner(std::shared_ptr<AudioLoader> audioLoader,
                                       QObject* parent)
    : QObject{parent}
    , m_worker{std::make_unique<KeyAnalyzerWorker>(std::move(audioLoader))}
{
    QObject::connect(&m_watcher, &QFutureWatcher<KeyResult>::resultReadyAt,
                     this, &KeyAnalyzerScanner::onResultReadyAt);
    QObject::connect(&m_watcher, &QFutureWatcher<KeyResult>::finished,
                     this, &KeyAnalyzerScanner::onFinished);
}

KeyAnalyzerScanner::~KeyAnalyzerScanner()
{
    m_watcher.cancel();
    m_watcher.waitForFinished();
}

void KeyAnalyzerScanner::close()
{
    m_cancelled.storeRelaxed(1);
    m_watcher.cancel();
}

void KeyAnalyzerScanner::scanTracks(const TrackList& tracks)
{
    m_cancelled.storeRelaxed(0);

    FySettings settings;
    const bool autoThreads =
        settings.value(QLatin1String{SettingConcurrencyAuto}, false).toBool();
    const int threadCount = autoThreads
        ? QThread::idealThreadCount()
        : std::max(1, settings.value(QLatin1String{SettingConcurrencyCount},
                                     DefaultConcurrencyCount).toInt());

    m_threadPool.setMaxThreadCount(threadCount);

    m_watcher.setFuture(QtConcurrent::mapped(
        &m_threadPool,
        tracks,
        [this](const Track& track) -> KeyResult {
            return m_worker->computeKey(track, m_cancelled);
        }));
}

void KeyAnalyzerScanner::onResultReadyAt(int index)
{
    const KeyResult result = m_watcher.resultAt(index);
    emit scanningTrack(result.track.filepath());
    emit trackScanned(result);
}

void KeyAnalyzerScanner::onFinished()
{
    if(m_cancelled.loadRelaxed()) {
        emit scanFinished({});
        return;
    }
    emit scanFinished(m_watcher.future().results());
}

} // namespace Fooyin::KeyAnalyzer

#include "moc_keyanalyzerscanner.cpp"
