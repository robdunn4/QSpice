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
  if (res) return res;

  cout << "Pin Defs File: " << parser.inPath.generic_string() << "\n";
  cout << "To write sch:  " << parser.outPathSch.generic_string() << "\n";
  cout << "To write sym:  " << parser.outPathSym.generic_string() << "\n";
  cout << "Base Name:     " << parser.baseName << "\n";

  if (!Fs::exists(parser.inPath)) {
    cout << "File does not exist or cannot be read: "
         << parser.inPath.generic_string() << "\n";
    return -1; // TODO: value already used...
  }

  StrList inPinDef;
  res = Fs::loadFile(parser.inPath, inPinDef);
  if (res) {
    cout << "Error loading pin definitions file\n";
    return res;
  }

  PinDefList pinList;
  pinList.partNbr = parser.baseName;
  res             = pinList.parseLines(inPinDef, std::cout);
  if (res) {
    cout << "Error parsing pin definitions file\n";
    return res;
  }

  // here we start the QParser2 stuff...
  // note that we will create a symbol unconditionally.  however, the symbol
  // type will be DLL if no GPIO pins are in pin defs.  if GPIO pins are
  // present, we the symbol type will be hierarchical and a schematic file
  // will also be created to implement the GPIO circuitry.
  bool doDLL = pinList.biDirPinCnt;

  // first, make symbol
  SymList symList;
  if (!symList.makeSymbol(pinList)) {
    cout << "Error constructing symbol from pinlist\n";
    return -1;
  }

  // Fs::writeFile(cout, symList);
  Fs::writeFile(parser.outPathSym, symList);

  SymList schList;
  if (!schList.makeSchematic(pinList)) {
    cout << "Error constructing schematic from pinlist\n";
    return -1;
  }

  // Fs::writeFile(cout, symList);
  Fs::writeFile(parser.outPathSch, schList);
}
