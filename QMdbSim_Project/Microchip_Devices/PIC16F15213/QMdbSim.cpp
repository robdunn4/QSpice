//------------------------------------------------------------------------------
// This file is part of the QMdbSim project, a Microchip Simulator framework for
// QSpice C-Block components.  See the GitHub repository at
// https://github.com/robdunn4/QSpice/ for the complete project, current
// sources, documentation, and demonstration code.
//------------------------------------------------------------------------------
/*
 * Notes:
 *
 * The below code is fragile.  That is, we're parsing text from the MDB.jar
 * interface.  We have the MDB Java code and ultimately need to revise it to
 * return "machine-friendly" responses to improve speed/efficiency.  But, for
 * now, it's potentially unreliable parsing.
 */

#include "QMdbSim.h"

#pragma warning(disable : 4996) // we'll take our chances...

static const char* VersionInfo = "QMdbSim v0.1.0";

char MdbSim::recvBuf[4096]; // shared buffer for MDB reads

/*
 * MdbSim class implementation
 */

MdbSim::MdbSim() {}

MdbSim::~MdbSim()
{
  if (simState == Running) stopSim();
}

const char* MdbSim::getVerInfo() { return VersionInfo; }

bool MdbSim::startSim(const char* deviceName, const char* pgmPath)
{
  std::string cmd;

  if (simState != NotStarted)
  {
    if (simState != ErrState) setError("startSim(invalid state)");
    return false;
  }

  simState = Running;
  sDevName = deviceName;
  sPgmPath = pgmPath;

  // create stdio pipes for MDB communication
  if (!createPipes()) return false;

  // start the MDB Java program
  if (!createMdbProcess()) return false;

  // read until prompt; expecting nothing but prompt
  if (!recvBuffer()) return false;

  // set sim device; expecting only prompt
  cmd = "device ";
  cmd += deviceName;
  cmd += "\r\n";
  if (!sendRecvBuffer(cmd.c_str()) || sList.size())
  {
    setError("startSim(set device)");
    return false;
  }

  // set sim device; expecting last line == "Resetting peripherals"
  if (!sendRecvBuffer("hwtool SIM\r\n") ||
      sList.back().compare("Resetting peripherals"))
  {
    setError("startSim(set hwtool=SIM)");
    return false;
  }

  // set program; expecting last line == "Program succeeded."
  cmd = "program ";
  cmd += pgmPath;
  cmd += "\r\n";
  if (!sendRecvBuffer(cmd.c_str()) ||
      sList.back().compare("Program succeeded."))
  {
    setError("startSim(set program)");
    return false;
  }

  return true;
}

// note: stopping a stopped sim is not an error
bool MdbSim::stopSim()
{
  if (simState == ErrState) return false;
  if (simState == NotStarted)
  {
    setError("stopSim(not started)");
    return false;
  }
  if (simState == Running && !sendBuffer("quit\r\n")) return false;

  simState = Stopped;
  return true;
}

// step simulation by one machine instruction;  in theory, this command
// cannot fail unless the command cannot be sent or previously erred out
//
// note that MDB returns breakpoint information, i.e.:
//
//   Stepping
//   Stop at
//     address:0x1
// or
//
//   Stepping
//   Stop at
//     address:0x7e9
//     file:C:/Users/rediv/AppData/Local/Temp/xcAsqhg/driver_tmp_1.s
//     source line:1736
//
// since the step instruction is called for every clock increment, we don't
// parse/return this info but, when debugging, it may be useful to see what was
// returned...
//
bool MdbSim::stepInst()
{
  // check state
  if (simState == ErrState) return false;
  if (simState != Running)
  {
    setError("stepInst(not running)");
    return false;
  }

  // fails only if MDB read/write error, i.e., no error response to check
  return sendRecvBuffer("stepi\r\n");
}

// get pin state information; we're expecting something like this:
//
//   Pin     Mode    Value   Owner or Mapping
//   RA0     Ain     5.0V    (RA0)/IOCA0/ANA0/ICDDAT/ICSPDAT
//
bool MdbSim::getPin(const char* pinName, PinState& pinState)
{
  // check state
  if (simState == ErrState) return false;
  if (simState != Running)
  {
    setError("getPin(not running)");
    return false;
  }

  std::string cmd;
  cmd = "print pin ";
  cmd += pinName;
  cmd += "\r\n";
  if (!sendRecvBuffer(cmd.c_str()))
  {
    setError("getPin(I/O failed)");
    return false;
  }

  if (sList.size() != 2)
  {
    setError("getPin(unexpected response, check pin name)");
    return false;
  }

  // strtok() modifies string; using a fixed sized buffer here...
  // TODO:  this is potentially unsafe code...
  static char str[128];
  strncpy(str, sList[1].c_str(), sizeof(str));
  char* name  = strtok(str, " \t\r\n");
  char* mode  = strtok(NULL, " \t\r\n");
  char* value = strtok(NULL, " \t\r\n");

  if (!mode || strlen(mode) < 3 || !value || strlen(value) < 3)
  {
    setError("getPin(invalid values)");
    return false;
  }

  pinState.daState = mode[0] == 'D' ? PIN_ANALOG : PIN_DIGITAL;
  pinState.ioState = mode[1] == 'i' ? PIN_INPUT : PIN_OUTPUT;
  pinState.hlState = value[0] == '0' || value[0] == 'L' ? PIN_LOW : PIN_HIGH;

  return true;
}

