/*------------------------------------------------------------------------------
 * Cblock.h -- Header file for custom QSpice CBlock template.
 *
 * This header #includes the appropriate *_DM.h, *_MGW.h or *_VC.h
 * compiler-specific header.
 *----------------------------------------------------------------------------*/
#ifndef CBLOCK_H
#define CBLOCK_H

#if defined(__DMC__)
#include "Cblock_DM.h"

#elif defined(_MSC_VER)
#include "Cblock_VC.h"

#elif defined(__MINGW32__)
#include "Cblock_MGW.h"
#endif

#ifndef TOOLSET
#error "Compiler not recognized."
#endif

// the data type union for ports and attributes
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

#endif   // CBLOCK_H
/*==============================================================================
 * End of Cblock.h
 *============================================================================*/
