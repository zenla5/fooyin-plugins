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

#include <core/coresettings.h>

#include <QDialog>

class QCheckBox;
class QComboBox;
class QLabel;
class QSlider;

namespace Fooyin::KeyAnalyzer {

class KeyAnalyzerSettingsDialog : public QDialog
{
    Q_OBJECT

public:
    explicit KeyAnalyzerSettingsDialog(QWidget* parent = nullptr);

    void accept() override;

private:
    FySettings m_settings;

    // Notation + analysis
    QComboBox* m_notation;
    QCheckBox* m_skipExisting;

    // Output
    QCheckBox* m_writeInitKey;
    QCheckBox* m_writeComment;
    QComboBox* m_commentMode;

    // Concurrency
    QCheckBox* m_autoConcurrency;
    QSlider*   m_concurrencySlider;
    QLabel*    m_concurrencyValueLabel;
};

} // namespace Fooyin::KeyAnalyzer
