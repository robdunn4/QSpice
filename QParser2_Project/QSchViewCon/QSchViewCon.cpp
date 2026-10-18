// QSchView.com -- QSchView's console launcher.
//
// QSchView.exe is a GUI program: started from a console, the prompt does
// not wait for it, its messages land on top of the prompt, and its result
// code is lost. This small CONSOLE program sits beside it as QSchView.com.
// Typing "QSchView ..." at a prompt runs the .com, because Windows tries
// .com before .exe, and the .com:
//
//   - with options (anything starting with "-", or "/?"): runs
//     QSchView.exe with the same command line, passes its messages
//     through to this console (stdout to stdout, stderr to stderr),
//     waits, and returns its result code;
//   - otherwise (no arguments, or only file names): starts the window and
//     returns at once.
//
// No logic of its own: options are QSchView.exe's, so this never changes
// when they do. Plain Win32, no Qt. Ctrl+C here ends QSchView.exe too.
//
// Built as QSchView.com into QSchView.exe's own output folder; the two
// must sit side by side.

#ifndef UNICODE
#define UNICODE
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <shellapi.h>

#include <string>
#include <thread>

namespace {

// Writes a line to stderr -- for the launcher's own failures.
void complain(const std::wstring &text) {
  const std::wstring line = L"QSchView: error: " + text + L"\r\n";
  DWORD              written = 0;
  HANDLE             err     = GetStdHandle(STD_ERROR_HANDLE);
  if (GetFileType(err) == FILE_TYPE_CHAR) {
    WriteConsoleW(err, line.c_str(), static_cast<DWORD>(line.size()), &written, nullptr);
    return;
  }
  // Redirected: as the console's code page would show it.
  const int bytes = WideCharToMultiByte(CP_ACP, 0, line.c_str(), static_cast<int>(line.size()),
                                        nullptr, 0, nullptr, nullptr);
  std::string narrow(static_cast<size_t>(bytes), '\0');
  WideCharToMultiByte(CP_ACP, 0, line.c_str(), static_cast<int>(line.size()), narrow.data(),
                      bytes, nullptr, nullptr);
  WriteFile(err, narrow.data(), static_cast<DWORD>(narrow.size()), &written, nullptr);
}

// The full path of QSchView.exe: this program's own path, .exe for .com.
std::wstring guiPath() {
  std::wstring path(MAX_PATH, L'\0');
  for (;;) {
    const DWORD length = GetModuleFileNameW(nullptr, path.data(), static_cast<DWORD>(path.size()));
    if (length == 0) return std::wstring();
    if (length < path.size()) {
      path.resize(length);
      break;
    }
    path.resize(path.size() * 2);
  }

  const size_t dot = path.find_last_of(L'.');
  const size_t sep = path.find_last_of(L"\\/");
  if (dot == std::wstring::npos || (sep != std::wstring::npos && dot < sep)) return path + L".exe";
  return path.substr(0, dot) + L".exe";
}

// This program's command line with its own name taken off, EXACTLY as
// typed -- quotes and all -- so QSchView.exe sees the same arguments.
std::wstring argumentsTail() {
  const wchar_t *at = GetCommandLineW();

  if (*at == L'"') {
    ++at;
    while (*at != L'\0' && *at != L'"') ++at;
    if (*at == L'"') ++at;
  } else {
    while (*at != L'\0' && *at != L' ' && *at != L'\t') ++at;
  }
  while (*at == L' ' || *at == L'\t') ++at;
  return std::wstring(at);
}

// As QSchView.exe decides (CliRun::wanted()): any option means the
// command line rather than a window.
bool wantsCommandLine() {
  int       count = 0;
  wchar_t **args  = CommandLineToArgvW(GetCommandLineW(), &count);
  if (args == nullptr) return false;

  bool wanted = false;
  for (int i = 1; i < count; ++i)
    if (args[i][0] == L'-' || std::wstring(args[i]) == L"/?") wanted = true;

  LocalFree(args);
  return wanted;
}

// Copies everything `from` delivers to `to`, until the writer closes it.
void relay(HANDLE from, HANDLE to) {
  char  buffer[4096];
  DWORD got = 0;
  while (ReadFile(from, buffer, sizeof(buffer), &got, nullptr) && got > 0) {
    DWORD written = 0;
    WriteFile(to, buffer, got, &written, nullptr);
  }
}

} // namespace

