// Automatically generated C++ file on Sun Oct 20 14:26:16 2024
//
// To build with Digital Mars C++ Compiler:
//
//    dmc -mn -WD qalert.cpp kernel32.lib shell32.lib (for shellexecute())

#include <Windows.h>
#include <malloc.h>
#include <shellapi.h>
#include <stdio.h>
#include <time.h>

extern "C" __declspec(dllexport) int (*Display)(
    const char *format, ...) = 0;   // works like printf()
extern "C" __declspec(dllexport) const double *DegreesC =
    0;   // pointer to current circuit temperature
extern "C" __declspec(dllexport) const int *StepNumber =
    0;   // pointer to current step number
extern "C" __declspec(dllexport) const int *NumberSteps =
    0;   // pointer to estimated number of steps
extern "C" __declspec(dllexport) const char *const *InstanceName =
    0;   // pointer to address of instance name
extern "C" __declspec(dllexport) const char *QUX = 0;   // path to QUX.exe
extern "C" __declspec(dllexport) const bool *ForKeeps =
    0;   // pointer to whether being evaluated non-hypothetically
extern "C" __declspec(dllexport) const bool *HoldICs =
    0;   // pointer to whether instance initial conditions are being held
extern "C" __declspec(dllexport) int (*DFFT)(struct sComplex *u, bool inv,
    unsigned int N, double scale) = 0;   // discrete Fast Fourier function

union uData {
  bool                   b;
  char                   c;
  unsigned char          uc;
  short                  s;
  unsigned short         us;
  int                    i;
  unsigned int           ui;
  float                  f;
  double                 d;
  long long int          i64;
  unsigned long long int ui64;
  char                  *str;
  unsigned char         *bytes;
};

// int DllMain() must exist and return 1 for a process to load the .DLL
// See https://docs.microsoft.com/en-us/windows/win32/dlls/dllmain for more
// information.
int __stdcall DllMain(void *module, unsigned int reason, void *reserved) {
  return 1;
}

// truly, there should be only one instance but for now...
struct InstData {
  int beepCode;
  int beepInterval;

  InstData(int beepCode, int beepInterval)
      : beepCode(beepCode), beepInterval(beepInterval) {}
};

void doBeep();   // fwd decl

bool runExtCmd(const char *extCmd, DWORD &exitCode) {
  // required for CreateProcess()
  STARTUPINFO si;
  ZeroMemory(&si, sizeof(si));
  si.cb = sizeof(si);

  PROCESS_INFORMATION pi;
  ZeroMemory(&pi, sizeof(pi));

  // execute command line
  if (!CreateProcess(NULL,   // No module name (use command line)
          (LPSTR)extCmd,     // Command line
          NULL,              // Process handle not inheritable
          NULL,              // Thread handle not inheritable
          FALSE,             // Set handle inheritance to FALSE
          // 0,                  // No window creation flags
          CREATE_NEW_CONSOLE | CREATE_NEW_PROCESS_GROUP,
          // DETACHED_PROCESS,
          // CREATE_NEW_CONSOLE,
          NULL,   // Use parent's environment block
          NULL,   // Use parent's starting directory
          &si,    // Pointer to STARTUPINFO structure
          &pi)    // Pointer to PROCESS_INFORMATION structure
  ) {
    // failed to start external program/command
    return false;
  }

  // // wait until launched process exits (see documentation)
  // WaitForSingleObject(pi.hProcess, INFINITE);

  // loop until process exits
  DWORD waitRes;
  while ((waitRes = WaitForSingleObject(pi.hProcess, 2000)) == WAIT_TIMEOUT) {
    doBeep();
  }

  // if failed?
  if (waitRes != WAIT_OBJECT_0) {
    // handle something failed?
    return false;   // ???
  }

  // try to get return value from post-processing command (should not fail?)
  bool bGetExitCode = GetExitCodeProcess(pi.hProcess, &exitCode);

  // Close process and thread handles
  // (doesn't kill the process, just releases?)
  CloseHandle(pi.hProcess);
  CloseHandle(pi.hThread);

  return bGetExitCode;
}

void openExtProcess() {
  DWORD exitCode;
  if (!runExtCmd("QAlertWait.exe", exitCode)) {
    Display("Error running QAlertWait.\n");
  }

  // QAlertWait doesn't return anything but zero yet...
  if (exitCode) {
    Display("Unexpected error returned from QAlertWait (%d)\n", exitCode);
  }
}

void doBeep() {
  // MessageBeep(beepCode);
  Beep(440, 200);
}

void doAlert() { openExtProcess(); }

// global variables
int  gInstCnt     = 0;
bool gPostProcess = false;

extern "C" __declspec(dllexport) void qalert(
    InstData **opaque, double t, union uData *data) {
  double  In           = data[0].d;   // input
  int     BeepCode     = data[1].i;   // input parameter
  int     BeepInterval = data[2].i;   // input parameter
  double &Out          = data[3].d;   // output

  InstData *inst = *opaque;
  if (!inst) {
    // there should be only one instance...
    if (gInstCnt) {
      // issue message and abort with prejudice
      Display(
          "There are multiple instances of QAlert in this schematic.  Please "
          "remove all but one.\nTerminating simulation.\n");
      exit(1);
    }

    //  *opaque = inst = (InstData *)calloc(sizeof(InstData), 1);
    *opaque = inst = new InstData(BeepCode, BeepInterval);
    if (!inst) {
      // add error handling...
    }
    gInstCnt++;
  }

  // Implement module evaluation code here:
}

extern "C" __declspec(dllexport) void Destroy(InstData *inst) {
  // if in post-process, beep
  if (gPostProcess) doAlert();

  delete inst;
}

extern "C" __declspec(dllexport) void PostProcess(InstData *inst) {
  gPostProcess = true;
}
