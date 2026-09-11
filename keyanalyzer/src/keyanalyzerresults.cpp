/*
 * fooyin-plugins - Key Analyzer plugin
 * Copyright © 2026
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#include "keyanalyzerresults.h"

#include "keyanalyzercomments.h"
#include "keyanalyzerdefs.h"
#include "keyanalyzerresultsmodel.h"
#include "keyanalyzerscanner.h"

#include <core/coresettings.h>
#include <core/library/musiclibrary.h>

#include <QApplication>
#include <QClipboard>
#include <QCloseEvent>
#include <QFileInfo>
#include <QGridLayout>
#include <QHeaderView>
#include <QHBoxLayout>
#include <QItemSelectionModel>
#include <QLabel>
#include <QMenu>
#include <QProgressBar>
#include <QPushButton>
#include <QSet>
#include <QSortFilterProxyModel>
#include <QStringList>
#include <QTableView>

using namespace Qt::StringLiterals;

namespace Fooyin::KeyAnalyzer {

KeyAnalyzerResults::KeyAnalyzerResults(MusicLibrary* library,
                                       std::shared_ptr<AudioLoader> audioLoader,
                                       TrackList tracks,
                                       QWidget* parent)
    : QDialog{parent}
    , m_library{library}
    , m_audioLoader{std::move(audioLoader)}
    , m_tracks{std::move(tracks)}
    , m_resultsView{new QTableView(this)}
    , m_resultsModel{new KeyAnalyzerResultsModel({}, this)}
    , m_proxyModel{new QSortFilterProxyModel(this)}
    , m_status{new QLabel(tr("Ready — %1 track(s) selected.").arg(m_tracks.size()), this)}
    , m_progressBar{new QProgressBar(this)}
    , m_analyzeButton{new QPushButton(tr("&Analyze"), this)}
    , m_forceButton{new QPushButton(tr("Re-analyze (force)"), this)}
    , m_saveButton{new QPushButton(tr("&Save to Tags"), this)}
    , m_cancelButton{new QPushButton(tr("Cancel"), this)}
    , m_closeButton{new QPushButton(tr("Close"), this)}
{
    setWindowTitle(tr("Key Analyzer"));
    setModal(false);

    m_proxyModel->setSourceModel(m_resultsModel);

    m_resultsView->setModel(m_proxyModel);
    m_resultsView->setSortingEnabled(true);
    m_resultsView->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_resultsView->setSelectionMode(QAbstractItemView::ExtendedSelection);
    m_resultsView->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_resultsView->verticalHeader()->hide();
    m_resultsView->horizontalHeader()->setStretchLastSection(false);
    m_resultsView->horizontalHeader()->setSortIndicatorShown(true);
    m_resultsView->sortByColumn(static_cast<int>(KeyAnalyzerResultsModel::Column::Filename),
                                Qt::AscendingOrder);
    m_resultsView->horizontalHeader()->setSectionResizeMode(QHeaderView::Interactive);
    m_resultsView->setContextMenuPolicy(Qt::CustomContextMenu);

    m_saveButton->setEnabled(false);
    m_cancelButton->setEnabled(false);
    m_closeButton->setDefault(true);

    m_progressBar->setRange(0, 1);
    m_progressBar->setValue(0);
    m_progressBar->setTextVisible(false);
    m_progressBar->setVisible(false);

    QObject::connect(m_analyzeButton, &QPushButton::clicked,
                     this, [this]() { startScan(m_tracks, false, false); });
    QObject::connect(m_forceButton, &QPushButton::clicked,
                     this, [this]() { startScan(m_tracks, true, false); });
    QObject::connect(m_saveButton, &QPushButton::clicked,
                     this, &KeyAnalyzerResults::saveToTags);
    QObject::connect(m_cancelButton, &QPushButton::clicked,
                     this, &KeyAnalyzerResults::cancelActive);
    QObject::connect(m_closeButton, &QPushButton::clicked,
                     this, &QDialog::close);

    auto* buttonLayout = new QHBoxLayout;
    buttonLayout->addWidget(m_analyzeButton);
    buttonLayout->addWidget(m_forceButton);
    buttonLayout->addStretch();
    buttonLayout->addWidget(m_saveButton);
    buttonLayout->addWidget(m_cancelButton);
    buttonLayout->addWidget(m_closeButton);

    auto* layout = new QGridLayout(this);
    layout->addWidget(m_resultsView, 0, 0);
    layout->addWidget(m_progressBar, 1, 0);
    layout->addWidget(m_status,      2, 0);
    layout->addLayout(buttonLayout,  3, 0);
    layout->setRowStretch(0, 1);

    setupContextMenu();
}

QSize KeyAnalyzerResults::sizeHint() const
{
    return {720, 480};
}

void KeyAnalyzerResults::startScan(const TrackList& tracks, bool force, bool merge)
{
    if(m_scanning || m_saving || tracks.empty())
        return;

    m_scanning = true;
    m_analyzeButton->setEnabled(false);
    m_forceButton->setEnabled(false);
    m_saveButton->setEnabled(false);
    m_cancelButton->setEnabled(true);

    const int total = static_cast<int>(tracks.size());
    m_progressBar->setRange(0, total);
    m_progressBar->setValue(0);
    m_progressBar->setVisible(true);
    m_status->setText(tr("Analyzing…"));

    if(!merge)
        m_resultsModel->setResults({});
    m_scanStart = std::chrono::steady_clock::now();

    m_scanner = new KeyAnalyzerScanner(m_audioLoader, this);

    QObject::connect(m_scanner, &KeyAnalyzerScanner::scanningTrack, this,
                     [this, total](const QString& filepath) {
                         const int done = m_progressBar->value() + 1;
                         m_progressBar->setValue(done);
                         m_status->setText(
                             tr("Analyzing %1 / %2: %3")
                                 .arg(done).arg(total)
                                 .arg(QFileInfo{filepath}.fileName()));
                     });

    QObject::connect(m_scanner, &KeyAnalyzerScanner::trackScanned, this,
                     [this, merge](const KeyResult& result) {
                         if(merge)
                             m_resultsModel->upsertResult(result);
                         else
                             m_resultsModel->appendResult(result);
                         m_resultsView->resizeColumnsToContents();
                     });

    QObject::connect(m_scanner, &KeyAnalyzerScanner::scanFinished,
                     this, &KeyAnalyzerResults::onScanFinished);

    m_scanner->scanTracks(tracks, force);
}

void KeyAnalyzerResults::onScanFinished(const QList<KeyResult>& /*results*/)
{
    m_scanning = false;
    if(m_scanner) {
        m_scanner->deleteLater();
        m_scanner = nullptr;
    }

    const auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now() - m_scanStart);

    m_resultsView->resizeColumnsToContents();
    m_progressBar->setVisible(false);
    m_status->setText(tr("Done in %1 ms.").arg(static_cast<qint64>(elapsed.count())));

    m_analyzeButton->setEnabled(true);
    m_cancelButton->setEnabled(false);
    updateButtons();
}

