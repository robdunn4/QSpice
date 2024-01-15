/*==============================================================================
 * CBlockBasics3.cpp -- DllMain() demonstration code (C Style).
 *
 * Requires QSpice 2023.12.31 release or later.
 *
 * To build with Digital Mars C++ Compiler:
 *    dmc -mn -WD cblockbasics3_c.cpp kernel32.lib
 *============================================================================*/

#include <Windows.h>   // added for DllMain()
#include <malloc.h>
#include <stdarg.h>
#include <stdio.h>
#include <time.h>

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

void display(const char *fmt, ...) {   // for diagnostic print statements
  msleep(30);
  fflush(stdout);
  va_list args = {0};
  va_start(args, fmt);
  vprintf(fmt, args);
  va_end(args);
  fflush(stdout);
  msleep(30);
}

void bzero(void *ptr, unsigned int count) {
  unsigned char *first = (unsigned char *)ptr;
  unsigned char *last  = first + count;
  while (first < last) *first++ = '\0';
}

// #undef pin names lest they collide with names in any header file(s) you might
// include.
#undef Vin
#undef Out

//------------------------------------------------------------------------------
// The instance data structure
//------------------------------------------------------------------------------
struct sCBLOCKBASICS3_C {
  // declare the structure here
  char instID;    // from schematic InstID attribute
  char instNbr;   // instance number based on creation order within step
};

//------------------------------------------------------------------------------
// Global Variables -- Not cleared between steps after QSpice 2023.12.31 update
//------------------------------------------------------------------------------
int   RefCtr  = 0;   // # of currently "active" instances (reference counter)
int   StepCtr = 0;   // current simulation step number
FILE *LogFile;       // log file ("shared resource")
const char *LogFname = "@logfile.txt";

//------------------------------------------------------------------------------
// The DllMain() function
//------------------------------------------------------------------------------
int __stdcall DllMain(void *module, unsigned int reason, void *reserved) {
  switch (reason) {
  case DLL_PROCESS_ATTACH:   // Initialize once for each new process.
    ::LogFile = fopen(::LogFname, "w");

    /* error handling must be included lest failure locks up component */
    if (!::LogFile) {
      display("Unable to create/open logfile (\"%s\").\n", ::LogFname);
      display("Aborting simulation run.\n");
      return 0;
    }

    display("Logfile opened for overwrite (\"%s\").\n", ::LogFname);
    fprintf(::LogFile, "*** Start of File ***\n");
    break;

  case DLL_THREAD_ATTACH:   // Do thread-specific initialization.
    break;

  case DLL_THREAD_DETACH:   // Do thread-specific cleanup.
    break;

  case DLL_PROCESS_DETACH:   // Perform any necessary cleanup.
    // ignoring "reserved" parameter -- don't care why...
    if (::LogFile) {
      fprintf(::LogFile, "*** End of File ***\n");
      fclose(::LogFile);
      ::LogFile = nullptr;
      display("Logfile closed.\n");
    }
    break;
  }

  return 1;
}

//------------------------------------------------------------------------------
// The "Evaluation Function"
//------------------------------------------------------------------------------
extern "C" __declspec(dllexport) void cblockbasics3_c(
    struct sCBLOCKBASICS3_C **opaque, double t, union uData *data) {
  double  Vin    = data[0].d;   // input
  char    InstID = data[1].c;   // input parameter
  double &Out    = data[2].d;   // output

  struct sCBLOCKBASICS3_C *inst = *opaque; /* moved from post-initialization */

  if (!*opaque) { /*** begin instance initialization section ***/
    inst = *opaque =
        (struct sCBLOCKBASICS3_C *)malloc(sizeof(struct sCBLOCKBASICS3_C));
    bzero(*opaque, sizeof(struct sCBLOCKBASICS3_C));

    if (!RefCtr)   // first active instance so increment step counter
      StepCtr++;

    RefCtr++;   // increment reference counter for new active instance

    // any additional instance initialization goes here
    inst->instNbr = RefCtr;
    inst->instID  = InstID;

    // for debugging
    display("Created instance: Schematic ID=%c, DLL Order=%i, Step #=%i\n",
        inst->instID, inst->instNbr, StepCtr);
    fprintf(LogFile,
        "Created instance: Schematic ID=%c, DLL Order=%i, Step #=%i\n",
        inst->instID, inst->instNbr, StepCtr);
  } /*** end of instance initialization section ***/

  // struct sCBLOCKBASICS3_C *inst = *opaque; /* moved to pre-initialization */

  // Implement module evaluation code here:
  Out = Vin * StepCtr;   // just doing something for demonstration purposes
}

/*------------------------------------------------------------------------------
 * The Destroy() function
 *----------------------------------------------------------------------------*/
extern "C" __declspec(dllexport) void Destroy(struct sCBLOCKBASICS3_C *inst) {
  // for debugging
  display("Deleting instance: Schematic ID=%c, DLL Order=%i, Step #=%i\n",
      inst->instID, inst->instNbr, StepCtr);
  fprintf(LogFile,
      "Deleting instance: Schematic ID=%c, DLL Order=%i, Step #=%i\n",
      inst->instID, inst->instNbr, StepCtr);

  free(inst);   // release instance memory
  RefCtr--;     // reduce active instance reference count
}
/*------------------------------------------------------------------------------
 * End of CBlockBasics3.cpp
 *----------------------------------------------------------------------------*/