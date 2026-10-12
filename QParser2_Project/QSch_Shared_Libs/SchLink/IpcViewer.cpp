#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable : 4996) // Qt headers reference APIs deprecated for Qt 7
#endif

#include "IpcViewer.h"

#include <QDir>
#include <QFileInfo>
#include <QProcess>
#include <QStringList>

#ifdef _MSC_VER
#pragma warning(pop)
#endif

#ifdef Q_OS_WIN
#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif

namespace sch::link {

namespace {

// How long a launched viewer has to check in before it is taken to have
// failed and another may be started.
constexpr qint64 kCheckInGraceMs = 15000;

// For each message sent to the viewer.
constexpr unsigned kSendTimeoutMs = 2000;

} // namespace

bool IpcViewer::isOpen() const {
#ifdef Q_OS_WIN
  if (viewerHwnd != 0 && IsWindow(reinterpret_cast<HWND>(viewerHwnd)))
    return true;
#endif
  return launchedAt.isValid() && launchedAt.elapsed() < kCheckInGraceMs;
}

void IpcViewer::checkedIn(quintptr hwnd) {
  viewerHwnd = hwnd;
  launchedAt.invalidate();
}

void IpcViewer::closed() {
  viewerHwnd = 0;
  launchedAt.invalidate();
}

bool IpcViewer::launch(const QString &quxExe, quintptr schematicHwnd,
                       const ipcmsg::Rect &rect, const QString &qraw,
                       qint64 simPid, QString &error) {
  // EXACTLY as QSpice writes it -- the HWND in decimal, the rectangle as
  // one argument, the path ALWAYS quoted. QProcess would quote the path
  // only if it held a space, and QUX may parse its own command line.
  commandLine = QStringLiteral("-viewer %1 %2 \"%3\" -simProcessID %4")
                    .arg(QString::number(static_cast<qulonglong>(schematicHwnd)),
                         rect.toString(), QDir::toNativeSeparators(qraw),
                         QString::number(simPid));

  QProcess process;
  process.setProgram(quxExe);
  process.setWorkingDirectory(QFileInfo(qraw).absolutePath());
#ifdef Q_OS_WIN
  process.setNativeArguments(commandLine);
#else
  process.setArguments(QProcess::splitCommand(commandLine));
#endif

  if (!process.startDetached()) {
    error = QStringLiteral("Could not start the waveform viewer (%1).")
                .arg(QDir::toNativeSeparators(quxExe));
    return false;
  }

  viewerHwnd = 0;
  launchedAt.start();
  return true;
}

qintptr IpcViewer::send(unsigned msg) const {
#ifdef Q_OS_WIN
  if (viewerHwnd == 0) return 0;

  const HWND target = reinterpret_cast<HWND>(viewerHwnd);
  if (!IsWindow(target)) return 0;

  DWORD_PTR answer = 0;
  if (SendMessageTimeoutW(target, msg, 0, 0, SMTO_ABORTIFHUNG | SMTO_NORMAL,
                          kSendTimeoutMs, &answer) == 0)
    return 0;

  return static_cast<qintptr>(answer);
#else
  Q_UNUSED(msg);
  return 0;
#endif
}

bool IpcViewer::post(unsigned msg, quint64 wParam, qint64 lParam) const {
#ifdef Q_OS_WIN
  if (viewerHwnd == 0) return false;

  const HWND target = reinterpret_cast<HWND>(viewerHwnd);
  if (!IsWindow(target)) return false;

  return PostMessageW(target, msg, static_cast<WPARAM>(wParam),
                      static_cast<LPARAM>(lParam)) != 0;
#else
  Q_UNUSED(msg);
  Q_UNUSED(wParam);
  Q_UNUSED(lParam);
  return false;
#endif
}

bool IpcViewer::onScreen(const ipcmsg::Rect &rect) {
  if (!rect.isValid()) return false;

#ifdef Q_OS_WIN
  // Its top-left corner region, where the title bar is: a window whose
  // title bar is off every screen cannot be moved back by the user.
  RECT probe;
  probe.left   = rect.left;
  probe.top    = rect.top;
  probe.right  = rect.left + 100;
  probe.bottom = rect.top + 30;
  return MonitorFromRect(&probe, MONITOR_DEFAULTTONULL) != nullptr;
#else
  return true;
#endif
}

ipcmsg::Rect IpcViewer::windowRect(quintptr hwnd) {
  ipcmsg::Rect rect;

#ifdef Q_OS_WIN
  RECT r;
  if (GetWindowRect(reinterpret_cast<HWND>(hwnd), &r) == 0) return rect;

  rect.left   = r.left;
  rect.top    = r.top;
  rect.right  = r.right;
  rect.bottom = r.bottom;
#else
  Q_UNUSED(hwnd);
#endif

  return rect;
}

} // namespace sch::link
