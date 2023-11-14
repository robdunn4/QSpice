/*------------------------------------------------------------------------------
 * DbgLog.h -- Log debugging information to file.
 *
 * This header #includes the appropriate *_DM.h, *_MGW, or *_VC.h
 * compiler-specific header.
 *
 * See DbgLogTest.cpp for a demonstration of logging with this toolset.
 *
 * Caution:  This is *strictly for debugging.*  It does not contain significant
 * error checking.
 *----------------------------------------------------------------------------*/
#ifndef DBGLOG_H
#define DBGLOG_H

#if defined(__DMC__)
#include "DbgLog_DM.h"
#elif defined(_MSC_VER)
#include "DbgLog_VC.h"
#elif defined(__MINGW32__)
#include "DbgLog_MGW.h"
#else
#error "Compiler not recognized."
#endif

#endif   // DBGLOG_H
/*------------------------------------------------------------------------------
 * End of DbgLog.h
 *----------------------------------------------------------------------------*/