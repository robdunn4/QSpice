#pragma once

#include <QString>

// The program's log, <programName>.log next to the executable: a Qt
// message handler that writes every qDebug/qInfo/qWarning/qCritical/qFatal
// message there IN ADDITION to its normal destination (the debugger's
// Output window, or stderr off Windows; and for fatal messages the usual
// abort()/debug-break). Q_ASSERT/Q_ASSERT_X failures route through
// qFatal(), so they are captured too.
//
// OFF unless the program's .ini says otherwise: [Log] MaxSizeKB, where 0
// means no log file. Each program gives its own default -- 0 in a release
// build, so nothing accumulates beside an installed program, and a size in
// a debug build. A user turns it on to report a problem.
//
// One file shared by every instance, APPENDED, each line tagged with the
// process id. When it passes MaxSizeKB it is renamed <programName>.log.old
// (replacing any older one) and a fresh file is started, so the previous
// history survives one rotation.
//
// Two steps, early in main(), after constructing QApplication (the log's
// folder is QCoreApplication::applicationDirPath()):
//
//   install()    -- at once: messages from here on are HELD in memory;
//   configure()  -- once the .ini is known: the held messages are written
//                   (or, with logging off, dropped), and later ones follow.
//
// Messages always reach the debugger output, whether or not there is a file.
namespace sch::app::log {

// The [Log] key, for the program's settings documentation.
inline constexpr const char *kLogGroup      = "Log";
inline constexpr const char *kMaxSizeKBKey  = "MaxSizeKB";

// The smallest size a nonzero MaxSizeKB is raised to.
inline constexpr int kMinMaxSizeKB = 64;

void install(const QString &programName);

// Reads [Log] MaxSizeKB from `iniPath` ("[]" or missing: `defaultMaxKB`),
// writes the key if the file does not have it, and starts -- or turns off
// -- the file log accordingly. Call once.
void configure(const QString &iniPath, int defaultMaxKB);

} // namespace sch::app::log
