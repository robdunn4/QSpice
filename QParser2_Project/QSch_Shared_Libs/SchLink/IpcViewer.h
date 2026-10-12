#pragma once

#include <QElapsedTimer>
#include <QString>
#include <QtGlobal>

#include "IpcMsg.h"

namespace sch::link {

// QSpice's waveform viewer -- QUX.exe started as
//
//   QUX.exe -viewer <schematic HWND> l,t,r,b "<qraw>" -simProcessID <pid>
//
// -- as seen from the schematic: launching it, knowing its window once
// it checks in (0x8402), and sending it the parameterless SCHEMATIC ->
// WAVEFORM messages of Appendix I.
//
// One per schematic window, kept across runs: QSpice reuses an open
// viewer for the next run (0x8412 then 0x8413) rather than starting
// another.
//
// The viewer is started DETACHED -- it is its own window, which the user
// closes. Its messages arrive at the schematic window; MainWin hands the
// relevant ones here.
//
// No signals -- no moc.
class IpcViewer {
public:
  // Its window, as it checked in; 0 when none is known.
  quintptr hwnd() const { return viewerHwnd; }

  // A viewer is open -- or was launched moments ago and has not checked
  // in yet, which counts: launching a second would be wrong.
  bool isOpen() const;

  void checkedIn(quintptr hwnd);
  void closed();

  // Starts one. False, with `error` set, if it could not be.
  bool launch(const QString &quxExe, quintptr schematicHwnd,
              const ipcmsg::Rect &rect, const QString &qraw, qint64 simPid,
              QString &error);

  // One SCHEMATIC -> WAVEFORM message, sent with a timeout so a hung
  // viewer cannot freeze the schematic. Returns the viewer's answer; 0
  // if there is no viewer or it did not answer.
  qintptr send(unsigned msg) const;

  // The same, POSTED -- as QSpice posts all but 0x8412 and 0x840E.
  // False if there is no viewer window to post to.
  bool post(unsigned msg, quint64 wParam = 0, qint64 lParam = 0) const;

  // The arguments of the last launch(), as passed -- for diagnosis.
  const QString &lastCommandLine() const { return commandLine; }

  // Whether a rectangle, in screen pixels, is on a monitor that exists.
  static bool onScreen(const ipcmsg::Rect &rect);

  // A window's rectangle, in screen pixels.
  static ipcmsg::Rect windowRect(quintptr hwnd);

protected:
  quintptr      viewerHwnd = 0;
  QElapsedTimer launchedAt; // valid from launch() to check-in
  QString       commandLine;
};

} // namespace sch::link
