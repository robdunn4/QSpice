#pragma once

#include <QString>

// Launching QSpice's own utility programs -- the model generators and
// the sound-card tools -- which live beside QUX.exe in the QSpice
// installation.
//
// They are INDEPENDENT programs, not helpers of ours: closing our
// program must leave them running, so they are started detached, with
// no parent link to this process.
//
// No dialogs: a failure comes back as text for the caller to show.
namespace sch::link::tools {

// Runs `exeName` from the QSpice installation folder, or brings the
// running copy to the front if there is one.
//
// `workingDir` is where the program starts -- the schematic's folder,
// so anything it writes lands beside the design rather than in the
// installation. Empty means the installation folder.
//
// Returns empty on success, otherwise why it could not be started.
QString run(const QString &exeName, const QString &workingDir);

} // namespace sch::link::tools
