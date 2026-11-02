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

  // Closes the viewer -- for the schematic window closing, which the
  // viewer must not outlive: WM_CLOSE posted to its frame (rootHwnd()).
  // A viewer launched here that never checked in is found by its
  // process and closed the same way. Forgets it either way.
  //
  // Not 0x840F, which Appendix I lists as "close now". Tested against
  // QSpice's build of 2026-10-16, on 2026-10-19, with zero parameters
  // and the viewer idle after a run: posted or sent, to the view or to
  // the frame, it did nothing within 2 s (sent, it answered 0). WM_CLOSE
  // to the frame closed it every time.
  void closeViewer();

  // The viewer's top-level window -- its frame, holding the view that
  // checked in -- or 0 if none is known.
  quintptr rootHwnd() const;

  // Starts one. False, with `error` set, if it could not be.
  bool launch(const QString &quxExe, quintptr schematicHwnd,
      const ipcmsg::Rect &rect, const QString &qraw, qint64 simPid,
      QString &error);

  // One SCHEMATIC -> WAVEFORM message, sent with a timeout so a hung
  // viewer cannot freeze the schematic. Returns the viewer's answer; 0
  // if there is no viewer or it did not answer.
  qintptr send(unsigned msg) const;

  // --- Cross-probing ---------------------------------------------------
  //
  // WM_COPYDATA, SENT (with the same timeout as send()) to the window
  // the viewer checked in with -- its plot view, an MFC
  // "AfxFrameOrView140su" child of the frame:
  //
  //   wParam   the request: 1 = "can you plot this?", 2 = "plot it"
  //   data     the expression, a NUL-terminated string in Windows-1252:
  //            "V(n01)", "I(R1)", "Ic(Q1)"
  //   answer   1 = yes / plotted, 0 = no
  //
  // As Appendix I's table gives it -- the request in the wParam column.
  // Tested against QSpice's build of 2026-10-16, on 2026-10-19:
  //
  //   wParam = the request                        works
  //   wParam = the sender's window (the usual     answers 0: the request
  //     Windows convention), request in dwData    is read from wParam
  //   sent to the frame instead of the view       answers 0
  //   dwData 1, 0 or 99                           no difference: unused
  //   NUL counted in cbData or not                no difference
  //   "V(n01)" or "V(N01)"                        no difference: names
  //                                               ignore case
  //   "V(no_such_net)"                            answers 0
  //
  // dwData is set to the request anyway, and the NUL counted, in case a
  // later build reads them.

  // Request 1: whether the viewer's data has `expression`.
  bool canPlot(const QString &expression) const;

  // Request 2: adds `expression` to the viewer's plots. True if the
  // viewer says it did.
  bool plot(const QString &expression) const;

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
  // One cross-probing request -- see canPlot(). 0 if there is no viewer
  // or it did not answer.
  qintptr request(quintptr which, const QString &expression) const;

  quintptr      viewerHwnd  = 0;
  qint64        launchedPid = 0;   // the last launch()'s, 0 if none
  QElapsedTimer launchedAt;        // valid from launch() to check-in
  QString       commandLine;
};

}   // namespace sch::link
