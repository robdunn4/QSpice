//-----------------------------------------------------------------------------
// This file is part of the the QParser2 project.  You can find the complete
// project here:  https://github.com/robdunn4/QSpice/
//-----------------------------------------------------------------------------
#pragma once
#include "FsUtil.h"
#include <string_view>

//  this class does very little currently; here mainly for future enhancement

class CmdParser {
public:
  CmdParser() = default;

  int parse(int argc, char **argv);

  static std::string_view getSyntax();

  // options, switches, etc.
  Fs::path    inPath;
  Fs::path    outPathSch;
  Fs::path    outPathSym;
  std::string baseName{"BaseName"};
};