void KeyAnalyzerResults::saveToTags()
{
    if(m_saving)
        return;

    const QList<KeyResult> toSave = m_resultsModel->resultsToSave();
    if(toSave.isEmpty())
        return;

    // Capture the exact model rows being saved so we never have to re-match
    // rows by file-path string after the write (avoids a rename race).
    const QList<int> saveRows = m_resultsModel->savedRows();

    FySettings settings;
    const bool writeInitKey =
        settings.value(QLatin1String{SettingWriteInitKey}, true).toBool();
    const bool writeComment =
        settings.value(QLatin1String{SettingWriteComment}, false).toBool();
    const auto commentMode = static_cast<CommentMode>(
        settings.value(QLatin1String{SettingCommentMode}, DefaultCommentMode).toInt());

    m_saving = true;
    m_progressBar->setRange(0, toSave.size());
    m_progressBar->setValue(0);
    m_progressBar->setVisible(true);
    m_status->setText(tr("Writing tags…"));

    TrackList tracks;
    tracks.reserve(toSave.size());
    for(const KeyResult& r : toSave) {
        Track t = r.track;
        if(writeInitKey)
            t.replaceExtraTag(QLatin1String{InitKeyTagField}, r.analyzedKey);

        if(writeComment) {
            const QString newComment =
                makeComment(commentMode, r.analyzedKey, r.existingComment);
            // "COMMENT" is a standard field in fooyin (Tag::Comment); writing it
            // as an extra tag gets dropped by normaliseExtraProperties() so the
            // DB/UI comment would stay empty. Set the standard property instead.
            t.setComment(newComment);
        }

        tracks.push_back(t);
    }

    const WriteRequest request = m_library->writeTrackMetadata(tracks);
    m_writeCancel = request.cancel;

    auto saved = std::make_shared<QSet<QString>>();

    QObject::connect(m_library, &MusicLibrary::tracksMetadataChanged,
                     this, [this, saved, toSave](const TrackList& changed) {
                         bool updated = false;
                         for(const Track& track : changed) {
                             const QString path = track.uniqueFilepath();
                             if(saved->contains(path))
                                 continue;
                             saved->insert(path);
                             updated = true;
                         }
                         if(updated) {
                             m_progressBar->setValue(saved->size());
                             m_status->setText(
                                 tr("Writing tags %1 / %2…")
                                     .arg(saved->size())
                                     .arg(toSave.size()));
                         }
                     });

    auto* watcher = new QFutureWatcher<WriteResult>(this);
    QObject::connect(watcher, &QFutureWatcher<WriteResult>::finished,
                     this, [this, saved, toSave, saveRows, watcher]() {
                         const WriteResult result = watcher->result();
                         watcher->deleteLater();

                         m_resultsModel->markSaved(saveRows);

                         m_progressBar->setVisible(false);
                         m_saving = false;
                         m_writeCancel = nullptr;
                         m_analyzeButton->setEnabled(true);
                         updateButtons();

                         if(result.failed == 0 && result.state == WriteState::Completed) {
                             m_progressBar->setValue(toSave.size());
                             m_status->setText(tr("Tags saved."));
                         }
                         else {
                             m_status->setText(
                                 tr("Saved %1 / %2 tag(s); %3 failed.")
                                     .arg(result.succeeded)
                                     .arg(toSave.size())
                                     .arg(result.failed));
                         }
                     });
    watcher->setFuture(request.finished);
}

