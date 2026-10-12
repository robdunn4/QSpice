#pragma once

#include <QString>
#include <QStringList>

// Locating and checking the QSpice installation.
//
// Programs built on these libraries depend on QSpice being installed:
// QUX.exe and the simulators are QSpice's, the Behavioral folder is part
// of the symbol library, and the colours come from its registry settings
// (QSpicePrefs, loaded separately). So the installation is verified once
// at startup, before any window is built, and a failure is reported
// rather than discovered piecemeal later.
//
// Every path here is derived from ONE value -- the VENDOR registry key,
// supplied by the program -- so a non-standard installation needs one
// setting changed, not several.
namespace sch::link::install {

// Reads the registry and checks the filesystem. Fills in the paths below
// as it goes.
//
// Returns every problem found, not just the first: a user fixing an
// installation should see the whole list at once rather than one dialog
// per restart. Empty means everything checked out.
//
// Does NOT load QSpicePrefs -- call prefs::load() with the same key.
QStringList verify(const QString &vendorKey);

// Valid after a successful verify(); empty otherwise.
//
//   installPath()    -- the QSpice installation folder, from the
//                       (Default) value of <vendor key>\InstallDir
//   quxExePath()     -- installPath\QUX.exe
//   behavioralPath() -- installPath\Behavioral
const QString &installPath();
const QString &quxExePath();
const QString &behavioralPath();

// installPath\Qorvo, or empty if that folder does not exist.
//
// OPTIONAL, unlike the paths above: verify() does not report it
// missing, since not every installation carries the Qorvo set and there
// is nothing for the user to fix.
const QString &qorvoPath();

// installPath\<fileName> -- a program or file shipped with QSpice.
// Empty when the installation folder is not known. Says nothing about
// whether the file exists.
QString filePath(const QString &fileName);

// The three programs one simulation needs: QUX.exe to netlist, the
// simulator FastMath selects (QSPICE64.exe when true, QSPICE80.exe when
// false), and QPOST.exe.
struct SimPrograms {
  QString     quxExe;
  QString     simExe;
  QString     qpostExe;
  QStringList missing; // native paths (or bare names) that were not found
};

// Fills in all three and lists any that do not exist. Nothing can be
// simulated unless `missing` is empty.
SimPrograms simPrograms(bool fastMath);

} // namespace sch::link::install