// set pin with digital "HIGH" or "LOW" text
bool MdbSim::setPin(const char* pinName, bool toHigh)
{
  //// check state
  if (simState == ErrState) return false;
  if (simState != Running)
  {
    setError("setPin(not running)");
    return false;
  }

  std::string cmd;
  cmd = "write pin ";
  cmd += pinName;
  cmd += toHigh ? " HIGH\r\n" : " LOW\r\n";
  if (!sendRecvBuffer(cmd.c_str()))
  {
    setError("setPin(I/O failure)");
    return false;
  }

  // we expect nothing but prompt on success
  if (sList.size())
  {
    setError("setPin(write pin error)");
    return false;
  }

  return true;
}

// set pin with specific voltage
bool MdbSim::setPin(const char* pinName, double toVoltage)
{
  //// check state
  if (simState == ErrState) return false;
  if (simState != Running)
  {
    setError("setPin(not running)");
    return false;
  }

  std::string cmd;
  cmd = "write pin ";
  cmd += pinName;
  cmd += " ";
  cmd += std::to_string(toVoltage); // TODO: check precision, etc.
  cmd += "V\r\n";

  if (!sendRecvBuffer(cmd.c_str()))
  {
    setError("setPin(I/O failure)");
    return false;
  }

  // we expect nothing but prompt on success
  if (sList.size())
  {
    setError("setPin(write pin error)");
    return false;
  }

  return true;
}

// set error state and save a useful error msg in lastErrMsg
void MdbSim::setError(const char* msg)
{
  // if already in an error state, don't change anyting...
  if (simState == ErrState) return;
  simState = ErrState;

  LPVOID lpMsgBuf;
  DWORD  dw = GetLastError();

  // if no system error, this is an unexpected MDB response...
  if (!dw)
  {
    lastErrMsg = msg;
    return;
  }

  // was a system error, format and save to lastErrMsg
  FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM |
                    FORMAT_MESSAGE_IGNORE_INSERTS,
      NULL, dw, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPTSTR) &lpMsgBuf,
      0, NULL);

  // remove trailing whitespace
  char* buf = (char*) lpMsgBuf;
  for (int i = strlen(buf) - 1; i >= 0 && ::isspace(buf[i]); i--) buf[i] = 0;
  lastErrMsg = buf;

  LocalFree(lpMsgBuf);
}

// create pipes for STDIN, STDOUT, and STDEERR
bool MdbSim::createPipes()
{
  SECURITY_ATTRIBUTES saAttr;

  // Set the bInheritHandle flag so pipe handles are inherited.
  saAttr.nLength              = sizeof(SECURITY_ATTRIBUTES);
  saAttr.bInheritHandle       = TRUE;
  saAttr.lpSecurityDescriptor = NULL;

  // Create a pipe for the child process's STDOUT.
  if (!CreatePipe(&mdbStdOut_Rd, &mdbStdOut_Wr, &saAttr, 0))
  {
    setError("StdoutRd CreatePipe");
    return false;
  }

  // Ensure the read handle to the pipe for STDOUT is not inherited.
  if (!SetHandleInformation(mdbStdOut_Rd, HANDLE_FLAG_INHERIT, 0))
  {
    setError("Stdout SetHandleInformation");
    return false;
  }

  //  Create a pipe for the child process's STDERR.
  if (!CreatePipe(&mdbStdErr_Rd, &mdbStdErr_Wr, &saAttr, 0))
  {
    setError("StderrRd CreatePipe");
    return false;
  }

  // Ensure the read handle to the pipe for STDERR is not inherited.
  if (!SetHandleInformation(mdbStdErr_Rd, HANDLE_FLAG_INHERIT, 0))
  {
    setError("Stderr SetHandleInformation");
    return false;
  }

  // Create a pipe for the child process's STDIN.
  if (!CreatePipe(&mdbStdIn_Rd, &mdbStdIn_Wr, &saAttr, 0))
  {
    setError("Stdin CreatePipe");
    return false;
  }

  // Ensure the write handle to the pipe for STDIN is not inherited.
  if (!SetHandleInformation(mdbStdIn_Wr, HANDLE_FLAG_INHERIT, 0))
  {
    setError("Stdin SetHandleInformation");
    return false;
  }

  return true;
}

