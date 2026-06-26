//-----------------------------------------------------------------------------
// This file is part of the the QSymGen3 command-line tool contained in the
// QParser2 project.  You can find the complete project here:
// https://github.com/robdunn4/QSpice/
//-----------------------------------------------------------------------------
#include "CmdParser.h"
#include "Config.h"
#include "VersionInfo.h"
#include <format>
#include <iostream>
#include <windows.h>

using std::cout;

int CmdParser::parse(int argc, char **argv) {
  // resolve the directory containing this executable
  char exeBuf[MAX_PATH] = {};
  GetModuleFileNameA(nullptr, exeBuf, MAX_PATH);
  exeDir = Fs::path(exeBuf).parent_path();

  if (argc < 2) {
    cout << getSyntax() << "\n";
    return -1;
  }

  if (argc != 2) {
    cout << "Error:  Invalid command line argument count\n\n";
    cout << getSyntax() << "\n";
    return -2;
  }

  inPath = Fs::absolute(argv[1]);

  // validate input file extension
  if (inPath.extension().string() != Config::inputExtension) {
    cout << std::format(
        "Error:  Input file must have extension \"{}\" (got \"{}\")\n",
        Config::inputExtension, inPath.extension().string());
    return -3;
  }

  // derive output paths
  baseName   = inPath.stem().string();
  outPathSch = Fs::path(inPath).replace_extension(".qsch");
  outPathSym = Fs::path(inPath).replace_extension(".qsym");
  outPathCpp = Fs::path(inPath).replace_extension(".cpp");

  return 0;
}

std::string_view CmdParser::getSyntax() {
  static const std::string syntax = std::format(
      "Syntax: {} PinDefsPath\n"
      "Where PinDefsPath is a path to the input \"{}\" file.",
      VersionInfo::progName, Config::inputExtension);
  return syntax;
}
