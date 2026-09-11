/*
 * fooyin-plugins - Key Analyzer plugin
 * Copyright © 2026
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

// Unit tests for the comment assembly logic (keyanalyzercomments).
// Pure logic only — no fooyin libs needed. Built via KEYANALYZER_BUILD_TESTS.

#include "keyanalyzercomments.h"

#include <QString>

#include <cstdio>
#include <string>

using namespace Fooyin::KeyAnalyzer;

static int failures = 0;

static std::string to(const QString& s)
{
    return s.toStdString();
}

#define CHECK(desc, expr)                                                                          \
    do {                                                                                           \
        if(expr) {                                                                                 \
            std::puts("PASS  " desc);                                                              \
        }                                                                                          \
        else {                                                                                     \
            std::printf("FAIL  %s\n", desc);                                                       \
            ++failures;                                                                            \
        }                                                                                          \
    } while(false)

#define CHECK_EQ(desc, got, want)                                                                  \
    do {                                                                                           \
        const std::string g = to(got);                                                             \
        const std::string w = want;                                                                \
        if(g == w) {                                                                               \
            std::puts("PASS  " desc);                                                       \
        }                                                                                          \
        else {                                                                                     \
            std::printf("FAIL  %s (got \"%s\" want \"%s\")\n", desc, g.c_str(), w.c_str());        \
            ++failures;                                                                            \
        }                                                                                          \
    } while(false)

int main()
{
    // ---- Overwrite ----
    CHECK_EQ("overwrite replaces existing",
             makeComment(CommentMode::Overwrite, "8B", "old comment"), "8B");
    CHECK_EQ("overwrite with empty comment",
             makeComment(CommentMode::Overwrite, "8B", {}), "8B");
    CHECK_EQ("overwrite with whitespace comment",
             makeComment(CommentMode::Overwrite, "8B", "   "), "8B");

    // ---- Append start ----
    CHECK_EQ("append-start fills empty comment",
             makeComment(CommentMode::AppendStart, "5A", {}), "5A");
    CHECK_EQ("append-start with plain comment",
             makeComment(CommentMode::AppendStart, "5A", "played in A minor"),
             "5A played in A minor");
    CHECK_EQ("append-start trims existing whitespace",
             makeComment(CommentMode::AppendStart, "5A", "  played  in A minor  "),
             "5A played  in A minor");
    CHECK_EQ("append-start keeps single space intact",
             makeComment(CommentMode::AppendStart, "5A", "played in A minor"),
             "5A played in A minor");

    // ---- Append end ----
    CHECK_EQ("append-end fills empty comment",
             makeComment(CommentMode::AppendEnd, "7B", {}), "7B");
    CHECK_EQ("append-end with plain comment",
             makeComment(CommentMode::AppendEnd, "7B", "some notes"),
             "some notes 7B");
    CHECK_EQ("append-end trims trailing whitespace",
             makeComment(CommentMode::AppendEnd, "7B", "some notes   "),
             "some notes 7B");

    // ---- Duplicate-marker guard ----
    CHECK_EQ("append-start skips when key already present",
             makeComment(CommentMode::AppendStart, "5A", "played 5A on stage"),
             "played 5A on stage");
    CHECK_EQ("append-end skips when key already present",
             makeComment(CommentMode::AppendEnd, "5A", "key is 5A"),
             "key is 5A");
    CHECK_EQ("key match is case-insensitive",
             makeComment(CommentMode::AppendEnd, "5A", "key is 5a"),
             "key is 5a");
    CHECK("no false positive on partial token (5Amp)",
          makeComment(CommentMode::AppendEnd, "5A", "big 5Amp band") == QStringLiteral("big 5Amp band 5A"));

    // ---- cleanComment ----
    CHECK_EQ("cleanComment trims both sides", cleanComment(QStringLiteral("  x  ")), "x");
    CHECK_EQ("cleanComment on empty", cleanComment(QString{}), "");

    if(failures == 0)
        std::puts("ALL PASS");
    return failures == 0 ? 0 : 1;
}