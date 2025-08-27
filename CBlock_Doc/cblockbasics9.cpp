// CBlockBasics9.cpp -- Demonstration code for the QSpice EngAtof() function.
// The complete project is available at https://github.com/robdunn4/QSpice.
//
// To build with Digital Mars C++ Compiler:
//    dmc -mn -WD cblockbasics9.cpp kernel32.lib
//
// Note:  isnan() isn't in std namespace for DMC compiler.  Use as is for DMC.
// For modern compilers, add the "std::" namespace qualifier.

#include <cmath>      // for isnan()
#include <cstdio>     // for sprintf()
#include <malloc.h>   // for calloc()

extern "C" __declspec(dllexport) void (*Display)(const char *format, ...) = 0;
extern "C" __declspec(dllexport) void (*EXIT)(const char *format, ...)    = 0;
extern "C" __declspec(dllexport) double (*EngAtof)(const char **string)   = 0;

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

struct InstData {
  double gain;
};

extern "C" __declspec(dllexport) void cblockbasics9(
    InstData **opaque, double t, uData *data) {
  double      IN          = data[0].d;     // input
  const char *ExprStrAttr = data[1].str;   // input parameter
  double     &OUT         = data[2].d;     // output

  /*
   * initialization section
   */
  InstData *inst = *opaque;
  if (!inst) {
    inst = *opaque = (InstData *)calloc(1, sizeof(InstData));

    /*
     * EngAtof() demonstrations
     */
    // Ex 1: EngAtof() parses a list of one or more space-delimited expressions.
    // (Expressions with embedded spaces should be enclosed in "()" or "{}".)
    // It then advances the pointer to the next expression or null byte at end
    // of string. It returns NaN (Not A Number) if expression is invalid.
    Display("(1) Evaluating ExprStrAttr: \"%s\"\n", ExprStrAttr);

    const char *pExpr = ExprStrAttr;
    double      dVal  = 0;

    while (*pExpr && !isnan(dVal)) {
      const char *pFail = pExpr;
      dVal              = EngAtof(&pExpr);
      if (isnan(dVal)) {
        Display("    ExprStrAttr contained an invalid expression.\n");
        Display("    Error detected beginning at \"%s\"\n", pFail);
      } else
        Display("    Value: %g\n", dVal);
    }

    // Ex 2: We can retrieve a .param value directly, i.e., without passing
    // via a String Attribute (e.g., "double val=EvilVal").  We still must set
    // up the indrect pointer (pExpr) as if parsing a list even though
    // evaluating a single expression.  Note that this provides a way to
    // determine if a user-defined variable exists.
    // const char evilValExpr[] = "EvilVal";   // this won't work
    const char evilValExpr[] = "evilval";   // must be lower case
    pExpr                    = (const char *)&evilValExpr;

    inst->gain = EngAtof(&pExpr);
    if (isnan(inst->gain))
      Display("\n(2) .param EvilVal is either missing or invalid.\n");
    else
      Display("\n(2) .param EvilVal=%g\n", inst->gain);

    // Ex 3: We can construct an expression at runtime.  Here, we'll evaluate a
    // user-defined function with an arbitrary runtime value.
    double rtVal = 3;     // some arbitrary runtime value
    char   rtExpr[128];   // buffer for expression string

    sprintf(rtExpr, "userfunc(%g)", rtVal);   // note lower case

    Display("\n(3) Evaluating \".func UserFunc(x)\" for x=%g\n", rtVal);
    Display("    Expression string is \"%s\"\n", rtExpr);

    pExpr       = (const char *)&rtExpr;
    double dRes = EngAtof(&pExpr);
    if (isnan(dRes))
      Display(
          "    .func UserFunc() missing or expression string was invalid.\n");
    else
      Display("    EngAtof(\"%s\") returned %g\n", rtExpr, dRes);
  }

  /*
   * evaluation section
   */
  // note:  we probably shouldn't call EngAtof() here due to computational
  // overhead...
  OUT = inst->gain * IN;
}

extern "C" __declspec(dllexport) void Destroy(InstData *inst) { free(inst); }
