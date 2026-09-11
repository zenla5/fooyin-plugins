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

#include <core/track.h>

#include <keyfinder/constants.h>

#include <QString>

namespace Fooyin::KeyAnalyzer {

struct KeyResult
{
    enum class Status
    {
        New,      ///< No existing key tag; value freshly computed
        Updated,  ///< Track had an existing key tag; new value computed
        Skipped,  ///< Track had an existing key tag and skip-existing option is on
        Error     ///< Decoding or analysis failure
    };

    Track   track;
    QString analyzedKey;   ///< The computed key in the selected notation (empty on error/skipped)
    QString theoryNote;    ///< Music-theory note, e.g. "C Major (rel. A minor)"
    QString storedKey;     ///< Value from the existing INITIALKEY tag (may be empty)
    QString existingComment; ///< Existing comment (used to compute the new comment text)
    Status  status{Status::New};
    QString errorString;   ///< Populated when status == Error
};

} // namespace Fooyin::KeyAnalyzer
