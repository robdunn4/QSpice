//-----------------------------------------------------------------------------
// This file is part of the the QParser2 project.  You can find the complete
// project here:  https://github.com/robdunn4/QSpice/
//-----------------------------------------------------------------------------
#include "CmdParser.h"
#include "FsUtil.h"
#include "PinDefs.h"
#include "StrList.h"
#include "SymList.h"
#include "VersionInfo.h"
#include <ItemAll.h>
#include <ItemTree.h>
#include <ItemTreeIO.h>
#include <iostream>

using std::cout;

int main(int argc, char **argv) {
  cout << VersionInfo::versionInfo << "\n\n";

  CmdParser parser;
  int       res = parser.parse(argc, argv);
  if (res) return -1;

  cout << "Pin Defs File: " << parser.inPath.generic_string() << "\n";
  cout << "To write sch:  " << parser.outPathSch.generic_string() << "\n";
  cout << "To write sym:  " << parser.outPathSym.generic_string() << "\n";
  // cout << "Base Name:     " << parser.baseName << "\n";

  if (!Fs::exists(parser.inPath)) {
    cout << "File does not exist or cannot be read: "
         << parser.inPath.generic_string() << "\nOperation aborted.\n";
    return -2; // TODO: value already used...
  }

  if (Fs::exists(parser.outPathSch) || Fs::exists(parser.outPathSym)) {
    // confirm overwrite
    cout << "One or both output files exist.  Overwrite? (y/N): ";

    std::string line;
    std::getline(std::cin, line);

    if (line.empty() ||
        std::tolower(static_cast<unsigned char>(line[0])) != 'y') {

      // cout << "\n";
      cout << "Operation cancelled by user.\n";
      return -3;
    }
  }

  StrList inPinDef;
  res = Fs::loadFile(parser.inPath, inPinDef);
  if (res) {
    cout << "Error loading pin definitions file.  Operation aborted.\n";
    return -4;
  }

  PinDefList pinList;
  pinList.partNbr = parser.baseName;

  res = pinList.parseLines(inPinDef, std::cout);
  if (res) {
    cout << "Error parsing pin definitions file.  Operation aborted.\n";
    return -5;
  }

  // here we start the QParser2 stuff...
  bool doDLL = pinList.biDirPinCnt;

  // first, make symbol
  SymList symList;
  if (!symList.makeSymbol(pinList)) {
    cout << "Error constructing symbol from pinlist.  Operation aborted.\n";
    return -6;
  }
  if (Fs::writeFile(parser.outPathSym, symList)) {
    cout << "Unable to write file: " << parser.outPathSym
         << ".  Operation aborted.\n";
    return -7;
  }
  cout << "File written successfully: " << parser.outPathSym << "\n";

  // make the schematic
  SymList schList;
  if (!schList.makeSchematic(pinList)) {
    cout << "Error constructing schematic from pinlist.  Operation aborted.\n";
    return -8;
  }
  if (Fs::writeFile(parser.outPathSch, schList)) {
    cout << "Unable to write file: " << parser.outPathSch
         << ".  Operation aborted.\n";
    return -9;
  }
  cout << "File written successfully: " << parser.outPathSch << "\n";

  cout << "\nAll operations completed successfully.\n";

  return 0;
}
