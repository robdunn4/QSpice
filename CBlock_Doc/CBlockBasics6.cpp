//==============================================================================
// CBlockBasics6.cpp -- Demonstration code for CBlockBasics6.qsch.
//==============================================================================
// To build with Digital Mars C++ Compiler:
//    dmc -mn -WD cblockbasics6.cpp kernel32.lib
//

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

// #undef pin names lest they collide with names in any header file(s) you might
// include.
#undef Bin_3_
#undef Bin_2_
#undef Bin_1_
#undef Bin_0_
#undef Bout_3_
#undef Bout_2_
#undef Bout_1_
#undef Bout_0_

struct InstData {
  // empty struct for now...
};

extern "C" __declspec(dllexport) void cblockbasics6(
    InstData **opaque, double t, uData *data) {
  bool  Bin_3_  = data[0].b;   // input
  bool  Bin_2_  = data[1].b;   // input
  bool  Bin_1_  = data[2].b;   // input
  bool  Bin_0_  = data[3].b;   // input
  bool &Bout_3_ = data[4].b;   // output
  bool &Bout_2_ = data[5].b;   // output
  bool &Bout_1_ = data[6].b;   // output
  bool &Bout_0_ = data[7].b;   // output

  InstData *inst = *opaque;

  if (!inst) {
    // allocate instance data -- production code should add error handling
    *opaque = inst = new InstData();

    printf("%s\n", __FILE__);   // display file name to confirm code version
  }

  // just copy input to output
  Bout_0_ = Bin_0_;
  Bout_1_ = Bin_1_;
  Bout_2_ = Bin_2_;
  Bout_3_ = Bin_3_;
}

extern "C" __declspec(dllexport) void Destroy(InstData *inst) { delete inst; }
//==============================================================================
// End of CBlockBasics6.cpp
//==============================================================================