void KeyAnalyzerResults::cancelActive()
{
    if(m_scanner)
        m_scanner->close();
    if(m_writeCancel)
        m_writeCancel();
}

void KeyAnalyzerResults::setupContextMenu()
{
    m_resultsView->setContextMenuPolicy(Qt::CustomContextMenu);
    QObject::connect(m_resultsView, &QWidget::customContextMenuRequested,
                     this, &KeyAnalyzerResults::showContextMenu);
}

void KeyAnalyzerResults::showContextMenu(const QPoint& pos)
{
    const QModelIndex index = m_resultsView->indexAt(pos);
    if(index.isValid() && !m_resultsView->selectionModel()->hasSelection())
        m_resultsView->selectRow(index.row());

    const bool hasSelection = !selectedTracks().empty();

    auto* menu = new QMenu(this);
    menu->setAttribute(Qt::WA_DeleteOnClose);

    QAction* copyAction = menu->addAction(tr("Copy &Keys"));
    copyAction->setEnabled(hasSelection);
    QObject::connect(copyAction, &QAction::triggered,
                     this, &KeyAnalyzerResults::copySelectedKeys);

    QAction* retryAction = menu->addAction(tr("&Re-analyze Selected"));
    retryAction->setEnabled(hasSelection && !m_scanning && !m_saving);
    QObject::connect(retryAction, &QAction::triggered,
                     this, &KeyAnalyzerResults::retrySelected);

    menu->popup(m_resultsView->viewport()->mapToGlobal(pos));
}

TrackList KeyAnalyzerResults::selectedTracks() const
{
    TrackList out;
    const QModelIndexList selected = m_resultsView->selectionModel()->selectedRows();
    out.reserve(selected.size());
    for(const QModelIndex& proxyIndex : selected) {
        const QModelIndex sourceIndex = m_proxyModel->mapToSource(proxyIndex);
        if(!sourceIndex.isValid() || sourceIndex.row() < 0
           || sourceIndex.row() >= m_resultsModel->results().size())
            continue;
        out.push_back(m_resultsModel->results().at(sourceIndex.row()).track);
    }
    return out;
}

void KeyAnalyzerResults::copySelectedKeys()
{
    QStringList lines;
    const QModelIndexList selected = m_resultsView->selectionModel()->selectedRows();
    for(const QModelIndex& proxyIndex : selected) {
        const QModelIndex sourceIndex = m_proxyModel->mapToSource(proxyIndex);
        if(!sourceIndex.isValid() || sourceIndex.row() < 0
           || sourceIndex.row() >= m_resultsModel->results().size())
            continue;
        const KeyResult& r = m_resultsModel->results().at(sourceIndex.row());
        const QString key = r.analyzedKey.isEmpty() ? tr("(error)") : r.analyzedKey;
        lines.append(QFileInfo{r.track.filepath()}.fileName() + u"\t"_s + key);
    }
    if(!lines.isEmpty())
        QApplication::clipboard()->setText(lines.join(QLatin1Char('\n')));
}

void KeyAnalyzerResults::retrySelected()
{
    const TrackList selected = selectedTracks();
    if(selected.empty())
        return;
    startScan(selected, true, true);
}

void KeyAnalyzerResults::updateButtons()
{
    const bool idle = !m_scanning && !m_saving;
    m_analyzeButton->setEnabled(idle);
    m_forceButton->setEnabled(idle);
    m_saveButton->setEnabled(idle && !m_resultsModel->results().isEmpty());
}

void KeyAnalyzerResults::closeEvent(QCloseEvent* event)
{
    cancelActive();
    QDialog::closeEvent(event);
}

} // namespace Fooyin::KeyAnalyzer

#include "moc_keyanalyzerresults.cpp"
