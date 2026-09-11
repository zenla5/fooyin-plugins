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

#include <core/plugins/coreplugin.h>
#include <core/plugins/plugin.h>
#include <gui/plugins/guiplugin.h>
#include <gui/plugins/pluginconfigguiplugin.h>

#include <QObject>

#include <memory>

namespace Fooyin {
class AudioLoader;
class MusicLibrary;
class TrackSelectionController;
} // namespace Fooyin

namespace Fooyin::KeyAnalyzer {

class KeyAnalyzerPlugin : public QObject,
                          public Plugin,
                          public CorePlugin,
                          public GuiPlugin,
                          public PluginConfigGuiPlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "org.fooyin.fooyin.plugin/1.0" FILE "keyanalyzer.json")
    Q_INTERFACES(Fooyin::Plugin Fooyin::CorePlugin Fooyin::GuiPlugin Fooyin::PluginConfigGuiPlugin)

public:
    void initialise(const CorePluginContext& context) override;
    void initialise(const GuiPluginContext& context) override;

    [[nodiscard]] std::unique_ptr<PluginSettingsProvider> settingsProvider() const override;

private:
    void setupContextMenu();

    std::shared_ptr<AudioLoader> m_audioLoader;
    MusicLibrary*             m_library{nullptr};
    TrackSelectionController* m_selectionController{nullptr};
};

} // namespace Fooyin::KeyAnalyzer
