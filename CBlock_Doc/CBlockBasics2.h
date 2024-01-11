/*------------------------------------------------------------------------------
 * CBlockBasics2.h -- Header file for custom QSpice CBlock template.
 *
 * This header #includes the appropriate *_DM.h, *_MGW.h or *_VC.h
 * compiler-specific header.
 *----------------------------------------------------------------------------*/
#ifndef CBlockBasics2_H
#define CBlockBasics2_H

#if defined(__DMC__)
#include "CBlockBasics2_DM.h"

#elif defined(_MSC_VER)
#include "CBlockBasics2_VC.h"

#elif defined(__MINGW32__)
#include "CBlockBasics2_MGW.h"
#endif

#ifndef TOOLSET
#error "Compiler not recognized."
#endif

#endif   // CBlockBasics2_H
/*==============================================================================
 * End of CBlockBasics2.h
 *============================================================================*/
