/*------------------------------------------------------------------------------
 * DbgLog_DM.h -- Log debugging information to file, DMC version.
 *
 * See DbgLogTest.cpp for a demonstration of logging with this toolset.
 *
 * Caution:  This is *strictly for debugging.*  It does not contain significant
 * error checking.
 *
 * Note:  This header contains both declarations and implementations.  This is
 * bad coding style but, for a single compilation unit (i.e., a single *.cpp),
 * it's simple and doesn't require more complicated multi-object compiles/links.
 *----------------------------------------------------------------------------*/
#ifndef DBGLOG_DM_H
#define DBGLOG_DM_H

#include <cstdio>
#include <cstdarg>
#include <time.h>   // for DMC msleep() only

// convenience #defines -- use LOG() inside functions that have a time parameter
// named "t" that contains the simulation time-point; use LOG() if you don't
// have (or don't care about) the simulation timepoint
#define LOG(...)  dbgLog.log_(__LINE__, __VA_ARGS__)
#define LOGT(...) dbgLog.log_(__LINE__, t, __VA_ARGS__)

class DbgLog {
public:
  DbgLog(const char *filePath, unsigned int maxLines = 100)
      : maxLines(maxLines) {
    file = fopen(filePath, "w");
    if (!file) {
      display(
          "DbgLog:  Unable to open \"%s\" for logging.  Logging is disabled.\n",
          filePath);
      return;
    }
    display(
        "DbgLog:  Logging to \"%s\".  Maximum lines=%u\n", filePath, maxLines);

    fprintf(file, "Simulation Time....@Line: Log Message...");
  }

  ~DbgLog() {
    fprintf(file, "\n*** EOF, %u lines logged ***\n", lineCnt);
    fflush(file);
    fclose(file);
  }

  // internal class copy of QSpice function to remove dependencies
  void display(const char *fmt, ...) {
    msleep(30);
    fflush(stdout);
    va_list args = {0};
    va_start(args, fmt);
    vprintf(fmt, args);
    va_end(args);
    fflush(stdout);
    msleep(30);
  }

  void log_(int lineNbr, const char *fmt...) {
    if (lineCnt == maxLines) return;
    lineCnt++;

    // prefix output with spacer & line #
    fprintf(file, "\n-----------------------@%04d: ", lineNbr);

    // output variable portion
    va_list args = {0};
    va_start(args, fmt);
    vfprintf(file, fmt, args);
    va_end(args);

    // flush in case of early simulation termination
    fflush(file);
  }

  void log_(int lineNbr, double t, const char *fmt...) {
    if (lineCnt == maxLines) return;
    lineCnt++;

    // prefix output with t (time) & line #
    fprintf(file, "\n%0.17e@%04d: ", t, lineNbr);

    // output variable portion
    va_list args = {0};
    va_start(args, fmt);
    vfprintf(file, fmt, args);
    va_end(args);

    // flush in case of early simulation termination
    fflush(file);
  }

protected:
  FILE        *file;
  unsigned int maxLines;
  unsigned int lineCnt;
};

#endif   // DBGLOG_DM_H
/*------------------------------------------------------------------------------
 * End of DbgLog_DM.h
 *----------------------------------------------------------------------------*/