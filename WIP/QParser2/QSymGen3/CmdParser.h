//-----------------------------------------------------------------------------
// This file is part of the the QSymGen3 command-line tool contained in the
// QParser2 project.  You can find the complete project here:
// https://github.com/robdunn4/QSpice/
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
  Fs::path    exeDir;      // directory containing QSymGen3.exe
  Fs::path    inPath;
  Fs::path    outPathSch;
  Fs::path    outPathSym;
  Fs::path    outPathCpp;  // <baseName>.cpp (filled DLL source)
  std::string baseName{"BaseName"};

  // the symbol width should be a command-line parameter; for now, it's
  // hard-coded; note that this must be a multiple of 200.  (QSpice will adjust
  // things silently to put pins on 100x100 unit grid points.  you won't see
  // this until you drop the symbol onto a schematic...)
  int symbolWidth = 1400;
};
