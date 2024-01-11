/*==============================================================================
 * CBlockBasics2.cpp -- Demonstration code (C++ Style).
 *
 * Requires QSpice 2023.12.31 release or later.
 *
 * To build with Digital Mars C++ Compiler:
 *    dmc -mn -WD cblockbasics2.cpp kernel32.lib
 *
 * The code should also compile/run successfully on MSVC and MinGW with the
 * included headers.
 *============================================================================*/
#include <cstdio>
#include <cstdlib>

/*------------------------------------------------------------------------------
 * Versioning Information
 *----------------------------------------------------------------------------*/
#define PROGRAM_NAME    "CBlockBasics2"
#define PROGRAM_VERSION "v0.1"
#define PROGRAM_INFO    PROGRAM_NAME " " PROGRAM_VERSION

// must follow above versioning information
#include "CBlockBasics2.h"

//------------------------------------------------------------------------------
// Global Variables -- Not cleared between steps after QSpice 2023.12.31 update
//------------------------------------------------------------------------------
int StepCtr = 0;   // current QSpice .step #

// work around DMC bug -- explicit initialization of LogFile causes DMC compile
// to hang in sccpn.exe process.  (WTF?!?)
#if !defined(__DMC__)
FILE *LogFile = nullptr;   // log file ("shared resource")
#else
FILE *LogFile;   // log file ("shared resource")
#endif

/*------------------------------------------------------------------------------
 * Per-instance Data
 *----------------------------------------------------------------------------*/
struct InstData {
  int        instNbr;   // instance number based on order of creation in DLL
  char       instID;    // instance ID from schematic InstID attribute
  static int refCtr;    // reference counter

  InstData(char instID) : instID(instID) {
    const char *logFname = "@logfile.txt";
    if (!refCtr) {        // if first instance within sim step
      if (!::StepCtr) {   // if first sim step
        ::LogFile = fopen(logFname, "w");
        /* error handling omitted for brevity */
        msg("Logfile opened for overwrite (\"%s\").\n", logFname);
      } else {
        ::LogFile = fopen(logFname, "a");
        /* error handling omitted for brevity */
        msg("Logfile opened for append (\"%s\").\n", logFname);
      }
      ::StepCtr++;
    }
    refCtr++;
    instNbr = refCtr;
  }

  ~InstData() {
    refCtr--;
    if (!refCtr) {   // if last active instance within sim step
      fclose(::LogFile);
      ::LogFile = nullptr;
      msg("Logfile closed.\n");
    }
  }
};
int InstData::refCtr = 0;

/*------------------------------------------------------------------------------
 * UDATA() definition -- regenerate the template with QSpice and revise this
 * whenever ports/attributes change; make input/attribute parameters const&
 *----------------------------------------------------------------------------*/
#define UDATA(data)                                                            \
  const double &Vin    = data[0].d; /* input */                                \
  const char   &InstID = data[1].c; /* input */                                \
  double       &Out    = data[2].d; /* output */

/*------------------------------------------------------------------------------
 * Evaluation Function -- name must match DLL name, all lower case
 *----------------------------------------------------------------------------*/
extern "C" __declspec(dllexport) void cblockbasics2(
    InstData **opaque, double t, uData data[]) {

  UDATA(data);

  InstData *inst = *opaque;

  if (!inst) { /*** begin instance initialization section ***/
    // allocate instance data
    inst = *opaque = new InstData(InstID);

    if (!inst) {
      // allocation failed -- terminate with prejudice
      msg("Memory allocation failure.  Terminating simulation.\n");
      exit(1);
    }

    // any additional instance initialization goes here

    msg("Created instance: Schematic ID=%c, DLL Order=%i, Step #=%i\n",
        inst->instID, inst->instNbr, ::StepCtr);
    fprintf(::LogFile,
        "Created instance: Schematic ID=%c, DLL Order=%i, Step #=%i\n",
        inst->instID, inst->instNbr, ::StepCtr);
  } /*** end instance initialization section ***/

  // evaluation function code begins here. for example:
  Out = Vin * ::StepCtr;   // multiply input by .step # for demo
}

/*------------------------------------------------------------------------------
 * Destroy()
 *----------------------------------------------------------------------------*/
extern "C" __declspec(dllexport) void Destroy(InstData *inst) {
  msg("Deleting instance: Schematic ID=%c, DLL Order=%i, Step #=%i\n",
      inst->instID, inst->instNbr, ::StepCtr);
  fprintf(::LogFile,
      "Deleting instance: Schematic ID=%c, DLL Order=%i, Step #=%i\n",
      inst->instID, inst->instNbr, ::StepCtr);
  delete inst;
}
/*==============================================================================
 * End of CBlockBasics2.cpp
 *============================================================================*/
