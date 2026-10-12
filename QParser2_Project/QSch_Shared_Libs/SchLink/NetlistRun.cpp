#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable : 4996) // Qt headers reference APIs deprecated for Qt 7
#endif

#include "NetlistRun.h"

#include <QCoreApplication>
#include <QDateTime>
#include <QDebug>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QRegularExpression>
#include <QStringDecoder>
#include <QStringList>
#include <QTimer>

#ifdef _MSC_VER
#pragma warning(pop)
#endif

#include "SchStringDecode.h"

#ifdef Q_OS_WIN
// Before windows.h, always: it defines min and max as macros otherwise.
#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif

namespace sch::link {

namespace {

// Long enough for a large hierarchical design on a slow disk; short
// enough that a QUX stuck on a dialog nobody can see is not left
// running for good.
constexpr int kTimeoutMs = 60000;

// Leftover copies older than this are assumed abandoned. Generous, so
// two instances working in one folder never sweep each
// other's live copy.
constexpr qint64 kStaleSecs = 24 * 60 * 60;

// A display name made safe as part of a file name: an untitled
// document's tab label could in principle hold anything.
QString fileSafe(const QString &name) {
  QString safe = name;
  safe.replace(QRegularExpression(QStringLiteral(R"([<>:"/\\|?*\x00-\x1F])")),
               QStringLiteral("_"));
  return safe.isEmpty() ? QStringLiteral("Untitled") : safe;
}

} // namespace

NetlistRun::NetlistRun(const QString &prefix, QObject *parent)
    : QObject(parent), tempPrefix(prefix) {
  watchdog = new QTimer(this);
  watchdog->setSingleShot(true);
  connect(watchdog, &QTimer::timeout, this, &NetlistRun::onTimeout);
}

NetlistRun::~NetlistRun() {
  // QProcess's own destructor kills a QUX still running, but the copy
  // must not outlive it -- so stop it here, first, and then delete.
  if (process != nullptr && process->state() != QProcess::NotRunning) {
    process->disconnect(this);
    process->kill();
    process->waitForFinished(2000);
  }

  removeTempCopy();
}

void NetlistRun::start(const QString &quxExe, const QString &schPath,
                       const QString &title, const QByteArray &content,
                       const QStringList &libPaths) {
  userLibPaths = libPaths;

  realBase = schPath.isEmpty() ? QFileInfo(title).completeBaseName()
                               : QFileInfo(schPath).completeBaseName();
  if (realBase.isEmpty()) realBase = QStringLiteral("Untitled");

  QString netlistPath = schPath;

  // Unsaved changes: QUX gets a copy beside the original -- see the
  // header for why beside, not in %TEMP%.
  if (!content.isEmpty()) {
    const QString dir = schPath.isEmpty() ? QDir::tempPath()
                                          : QFileInfo(schPath).absolutePath();
    sweepStaleCopies(dir);

    QString error;
    netlistPath = writeTempCopy(dir, fileSafe(realBase), content, error);

    if (netlistPath.isEmpty()) {
      fail(error);
      return;
    }
  }

  if (netlistPath.isEmpty()) {
    fail(tr("There is no schematic file to netlist."));
    return;
  }

  process = new QProcess(this);
  process->setProgram(quxExe);
  process->setArguments({QStringLiteral("-Netlist"),
                         QDir::toNativeSeparators(netlistPath),
                         QStringLiteral("-stdout")});
  process->setWorkingDirectory(QFileInfo(netlistPath).absolutePath());

#ifdef Q_OS_WIN
  // QUX is a GUI program. Asked for a netlist on stdout it should not
  // open a window, but if it ever does -- an error box, say -- the
  // window starts hidden rather than flashing up, and the watchdog
  // ends it.
  process->setCreateProcessArgumentsModifier(
      [](QProcess::CreateProcessArguments *args) {
        args->startupInfo->dwFlags |= STARTF_USESHOWWINDOW;
        args->startupInfo->wShowWindow = SW_HIDE;
      });
#endif

  connect(process, &QProcess::finished, this, &NetlistRun::onFinished);
  connect(process, &QProcess::errorOccurred, this,
          &NetlistRun::onErrorOccurred);

  watchdog->start(kTimeoutMs);
  process->start();
}

void NetlistRun::onFinished(int exitCode, QProcess::ExitStatus status) {
  if (runOver) return;
  runOver = true;
  watchdog->stop();

  // The netlist is kept as QUX's own BYTES. It is what the simulator
  // will be fed, and QUX writes it in its own single-byte character set
  // (the help warns that -stdout output is not terminal-safe: µ, Ω and
  // the like). Decoding to text and encoding back could change those
  // bytes, so text is derived from the bytes for display only.
  QByteArray      raw = process->readAllStandardOutput();
  const QString   err = decodeOutput(process->readAllStandardError()).trimmed();

  removeTempCopy();

  const QString errPart =
      err.isEmpty() ? QString() : QStringLiteral("\n\n") + err;

  if (status == QProcess::CrashExit) {
    emit failed(tr("QUX.exe crashed while netlisting.%1").arg(errPart));
    return;
  }

  emit exited(exitCode);

  if (raw.trimmed().isEmpty()) {
    emit failed(tr("QUX.exe produced no netlist (exit code %1).%2")
                    .arg(QString::number(exitCode), errPart));
    return;
  }

  // The temporary name, wherever QUX echoed it, becomes the real one --
  // as bytes, in the netlist's own encoding.
  if (!tempBase.isEmpty())
    raw.replace(netlistBytes(tempBase), netlistBytes(realBase));

  raw = withLibPaths(raw, userLibPaths);

  QStringList notes;
  if (exitCode != 0) notes << tr("QUX.exe exit code %1.").arg(exitCode);
  if (!err.isEmpty()) notes << err;

  emit succeeded(raw, decodeOutput(raw), notes.join(QLatin1Char('\n')));
}

void NetlistRun::onErrorOccurred(QProcess::ProcessError error) {
  // A crash or a kill also arrives through finished(), which reports it
  // with whatever output there was. Only a failure to start never does.
  if (error != QProcess::FailedToStart) return;

  fail(tr("Could not start QUX.exe:\n%1\n\n%2")
           .arg(QDir::toNativeSeparators(process->program()),
                process->errorString()));
}

void NetlistRun::onTimeout() {
  if (runOver) return;

  process->disconnect(this);
  process->kill();
  process->waitForFinished(2000);

  fail(tr("QUX.exe did not finish within %1 seconds and was stopped.")
           .arg(kTimeoutMs / 1000));
}

QByteArray NetlistRun::withLibPaths(const QByteArray &netlist,
                                    const QStringList &libPaths) {
  if (libPaths.isEmpty()) return netlist;

  // After QUX's first line (its "* ..." title), as QSpice does it:
  //
  //     * <title>
  //     <blank>
  //     .libpath "<folder>"      one per user symbol folder, as entered
  //     <blank>
  //     <the rest of QUX's netlist, unchanged>
  //
  // Lines end the way QUX ends its own, so the result is not a mixture.
  const qsizetype eol = netlist.indexOf('\n');
  if (eol < 0) return netlist; // no title line -- nothing sensible to do

  const QByteArray lineEnd =
      (eol > 0 && netlist.at(eol - 1) == '\r') ? QByteArray("\r\n")
                                                : QByteArray("\n");

  QByteArray block = lineEnd;
  for (const QString &libPath : libPaths)
    block += ".libpath \"" + netlistBytes(libPath) + "\"" + lineEnd;
  block += lineEnd;

  return netlist.left(eol + 1) + block + netlist.mid(eol + 1);
}

QByteArray NetlistRun::netlistBytes(const QString &text) {
  // Windows-1252, QSpice's own single-byte set. A character it cannot
  // represent (Greek, CJK in a folder name) falls back to UTF-8 rather
  // than being lost; whether the simulator then finds that path is
  // untested.
  bool             fits  = false;
  const QByteArray bytes = sch::core::encodeCp1252(text, &fits);
  return fits ? bytes : text.toUtf8();
}

void NetlistRun::fail(const QString &message) {
  if (runOver) return;
  runOver = true;
  watchdog->stop();

  removeTempCopy();
  emit failed(message);
}

QString NetlistRun::writeTempCopy(const QString &dir, const QString &base,
                                  const QByteArray &content, QString &error) {
  static int counter = 0;
  ++counter;

  tempBase = QStringLiteral("%1%2_%3_%4")
                 .arg(tempPrefix)
                 .arg(QCoreApplication::applicationPid())
                 .arg(counter)
                 .arg(base);

  const QString path = QDir(dir).filePath(tempBase + QStringLiteral(".qsch"));

  QFile file(path);
  if (!file.open(QIODevice::WriteOnly)) {
    error = tr("Could not write the temporary copy for QUX:\n%1\n\n%2")
                .arg(QDir::toNativeSeparators(path), file.errorString());
    tempBase.clear();
    return QString();
  }

  const qint64 written = file.write(content);
  file.close();

  // Recorded before checking the write, so a partial file is removed too.
  tempPath = path;

  if (written != content.size()) {
    error = tr("Could not write the temporary copy for QUX:\n%1\n\n%2")
                .arg(QDir::toNativeSeparators(path), file.errorString());
    removeTempCopy();
    return QString();
  }

#ifdef Q_OS_WIN
  // Hidden from the user's folder view; TEMPORARY asks Windows to keep
  // it in the cache rather than write it to disk. Neither matters to
  // the result, so a failure here is ignored.
  const QString native = QDir::toNativeSeparators(path);
  SetFileAttributesW(reinterpret_cast<LPCWSTR>(native.utf16()),
                     FILE_ATTRIBUTE_HIDDEN | FILE_ATTRIBUTE_TEMPORARY);
#endif

  return path;
}

void NetlistRun::removeTempCopy() {
  if (tempPath.isEmpty()) return;

  if (!QFile::remove(tempPath))
    qWarning().noquote() << "Netlist: could not delete temporary copy"
                         << QDir::toNativeSeparators(tempPath);

  tempPath.clear();
}

QString NetlistRun::decodeOutput(const QByteArray &bytes) {
  // For DISPLAY only. Windows-1252 unless the bytes are valid UTF-8 --
  // pure ASCII reads the same either way, and a lone 1252 byte like µ
  // (0xB5) is never valid UTF-8, so the test cannot mistake one for the
  // other in practice. Explicit 1252 rather than the ANSI code page,
  // which is not 1252 on every machine.
  QStringDecoder decoder(QStringDecoder::Utf8);
  QString        text = decoder(bytes);

  if (decoder.hasError()) text = sch::core::decodeCp1252(bytes);

  text.replace(QStringLiteral("\r\n"), QStringLiteral("\n"));
  return text;
}

void NetlistRun::sweepStaleCopies(const QString &dir) const {
  const QDateTime cutoff = QDateTime::currentDateTime().addSecs(-kStaleSecs);

  const QFileInfoList leftovers = QDir(dir).entryInfoList(
      {tempPrefix + QStringLiteral("*.qsch")}, QDir::Files | QDir::Hidden);

  for (const QFileInfo &info : leftovers) {
    if (info.lastModified() > cutoff) continue;
    QFile::remove(info.absoluteFilePath());
  }
}

} // namespace sch::link
