/*==============================================================================
 * Cblock.cpp -- Custom QSpice C-Block component code template.
 *============================================================================*/
#include <cstdlib>

/*------------------------------------------------------------------------------
 * Versioning Information
 *----------------------------------------------------------------------------*/
#define PROGRAM_NAME    "Cblock"
#define PROGRAM_VERSION "v0.2"
#define PROGRAM_INFO    PROGRAM_NAME " " PROGRAM_VERSION

// must follow above versioning information
#include "Cblock.h"

/*------------------------------------------------------------------------------
 * DbgLog
 *----------------------------------------------------------------------------*/
// declare DbgLog instance for logging; instance name must be dbgLog; change
// file name and max line limit if desired (-1 = max).
#include "DbgLog.h"
DbgLog dbgLog("@qdebug.log", 100);

/*------------------------------------------------------------------------------
 * Per-instance Data
 *----------------------------------------------------------------------------*/
struct InstData {
  int    someInt;   // for example
  double someDbl;   // for example

  // by default, C++ initializes trivial data members to zero bytes; if
  // different initial values are needed, you can use something like this...
  InstData() : someInt(1), someDbl(1.0) {}
};

/*------------------------------------------------------------------------------
 * UDATA() definition -- regenerate the template with QSpice and revise this
 * whenever ports/attributes change; make input/attribute parameters const&
 *----------------------------------------------------------------------------*/
#define UDATA(data)                                                            \
  const double &In            = data[0].d; /* input */                         \
  const int    &SomeAttribute = data[1].i; /* input parameter */               \
  double       &Out           = data[2].d; /* output */

/*------------------------------------------------------------------------------
 * Evaluation Function -- name must match DLL name, all lower case
 *----------------------------------------------------------------------------*/
extern "C" __declspec(dllexport) void cblock(
    InstData **opaque, double t, uData data[]) {

  UDATA(data);

  InstData *inst = *opaque;

  if (!inst) {
    // allocate instance data
    inst = *opaque = new InstData;

    if (!inst) {
      // terminate with prejudice
      msg("Memory allocation failure.  Terminating simulation.\n");
      exit(1);
    }

    // if important, output component parameters
    msg("DLL compiled with toolset: %s\n", TOOLSET);
    msg("Component loaded (SomeAttribute=%d).\n", SomeAttribute);
  }

  // evaluation function code begins here. for example:
  if (inst->someInt == 1) {
    // do something
  }
  Out = In * SomeAttribute;

  // some debugging stuff
  LOGT("evalfunc() t=%0.17e", t);
}

/*------------------------------------------------------------------------------
 * MaxExtStepSize()
 *----------------------------------------------------------------------------*/
extern "C" __declspec(dllexport) double MaxExtStepSize(InstData *inst) {

  const double abortVal = -1e308;   // cancels simulation
  double       retVal   = 1e308;    // default == eternity

  // some testing/debugging stuff...
  // retVal = 5e-6;
  LOG("MaxExtStepSize() returning %0.17e", retVal);

  return retVal;
}

/*------------------------------------------------------------------------------
 * Trunc()
 *----------------------------------------------------------------------------*/
extern "C" __declspec(dllexport) void Trunc(
    InstData *inst, double t, uData data[], double *timestep) {

  UDATA(data);
  const double ttol = 1e-9;

  // note: the below is modeled on the QSpice sample code.  it is not, however,
  // the only way -- or even an efficient way for a particular use case -- to
  // determine what to return via *timestep from Trunc().

  if (*timestep > ttol) {
    // we're going to pass the uData parameter to the evaluation function.  save
    // a copy of any array elements that might be changed (i.e., output ports)
    const double _Out = Out;

    // call the evaluation function with a copy of the instance data
    InstData  tmp  = *inst;   // make a copy of inst data
    InstData *pTmp = &tmp;
    cblock(&pTmp, t, data);

    // compare elements of the original (inst) and evaluated (tmp) to determine
    // if the timestep should change, for example:
    if (tmp.someInt != inst->someInt || tmp.someDbl != inst->someDbl)
      *timestep = ttol;

    // Restore uData output values that might have been changed during the
    // evaluation function call
    Out = _Out;
  }

  // some debugging stuff
  LOGT("Trunc()    t=%0.17e, *timestep=%0.17e", t, *timestep);
}

/*------------------------------------------------------------------------------
 * Destroy()
 *----------------------------------------------------------------------------*/
extern "C" __declspec(dllexport) void Destroy(InstData *inst) {
  // if important, output a final component message; for example:
  msg("Done, records processed, file closed, whatever.\n");

  // free allocated memory
  delete inst;
}
/*==============================================================================
 * End of Cblock.cpp
 *============================================================================*/
