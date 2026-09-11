/*
 * fooyin-plugins - Key Analyzer plugin
 * Copyright © 2026
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#pragma once

#include "keyanalyzerresult.h"

#include <core/track.h>

#include <QAtomicInt>
#include <QFutureWatcher>
#include <QObject>
#include <QThreadPool>

#include <memory>

namespace Fooyin {
class AudioLoader;
} // namespace Fooyin

namespace Fooyin::KeyAnalyzer {

class KeyAnalyzerWorker;

class KeyAnalyzerScanner : public QObject
{
    Q_OBJECT

public:
    explicit KeyAnalyzerScanner(std::shared_ptr<AudioLoader> audioLoader,
                                QObject* parent = nullptr);
    ~KeyAnalyzerScanner() override;

    void close();
    //! Scan @p tracks. When @p force is true, tracks that already have an
    //! INITIALKEY tag are re-analyzed regardless of the skip-existing setting.
    void scanTracks(const TrackList& tracks, bool force = false);

signals:
    void scanningTrack(const QString& filepath);
    void trackScanned(const KeyResult& result);
    void scanFinished(const QList<KeyResult>& results);

private:
    void onResultReadyAt(int index);
    void onFinished();

    std::unique_ptr<KeyAnalyzerWorker> m_worker;
    QFutureWatcher<KeyResult>          m_watcher;
    QThreadPool                        m_threadPool;
    QAtomicInt                         m_cancelled{0};
};

} // namespace Fooyin::KeyAnalyzer
