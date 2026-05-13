//-----------------------------------------------------------------------------
// This file is part of the the QParser2 project.  You can find the complete
// project here:  https://github.com/robdunn4/QSpice/
//-----------------------------------------------------------------------------
#include "CmdParser.h"
#include "VersionInfo.h"
#include <format>
#include <iostream>

using std::cout;

int CmdParser::parse(int argc, char **argv) {
  if (argc < 2) {
    cout << getSyntax() << "\n";
    return -1;
  }

  if (argc != 2) {
    cout << "Error:  Invalid command line argument count\n\n";
    cout << getSyntax() << "\n";
    return -2;
  }

  // make the paths...
  inPath     = Fs::absolute(argv[1]);
  outPathSch = Fs::path(inPath).replace_extension(".qsch");
  outPathSym = Fs::path(inPath).replace_extension(".qsym");
  baseName   = Fs::path(inPath).stem().string();

  return 0;
}

std::string_view CmdParser::getSyntax() {
  static const std::string syntax =
      std::format("Syntax: {} PinDefsPath\n"
                  "Where PinDefsPath is a path to the input text file.",
                  VersionInfo::progName);
  return syntax;
}
