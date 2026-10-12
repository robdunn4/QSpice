#pragma once

#include <QString>

// QSpice's own help file, QSPICE.chm, in the QSpice installation folder.
//
// Opened through the HTML Help API rather than by running hh.exe, for
// two reasons: the help window then belongs to THIS process, so it
// closes when the program does, and the API can raise an already-open
// window instead of opening a second one.
//
// hhctrl.ocx is loaded at RUN time, so there is no link-time dependency
// on it and a machine without HTML Help simply gets a message rather
// than failing to start.
//
// No dialogs: a failure comes back as text for the caller to show.
namespace sch::link::help {

// installPath\QSPICE.chm; empty when the installation folder is not
// known. Says nothing about whether the file exists.
QString chmPath();

// Reads page-name overrides from an index file (QSpiceHelpNdx.ini, kept
// beside the program's settings), [SymbolTypes] section, "<type>=<page>".
//
// The built-in mapping is correct for the help shipped today, so the
// file is a PATCH mechanism rather than the source of truth: it need
// contain only what QSpice has since renamed, and help works unchanged
// when the file is absent. Replaces any overrides read before.
void loadOverrides(const QString &iniPath);

// The help page for a symbol type, as the «type:» record stores it:
// "R", "QN", "MN", "Ø(.dll)". Empty when there is no page for it.
//
// Mostly a RULE rather than a table -- the CHM names its device pages
// after the SPICE letter, so stripping a trailing N or P covers every
// polarity pair -- which is why it keeps working when QSpice adds a
// device. Only the handful that break the rule are listed, and the
// overrides come first.
QString pageForSymbolType(const QString &type);

// Opens the help at one topic -- a page name inside the CHM, such as
// "R-device.htm" -- or at its default page when pageName is empty. A
// topic already showing is brought forward.
//
// Returns empty on success, otherwise why the help could not be shown.
QString show(const QString &pageName);

// Closes any help window this process opened.
//
// Only OURS: a copy of the help the user opened from QSpice itself is
// left alone.
void closeAll();

} // namespace sch::link::help
