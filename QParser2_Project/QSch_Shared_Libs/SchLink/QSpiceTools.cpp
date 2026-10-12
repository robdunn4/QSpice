#include "QSpiceTools.h"

#include <QDir>
#include <QFileInfo>
#include <QObject>
#include <QProcess>
#include <QStringList>

#include "QSpiceInstall.h"

#ifdef Q_OS_WIN
// Before windows.h, always: it defines min and max as macros, which
// breaks std::min and std::max in headers that have nothing to do with
// Windows.
#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include <tlhelp32.h>
#endif

namespace sch::link::tools {
namespace {

#ifdef Q_OS_WIN

// The first running process with this executable name, or 0.
//
// Found by NAME rather than by remembering what we launched: the
// program may have been started from QSpice itself, or by an earlier
// run of our program, and bringing that copy forward is what the user
// means by "it is already open".
DWORD findProcess(const QString &exeName) {
  const HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
  if (snapshot == INVALID_HANDLE_VALUE) return 0;

  PROCESSENTRY32W entry;
  entry.dwSize = sizeof(entry);

  DWORD found = 0;

  if (Process32FirstW(snapshot, &entry)) {
    do {
      const QString name = QString::fromWCharArray(entry.szExeFile);

      if (name.compare(exeName, Qt::CaseInsensitive) == 0) {
        found = entry.th32ProcessID;
        break;
      }
    } while (Process32NextW(snapshot, &entry));
  }

  CloseHandle(snapshot);
  return found;
}

struct FindWindowData {
  DWORD pid    = 0;
  HWND  window = nullptr;
};

BOOL CALLBACK windowForPid(HWND window, LPARAM param) {
  FindWindowData *data = reinterpret_cast<FindWindowData *>(param);

  DWORD pid = 0;
  GetWindowThreadProcessId(window, &pid);

  // A TOP-LEVEL visible window: a process owns several, most of them
  // tool windows and hidden message sinks, and raising one of those
  // would look like nothing happened.
  if (pid != data->pid || !IsWindowVisible(window) ||
      GetWindow(window, GW_OWNER) != nullptr)
    return TRUE;

  data->window = window;
  return FALSE;
}

bool bringForward(DWORD pid) {
  FindWindowData data;
  data.pid = pid;

  EnumWindows(windowForPid, reinterpret_cast<LPARAM>(&data));

  if (data.window == nullptr) return false;

  // Restored first: a minimised window cannot be raised, and the copy
  // the user is looking for is often the one they minimised.
  if (IsIconic(data.window)) ShowWindow(data.window, SW_RESTORE);

  // Windows may REFUSE to give focus to a process the user is not
  // interacting with, in which case it flashes the taskbar button
  // instead. That is the OS's decision and there is no honest way
  // around it -- the window is still raised either way.
  SetForegroundWindow(data.window);

  return true;
}

#endif // Q_OS_WIN

} // namespace

QString run(const QString &exeName, const QString &workingDir) {
  const QString installPath = install::installPath();

  if (installPath.isEmpty())
    return QObject::tr("The QSpice installation folder is not known, so %1 "
                       "cannot be started.")
        .arg(exeName);

  const QString exePath = QDir(installPath).absoluteFilePath(exeName);

#ifdef Q_OS_WIN
  // ALREADY RUNNING: raise it rather than starting a second copy.
  if (const DWORD pid = findProcess(exeName))
    if (bringForward(pid)) return QString();
#endif

  if (!QFileInfo::exists(exePath))
    return QObject::tr("Not found:\n\n%1\n\nThis program is part of QSpice "
                       "and is expected beside QUX.exe.")
        .arg(QDir::toNativeSeparators(exePath));

  // DETACHED: the program outlives ours. startDetached() leaves no
  // parent link, so closing our window does not take the tool with it.
  //
  // The working directory is the SCHEMATIC's folder, so anything the
  // tool writes lands beside the design rather than in the QSpice
  // installation -- which on a normal machine is under Program Files
  // and not writable anyway.
  const QString startIn = workingDir.isEmpty() ? installPath : workingDir;

  if (QProcess::startDetached(exePath, QStringList(), startIn)) return QString();

  return QObject::tr("Could not start:\n\n%1")
      .arg(QDir::toNativeSeparators(exePath));
}

} // namespace sch::link::tools
