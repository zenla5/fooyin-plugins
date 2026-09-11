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

#include "keyanalyzerdefs.h"

#include <core/track.h>

#include <QAtomicInt>

#include <memory>

namespace Fooyin {
class AudioLoader;
} // namespace Fooyin

namespace Fooyin::KeyAnalyzer {

/*!
 * Per-scan analysis configuration. Resolved once by the caller (from settings
 * and/or GUI state) so worker threads never touch QSettings themselves.
 */
struct AnalysisOptions
{
    Notation notation{Notation::Camelot};
    //! Skip tracks that already carry an INITIALKEY tag.
    bool skipExisting{false};
    //! Override @c skipExisting — used by "Re-analyze (force)".
    bool force{false};
};

/*!
 * Pure computation helper that decodes a track to mono PCM, feeds the samples
 * to libkeyfinder's Krumhansl-Schmuckler key detector and returns the key in
 * the configured notation.
 *
 * computeKey() is safe to call from multiple threads concurrently because each
 * invocation creates its own decoder and analysis state; there is no shared
 * mutable state other than the thread-safe AudioLoader.
 */
class KeyAnalyzerWorker
{
public:
    explicit KeyAnalyzerWorker(std::shared_ptr<AudioLoader> audioLoader);

    [[nodiscard]] KeyResult computeKey(const Track& track,
                                       const QAtomicInt& cancelled,
                                       const AnalysisOptions& options) const;

private:
    std::shared_ptr<AudioLoader> m_audioLoader;
};

} // namespace Fooyin::KeyAnalyzer
