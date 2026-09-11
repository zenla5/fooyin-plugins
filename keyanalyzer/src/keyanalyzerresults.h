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

#include <chrono>
#include <functional>
#include <memory>

class QCloseEvent;
class QLabel;
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
    void startScan();
    void onScanFinished(const QList<KeyResult>& results);
    void saveToTags();
    void cancelActive();
    void setupContextMenu();
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
    QPushButton*               m_saveButton;
    QPushButton*               m_cancelButton;
    QPushButton*               m_closeButton;

    std::function<void()> m_writeCancel;
    std::chrono::steady_clock::time_point m_scanStart;

    bool m_scanning{false};
    bool m_saving{false};
};

} // namespace Fooyin::KeyAnalyzer
