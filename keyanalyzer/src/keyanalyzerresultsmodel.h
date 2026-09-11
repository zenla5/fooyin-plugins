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

#include <QAbstractTableModel>
#include <QList>
#include <QSet>

namespace Fooyin::KeyAnalyzer {

class KeyAnalyzerResultsModel : public QAbstractTableModel
{
    Q_OBJECT

public:
    enum class Column
    {
        Filename    = 0,
        AnalyzedKey,
        StoredKey,
        Count  // sentinel — keep last
    };

    explicit KeyAnalyzerResultsModel(QList<KeyResult> results,
                                     QObject* parent = nullptr);

    [[nodiscard]] QVariant headerData(int section, Qt::Orientation orientation,
                                      int role) const override;
    [[nodiscard]] QVariant data(const QModelIndex& index, int role) const override;
    [[nodiscard]] int rowCount(const QModelIndex& parent = QModelIndex{}) const override;
    [[nodiscard]] int columnCount(const QModelIndex& parent = QModelIndex{}) const override;
    void sort(int column, Qt::SortOrder order) override;

    void setResults(QList<KeyResult> results);
    void appendResult(const KeyResult& result);

    [[nodiscard]] QList<KeyResult> resultsToSave() const;
    void markSaved(const QSet<QString>& filepaths);

    [[nodiscard]] const QList<KeyResult>& results() const;

private:
    QList<KeyResult> m_results;
};

} // namespace Fooyin::KeyAnalyzer
