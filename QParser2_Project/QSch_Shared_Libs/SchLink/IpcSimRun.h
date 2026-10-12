#pragma once

#include <QByteArray>
#include <QDateTime>
#include <QObject>
#include <QProcess>
#include <QString>
#include <QStringList>

#include <functional>

class QTimer;

namespace sch::link {

class NetlistRun;

// One simulation, start to finish -- QSpice's own sequence, as captured:
//
//   1. netlist the schematic (NetlistRun: QUX -Netlist, with the
//      temporary copy for unsaved changes and the .libpath block)
//   2. QSPICE64.exe or QSPICE80.exe  -pipe -QUX -Op "<sch>" -r "<qraw>"
//      -hWnd=<decimal>, the netlist written to its stdin
//   3. once it exits normally, QPOST.exe -pipe -r "<qraw>" -hWnd=<...>,
//      the same netlist on stdin
//
// The simulator and QPOST report their console and status text as
// WINDOW MESSAGES to the -hWnd window (the caller's), not through this class
// -- see ipcmsg. This class reports only the run's own milestones.
//
// While the simulator runs, the .qraw is watched (Appendix I, step 3):
// once it holds data from THIS run, onQrawReady fires -- once -- and the
// waveform viewer can be launched or told to reopen. A simulation too
// quick for the timer is caught when the simulator exits.
//
// abort() stops whatever is running; QPOST is then skipped. Destroying
// the object aborts too.
//
// No signals of its own -- callbacks instead -- so no moc.
class IpcSimRun : public QObject {
public:
  struct Params {
    QString     quxExe;     // for netlisting
    QString     simExe;     // QSPICE64.exe or QSPICE80.exe
    QString     qpostExe;
    QString     schPath;    // the schematic's file, empty if never saved
    QString     title;      // its display name
    QByteArray  content;    // serialized schematic, or empty: use schPath
    QStringList libPaths;   // user symbol folders, for .libpath lines
    QString     hwnd;       // the -hWnd window, decimal
    QString     tempPrefix; // NetlistRun's temporary-copy prefix, "~<Program>_"
  };

  enum class Outcome { Completed, Aborted, Failed };

  // A line of the run's OWN commentary for a console -- a milestone,
  // an exit code, an error. `toPost` selects the Post Process console.
  std::function<void(const QString &line, bool toPost)> onNote;

  // Called once, at the very end.
  std::function<void(Outcome outcome)> onDone;

  // The .qraw has this run's data: time for the viewer. At most once a
  // run; not after an abort.
  std::function<void()> onQrawReady;

  // The simulator has exited normally -- before QPOST starts. The viewer
  // is told to stop marching.
  std::function<void()> onSimExited;

  explicit IpcSimRun(QObject *parent = nullptr);
  ~IpcSimRun() override;

  void start(const Params &params);
  void abort();

  bool isRunning() const { return running; }

  // The simulator process itself is still running.
  bool simRunning() const {
    return sim != nullptr && sim->state() != QProcess::NotRunning;
  }

  // The .qraw this run writes -- for the waveform viewer, later.
  const QString &qrawPath() const { return qraw; }

  // The simulator's process id, from when it started; 0 before then.
  qint64 simPid() const { return simProcessId; }

protected:
  void onNetlist(const QByteArray &bytes);
  void onNetlistFailed(const QString &message);
  void startSim();
  void onSimFinished(int exitCode, QProcess::ExitStatus status);
  void startPost();
  void onPostFinished(int exitCode, QProcess::ExitStatus status);
  void finish(Outcome outcome);

  // The .qraw holds data written since this run started. Fires
  // onQrawReady the first time it does.
  void checkQraw();

  // A process with the netlist on its stdin. Its own stdout/stderr are
  // read as they come -- an unread pipe that fills would stall it -- and
  // whatever arrived is reported when it exits.
  QProcess *launch(const QString &exe, const QStringList &args);
  void      note(const QString &line, bool toPost = false);

  // Whatever a process printed itself, as one note, if anything.
  void noteOutput(QProcess *process, bool toPost);

  Params      params;
  QString     qraw;
  QString     schArg; // the "<Schematic Path>" argument
  QByteArray  netlist;
  NetlistRun *netlistRun = nullptr;
  QProcess   *sim        = nullptr;
  QProcess   *post       = nullptr;
  QTimer     *qrawTimer  = nullptr;
  QDateTime   simStarted;
  qint64      simProcessId = 0;
  bool        qrawReady  = false;
  bool        running    = false;
  bool        aborted    = false;
};

} // namespace sch::link
