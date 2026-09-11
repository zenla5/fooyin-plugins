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

#include "keyanalyzerdefs.h"

#include <keyfinder/constants.h>

#include <QString>

#include <map>

namespace Fooyin::KeyAnalyzer {

using KeyMap = std::map<KeyFinder::key_t, QString>;

// Same tables as Evan Purkhiser's keyfinder-cli (key_notations.h), mapping the
// 24 libkeyfinder keys + SILENCE to the three supported notations.
const KeyMap &notationMap(Notation notation);

/*!
 * Render a libkeyfinder key_t as a string in the requested notation.
 * Returns an empty string for SILENCE or unknown keys.
 */
QString keyToNotation(KeyFinder::key_t key, Notation notation);

/*!
 * Render a human-friendly music-theory note for @p key, e.g.
 * "C Major (rel. A minor)" or "A minor (rel. C major)".
 * Returns an empty string for SILENCE.
 */
QString keyTheoryNote(KeyFinder::key_t key);

} // namespace Fooyin::KeyAnalyzer
