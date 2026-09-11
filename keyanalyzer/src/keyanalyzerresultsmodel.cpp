/*
 * fooyin-plugins - Key Analyzer plugin
 * Copyright © 2026
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#include "keyanalyzerresultsmodel.h"

#include <QColor>
#include <QFileInfo>
#include <QVariant>

#include <algorithm>

namespace Fooyin::KeyAnalyzer {

KeyAnalyzerResultsModel::KeyAnalyzerResultsModel(QList<KeyResult> results,
                                                 QObject* parent)
    : QAbstractTableModel{parent}
    , m_results{std::move(results)}
{ }

QVariant KeyAnalyzerResultsModel::headerData(int section, Qt::Orientation orientation,
                                             int role) const
{
    if(role != Qt::DisplayRole || orientation != Qt::Horizontal)
        return {};

    switch(static_cast<Column>(section)) {
        case Column::Filename:
            return tr("File");
        case Column::AnalyzedKey:
            return tr("Analyzed Key");
        case Column::StoredKey:
            return tr("Stored Key");
        case Column::Count:
            break;
    }
    return {};
}

QVariant KeyAnalyzerResultsModel::data(const QModelIndex& index, int role) const
{
    if(!index.isValid() || index.row() < 0 || index.row() >= m_results.size())
        return {};

    const KeyResult& result = m_results.at(index.row());

    if(role == Qt::DisplayRole) {
        switch(static_cast<Column>(index.column())) {
            case Column::Filename:
                return QFileInfo(result.track.filepath()).fileName();
            case Column::AnalyzedKey:
                return result.analyzedKey;
            case Column::StoredKey:
                return result.storedKey;
            case Column::Count:
                break;
        }
    }
    else if(role == Qt::ForegroundRole) {
        if(result.status == KeyResult::Status::Error)
            return QColor(Qt::red);
        if(result.status == KeyResult::Status::Skipped)
            return QColor(Qt::gray);
    }
    else if(role == Qt::ToolTipRole) {
        if(result.status == KeyResult::Status::Error)
            return result.errorString;
    }

    return {};
}

int KeyAnalyzerResultsModel::rowCount(const QModelIndex& parent) const
{
    return parent.isValid() ? 0 : static_cast<int>(m_results.size());
}

int KeyAnalyzerResultsModel::columnCount(const QModelIndex& parent) const
{
    return parent.isValid() ? 0 : static_cast<int>(Column::Count);
}

void KeyAnalyzerResultsModel::sort(int column, Qt::SortOrder order)
{
    beginResetModel();

    const auto byCol = [this, column](const KeyResult& a, const KeyResult& b) {
        switch(static_cast<Column>(column)) {
            case Column::Filename:
                return QFileInfo(a.track.filepath()).fileName()
                    < QFileInfo(b.track.filepath()).fileName();
            case Column::AnalyzedKey:
                return a.analyzedKey < b.analyzedKey;
            case Column::StoredKey:
                return a.storedKey < b.storedKey;
            case Column::Count:
                break;
        }
        return a.track.filepath() < b.track.filepath();
    };

    std::sort(m_results.begin(), m_results.end(), byCol);
    if(order == Qt::DescendingOrder)
        std::reverse(m_results.begin(), m_results.end());

    endResetModel();
}

void KeyAnalyzerResultsModel::setResults(QList<KeyResult> results)
{
    beginResetModel();
    m_results = std::move(results);
    endResetModel();
}

void KeyAnalyzerResultsModel::appendResult(const KeyResult& result)
{
    beginInsertRows(QModelIndex{}, m_results.size(), m_results.size());
    m_results.append(result);
    endInsertRows();
}

QList<KeyResult> KeyAnalyzerResultsModel::resultsToSave() const
{
    QList<KeyResult> out;
    for(const KeyResult& r : m_results) {
        if(r.status == KeyResult::Status::New || r.status == KeyResult::Status::Updated)
            out.append(r);
    }
    return out;
}

void KeyAnalyzerResultsModel::markSaved(const QSet<QString>& filepaths)
{
    for(int i = 0; i < m_results.size(); ++i) {
        if(filepaths.contains(m_results.at(i).track.uniqueFilepath()))
            m_results[i].status = KeyResult::Status::Skipped;  // reused as "saved" marker
    }
    dataChanged(index(0, 0), index(m_results.size() - 1, columnCount() - 1));
}

const QList<KeyResult>& KeyAnalyzerResultsModel::results() const
{
    return m_results;
}

} // namespace Fooyin::KeyAnalyzer
