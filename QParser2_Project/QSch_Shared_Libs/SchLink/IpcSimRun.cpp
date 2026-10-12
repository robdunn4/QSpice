#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable : 4996) // Qt headers reference APIs deprecated for Qt 7
#endif

#include "IpcSimRun.h"

#include <QDir>
#include <QFileInfo>
#include <QRegularExpression>
#include <QTimer>

#ifdef _MSC_VER
#pragma warning(pop)
#endif

#include "NetlistRun.h"
#include "SchStringDecode.h"

#ifdef Q_OS_WIN
// Before windows.h, always: it defines min and max as macros otherwise.
#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif

namespace sch::link {

namespace {

// How often the .qraw is looked at while the simulator runs -- QSpice's
// own timer, as near as the captures show.
constexpr int kQrawPollMs = 100;

// A display name made safe as a file name, for a never-saved schematic.
QString fileSafe(const QString &name) {
  QString safe = name;
  safe.replace(QRegularExpression(QStringLiteral(R"([<>:"/\\|?*\x00-\x1F])")),
               QStringLiteral("_"));
  return safe.isEmpty() ? QStringLiteral("Untitled") : safe;
}

} // namespace

IpcSimRun::IpcSimRun(QObject *parent) : QObject(parent) {}

IpcSimRun::~IpcSimRun() {
  // Nothing outlives the run. The netlister, if still going, is a child
  // and its own destructor stops QUX and removes its copy.
  for (QProcess *process : {sim, post}) {
    if (process == nullptr || process->state() == QProcess::NotRunning)
      continue;
    process->disconnect(this);
    process->kill();
    process->waitForFinished(2000);
  }
}

void IpcSimRun::start(const Params &p) {
  params  = p;
  running = true;
  aborted = false;

  // Where QSpice puts them: beside the schematic, same base name. A
  // never-saved schematic has no folder, so %TEMP% stands in -- for the
  // .qraw, and for the schematic path the simulator only uses to find
  // the folder for relative .inc/.lib statements.
  if (!params.schPath.isEmpty()) {
    const QFileInfo info(params.schPath);
    schArg = info.absoluteFilePath();
    qraw   = info.dir().filePath(info.completeBaseName() + QStringLiteral(".qraw"));
  } else {
    const QString base = fileSafe(QFileInfo(params.title).completeBaseName());
    schArg = QDir::temp().filePath(base + QStringLiteral(".qsch"));
    qraw   = QDir::temp().filePath(base + QStringLiteral(".qraw"));
  }

  netlistRun = new NetlistRun(params.tempPrefix, this);

  connect(netlistRun, &NetlistRun::succeeded, this,
          [this](const QByteArray &bytes, const QString &, const QString &) {
            onNetlist(bytes);
          });
  connect(netlistRun, &NetlistRun::failed, this,
          [this](const QString &message) { onNetlistFailed(message); });

  netlistRun->start(params.quxExe, params.schPath, params.title,
                    params.content, params.libPaths);
}

void IpcSimRun::abort() {
  if (!running) return;
  aborted = true;

  note(tr("Simulation aborted."));

  // Still netlisting: nothing else has started. Deleting the netlister
  // stops QUX and removes the temporary copy.
  if (sim == nullptr) {
    if (netlistRun != nullptr) {
      netlistRun->disconnect(this);
      delete netlistRun;
      netlistRun = nullptr;
    }
    finish(Outcome::Aborted);
    return;
  }

  // The finished() handlers see `aborted` and end the run there.
  if (sim->state() != QProcess::NotRunning) {
    sim->kill();
    return;
  }
  if (post != nullptr && post->state() != QProcess::NotRunning) post->kill();
}

void IpcSimRun::onNetlist(const QByteArray &bytes) {
  if (aborted) return;

  netlist = bytes;
  startSim();
}

void IpcSimRun::onNetlistFailed(const QString &message) {
  if (aborted) return;

  note(tr("Netlist failed: %1").arg(message));
  finish(Outcome::Failed);
}

void IpcSimRun::startSim() {
  const QStringList args = {
      QStringLiteral("-pipe"),
      QStringLiteral("-QUX"),
      QStringLiteral("-Op"),
      QDir::toNativeSeparators(schArg),
      QStringLiteral("-r"),
      QDir::toNativeSeparators(qraw),
      QStringLiteral("-hWnd=") + params.hwnd,
  };

  // A .qraw older than this is the LAST run's. A second's slack for
  // file-time granularity.
  simStarted = QDateTime::currentDateTime().addSecs(-1);
  qrawReady  = false;

  sim = launch(params.simExe, args);
  if (!running) return; // failed to start

  simProcessId = sim->processId();

  connect(sim, &QProcess::finished, this,
          [this](int exitCode, QProcess::ExitStatus status) {
            onSimFinished(exitCode, status);
          });

  qrawTimer = new QTimer(this);
  qrawTimer->setInterval(kQrawPollMs);
  connect(qrawTimer, &QTimer::timeout, this, [this]() { checkQraw(); });
  qrawTimer->start();
}

void IpcSimRun::checkQraw() {
  if (qrawReady || aborted) return;

  const QFileInfo info(qraw);
  if (!info.exists() || info.size() == 0) return;
  if (info.lastModified() < simStarted) return;

  qrawReady = true;
  if (qrawTimer != nullptr) qrawTimer->stop();

  if (onQrawReady) onQrawReady();
}

void IpcSimRun::onSimFinished(int exitCode, QProcess::ExitStatus status) {
  if (qrawTimer != nullptr) qrawTimer->stop();

  noteOutput(sim, false);

  if (aborted) {
    finish(Outcome::Aborted);
    return;
  }

  if (status == QProcess::CrashExit) {
    note(tr("%1 crashed.").arg(QFileInfo(params.simExe).fileName()));
    finish(Outcome::Failed);
    return;
  }

  // The simulator prints its own summary; only a failure is worth adding.
  if (exitCode != 0)
    note(tr("%1 exit code %2.")
             .arg(QFileInfo(params.simExe).fileName())
             .arg(exitCode));

  // Too quick for the timer: the viewer still gets its data.
  checkQraw();
  if (onSimExited) onSimExited();

  // The callbacks may have aborted the run.
  if (aborted || !running) return;

  // QSpice runs QPOST after EVERY simulation, .meas or not.
  startPost();
}

void IpcSimRun::startPost() {
  const QStringList args = {
      QStringLiteral("-pipe"),
      QStringLiteral("-r"),
      QDir::toNativeSeparators(qraw),
      QStringLiteral("-hWnd=") + params.hwnd,
  };

  post = launch(params.qpostExe, args);

  connect(post, &QProcess::finished, this,
          [this](int exitCode, QProcess::ExitStatus status) {
            onPostFinished(exitCode, status);
          });
}

void IpcSimRun::onPostFinished(int exitCode, QProcess::ExitStatus status) {
  noteOutput(post, true);

  if (aborted) {
    finish(Outcome::Aborted);
    return;
  }

  if (status == QProcess::CrashExit)
    note(tr("QPOST.exe crashed."), true);
  else if (exitCode != 0)
    note(tr("QPOST.exe exit code %1.").arg(exitCode), true);

  finish(Outcome::Completed);
}

void IpcSimRun::finish(Outcome outcome) {
  if (!running) return;
  running = false;

  // LAST: the receiver may delete this object (later).
  if (onDone) onDone(outcome);
}

QProcess *IpcSimRun::launch(const QString &exe, const QStringList &args) {
  QProcess *process = new QProcess(this);
  process->setProgram(exe);
  process->setArguments(args);
  process->setWorkingDirectory(QFileInfo(schArg).absolutePath());
  process->setProcessChannelMode(QProcess::MergedChannels);

#ifdef Q_OS_WIN
  // Console programs: no console window of their own. Their text comes
  // as window messages anyway.
  process->setCreateProcessArgumentsModifier(
      [](QProcess::CreateProcessArguments *a) { a->flags |= CREATE_NO_WINDOW; });
#endif

  // Drained as it arrives -- an unread pipe that fills would stall the
  // process -- and kept for noteOutput().
  connect(process, &QProcess::readyRead, process, [process]() {
    const QByteArray more = process->readAll();
    process->setProperty("ipcOut", process->property("ipcOut").toByteArray() + more);
  });

  connect(process, &QProcess::errorOccurred, this,
          [this, process, exe](QProcess::ProcessError error) {
            if (error != QProcess::FailedToStart) return;
            note(tr("Could not start %1: %2")
                     .arg(QDir::toNativeSeparators(exe), process->errorString()),
                 exe == params.qpostExe); // `post` is not set yet
            finish(Outcome::Failed);
          });

  process->start();

  // Failed to start: errorOccurred has already ended the run.
  if (process->state() == QProcess::NotRunning) return process;

  // Buffered by QProcess until the process is up; closing the channel
  // then gives it end-of-file, which is how it knows the netlist ended.
  process->write(netlist);
  process->closeWriteChannel();

  return process;
}

void IpcSimRun::note(const QString &line, bool toPost) {
  if (onNote) onNote(line, toPost);
}

void IpcSimRun::noteOutput(QProcess *process, bool toPost) {
  if (process == nullptr) return;

  QByteArray out = process->property("ipcOut").toByteArray() + process->readAll();
  out            = out.trimmed();
  if (out.isEmpty()) return;

  note(sch::core::decodeCp1252(out), toPost);
}

} // namespace sch::link
