#pragma once

#include <QByteArray>
#include <QObject>
#include <QProcess>
#include <QString>
#include <QStringList>

class QTimer;

namespace sch::link {

// Runs "QUX.exe -Netlist <file> -stdout" in the background and reports
// the netlist, or why there is none. No UI of its own: View > Netlist
// shows the result in a TextViewWin.
//
// QUX reads only FILES, so a schematic with unsaved changes is written
// to a temporary copy first -- the same bytes Save would write --
// placed BESIDE the original, so everything QUX resolves relative to
// the schematic's folder (hierarchical sheets, local symbols, relative
// .include and .lib paths) resolves exactly as it would for the real
// file. Only a never-saved schematic, which has no folder, goes to
// %TEMP%. An unmodified schematic is netlisted from its own file.
//
// The copy is hidden, marked temporary (so Windows keeps it in the file
// cache rather than writing it out), named
//
//     <tempPrefix><pid>_<n>_<base>.qsch     e.g. ~ProjectQ_1234_1_Amp.qsch
//
// and deleted as soon as QUX finishes. Copies a crash left behind are
// swept up the next time a netlist is made in that folder. The
// temporary name is replaced by the real one wherever it appears in the
// netlist.
//
// Give it the viewer as parent: destroying it kills a QUX still running
// and deletes the copy, so closing the window cleans up.
class NetlistRun : public QObject {
  Q_OBJECT

public:
  // tempPrefix names the temporary copies -- the PROGRAM's name, as
  // "~<Program>_", so each program sweeps up only its own leftovers.
  // Required: a library has no program name of its own.
  NetlistRun(const QString &tempPrefix, QObject *parent = nullptr);
  ~NetlistRun() override;

  // schPath   the schematic's file, or empty if never saved
  // title     the document's display name, used for an untitled copy
  // content   the serialized schematic, or empty to netlist schPath as
  //           it is on disk (which requires schPath)
  // libPaths  the user's symbol folders, added to the NETLIST QUX
  //           returns as ".libpath" lines -- see withLibPaths(). QUX
  //           itself never sees them.
  //
  // Exactly one of succeeded() or failed() follows -- possibly before
  // this returns, if the run cannot even start.
  void start(const QString &quxExe, const QString &schPath,
             const QString &title, const QByteArray &content,
             const QStringList &libPaths = QStringList());

signals:
  // QUX ran to completion and returned exitCode. Emitted BEFORE
  // succeeded() or failed(); not emitted if QUX could not be started,
  // crashed, or was stopped.
  void exited(int exitCode);

  // bytes: the netlist EXACTLY as QUX wrote it, plus the .libpath block
  //        and the temporary name swapped back -- what a simulator
  //        should be fed and what Save should write.
  // text:  the same, decoded for display only.
  // notes: QUX's stderr and a non-zero exit code, if any.
  void succeeded(const QByteArray &bytes, const QString &text,
                 const QString &notes);
  void failed(const QString &message);

protected:
  void onFinished(int exitCode, QProcess::ExitStatus status);
  void onErrorOccurred(QProcess::ProcessError error);
  void onTimeout();

  // Writes content to a fresh temporary copy in dir; empty on failure,
  // with the reason in error.
  QString writeTempCopy(const QString &dir, const QString &base,
                        const QByteArray &content, QString &error);

  void removeTempCopy();

  // The netlist with a ".libpath "<folder>"" line per user symbol folder
  // inserted after QUX's first line, set off by a blank line either
  // side. Unchanged when there are none.
  static QByteArray withLibPaths(const QByteArray &netlist,
                                 const QStringList &libPaths);

  // Text as the netlist's own bytes: Windows-1252, else UTF-8.
  static QByteArray netlistBytes(const QString &text);
  void fail(const QString &message);

  // QUX's output as text, for display: UTF-8 if it decodes cleanly,
  // else Windows-1252.
  static QString decodeOutput(const QByteArray &bytes);

  // Deletes leftover copies in dir older than a day.
  void sweepStaleCopies(const QString &dir) const;

  QProcess *process  = nullptr;
  QTimer   *watchdog = nullptr;

  QString tempPrefix; // "~<Program>_" -- see the constructor
  QString tempPath; // the copy, while it exists
  QString tempBase; // its name without suffix, to swap back in the output
  QString realBase; // the name to swap in
  QStringList userLibPaths; // for withLibPaths()
  bool    runOver = false; // finished, failed, or stopped -- reported once
};

} // namespace sch::link
