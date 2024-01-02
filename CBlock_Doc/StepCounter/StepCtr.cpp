/*==============================================================================
 * StepCtr.cpp -- QSpice C-Block component with reliable ".step" detection and
 * multiple schematic component instances.
 *
 * Requires QSpice 2023.12.31 release or later.
 *
 * Build for DMC with:
 *   "C:\program files\qspice\dm\bin\dmc.exe" -mn -WD StepCtr.cpp kernel32.lib
 *
 * The code should also compile/run successfully on MSVC and MinGW with the
 * included headers.
 *============================================================================*/
#include <cstdlib>

/*------------------------------------------------------------------------------
 * Versioning Information
 *----------------------------------------------------------------------------*/
#define PROGRAM_NAME    "StepCtr"
#define PROGRAM_VERSION "v0.1"
#define PROGRAM_INFO    PROGRAM_NAME " " PROGRAM_VERSION

// must follow above versioning information
#include "Cblock.h"

/*------------------------------------------------------------------------------
 * Globals -- Not cleared between steps after QSpice 2023.12.31 update
 *----------------------------------------------------------------------------*/
int StepCtr = 0;   // current QSpice .step #

/*------------------------------------------------------------------------------
 * Per-instance Data
 *----------------------------------------------------------------------------*/
struct InstData {
  int        instNbr;   // just for testing order of distruction
  static int refCtr;    // reference counter

  // default constructor
  InstData() {
    if (!refCtr++) ::StepCtr++;
    instNbr = refCtr;
  }
  // destructor
  ~InstData() { refCtr--; }
};
int InstData::refCtr = 0;

/*------------------------------------------------------------------------------
 * UDATA() definition -- regenerate the template with QSpice and revise this
 * whenever ports/attributes change; make input/attribute parameters const&
 *----------------------------------------------------------------------------*/
#define UDATA(data)                                                            \
  const double &Vin = data[0].d; /* input */                                   \
  double       &Out = data[1].d; /* output */

/*------------------------------------------------------------------------------
 * Evaluation Function -- name must match DLL name, all lower case
 *----------------------------------------------------------------------------*/
extern "C" __declspec(dllexport) void stepctr(
    InstData **opaque, double t, uData data[]) {

  UDATA(data);

  InstData *inst = *opaque;

  if (!inst) {
    // allocate instance data
    inst = *opaque = new InstData();

    if (!inst) {
      // terminate with prejudice
      msg("Memory allocation failure.  Terminating simulation.\n");
      exit(1);
    }

    msg("Created instance: Instance #=%i, Step #=%i\n", inst->instNbr,
        ::StepCtr);

    // other instance initialization goes here
  }

  // evaluation function code begins here. for example:
  Out = Vin * ::StepCtr;   // multiply input by .step #
}

/*------------------------------------------------------------------------------
 * Destroy()
 *----------------------------------------------------------------------------*/
extern "C" __declspec(dllexport) void Destroy(InstData *inst) {
  msg("Deleting instance: Instance #=%i, Step #=%i\n", inst->instNbr,
      ::StepCtr);
  delete inst;   // free allocated memory
}
/*==============================================================================
 * End of StepCtr.cpp
 *============================================================================*/