// launch MDB process
bool MdbSim::createMdbProcess()
{
  PROCESS_INFORMATION piProcInfo;
  STARTUPINFO         siStartInfo;
  BOOL                bSuccess = FALSE;

  // Set up members of the PROCESS_INFORMATION structure.
  ZeroMemory(&piProcInfo, sizeof(PROCESS_INFORMATION));

  // Set up members of the STARTUPINFO structure.
  // This structure specifies the STDIN, STDOUT, & STDERR handles for
  // redirection.
  ZeroMemory(&siStartInfo, sizeof(STARTUPINFO));
  siStartInfo.cb = sizeof(STARTUPINFO);

  // siStartInfo.hStdError  = mdbStdErr_Wr; // set MDB STDERR to STDERR input
  siStartInfo.hStdError  = mdbStdOut_Wr; // set MDB STDERR to STDOUT input
  siStartInfo.hStdOutput = mdbStdOut_Wr; // set MDB STDOUT to STDOUT input
  siStartInfo.hStdInput  = mdbStdIn_Rd;  // set MDB STDIN to STDIN output
  siStartInfo.dwFlags |= STARTF_USESTDHANDLES;

  // Create the MDB child process.
  bSuccess = CreateProcess(MdbSimPath, // command line
      NULL,                            // arguments
      NULL,                            // process security attributes
      NULL,                            // primary thread security attributes
      TRUE,                            // handles are inherited
      0,                               // creation flags
      NULL,                            // use parent's environment
      NULL,                            // use parent's current directory
      &siStartInfo,                    // STARTUPINFO pointer
      &piProcInfo);                    // receives PROCESS_INFORMATION

  // if an error occurs, set error and return failure
  if (!bSuccess)
  {
    setError("CreateMdbProcess");
    return false;
  }

  // Close handles to the child process and its primary thread.
  // Some applications might keep these handles to monitor the status
  // of the child process, for example.
  CloseHandle(piProcInfo.hProcess);
  CloseHandle(piProcInfo.hThread);

  // Close handles to the stdin and stdout pipes no longer needed by the child
  // process. If they are not explicitly closed, there is no way to recognize
  // that the child process has ended.
  CloseHandle(mdbStdErr_Wr);
  CloseHandle(mdbStdOut_Wr);
  CloseHandle(mdbStdIn_Rd);

  return true;
}

// send command to MDB.  caller should ensure that cmd has CRLF and is
// zero-terminated.
bool MdbSim::sendBuffer(const char* cmd)
{
  // clear receive list
  sList.clear();

  if (simState != Running)
  {
    if (simState != ErrState) setError("sendBuffer(not running)");
    return false;
  }

  DWORD dwWritten;
  if (!WriteFile(mdbStdIn_Wr, cmd, strlen(cmd), &dwWritten, NULL))
  {
    setError("sendBuffer(WriteFile)");
    return false;
  }

  return true;
}

// note:  return of false is ambiguous -- could be failed read or missing
// trailing prompt; failed read sets error state, missing prompt does not set
// error state.
//
// also, this is fragile code and could easily break into infinite loop?
//
// returns false on failure.  On success, StringList is updated with empty lines
// removed and trailing whitespace removed. final prompt also removed.
bool MdbSim::recvBuffer()
{
  sList.clear();

  if (simState != Running)
  {
    if (simState != ErrState) setError("recvBuffer(not running)");
    return false;
  }

  DWORD dwRead;
  BOOL  bSuccess  = FALSE;
  bool  gotPrompt = false;

  while (!gotPrompt)
  {
    bSuccess =
        ReadFile(mdbStdOut_Rd, recvBuf, sizeof(recvBuf) - 1, &dwRead, NULL);
    if (!bSuccess)
    {
      setError("recvBuffer (ReadFile)");
      return false;
    }

    // if empty line, ignore
    if (!dwRead) break;

    // ensure termination
    recvBuf[dwRead] = '\0';

    // break embedded CRLF into lines, remove trailing CRLF
    char* tok = strtok(recvBuf, "\r\n");
    while (tok && !gotPrompt)
    {
      // if not empty line, add to list
      if (strlen(tok) > 0)
      {
        sList.push_back(tok);
        gotPrompt = tok[0] == '>' && tok[1] == '\0';
        tok       = strtok(NULL, "\r\n");
        continue;
      }
    }
  }

  // fail if no prompt else remove prompt line
  if (!gotPrompt) return false;
  sList.pop_back();
  return true;
}

// use this to ensure that receive buffer is called after send buffer
bool MdbSim::sendRecvBuffer(const char* cmd)
{
  if (!sendBuffer(cmd)) return false;
  return recvBuffer();
}
