/*------------------------------------------------------------------------------
 * DbgLog.h -- Log debugging information to file.
 *
 * See DbgLogTest.cpp for a demonstration of logging with this toolset.
 *
 * Caution:  This is *strictly for debugging.*  It does not contain significant
 * error checking.
 *
 * Note:  This header file works with the Digital Mars compiler supplied with
 * QSpice.  I will likely be making a version for more modern compilers that
 * works around some of the compromises and ugliness that the DM compiler
 * imposes.
 *----------------------------------------------------------------------------*/
#ifndef DBGLOG_H
#define DBGLOG_H

#include <cstdio>
#include <cstdarg>

// fwd decl -- must be in main unit
void display(const char *fmt, ...);

// convenience #defines -- use LOG() inside functions that have a time parameter
// named "t" that contains the simulation time-point; use LOG() if you don't
// have (or don't care about) the simulation timepoint
#define LOG(...)  dbgLog.log(__LINE__, __VA_ARGS__)
#define LOGT(...) dbgLog.log(__LINE__, t, __VA_ARGS__)

class DbgLog {
public:
  DbgLog(const char *filePath, int maxLines = 100) : maxLines(maxLines) {
    file = fopen(filePath, "w");
    if (!file) {
      display(
          "DbgLog:  Unable to open \"%s\" for logging.  Logging is disabled.\n",
          filePath);
      return;
    }
    display(
        "DbgLog:  Logging to \"%s\".  Maximum lines=%d\n", filePath, maxLines);

    fprintf(file, "Simulation Time....@Line: Log Message...");
  }

  ~DbgLog() {
    fprintf(file, "\n*** EOF ***\n");
    fclose(file);
  }

  void log(int lineNbr, const char *fmt...) {
    if (lineCnt == maxLines) return;
    lineCnt++;

    // prefix output with spacer & line #
    fprintf(file, "\n-------------------@%04d: ", lineNbr);

    // output variable portion
    va_list args = {0};
    va_start(args, fmt);
    vfprintf(file, fmt, args);
    va_end(args);

    // flush in case of early simulation termination
    fflush(file);
  }

  void log(int lineNbr, double t, const char *fmt...) {
    if (lineCnt == maxLines) return;
    lineCnt++;

    // prefix output with t (time) & line #
    fprintf(file, "\n%.17lf@%04d: ", t, lineNbr);

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

#endif DBGLOG_H
/*------------------------------------------------------------------------------
 * End of DbgLog.h
 *----------------------------------------------------------------------------*/