int wmain() {
  const std::wstring exe = guiPath();
  if (GetFileAttributesW(exe.c_str()) == INVALID_FILE_ATTRIBUTES) {
    complain(L"cannot find " + exe);
    return 1;
  }

  std::wstring commandLine = L"\"" + exe + L"\"";
  const std::wstring tail  = argumentsTail();
  if (!tail.empty()) commandLine += L" " + tail;

  STARTUPINFOW        startup{};
  PROCESS_INFORMATION process{};
  startup.cb = sizeof(startup);

  // --- The window: started, not waited for. ------------------------------
  if (!wantsCommandLine()) {
    if (!CreateProcessW(exe.c_str(), commandLine.data(), nullptr, nullptr, FALSE, 0, nullptr,
                        nullptr, &startup, &process)) {
      complain(L"cannot start " + exe);
      return 1;
    }
    CloseHandle(process.hThread);
    CloseHandle(process.hProcess);
    return 0;
  }

  // --- The command line: output passed through, then the result code. ---
  //
  // Pipes rather than this console's own handles, so it works the same
  // whether the output goes to the console, a file or another program.
  SECURITY_ATTRIBUTES inheritable{sizeof(SECURITY_ATTRIBUTES), nullptr, TRUE};

  HANDLE outRead = nullptr, outWrite = nullptr, errRead = nullptr, errWrite = nullptr;
  if (!CreatePipe(&outRead, &outWrite, &inheritable, 0) ||
      !CreatePipe(&errRead, &errWrite, &inheritable, 0)) {
    complain(L"cannot create pipes");
    return 1;
  }
  // Only the WRITE ends go to the child.
  SetHandleInformation(outRead, HANDLE_FLAG_INHERIT, 0);
  SetHandleInformation(errRead, HANDLE_FLAG_INHERIT, 0);

  startup.dwFlags    = STARTF_USESTDHANDLES;
  startup.hStdInput  = GetStdHandle(STD_INPUT_HANDLE);
  startup.hStdOutput = outWrite;
  startup.hStdError  = errWrite;

  // A job that ends QSchView.exe when this program ends -- on Ctrl+C, or
  // the console closing -- rather than leaving it printing unseen.
  HANDLE                               job = CreateJobObjectW(nullptr, nullptr);
  JOBOBJECT_EXTENDED_LIMIT_INFORMATION limits{};
  limits.BasicLimitInformation.LimitFlags = JOB_OBJECT_LIMIT_KILL_ON_JOB_CLOSE;
  if (job != nullptr)
    SetInformationJobObject(job, JobObjectExtendedLimitInformation, &limits, sizeof(limits));

  if (!CreateProcessW(exe.c_str(), commandLine.data(), nullptr, nullptr, TRUE, CREATE_SUSPENDED,
                      nullptr, nullptr, &startup, &process)) {
    complain(L"cannot start " + exe);
    return 1;
  }
  if (job != nullptr) AssignProcessToJobObject(job, process.hProcess);
  ResumeThread(process.hThread);
  CloseHandle(process.hThread);

  // Closed here, so each relay ends when the CHILD closes its end.
  CloseHandle(outWrite);
  CloseHandle(errWrite);

  std::thread outRelay(relay, outRead, GetStdHandle(STD_OUTPUT_HANDLE));
  std::thread errRelay(relay, errRead, GetStdHandle(STD_ERROR_HANDLE));

  WaitForSingleObject(process.hProcess, INFINITE);
  outRelay.join();
  errRelay.join();

  DWORD code = 1;
  GetExitCodeProcess(process.hProcess, &code);

  CloseHandle(process.hProcess);
  CloseHandle(outRead);
  CloseHandle(errRead);
  if (job != nullptr) CloseHandle(job);
  return static_cast<int>(code);
}
