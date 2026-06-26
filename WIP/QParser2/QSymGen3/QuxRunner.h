//-----------------------------------------------------------------------------
// This file is part of the the QSymGen3 command-line tool contained in the
// QParser2 project.  You can find the complete project here:
// https://github.com/robdunn4/QSpice/
//-----------------------------------------------------------------------------
#pragma once
#include "FsUtil.h"
#include "NullStream.h"
#include "StrList.h"
#include <iostream>

namespace QuxRunner {

// Invokes:  QUX.exe -DLLvariables <qschPath> -stdout
// Captures stdout line-by-line into the returned StrList.
// Returns an empty StrList on any failure; diagnostics go to errStrm.
StrList run(const Fs::path &quxExePath, const Fs::path &qschPath,
            std::ostream &errStrm = nullStream);

} // namespace QuxRunner
