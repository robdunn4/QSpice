/*******************************************************************************
 * CBlockBasics8.cpp -- Component DLL code for C-Block Basics #8 project.
 *
 * See https://github.com/robdunn4/QSpice/ for additional documentation/code.
 ******************************************************************************/
//
// To build with Digital Mars C++ Compiler:
//    dmc -mn -WD cblockbasics8.cpp kernel32.lib
//
#include "cblockbasics8.h"
#include <Windows.h>
#include <malloc.h>
#include <stdio.h>

/*------------------------------------------------------------------------------
 * Isolating compiler-dependent stuff
 *----------------------------------------------------------------------------*/

#if defined(__DMC__)
#pragma message("*** Compiling with DMC (supported)")

#include <io.h>
inline bool fileExists(const char *fname) { return !_access(fname, 0); }

// DMC doesn't have snprinf(), only vsnprintf()... (really? really?!?)
// so we make our own....
inline int snprintf(char *s, size_t n, const char *format, ...) {
  va_list args;
  va_start(args, format);
  int cnt = vsnprintf(s, n, format, args);
  va_end(args);
  return cnt;
}

#else   // __DMC__ not defined...

#if defined(_MSC_VER)   // MSVC tested
#pragma message("*** Compiling with MSVC (supported)")
#elif defined(__MINGW32__)   // MinGW (GCC) untested
#pragma message("*** Compiling with MinGW (untested, should work)")
#else   // some other compiler else (untested)
#pragma message("*** Compiling with some other compiler (untested, might work)")
#endif

#include <filesystem>
inline bool fileExists(const char *fname) {
  return std::filesystem::exists(fname);
}

#endif

/*------------------------------------------------------------------------------
 * Standard QSpice stuff
 *----------------------------------------------------------------------------*/

extern "C" __declspec(dllexport) int (*Display)(const char *format, ...) = 0;
extern "C" __declspec(dllexport) const int   *StepNumber                 = 0;
extern "C" __declspec(dllexport) const int   *NumberSteps                = 0;
extern "C" __declspec(dllexport) const char **InstanceName               = 0;

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

int __stdcall DllMain(void *module, unsigned int reason, void *reserved) {
  return 1;
}

/*------------------------------------------------------------------------------
 * Global Variables
 *----------------------------------------------------------------------------*/

const int MaxPath = 256;   // maximum filename/path length (beware, hardcoded!)

FILE *gLogFile                 = NULL;   // shared file handle
char  gPgmName[MaxPath]        = "";     // post-processing exe
char  gLogFileBase[MaxPath]    = "";     // base logfile name (no extension)
char  gLogFileBin[MaxPath + 5] = "";     // logFileBase + ".bin"
char  gLogFileCsv[MaxPath + 5] = "";     // logFileBase + ".csv"
char  gPostProcessCmd[(MaxPath + 5) * 4] =
    "";   // four parameters + quotes & spaces?

bool gPostProcess = false;   // set to true in PostProcess()
int  gInstRefCnt  = 0;       // active component instance reference counter

/*------------------------------------------------------------------------------
 * Functions
 *----------------------------------------------------------------------------*/

// openLogFile() -- open log file for binary create/overwrite.
//
// parameters:
//   fname -- pointer to file system file name/path for file.
//
// returns:
//   non-null pointer on success; null on failure
//
FILE *openLogFile(const char *fname) {
  // open file
  FILE *file = fopen(fname, "w+b");

  // if you want to write some file header data, you'd do that here.
  // if (file) {
  //   // write some header data here
  // }

  return file;
}

