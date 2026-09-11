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

#include <QDialog>
#include <QList>
#include <QPoint>

#include <chrono>
#include <functional>
#include <memory>

class QCloseEvent;
class QLabel;
class QMenu;
class QPoint;
class QProgressBar;
class QPushButton;
class QSortFilterProxyModel;
class QTableView;

namespace Fooyin {
class AudioLoader;
class MusicLibrary;
} // namespace Fooyin

namespace Fooyin::KeyAnalyzer {

class KeyAnalyzerResultsModel;
class KeyAnalyzerScanner;

class KeyAnalyzerResults : public QDialog
{
    Q_OBJECT

public:
    KeyAnalyzerResults(MusicLibrary* library,
                       std::shared_ptr<AudioLoader> audioLoader,
                       TrackList tracks,
                       QWidget* parent = nullptr);

    [[nodiscard]] QSize sizeHint() const override;

protected:
    void closeEvent(QCloseEvent* event) override;

private:
    //! Run a scan. When @p force, existing keys are ignored (re-analysis).
    //! When @p merge, results are upserted into the model instead of clearing it.
    void startScan(const TrackList& tracks, bool force, bool merge);
    void onScanFinished(const QList<KeyResult>& results);
    void saveToTags();
    void cancelActive();
    void setupContextMenu();
    void showContextMenu(const QPoint& pos);
    //! Tracks of the currently selected rows (proxy ordering).
    [[nodiscard]] TrackList selectedTracks() const;
    void copySelectedKeys();
    void retrySelected();
    void updateButtons();

    MusicLibrary* m_library;
    std::shared_ptr<AudioLoader> m_audioLoader;
    TrackList m_tracks;

    KeyAnalyzerScanner* m_scanner{nullptr};

    QTableView*                m_resultsView;
    KeyAnalyzerResultsModel*   m_resultsModel;
    QSortFilterProxyModel*     m_proxyModel;
    QLabel*                    m_status;
    QProgressBar*              m_progressBar;
    QPushButton*               m_analyzeButton;
    QPushButton*               m_forceButton;
    QPushButton*               m_saveButton;
    QPushButton*               m_cancelButton;
    QPushButton*               m_closeButton;

    std::function<void()> m_writeCancel;
    std::chrono::steady_clock::time_point m_scanStart;

    bool m_scanning{false};
    bool m_saving{false};
};

} // namespace Fooyin::KeyAnalyzer
