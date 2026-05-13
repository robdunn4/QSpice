// Automatically generated C++ file on Tue May 12 17:30:05 2026
//
// To build with Digital Mars C++ Compiler:
//
//    dmc -mn -WD -o pic16f1521x_dll.cpp kernel32.lib


union uData
{
   bool b;
   char c;
   unsigned char uc;
   short s;
   unsigned short us;
   int i;
   unsigned int ui;
   float f;
   double d;
   long long int i64;
   unsigned long long int ui64;
   char *str;
   unsigned char *bytes;
};

// int DllMain() must exist and return 1 for a process to load the .DLL
// See https://docs.microsoft.com/en-us/windows/win32/dlls/dllmain for more information.
int __stdcall DllMain(void *module, unsigned int reason, void *reserved) { return 1; }

// #undef pin names lest they collide with names in any header file(s) you might include.
#undef VDD
#undef RA0_I
#undef RA0_O
#undef RA0_C
#undef RA1_I
#undef RA1_O
#undef RA1_C
#undef RA2
#undef RA3_I
#undef RA3_O
#undef RA3_C
#undef RA4_I
#undef RA4_O
#undef RA4_C
#undef RA5_I
#undef RA5_O
#undef RA5_C
#undef VSS

extern "C" __declspec(dllexport) void pic16f1521x_dll(void **opaque, double t, union uData *data)
{
   double  VDD   = data[ 0].d; // input
   double  RA0_I = data[ 1].d; // input
   double  RA1_I = data[ 2].d; // input
   double  RA2   = data[ 3].d; // input
   double  RA3_I = data[ 4].d; // input
   double  RA4_I = data[ 5].d; // input
   double  RA5_I = data[ 6].d; // input
   double  VSS   = data[ 7].d; // input
   double &RA0_O = data[ 8].d; // output
   double &RA0_C = data[ 9].d; // output
   double &RA1_O = data[10].d; // output
   double &RA1_C = data[11].d; // output
   double &RA3_O = data[12].d; // output
   double &RA3_C = data[13].d; // output
   double &RA4_O = data[14].d; // output
   double &RA4_C = data[15].d; // output
   double &RA5_O = data[16].d; // output
   double &RA5_C = data[17].d; // output

   // Implement module evaluation code here:
   RA0_C = 1; // input
   RA1_C = 0; // output
   RA1_O = RA0_I * RA0_I;
}
