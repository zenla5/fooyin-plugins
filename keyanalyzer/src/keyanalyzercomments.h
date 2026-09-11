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

#include <QString>

namespace Fooyin::KeyAnalyzer {

/*!
 * Normalise a stored comment for display/processing: trims surrounding
 * whitespace that tag editors often leave behind.
 */
[[nodiscard]] QString cleanComment(const QString& comment);

/*!
 * Compute the comment text that should be written for @p key according to
 * @p mode, given the comment currently stored in the file (@p existing).
 *
 * Behaviour:
 *  - Overwrite:  always replaces the comment with @p key.
 *  - AppendStart/AppendEnd: trims @p existing and appends/prepends @p key,
 *    preserving existing whitespace-free content. If the comment already
 *    contains @p key as a token (case-insensitive), it is left as-is so
 *    re-analysis does not accumulate duplicate markers.
 */
[[nodiscard]] QString makeComment(CommentMode mode, const QString& key,
                                  const QString& existing);

} // namespace Fooyin::KeyAnalyzer