//==============================================================================
// CBlockBasics6_Alt1.cpp -- Demonstration code for CBlockBasics6.qsch.
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

// structure to contain an 8-bit register
union BitReg8 {
  unsigned char val8;
  struct {
    unsigned char b0 : 1;
    unsigned char b1 : 1;
    unsigned char b2 : 1;
    unsigned char b3 : 1;
    unsigned char b4 : 1;
    unsigned char b5 : 1;
    unsigned char b6 : 1;
    unsigned char b7 : 1;
  };
  struct {
    unsigned char ls4 : 4;   // low nibble
    unsigned char ms4 : 4;   // high nibble
  };

  // constructor initializes to zero
  BitReg8() : val8(0) {}

  // assignment operator
  BitReg8 &operator=(const BitReg8 &rhs) {
    this->val8 = rhs.val8;
    return *this;
  }
};

// initialize a BitReg8 from uData array (high bit to low bit order)
BitReg8 getReg8(const uData *data, int bitCnt) {
  BitReg8 reg;
  for (int i = 0; i < bitCnt; i++) {
    reg.val8 <<= 1;
    reg.val8 |= (unsigned char)data[i].b;
  }

  return reg;
}

// set uData array values from a BitReg8
// note:  do not write to an input port!
BitReg8 setReg8(uData *data, int bitCnt, BitReg8 bitReg8) {
  unsigned char val = bitReg8.val8;
  for (int i = bitCnt - 1; i >= 0; i--) {
    data[i].b = bool(val & 0x01);
    val >>= 1;
  }

  return bitReg8;
}

// Per-instance data structure
struct InstData {
  BitReg8 inReg;
  BitReg8 outReg;
};

extern "C" __declspec(dllexport) void cblockbasics6_alt1(
    InstData **opaque, double t, uData *data) {
  // QSpice generates these but don't actually need them in the example although
  // it's useful to keep the commented code for reference
  // bool  Bin_3_  = data[0].b;   // input
  // bool  Bin_2_  = data[1].b;   // input
  // bool  Bin_1_  = data[2].b;   // input
  // bool  Bin_0_  = data[3].b;   // input
  // bool &Bout_3_ = data[4].b;   // output
  // bool &Bout_2_ = data[5].b;   // output
  // bool &Bout_1_ = data[6].b;   // output
  // bool &Bout_0_ = data[7].b;   // output

  // set up uData array offset pointers to bus data
  // note: input bus is const to reduce chance of accidental writing to
  // inputs
  const uData *pInBusData4  = &data[0];
  uData       *pOutBusData4 = &data[4];

  // local register variables initialized with bus bit values
  BitReg8 inReg  = getReg8(pInBusData4, 4);
  BitReg8 outReg = getReg8(pOutBusData4, 4);

  // initialization code
  InstData *inst = *opaque;
  if (!inst) {
    // allocate instance data -- production code should add error handling
    *opaque = inst = new InstData();
    // display cpp file name during debugging of multiple versions
    printf("%s\n", __FILE__);
  }

  // save bus values into instance data (not required, just for demo)
  inst->inReg  = inReg;
  inst->outReg = outReg;

  // we can manipulate the BitReg8 stuff using the val8, ls4, ms4,
  // or bit# union definitions
  // first, let's assign in reg values to out reg values in instance data
  inst->outReg = inst->inReg;
  // and invert the low nibble just for demonstration purposes
  inst->outReg.ls4 ^= 0x0f;

  // finally, set the output port values from the per-instance data
  setReg8(pOutBusData4, 4, inst->outReg);
}

extern "C" __declspec(dllexport) void Destroy(InstData *inst) { delete inst; }
//==============================================================================
// End of CBlockBasics6_Alt1.cpp
//==============================================================================