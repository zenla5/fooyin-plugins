/*
 * fooyin-plugins - Key Analyzer plugin
 * Copyright © 2026
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#include "keyanalyzerplugin_settings.h"

#include "keyanalyzerdefs.h"

#include <core/coresettings.h>

#include <QCheckBox>
#include <QComboBox>
#include <QDialogButtonBox>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QSlider>
#include <QThread>
#include <QVBoxLayout>

using namespace Qt::StringLiterals;

namespace Fooyin::KeyAnalyzer {

KeyAnalyzerSettingsDialog::KeyAnalyzerSettingsDialog(QWidget* parent)
    : QDialog{parent}
    , m_notation{new QComboBox(this)}
    , m_skipExisting{new QCheckBox(tr("Skip tracks that already have a key tag"), this)}
    , m_writeInitKey{new QCheckBox(tr("Write to INITIALKEY tag"), this)}
    , m_writeComment{new QCheckBox(tr("Write to Comment tag"), this)}
    , m_commentMode{new QComboBox(this)}
    , m_autoConcurrency{new QCheckBox(tr("Auto (use all CPU cores)"), this)}
    , m_concurrencySlider{new QSlider(Qt::Horizontal, this)}
    , m_concurrencyValueLabel{new QLabel(this)}
{
    setWindowTitle(tr("Key Analyzer Settings"));
    setModal(true);

    // ---- Notation + analysis ----
    m_notation->addItem(tr("Camelot (e.g. 8B)"),   static_cast<int>(Notation::Camelot));
    m_notation->addItem(tr("Open Key (e.g. 1m)"),  static_cast<int>(Notation::OpenKey));
    m_notation->addItem(tr("Standard (e.g. C# Minor)"), static_cast<int>(Notation::Standard));
    {
        const int saved = m_settings.value(QLatin1String{SettingNotation},
                                           DefaultNotation).toInt();
        const int idx = m_notation->findData(saved);
        m_notation->setCurrentIndex(idx >= 0 ? idx : 0);
    }
    m_skipExisting->setChecked(
        m_settings.value(QLatin1String{SettingSkipExisting}, false).toBool());

    auto* notationLabel  = new QLabel(tr("Key notation:"), this);
    auto* notationLayout = new QHBoxLayout;
    notationLayout->addWidget(notationLabel);
    notationLayout->addWidget(m_notation, 1);

    auto* analysisGroup  = new QGroupBox(tr("Analysis"), this);
    auto* analysisVLayout = new QVBoxLayout(analysisGroup);
    analysisVLayout->addLayout(notationLayout);
    analysisVLayout->addWidget(m_skipExisting);

    // ---- Output ----
    m_writeInitKey->setChecked(
        m_settings.value(QLatin1String{SettingWriteInitKey}, true).toBool());
    m_writeComment->setChecked(
        m_settings.value(QLatin1String{SettingWriteComment}, false).toBool());

    m_commentMode->addItem(tr("Overwrite"), static_cast<int>(CommentMode::Overwrite));
    m_commentMode->addItem(tr("Append at start"), static_cast<int>(CommentMode::AppendStart));
    m_commentMode->addItem(tr("Append at end"),   static_cast<int>(CommentMode::AppendEnd));
    {
        const int saved = m_settings.value(QLatin1String{SettingCommentMode},
                                           DefaultCommentMode).toInt();
        const int idx = m_commentMode->findData(saved);
        m_commentMode->setCurrentIndex(idx >= 0 ? idx : 0);
    }
    m_commentMode->setEnabled(m_writeComment->isChecked());
    QObject::connect(m_writeComment, &QCheckBox::toggled,
                     m_commentMode, &QWidget::setEnabled);

    auto* modeLabel  = new QLabel(tr("Comment mode:"), this);
    auto* modeLayout = new QHBoxLayout;
    modeLayout->addWidget(modeLabel);
    modeLayout->addWidget(m_commentMode, 1);

    auto* outputGroup  = new QGroupBox(tr("Write to tags"), this);
    auto* outputVLayout = new QVBoxLayout(outputGroup);
    outputVLayout->addWidget(m_writeInitKey);
    outputVLayout->addWidget(m_writeComment);
    outputVLayout->addLayout(modeLayout);

    // ---- Concurrency ----
    const int maxThreads = std::max(1, QThread::idealThreadCount());
    m_concurrencySlider->setRange(1, maxThreads);
    m_concurrencySlider->setTickPosition(QSlider::TicksBelow);
    m_concurrencySlider->setTickInterval(1);
    m_concurrencySlider->setSingleStep(1);
    m_concurrencySlider->setPageStep(1);

    const bool isAuto =
        m_settings.value(QLatin1String{SettingConcurrencyAuto}, false).toBool();
    const int savedCount =
        m_settings.value(QLatin1String{SettingConcurrencyCount},
                         DefaultConcurrencyCount).toInt();
    m_autoConcurrency->setChecked(isAuto);
    m_concurrencySlider->setValue(std::clamp(savedCount, 1, maxThreads));
    m_concurrencySlider->setEnabled(!isAuto);

    const auto updateValueLabel = [this, maxThreads]() {
        m_concurrencyValueLabel->setText(
            QString::number(m_concurrencySlider->value()) + " / "_L1 +
            QString::number(maxThreads));
    };
    updateValueLabel();
    QObject::connect(m_concurrencySlider, &QSlider::valueChanged,
                     this, updateValueLabel);
    QObject::connect(m_autoConcurrency, &QCheckBox::toggled,
                     this, [this, updateValueLabel](bool checked) {
                         m_concurrencySlider->setEnabled(!checked);
                         updateValueLabel();
                     });

    auto* threadsLabel = new QLabel(tr("Threads:"), this);
    auto* sliderRow    = new QHBoxLayout;
    sliderRow->addWidget(threadsLabel);
    sliderRow->addWidget(m_concurrencySlider, 1);
    sliderRow->addWidget(m_concurrencyValueLabel);

    auto* concurrencyGroup  = new QGroupBox(tr("Concurrency"), this);
    auto* concurrencyLayout = new QVBoxLayout(concurrencyGroup);
    concurrencyLayout->addWidget(m_autoConcurrency);
    concurrencyLayout->addLayout(sliderRow);

    auto* buttons = new QDialogButtonBox(
        QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    QObject::connect(buttons, &QDialogButtonBox::accepted,
                     this, &KeyAnalyzerSettingsDialog::accept);
    QObject::connect(buttons, &QDialogButtonBox::rejected,
                     this, &KeyAnalyzerSettingsDialog::reject);

    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->addWidget(analysisGroup);
    mainLayout->addWidget(outputGroup);
    mainLayout->addWidget(concurrencyGroup);
    mainLayout->addStretch();
    mainLayout->addWidget(buttons);
}

void KeyAnalyzerSettingsDialog::accept()
{
    m_settings.setValue(QLatin1String{SettingNotation},
                        m_notation->currentData().toInt());
    m_settings.setValue(QLatin1String{SettingSkipExisting},
                        m_skipExisting->isChecked());
    m_settings.setValue(QLatin1String{SettingWriteInitKey},
                        m_writeInitKey->isChecked());
    m_settings.setValue(QLatin1String{SettingWriteComment},
                        m_writeComment->isChecked());
    m_settings.setValue(QLatin1String{SettingCommentMode},
                        m_commentMode->currentData().toInt());
    m_settings.setValue(QLatin1String{SettingConcurrencyAuto},
                        m_autoConcurrency->isChecked());
    m_settings.setValue(QLatin1String{SettingConcurrencyCount},
                        m_concurrencySlider->value());
    done(Accepted);
}

} // namespace Fooyin::KeyAnalyzer

#include "moc_keyanalyzerplugin_settings.cpp"
