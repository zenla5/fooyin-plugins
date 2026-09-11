/*
 * fooyin-plugins - Key Analyzer plugin
 * Copyright © 2026
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#include "keyanalyzernotations.h"

#include <array>

using namespace Qt::StringLiterals;

namespace Fooyin::KeyAnalyzer {

namespace {

constexpr std::array<const char *, 12> ToneNames = {
    "A", "Bb", "B", "C", "Db", "D", "Eb", "E", "F", "Gb", "G", "Ab"
};

const KeyMap &standardMap()
{
    static const KeyMap map{
        {KeyFinder::A_MAJOR, QStringLiteral("A")},       {KeyFinder::A_MINOR, QStringLiteral("Am")},
        {KeyFinder::B_FLAT_MAJOR, QStringLiteral("Bb")}, {KeyFinder::B_FLAT_MINOR, QStringLiteral("Bbm")},
        {KeyFinder::B_MAJOR, QStringLiteral("B")},       {KeyFinder::B_MINOR, QStringLiteral("Bm")},
        {KeyFinder::C_MAJOR, QStringLiteral("C")},       {KeyFinder::C_MINOR, QStringLiteral("Cm")},
        {KeyFinder::D_FLAT_MAJOR, QStringLiteral("Db")}, {KeyFinder::D_FLAT_MINOR, QStringLiteral("Dbm")},
        {KeyFinder::D_MAJOR, QStringLiteral("D")},       {KeyFinder::D_MINOR, QStringLiteral("Dm")},
        {KeyFinder::E_FLAT_MAJOR, QStringLiteral("Eb")}, {KeyFinder::E_FLAT_MINOR, QStringLiteral("Ebm")},
        {KeyFinder::E_MAJOR, QStringLiteral("E")},       {KeyFinder::E_MINOR, QStringLiteral("Em")},
        {KeyFinder::F_MAJOR, QStringLiteral("F")},       {KeyFinder::F_MINOR, QStringLiteral("Fm")},
        {KeyFinder::G_FLAT_MAJOR, QStringLiteral("Gb")}, {KeyFinder::G_FLAT_MINOR, QStringLiteral("Gbm")},
        {KeyFinder::G_MAJOR, QStringLiteral("G")},       {KeyFinder::G_MINOR, QStringLiteral("Gm")},
        {KeyFinder::A_FLAT_MAJOR, QStringLiteral("Ab")}, {KeyFinder::A_FLAT_MINOR, QStringLiteral("Abm")},
    };
    return map;
}

const KeyMap &camelotMap()
{
    static const KeyMap map{
        {KeyFinder::A_MAJOR, QStringLiteral("11B")}, {KeyFinder::A_MINOR, QStringLiteral("8A")},
        {KeyFinder::B_FLAT_MAJOR, QStringLiteral("6B")}, {KeyFinder::B_FLAT_MINOR, QStringLiteral("3A")},
        {KeyFinder::B_MAJOR, QStringLiteral("1B")},     {KeyFinder::B_MINOR, QStringLiteral("10A")},
        {KeyFinder::C_MAJOR, QStringLiteral("8B")},     {KeyFinder::C_MINOR, QStringLiteral("5A")},
        {KeyFinder::D_FLAT_MAJOR, QStringLiteral("3B")}, {KeyFinder::D_FLAT_MINOR, QStringLiteral("12A")},
        {KeyFinder::D_MAJOR, QStringLiteral("10B")},    {KeyFinder::D_MINOR, QStringLiteral("7A")},
        {KeyFinder::E_FLAT_MAJOR, QStringLiteral("5B")}, {KeyFinder::E_FLAT_MINOR, QStringLiteral("2A")},
        {KeyFinder::E_MAJOR, QStringLiteral("12B")},    {KeyFinder::E_MINOR, QStringLiteral("9A")},
        {KeyFinder::F_MAJOR, QStringLiteral("7B")},     {KeyFinder::F_MINOR, QStringLiteral("4A")},
        {KeyFinder::G_FLAT_MAJOR, QStringLiteral("2B")}, {KeyFinder::G_FLAT_MINOR, QStringLiteral("11A")},
        {KeyFinder::G_MAJOR, QStringLiteral("9B")},     {KeyFinder::G_MINOR, QStringLiteral("6A")},
        {KeyFinder::A_FLAT_MAJOR, QStringLiteral("4B")}, {KeyFinder::A_FLAT_MINOR, QStringLiteral("1A")},
    };
    return map;
}

const KeyMap &openKeyMap()
{
    static const KeyMap map{
        {KeyFinder::A_MAJOR, QStringLiteral("4d")},      {KeyFinder::A_MINOR, QStringLiteral("1m")},
        {KeyFinder::B_FLAT_MAJOR, QStringLiteral("11d")}, {KeyFinder::B_FLAT_MINOR, QStringLiteral("8m")},
        {KeyFinder::B_MAJOR, QStringLiteral("6d")},      {KeyFinder::B_MINOR, QStringLiteral("3m")},
        {KeyFinder::C_MAJOR, QStringLiteral("1d")},      {KeyFinder::C_MINOR, QStringLiteral("10m")},
        {KeyFinder::D_FLAT_MAJOR, QStringLiteral("8d")},  {KeyFinder::D_FLAT_MINOR, QStringLiteral("5m")},
        {KeyFinder::D_MAJOR, QStringLiteral("3d")},      {KeyFinder::D_MINOR, QStringLiteral("12m")},
        {KeyFinder::E_FLAT_MAJOR, QStringLiteral("10d")}, {KeyFinder::E_FLAT_MINOR, QStringLiteral("7m")},
        {KeyFinder::E_MAJOR, QStringLiteral("5d")},      {KeyFinder::E_MINOR, QStringLiteral("2m")},
        {KeyFinder::F_MAJOR, QStringLiteral("12d")},     {KeyFinder::F_MINOR, QStringLiteral("9m")},
        {KeyFinder::G_FLAT_MAJOR, QStringLiteral("7d")},  {KeyFinder::G_FLAT_MINOR, QStringLiteral("4m")},
        {KeyFinder::G_MAJOR, QStringLiteral("2d")},      {KeyFinder::G_MINOR, QStringLiteral("11m")},
        {KeyFinder::A_FLAT_MAJOR, QStringLiteral("9d")},  {KeyFinder::A_FLAT_MINOR, QStringLiteral("6m")},
    };
    return map;
}

} // namespace

const KeyMap &notationMap(Notation notation)
{
    switch(notation) {
        case Notation::OpenKey:
            return openKeyMap();
        case Notation::Standard:
            return standardMap();
        case Notation::Camelot:
        default:
            return camelotMap();
    }
}

QString keyToNotation(KeyFinder::key_t key, Notation notation)
{
    if(key == KeyFinder::SILENCE)
        return {};
    const KeyMap &map = notationMap(notation);
    const auto it = map.find(key);
    return it == map.cend() ? QString{} : it->second;
}

QString keyTheoryNote(KeyFinder::key_t key)
{
    if(key == KeyFinder::SILENCE || key < KeyFinder::A_MAJOR || key > KeyFinder::A_FLAT_MINOR)
        return {};

    const int pc            = key / 2;             // 0=A ... 11=Ab (even=Maj, odd=min)
    const bool isMajor      = (key % 2) == 0;
    const int relativePc    = isMajor ? ((pc - 3) + 12) % 12 : (pc + 3) % 12;

    const QString tonic     = QString::fromLatin1(ToneNames[pc]);
    const QString relTonic  = QString::fromLatin1(ToneNames[relativePc]);

    if(isMajor)
        return tonic + u" Major (rel. "_s + relTonic + u" minor)"_s;
    return tonic + u" minor (rel. "_s + relTonic + u" major)"_s;
}

} // namespace Fooyin::KeyAnalyzer