// runExtCmd() -- launches an external post-processing command. the function
// does not return until the command completes.  the command can be an
// executable or batch file (*.exe, *.bat, *.cmd).
//
// parameters:
//   extCmd -- pointer to command line to execute.
//   exitCode -- exit value returned by command on successful execution.
//
// returns:
//    true if command is successfully executed; false otherwise.
//
// note:  other Windows APIs could be used to launch external process.  see
//        documentation.
//
bool runExtCmd(const char *extCmd, DWORD &exitCode) {
  // required for CreateProcess()
  STARTUPINFO si;
  ZeroMemory(&si, sizeof(si));
  si.cb = sizeof(si);

  PROCESS_INFORMATION pi;
  ZeroMemory(&pi, sizeof(pi));

  // execute command line
  if (!CreateProcessA(NULL,   // No module name (use command line)
          (LPSTR)extCmd,      // Command line
          NULL,               // Process handle not inheritable
          NULL,               // Thread handle not inheritable
          FALSE,              // Set handle inheritance to FALSE
          // 0,                  // No window creation flags
          CREATE_NEW_CONSOLE | CREATE_NEW_PROCESS_GROUP,
          NULL,   // Use parent's environment block
          NULL,   // Use parent's starting directory
          &si,    // Pointer to STARTUPINFO structure
          &pi)    // Pointer to PROCESS_INFORMATION structure
  ) {
    // failed to start external program/command
    return false;
  }

  // wait until launched process exits (see documentation)
  WaitForSingleObject(pi.hProcess, INFINITE);

  // try to get return value from post-processing command (should not fail?)
  bool bGetExitCode = GetExitCodeProcess(pi.hProcess, &exitCode);

  // Close process and thread handles
  CloseHandle(pi.hProcess);
  CloseHandle(pi.hThread);

  return bGetExitCode;
}

// logBinData() -- writes a record to the binary logfile.
//
// parameters:
//   inst -- pointer to per-instance data.
//   instName -- pointer to component instance name.
//
void logBinData(InstData *inst, const char *instName) {
  BinData data(inst, instName);
  fwrite(&data, sizeof(data), 1, gLogFile);   // should add error handling...
}

// makeFileNames() -- populates global variables with output filenames and
// post-processing command line; verifies that post-processing file exists (does
// not validate that it can be opened/launched); does not return if fatal error.
//
// parameters:
//   logName -- pointer to base name for logs (no extension).
//   pgmName -- pointer to external program or command file (*.exe, *.bat,
//     *.cmd)
//
void makeFileNames(const char *logName, const char *pgmName) {
  // do some minimal error checking of name/path lengths
  if (strlen(logName) > sizeof(gLogFileBase) - 1) {
    Display("LogFile attribute is too long.  Aborting...\n");
    exit(1);   // abort with prejudice
  }
  if (strlen(pgmName) > sizeof(gPgmName) - 1) {
    Display("PgmName attribute is too long.  Aborting...\n");
    exit(2);   // abort with prejudice
  }

  // copy to globals; build names
  strncpy(gLogFileBase, logName, sizeof(gLogFileBase));
  strcpy(gLogFileBin, gLogFileBase);
  strcat(gLogFileBin, ".bin");
  strcpy(gLogFileCsv, gLogFileBase);
  strcat(gLogFileCsv, ".csv");
  strncpy(gPgmName, pgmName, sizeof(gPgmName));

  // build command line for post-processing
  int cmdLen = snprintf(gPostProcessCmd, sizeof(gPostProcessCmd) - 1,
      "\"%s\" \"%s\" \"%s\" \"%s.log\"", gPgmName, gLogFileBin, gLogFileCsv,
      gLogFileBase);

  if (cmdLen < 0 || cmdLen > sizeof(gPostProcessCmd) - 1) {
    // command does not fit inside buffer (should not happen)
    Display(
        "Unexpected error formatting post-processing command (command line too "
        "long)");
    exit(3);
  }

  // verify that executable exists
  if (!fileExists(gPgmName)) {
    Display(
        "Post-processing program/command \"%s\" does not exist.  Aborting...\n",
        gPgmName);
    exit(4);   // abort with prejudice
  }
}

