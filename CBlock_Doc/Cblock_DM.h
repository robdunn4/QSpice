/*==============================================================================
 * Cblock_DM.h -- Header file for custom QSpice CBlock template for DMC.
 *
 * Note:  This header contains both declarations and implementations.  This is
 * bad coding style but, for a single compilation unit (i.e., a single *.cpp),
 * it's simple and doesn't require more complicated multi-object compiles/links.
 *============================================================================*/
#ifndef CBLOCK_DM_H
#define CBLOCK_DM_H

#define TOOLSET "DMC"

#include <cstdio>
#include <cstdarg>

// were stuck with this for DMC because of the msleep() call
#include <time.h>

#define msg(...) msg_(__LINE__, __VA_ARGS__)

// QSpice printf() to Output window with embedded process waits...
void msg_(int lineNbr, const char *fmt, ...) {
  msleep(30);   // proprietary to Digital Mars libraries?
  fflush(stdout);
  fprintf(stdout, PROGRAM_INFO " @%d: ", lineNbr);
  va_list args = {0};
  va_start(args, fmt);
  vprintf(fmt, args);
  va_end(args);
  fflush(stdout);
  msleep(30);   // proprietary to Digital Mars libraries?
}

// int DllMain() must exist and return 1 for a process to load the .DLL
int __stdcall DllMain(void *module, unsigned int reason, void *reserved) {
  return 1;
}

#endif   // CBLOCK_DM_H
/*==============================================================================
 * End of Cblock_DM.h
 *============================================================================*/
