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

#include <QString>

namespace Fooyin::KeyAnalyzer {

// Standard tag field for the musical key, read/written by fooyin's tag editor.
constexpr auto InitKeyTagField = "INITIALKEY";
// The general comment tag, appended to / overwritten by the optional comment
// write feature.
constexpr auto CommentTagField = "COMMENT";

// Settings keys
constexpr auto SettingNotation          = "KeyAnalyzer/Notation";
constexpr auto SettingSkipExisting      = "KeyAnalyzer/SkipExisting";
constexpr auto SettingWriteInitKey      = "KeyAnalyzer/WriteInitKey";
constexpr auto SettingWriteComment      = "KeyAnalyzer/WriteComment";
constexpr auto SettingCommentMode       = "KeyAnalyzer/CommentMode";
constexpr auto SettingConcurrencyAuto   = "KeyAnalyzer/ConcurrencyAuto";
constexpr auto SettingConcurrencyCount  = "KeyAnalyzer/ConcurrencyCount";

// Defaults
constexpr int DefaultNotation         = 0;  // Camelot
constexpr int DefaultCommentMode      = 0;  // Overwrite
constexpr int DefaultConcurrencyCount = 1;

/*!
 * Key notation used to render the detected musical key.
 */
enum class Notation : int
{
    Camelot  = 0,   ///< e.g. "8B" (Mixed In Key / Camelot wheel)
    OpenKey  = 1,   ///< e.g. "1m" (Traktor / BeaTunes Open Key)
    Standard = 2,   ///< e.g. "C# Minor" (classic music-theory names)
};

/*!
 * How the key is written into the Comment tag.
 */
enum class CommentMode : int
{
    Overwrite     = 0,  ///< Replace the whole comment with the key
    AppendStart   = 1,  ///< Prepend the key to the existing comment
    AppendEnd     = 2,  ///< Append the key to the existing comment
};

} // namespace Fooyin::KeyAnalyzer
