//------------------------------------------------------------------------------
// This file is part of the QMdbSim2 project, a Microchip Simulator framework
// for QSpice C-Block components.  See the GitHub repository at
// https://github.com/robdunn4/QSpice/ for the complete project, current
// sources, documentation, and demonstration code.
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
// PerfTimer.h -- Lightweight header-only accumulated lap timer.
//
// Records named timing intervals.  Each named interval accumulates across
// multiple start()/stop() pairs, making it suitable for both one-shot phase
// timing (count=1) and high-frequency per-step timing (count=N).
//
// Usage:
//
//   PerfTimer pt;
//
//   pt.start("JVM init");
//   jvmHost_.start(hc);
//   pt.stop("JVM init");
//
//   pt.start("stepInst");       // called thousands of times
//   dbg_.stepInstr();
//   pt.stop("stepInst");
//
//   pt.report(std::cout);       // or std::cerr, std::ofstream, etc.
//
// For use with QSpice Display(), wrap in an ostringstream:
//
//   std::ostringstream oss;
//   pt.report(oss);
//   Display("%s", oss.str().c_str());
//
// Notes:
//   - Header-only: no .cpp required; just include this file.
//   - No heap allocation after construction (entries use std::vector but
//     are reserved once on first use per name).
//   - Not thread-safe; intended for single-threaded simulation use.
//   - Intervals with the same name accumulate; order of first appearance
//     is preserved in report() output.
//   - Call reserve() before start() to pre-register entries in a specific order
//     (e.g. in a constructor) without beginning measurement.
//   - Calling stop() without a matching start() is a no-op.
//   - Calling start() twice without stop() restarts the interval (previous
//     start time is discarded).
//   - PerfTimer::ScopedTimer provides RAII start/stop tied to a stack frame.
//
// Compile-time enable/disable:
//   PerfTimer is enabled automatically in Debug builds (_DEBUG defined).
//   To force-enable in Release:  #define PERFTIMER_ENABLED before #include
//   To force-disable in Debug:   #define PERFTIMER_DISABLED before #include
//   When disabled, all methods are no-op inlines and the compiler removes
//   them entirely.  Call sites require no changes.
//------------------------------------------------------------------------------
#pragma once

// ── Compile-time activation
// ───────────────────────────────────────────────────
#if defined(PERFTIMER_DISABLED)
#define PERFTIMER_ACTIVE 0
#elif defined(PERFTIMER_ENABLED) || defined(_DEBUG)
#define PERFTIMER_ACTIVE 1
#else
#define PERFTIMER_ACTIVE 0
#endif

#if PERFTIMER_ACTIVE

#include <chrono>
#include <cstdint>
#include <iomanip>
#include <ostream>
#include <string>
#include <vector>

class PerfTimer {
public:
  // ── Control ──────────────────────────────────────────────────────────────

  // Reserve a named slot without starting it.  Use this to fix the order of
  // entries in report() output before any start()/stop() calls occur.
  // No-op if the name already exists.
  void reserve(const char *name) { findOrCreate(name); }

  // Begin a named interval.  If the name already exists, restarts its
  // pending measurement (previous unmatched start is discarded).
  void start(const char *name) {
    Entry &e  = findOrCreate(name);
    e.pending = std::chrono::steady_clock::now();
    e.running = true;
  }

  // End a named interval and accumulate elapsed time.
  // No-op if start() was not called for this name.
  void stop(const char *name) {
    auto   now = std::chrono::steady_clock::now();
    Entry *e   = find(name);
    if (!e || !e->running) return;
    e->totalNs +=
        std::chrono::duration_cast<std::chrono::nanoseconds>(now - e->pending)
            .count();
    ++e->count;
    e->running = false;
  }

  // Reset all accumulated data.
  void reset() { entries_.clear(); }

  // ── Scoped Timer ─────────────────────────────────────────────────────────

  // RAII helper: starts a named interval on construction, stops it on
  // destruction.  Useful for timing an entire function or block scope.
  //
  // Usage:
  //
  //   void MyClass::stepInst() {
  //     PerfTimer::ScopedTimer st(pt_, "stepInst");
  //     dbg_.stepInstr();
  //   }  // stops automatically on any exit path
  //
  class ScopedTimer {
  public:
    ScopedTimer(PerfTimer &pt, const char *name) : pt_(pt), name_(name) {
      pt_.start(name_);
    }
    ~ScopedTimer() { pt_.stop(name_); }

    ScopedTimer(const ScopedTimer &)            = delete;
    ScopedTimer &operator=(const ScopedTimer &) = delete;

  private:
    PerfTimer  &pt_;
    const char *name_;
  };

  // ── Reporting ─────────────────────────────────────────────────────────────

  // Write a formatted report to the given stream.
  // Intervals are listed in the order they were first start()ed.
  // Elapsed times auto-scale: ns / us / ms / s.
  void report(std::ostream &os) const {
    // find the longest name for alignment
    size_t nameWidth = 12; // minimum
    for (const Entry &e : entries_)
      if (e.name.size() > nameWidth) nameWidth = e.name.size();

    // Divider matches content line width:
    //   2 (indent) + nameWidth + 3 (" : ") + 8 (count) + 8 (" calls, ")
    //   + 12 (totalMs) + 11 (" ms total, ") + 12 (avgMs) + 8 (" ms avg")
    const std::string kDiv(2 + nameWidth + 62, '-');

    os << kDiv << "\n";

    for (const Entry &e : entries_) {
      double totalMs = static_cast<double>(e.totalNs) / 1000000.0;
      double avgMs = e.count > 0 ? totalMs / static_cast<double>(e.count) : 0.0;

      os << "  " << std::left << std::setw(static_cast<int>(nameWidth))
         << e.name << std::right << " : " << std::setw(8) << e.count
         << (e.count == 1 ? " call , " : " calls, ") << std::setw(12)
         << std::fixed << std::setprecision(3) << totalMs << " ms total, "
         << std::setw(12) << std::fixed << std::setprecision(3) << avgMs
         << " ms avg";

      if (e.running) os << "  [still running]";

      os << "\n";
    }

    os << kDiv << "\n";
  }

private:
  using Clock     = std::chrono::steady_clock;
  using TimePoint = Clock::time_point;

  struct Entry {
    std::string name;
    int64_t     totalNs = 0;
    uint64_t    count   = 0;
    TimePoint   pending = {};
    bool        running = false;
  };

  std::vector<Entry> entries_;

  Entry *find(const char *name) {
    for (Entry &e : entries_)
      if (e.name == name) return &e;
    return nullptr;
  }

  Entry &findOrCreate(const char *name) {
    Entry *e = find(name);
    if (e) return *e;
    entries_.push_back({name, 0, 0, {}, false});
    return entries_.back();
  }
};

#else // PERFTIMER_ACTIVE == 0 -- no-op stub

#include <ostream>

class PerfTimer {
public:
  void reserve(const char *) {}
  void start(const char *) {}
  void stop(const char *) {}
  void reset() {}
  void report(std::ostream &) const {}

  class ScopedTimer {
  public:
    ScopedTimer(PerfTimer &, const char *) {}
    ~ScopedTimer() {}

    ScopedTimer(const ScopedTimer &)            = delete;
    ScopedTimer &operator=(const ScopedTimer &) = delete;
  };
};

#endif // PERFTIMER_ACTIVE
