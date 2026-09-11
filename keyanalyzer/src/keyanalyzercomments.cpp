/*
 * fooyin-plugins - Key Analyzer plugin
 * Copyright © 2026
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#include "keyanalyzercomments.h"

#include <QStringList>

using namespace Qt::StringLiterals;

namespace Fooyin::KeyAnalyzer {

namespace {

bool containsKey(const QString& comment, const QString& key)
{
    const QStringList tokens = comment.split(u" "_s, Qt::SkipEmptyParts);
    for(const QString& token : tokens) {
        if(token.compare(key, Qt::CaseInsensitive) == 0)
            return true;
    }
    return false;
}

} // namespace

QString cleanComment(const QString& comment)
{
    return comment.trimmed();
}

QString makeComment(CommentMode mode, const QString& key, const QString& existing)
{
    const QString existingTrim = existing.trimmed();

    switch(mode) {
        case CommentMode::Overwrite:
            return key;
        case CommentMode::AppendStart:
            if(existingTrim.isEmpty() || containsKey(existingTrim, key))
                return existingTrim.isEmpty() ? key : existingTrim;
            return key + u" "_s + existingTrim;
        case CommentMode::AppendEnd:
            if(existingTrim.isEmpty() || containsKey(existingTrim, key))
                return existingTrim.isEmpty() ? key : existingTrim;
            return existingTrim + u" "_s + key;
    }
    return key;
}

} // namespace Fooyin::KeyAnalyzer