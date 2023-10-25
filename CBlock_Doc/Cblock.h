/*------------------------------------------------------------------------------
 * Cblock.h -- Header file for custom QSpice CBlock template.
 *
 * This header #includes the appropriate *_DM.h or *_VC.h compiler-specific
 * header.
 *----------------------------------------------------------------------------*/
#ifndef CBLOCK_H
#define CBLOCK_H

#if defined(__DMC__)
#include "Cblock_DM.h"
#elif defined(_MSC_VER)
#include "Cblock_VC.h"
#else
#error "Compiler not recognized."
#endif

#endif   // CBLOCK_H
/*==============================================================================
 * End of Cblock.h
 *============================================================================*/