// initOnce() -- initialize global stuff; open log file; no return on fatal
// error.
//
// parameters:
//   logName -- pointer to base log file name (no extension).
//   pgmName -- name of post-processing program or command (*.exe, *.bat, *.cmd)
//     to launch.
//
void initOnce(const char *logName, const char *pgmName) {
  // make the needed filenames & post-processing command line
  makeFileNames(logName, pgmName);

  // open log file (*.bin)
  gLogFile = openLogFile(gLogFileBin);
  if (!gLogFile) {
    Display("Unable to open binary log file \"%s\" for writing.  Aborting...\n",
        gLogFileBin);
    exit(5);   // abort with prejudice
  }

  // displaly values in use
  Display("Binary LogFile: \"%s\"\nPost-Processor: \"%s\"\n", gLogFileBin,
      gPgmName);
}

/*------------------------------------------------------------------------------
 * QSpice component entry points
 *----------------------------------------------------------------------------*/

extern "C" __declspec(dllexport) void cblockbasics8(
    InstData **opaque, double t, union uData *data) {
  double      Vin     = data[0].d;     // input
  const char *LogFile = data[1].str;   // input parameter
  const char *PgmName = data[2].str;   // input parameter
  int         Step    = data[3].i;     // input parameter
  double     &Vout    = data[4].d;     // output

  InstData *inst = *opaque;

  if (!inst) { /* per-instance component initialization */
    // if first instance & first step, do one-time startup initialization
    if (!gInstRefCnt && (*StepNumber == 1)) initOnce(LogFile, PgmName);

    // increment number of active instances
    gInstRefCnt++;

    // allocate per-instance structure storage (error handling omitted)
    inst = *opaque = (InstData *)calloc(1, sizeof(InstData));
    Display("Initializing %s (instNbr %i) for step %d of %d\n", *InstanceName,
        gInstRefCnt, *StepNumber, *NumberSteps);

    // additional one-time instance intialization
    inst->instNbr  = gInstRefCnt;
    inst->stepNbr  = *StepNumber;
    inst->totSteps = *NumberSteps;
  } /* end per-instance initialization */

  /* Implement module evaluation code here... */

  // do something useful (example)
  Vout = Vin * Step;

  // update per-instance data
  //
  // note:  QSpice calls the evaluation function a few times during
  // initialization.  that is, some of these calls aren't actually recorded by
  // QSpice as simulation points; there may be multiple calls with t==0.0, etc.
  // we'll simply log all of the calls and state information...
  inst->simPtCnt++;
  inst->stepVal  = Step;
  inst->lastT    = t;
  inst->lastVin  = Vin;
  inst->lastVout = Vout;

  // log some data
  logBinData(inst, *InstanceName);
}

extern "C" __declspec(dllexport) void Destroy(InstData *inst) {
  // do anything required before releasing this per-instance data for this
  // simulation step
  Display(
      "Destroy() called for InstanceName=%s, instNbr=%i, stepNbr=%i, "
      "stepVal=%i, StepNumber=%i of NumberSteps=%i, SimPtCount=%i\n",
      *InstanceName, inst->instNbr, inst->stepNbr, inst->stepVal, *StepNumber,
      *NumberSteps, inst->simPtCnt);

  // free the per-instance data and decrement the number of active instances
  free(inst);
  gInstRefCnt--;

  // return if there are more active instances or post-processing flag not set
  if (!gPostProcess || gInstRefCnt) return;

  // this is the last remaining instance of the final simulation step; close
  // log file and do post-processing
  fclose(gLogFile);
  Display("\nLogFile closed...\n");

  // run the post-processing command and display results
  Display("Launching post-processing command: %s\n", gPostProcessCmd);

  DWORD exitCode = 0;
  bool  runRes   = runExtCmd(gPostProcessCmd, exitCode);

  if (!runRes) {
    Display("Failed to execute post-processing command.\n");
    return;
  }

  Display(
      "Post-processing command was executed.  Exit code was %ld.\n", exitCode);
}

extern "C" __declspec(dllexport) void PostProcess(InstData *inst) {
  // if there is any per-instance finalization to be done, it could be done
  // here or in Destroy();  here, we delegate those to Destroy()
  Display("PostProcess() called for instance %s\n", *InstanceName);

  // set the global flag indicating that we have completed the final
  // simulation step for the simulation
  gPostProcess = true;
}

/*******************************************************************************
 * EOF CBlockBasics8.cpp
 ******************************************************************************/
