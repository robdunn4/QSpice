/*------------------------------------------------------------------------------
 * DbgLog_VC.h -- Log debugging information to file, Microsoft VC version.
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
#ifndef DBGLOG_VC_H
#define DBGLOG_VC_H

#include <cstdio>
#include <chrono>
#include <thread>

#ifndef _MSVC_LANG
#error This header requires MSVC
#elif _MSVC_LANG < 201703L
#pragma message("This header requires C++17 standard or newer")
#endif

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

  template <typename... Args> void display(const char *fmt, Args... args) {
    std::this_thread::sleep_for(std::chrono::milliseconds(30));
    fflush(stdout);
    fprintf(stdout, fmt, args...);
    fflush(stdout);
    std::this_thread::sleep_for(std::chrono::milliseconds(30));
  }

  template <typename... Args>
  void log_(int lineNbr, const char *fmt, Args... args) {
    if (lineCnt == maxLines) return;
    lineCnt++;
    fprintf(file, "\n-----------------------@%04d: ", lineNbr);
    fprintf(file, fmt, args...);
    // flush in case of early simulation termination
    fflush(file);
  }

  template <typename... Args>
  void log_(int lineNbr, double t, const char *fmt, Args... args) {
    if (lineCnt == maxLines) return;
    lineCnt++;
    fprintf(file, "\n%0.17e@%04d: ", t, lineNbr);
    fprintf(file, fmt, args...);
    // flush in case of early simulation termination
    fflush(file);
  }

protected:
  FILE        *file;
  unsigned int maxLines;
  unsigned int lineCnt;
};

#endif DBGLOG_VC_H
/*------------------------------------------------------------------------------
 * End of DbgLog_VC.h
 *----------------------------------------------------------------------------*/