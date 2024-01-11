/*==============================================================================
 * CBlockBasics2.cpp -- Demonstration code (C Style).
 *
 * Requires QSpice 2023.12.31 release or later.
 *
 * To build with Digital Mars C++ Compiler:
 *    dmc -mn -WD cblockbasics2_c.cpp kernel32.lib
 *============================================================================*/
//------------------------------------------------------------------------------
// begin unmodified QSpice-generated template
//------------------------------------------------------------------------------
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

// int DllMain() must exist and return 1 for a process to load the .DLL
// See https://docs.microsoft.com/en-us/windows/win32/dlls/dllmain for more
// information.
int __stdcall DllMain(void *module, unsigned int reason, void *reserved) {
  return 1;
}

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
// end unmodified QSpice-generated template
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
// The instance data structure
//------------------------------------------------------------------------------
struct sCBLOCKBASICS2_C {
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
// The "Evaluation Function"
//------------------------------------------------------------------------------
extern "C" __declspec(dllexport) void cblockbasics2_c(
    struct sCBLOCKBASICS2_C **opaque, double t, union uData *data) {
  double  Vin    = data[0].d;   // input
  char    InstID = data[1].c;   // input parameter
  double &Out    = data[2].d;   // output

  struct sCBLOCKBASICS2_C *inst = *opaque; /* moved from post-initialization */

  if (!*opaque) { /*** begin instance initialization section ***/
    // *opaque =
    //     (struct sCBLOCKBASICS2_C *)malloc(sizeof(struct sCBLOCKBASICS2_C));
    inst = *opaque =
        (struct sCBLOCKBASICS2_C *)malloc(sizeof(struct sCBLOCKBASICS2_C));
    bzero(*opaque, sizeof(struct sCBLOCKBASICS2_C));

    if (!RefCtr) {                        // if first active instance
      if (!StepCtr) {                     // if first simulation step
        LogFile = fopen(LogFname, "w");   // overwrite
        /* error handling omitted for brevity */
        display("Logfile opened for overwrite (\"%s\").\n", LogFname);
      } else {                            // not first simulation step
        LogFile = fopen(LogFname, "a");   // append
        /* error handling omitted for brevity */
        display("Logfile opened for append (\"%s\").\n", LogFname);
      }
      StepCtr++;   // first active instance so increment step counter
    }

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

  // struct sCBLOCKBASICS2_C *inst = *opaque; /* moved to pre-initialization */

  // Implement module evaluation code here:
  Out = Vin * StepCtr;   // just doing something for demonstration purposes
}

/*------------------------------------------------------------------------------
 * The Destroy() function
 *----------------------------------------------------------------------------*/
extern "C" __declspec(dllexport) void Destroy(struct sCBLOCKBASICS2_C *inst) {
  // for debugging
  display("Deleting instance: Schematic ID=%c, DLL Order=%i, Step #=%i\n",
      inst->instID, inst->instNbr, StepCtr);
  fprintf(LogFile,
      "Deleting instance: Schematic ID=%c, DLL Order=%i, Step #=%i\n",
      inst->instID, inst->instNbr, StepCtr);

  free(inst);      // release instance memory
  RefCtr--;        // reduce active instance reference count
  if (!RefCtr) {   // if no more active instances
    fclose(LogFile);
    display("Logfile closed.\n");
  }
}
/*------------------------------------------------------------------------------
 * End of CBlockBasics2.cpp
 *----------------------------------------------------------------------------*/