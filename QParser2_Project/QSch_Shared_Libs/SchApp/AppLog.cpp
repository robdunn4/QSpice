#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable : 4996) // Qt headers reference APIs deprecated for Qt 7
#endif

#include "AppLog.h"

#include <QCoreApplication>
#include <QDateTime>
#include <QFile>
#include <QFileInfo>
#include <QMutex>
#include <QMutexLocker>
#include <QSettings>
#include <QStringList>
#include <QTextStream>

#ifdef Q_OS_WIN
#include <windows.h>
#endif

#ifdef _MSC_VER
#pragma warning(pop)
#endif

#include <cstdlib>

#include "IniValues.h"

namespace sch::app::log {
namespace {

// Set once, by install().
QString programName = QStringLiteral("program");

// Until configure(): messages held here. After it: the limit in bytes, 0
// for no file.
enum class State { Holding, Writing, Off };
State       state    = State::Holding;
qint64      maxBytes = 0;
QStringList held;

// Messages can come from any thread.
QMutex &logMutex() {
  static QMutex mutex;
  return mutex;
}

const QString &logFilePath() {
  // One fixed filename, not a timestamped one. The per-run timestamp
  // originally kept each run's messages in their own file, but it
  // scattered a debugging session across a directory of near-identical
  // files and left them to accumulate forever. A single file is easier
  // to tail, open in an editor, and attach to a report.
  //
  // Still computed once on first use (thread-safe magic statics) rather
  // than per call -- there is no reason to rebuild the string for every
  // message.
  static const QString path =
      QCoreApplication::applicationDirPath() + "/" +
      programName + QStringLiteral(".log");
  return path;
}

const char *severityLbl(QtMsgType type) {
  switch (type) {
  case QtDebugMsg:
    return "DEBUG";
  case QtInfoMsg:
    return "INFO";
  case QtWarningMsg:
    return "WARNING";
  case QtCriticalMsg:
    return "CRITICAL";
  case QtFatalMsg:
    return "FATAL";
  default:
    return "UNKNOWN";
  }
}

QString formatLogLine(QtMsgType type, const QMessageLogContext &context,
                      const QString &message) {
  const QString timestamp = QDateTime::currentDateTime().toString(
      QStringLiteral("yyyy-MM-dd HH:mm:ss.zzz"));

  QString location = QStringLiteral("unknown");
  if (context.file != nullptr) {
    location = QStringLiteral("%1:%2")
                   .arg(QString::fromUtf8(context.file))
                   .arg(context.line);
    if (context.function != nullptr) {
      location +=
          QStringLiteral(" (%1)").arg(QString::fromUtf8(context.function));
    }
  }

  return QStringLiteral("%1 <%2> [%3] %4 - %5")
      .arg(timestamp,
           QString::number(QCoreApplication::applicationPid()),
           QString::fromLatin1(severityLbl(type)), location, message);
}

void writeToLogFile(const QString &line) {
  // APPENDED, not truncated per run: with one process per schematic,
  // several instances share this file, and truncating at each start
  // wiped the log of the instance that mattered -- the one that ran
  // BEFORE the restart being investigated. Every line carries its pid
  // (see formatLogLine) so the runs can be told apart.
  //
  // The size is read from the FILE before each write, not counted here:
  // other instances write to it too, and one going by its own count
  // would rotate a file another had already rotated, losing the .old.
  if (QFileInfo(logFilePath()).size() > maxBytes) {
    const QString old = logFilePath() + QStringLiteral(".old");
    QFile::remove(old);
    QFile::rename(logFilePath(), old);
  }

  QFile file(logFilePath());
  if (file.open(QIODevice::Append | QIODevice::WriteOnly | QIODevice::Text)) {
    QTextStream stream(&file);
    stream << line << Qt::endl;
  }
  // If the log file can't be opened (e.g. read-only install
  // directory), there's nowhere safe left to report that here —
  // continue silently rather than risk recursing back into the
  // message handler.
}

void forwardToDebugOutput(const QString &line) {
  // Preserves the existing developer workflow (messages visible in
  // the debugger's Output window / console) — logging to file is
  // additive, not a replacement.
#ifdef Q_OS_WIN
  OutputDebugStringW(
      reinterpret_cast<const wchar_t *>((line + QStringLiteral("\n")).utf16()));
#else
  fprintf(stderr, "%s\n", qPrintable(line));
#endif
}

void qtMessageHandler(QtMsgType type, const QMessageLogContext &context,
                      const QString &message) {
  const QString line = formatLogLine(type, context, message);

  {
    const QMutexLocker lock(&logMutex());
    if (state == State::Holding) held.append(line);
    if (state == State::Writing) writeToLogFile(line);
  }
  forwardToDebugOutput(line);

  if (type == QtFatalMsg) {
    // Installing a custom message handler replaces Qt's default
    // handling entirely, including the abort() it would normally
    // perform after a fatal message. Q_ASSERT/Q_ASSERT_X failures
    // route through here, so this call is what still stops the
    // program (and triggers the usual debug-break under a
    // debugger) instead of silently continuing past a real
    // assertion failure.
    abort();
  }
}
} // namespace

void install(const QString &name) {
  if (!name.isEmpty()) programName = name;
  qInstallMessageHandler(qtMessageHandler);
}

void configure(const QString &iniPath, int defaultMaxKB) {
  QSettings settings(iniPath, QSettings::IniFormat);
  settings.beginGroup(QString::fromLatin1(kLogGroup));

  // Missing only: the file is shared, and may have been edited by hand.
  {
    const ini::OnlyMissing scope;
    ini::setPrefValue(settings, kMaxSizeKBKey, QString::number(defaultMaxKB),
                      QString::number(defaultMaxKB));
  }

  bool      ok    = false;
  const int maxKB = ini::prefValue(settings, kMaxSizeKBKey, QString::number(defaultMaxKB))
                        .toInt(&ok);
  settings.endGroup();
  settings.sync();

  // Nonsense or 0: no file. A size too small to be useful is raised.
  int kb = ok ? maxKB : 0;
  if (kb > 0 && kb < kMinMaxSizeKB) kb = kMinMaxSizeKB;

  QStringList pending;
  {
    const QMutexLocker lock(&logMutex());
    maxBytes = static_cast<qint64>(kb) * 1024;
    state    = (kb > 0) ? State::Writing : State::Off;
    pending.swap(held);

    if (state == State::Writing)
      for (const QString &line : pending) writeToLogFile(line);
  }

  if (kb > 0) qInfo("Log: on, up to %d KB ([%s] %s)", kb, kLogGroup, kMaxSizeKBKey);
}

} // namespace sch::app::log
