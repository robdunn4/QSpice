#pragma once

#include <QByteArray>
#include <QString>
#include <QtGlobal>

// QSpice's inter-process messages, AS OBSERVED -- see the checkpoint's
// IPC section. Appendix I of QSPICE.chm documents them, but its table
// has merged footnote numbers into the message values and two rows
// shifted; these are the corrected values.
//
// Decoding only: no windows, no processes. the program's window procedure
// hands each message here.
namespace sch::link::ipcmsg {

// --- SIMULATOR / QPOST -> SCHEMATIC ----------------------------------

// POSTED. wParam 0 with lParam an HWND: the simulator checking in.
// Otherwise status-bar text, packed as below; 0/0 shows it.
inline constexpr unsigned kSimStatus = 0x80FF;

// SENT. Simulator console text; 0/0 flushes it.
inline constexpr unsigned kSimConsole = 0x8100;

// QPOST console text; 0/0 flushes it.
inline constexpr unsigned kPostConsole = 0x8101;

// SENT. The simulator will launch the waveform viewer itself.
inline constexpr unsigned kNoViewer = 0x8102;

// Pause/resume from the viewer (wParam 1/2), and -- schematic to
// simulator -- clear the waveform data (wParam 0). A SendMessage of it TO
// the schematic returns the simulator's HWND.
inline constexpr unsigned kSimControl = 0x80F0;

// --- WAVEFORM VIEWER -> SCHEMATIC -----------------------------------
//
// 0x8403-0x8405 as OBSERVED: the appendix lists 0x8403 and 0x8404 both
// as "Rerun" and 0x8405 as "exited"; the captures show 0x8404 is the
// exit and 0x8405 a move.

inline constexpr unsigned kViewerOpenFailed = 0x8401; // SENT
inline constexpr unsigned kViewerCheckIn    = 0x8402; // wParam its HWND
inline constexpr unsigned kViewerRerun      = 0x8403;
inline constexpr unsigned kViewerClosed     = 0x8404; // with its rectangle
inline constexpr unsigned kViewerMoved      = 0x8405; // with its rectangle
inline constexpr unsigned kViewerAbort      = 0x8406;
inline constexpr unsigned kViewerOpen       = 0x8407; // File > Open
inline constexpr unsigned kViewerOpenDemo   = 0x8408; // File > Open Demo
inline constexpr unsigned kViewerNewSym     = 0x840A; // File > New Symbol
inline constexpr unsigned kViewerNewSch     = 0x840B; // File > New Schematic
inline constexpr unsigned kViewerNewAscii   = 0x840C; // File > New ASCII File
inline constexpr unsigned kViewerOpenMru    = 0x840D; // wParam: MRU number
inline constexpr unsigned kViewerClear      = 0x8439; // -> kSimControl 0

// --- SCHEMATIC -> WAVEFORM VIEWER ------------------------------------
//
// The appendix gives none of these parameters; the capture of a run
// started with the viewer already open shows QSpice sending, in order:
//
//   P 0x8414  0, 0               stop marching (the previous run)
//   P 0x8411  0, 0               UNDOCUMENTED
//   S 0x8412  0, 0               data invalid
//   ... the new .qraw has data ...
//   P 0x8413  -1, <sim PID>      reopen, following that simulator
//   P 0x80FF  0, <sim HWND>      the simulator's check-in, passed on
//
// P = PostMessage, S = SendMessage.

inline constexpr unsigned kToViewerIsStepped   = 0x840E; // SENT: returns it
inline constexpr unsigned kToViewerClose       = 0x840F;
inline constexpr unsigned kToViewer8411        = 0x8411; // undocumented
inline constexpr unsigned kToViewerDataInvalid = 0x8412; // SENT
inline constexpr unsigned kToViewerReopen      = 0x8413; // -1, sim PID
inline constexpr unsigned kToViewerSimDone     = 0x8414; // stop marching
inline constexpr unsigned kToViewerToTop       = 0x8415;

// The rectangle 0x8404/0x8405 carry, in screen pixels: wParam = left |
// top << 32, lParam = right | bottom << 32. Signed: a monitor left of or
// above the primary has negative coordinates.
struct Rect {
  int left   = 0;
  int top    = 0;
  int right  = 0;
  int bottom = 0;

  bool isValid() const { return right > left && bottom > top; }

  static Rect fromParams(quint64 wParam, qint64 lParam);

  // "l,t,r,b" -- the viewer's command-line form, and how it is saved.
  QString toString() const;
  static Rect fromString(const QString &text);
};

// Console and status text arrive EIGHT BYTES per message: the low 32
// bits of wParam, then of lParam, each little-endian; a zero byte ends
// the text early. Both zero means "show what has accumulated". One
// buffer per message number -- the simulator interleaves them.
//
// Bytes are QSpice's single-byte set and decoded as Windows-1252, with
// "\n" line ends.
class TextBuf {
public:
  // Adds one message's bytes. Returns true on a flush, with the
  // accumulated text in `text` and the buffer emptied.
  bool add(quint64 wParam, qint64 lParam, QString &text);

  void clear() { pending.clear(); }

protected:
  QByteArray pending;
};

} // namespace sch::link::ipcmsg
