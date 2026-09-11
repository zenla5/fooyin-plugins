/*
 * fooyin-plugins - Key Analyzer plugin
 * Copyright © 2026
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#include "keyanalyzerplugin.h"

#include "keyanalyzerresults.h"
#include "keyanalyzerplugin_settings.h"

#include <core/library/musiclibrary.h>
#include <core/plugins/coreplugincontext.h>
#include <gui/guiconstants.h>
#include <gui/plugins/guiplugincontext.h>
#include <gui/plugins/pluginsettingsprovider.h>
#include <gui/trackselectioncontroller.h>
#include <utils/utils.h>

#include <QAction>
#include <QMainWindow>
#include <QMenu>

namespace {

class KeyAnalyzerSettingsProvider : public Fooyin::PluginSettingsProvider
{
public:
    [[nodiscard]] QDialog* createSettings(QWidget* parent) override
    {
        return new Fooyin::KeyAnalyzer::KeyAnalyzerSettingsDialog(parent);
    }
};

} // namespace

namespace Fooyin::KeyAnalyzer {

void KeyAnalyzerPlugin::initialise(const CorePluginContext& context)
{
    m_audioLoader = context.audioLoader;
    m_library     = context.library;
}

void KeyAnalyzerPlugin::initialise(const GuiPluginContext& context)
{
    m_selectionController = context.trackSelection;

    setupContextMenu();
}

void KeyAnalyzerPlugin::setupContextMenu()
{
    auto* action = new QAction(tr("Key Analyzer…"), this);
    action->setStatusTip(tr("Detect musical key for selected tracks and write it to tags"));

    QObject::connect(action, &QAction::triggered, this, [this]() {
        const TrackList tracks = m_selectionController->selectedTracks();
        if(tracks.empty())
            return;
        auto* dlg = new KeyAnalyzerResults(m_library, m_audioLoader, tracks,
                                           Utils::getMainWindow());
        dlg->setAttribute(Qt::WA_DeleteOnClose);
        dlg->show();
    });

    m_selectionController->registerTrackContextAction(
        this, TrackContextMenuArea::Track,
        Constants::Menus::Context::Utilities,
        "KeyAnalyzer.Analyze",
        tr("Key Analyzer…"),
        [action](QMenu* menu, const TrackSelection& selection) {
            action->setEnabled(!selection.tracks.empty());
            menu->addAction(action);
        });
}

std::unique_ptr<PluginSettingsProvider> KeyAnalyzerPlugin::settingsProvider() const
{
    return std::make_unique<KeyAnalyzerSettingsProvider>();
}

} // namespace Fooyin::KeyAnalyzer

#include "moc_keyanalyzerplugin.cpp"
