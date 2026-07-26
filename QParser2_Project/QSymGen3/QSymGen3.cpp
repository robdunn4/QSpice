//-----------------------------------------------------------------------------
// This file is part of the the QSymGen3 command-line tool contained in the
// QParser2 project.  You can find the complete project here:
// https://github.com/robdunn4/QSpice/
//-----------------------------------------------------------------------------
#include "CmdParser.h"
#include "Config.h"
#include "FsUtil.h"
#include "PinDefs.h"
#include "QuxRunner.h"
#include "RegUtil.h"
#include "StrList.h"
#include "SymList.h"
#include "TemplateProcessor.h"
#include "VersionInfo.h"
#include <algorithm>
#include <cctype>

using std::cout;

int main(int argc, char **argv) {
  cout << VersionInfo::versionInfo << "\n\n";

  // Step 1: parse and validate command-line arguments
  CmdParser parser;
  if (parser.parse(argc, argv)) return -1;

  cout << "Pin Defs File: " << parser.inPath.generic_string() << "\n";
  cout << "To write sym:  " << parser.outPathSym.generic_string() << "\n";
  cout << "To write sch:  " << parser.outPathSch.generic_string() << "\n";
  cout << "To write cpp:  " << parser.outPathCpp.generic_string() << "\n";

  // Step 2: locate QUX.exe via registry
  std::string qspiceDir = RegUtil::getQSpiceInstallDir();
  if (qspiceDir.empty()) {
    cout << "Error:  Could not read QSpice installation directory from "
            "registry.\n"
            "        Is QSpice installed?\n";
    return -2;
  }

  Fs::path quxPath = Fs::path(qspiceDir) / Config::quxExeName.data();
  if (!Fs::exists(quxPath)) {
    cout << "Error:  QUX.exe not found at: " << quxPath.generic_string()
         << "\n";
    return -3;
  }

  // Step 3: locate DLL template -- working directory first, then exe directory
  Fs::path templatePath =
      Fs::absolute(Fs::path(Config::dllTemplateName.data()));
  if (!Fs::exists(templatePath)) {
    templatePath = parser.exeDir / Config::dllTemplateName.data();
    if (!Fs::exists(templatePath)) {
      cout << "Error:  DLL template \"" << Config::dllTemplateName
           << "\" not found in working directory or exe directory.\n";
      return -4;
    }
  }
  cout << "Template file: " << templatePath.generic_string() << "\n\n";

  // Step 4: verify input file exists
  if (!Fs::exists(parser.inPath)) {
    cout << "Error:  File does not exist or cannot be read: "
         << parser.inPath.generic_string() << "\nOperation aborted.\n";
    return -5;
  }

  // Step 5: confirm overwrite of any existing output files
  if (Fs::exists(parser.outPathSch) || Fs::exists(parser.outPathSym) ||
      Fs::exists(parser.outPathCpp)) {
    cout << "One or more output files exist.  Overwrite? (y/N): ";
    std::string line;
    std::getline(std::cin, line);
    if (line.empty() ||
        std::tolower(static_cast<unsigned char>(line[0])) != 'y') {
      cout << "Operation cancelled by user.\n";
      return -6;
    }
  }

  // Step 6: load and parse pin definitions
  StrList inPinDef;
  if (Fs::loadFile(parser.inPath, inPinDef)) {
    cout << "Error loading pin definitions file.  Operation aborted.\n";
    return -7;
  }

  PinDefList pinList;
  pinList.partNbr = parser.baseName;
  if (pinList.parseLines(inPinDef, std::cout)) {
    cout << "Error parsing pin definitions file.  Operation aborted.\n";
    return -8;
  }

  // Step 7: generate and write the .qsym symbol file
  SymList symList;
  if (!symList.makeSymbol(pinList, parser.symbolWidth)) {
    cout << "Error constructing symbol from pinlist.  Operation aborted.\n";
    return -9;
  }
  if (Fs::writeFile(parser.outPathSym, symList)) {
    cout << "Unable to write file: " << parser.outPathSym
         << ".  Operation aborted.\n";
    return -10;
  }
  cout << "File written successfully: " << parser.outPathSym << "\n";

  // Step 8: generate and write the .qsch schematic file
  SymList schList;
  if (!schList.makeSchematic(pinList)) {
    cout << "Error constructing schematic from pinlist.  Operation aborted.\n";
    return -11;
  }
  if (Fs::writeFile(parser.outPathSch, schList)) {
    cout << "Unable to write file: " << parser.outPathSch
         << ".  Operation aborted.\n";
    return -12;
  }
  cout << "File written successfully: " << parser.outPathSch << "\n";

  // Step 9: invoke QUX.exe to generate uData code snippet
  StrList uDataLines = QuxRunner::run(quxPath, parser.outPathSch, std::cout);
  if (uDataLines.empty()) {
    cout << "Error:  QUX.exe produced no output.  Operation aborted.\n";
    return -13;
  }

  // Step 9b: append a local reference alias to the V-record's uData variable,
  // e.g. "double &vddRef = VCC;", immediately following the uData declarations.
  // Used only at the setVDD() call site in the template; getVDD() is the
  // path for retrieving supply voltage afterward.
  auto vIter = std::find_if(pinList.cbegin(), pinList.cend(),
                            [](const PinDef &pd) { return pd.type == 'V'; });
  if (vIter == pinList.cend()) {
    cout << "Error:  No V record found in pin definitions.  Operation "
            "aborted.\n";
    return -13; // TODO:  Create unique error code
  }
  uDataLines.push_back("double &vddRef = " + vIter->name + ";");

  // Step 10: generate QSymGen3 code snippet (no file write)
  SymList cppSnippet;
  if (!cppSnippet.makeCppSnippet(pinList, std::cout)) {
    cout << "Error constructing C++ snippet from pinlist.  Operation "
            "aborted.\n";
    return -14;
  }

  // Step 11: load template, register substitutions, emit final .cpp
  TemplateProcessor tmpl;
  if (!tmpl.load(templatePath, std::cout)) {
    cout << "Error loading DLL template.  Operation aborted.\n";
    return -15;
  }

  // inline scalar: eval function name is baseName lowercased
  std::string evalFuncName = parser.baseName;
  std::transform(evalFuncName.begin(), evalFuncName.end(), evalFuncName.begin(),
                 [](unsigned char c) { return std::tolower(c); });
  tmpl.addScalar(Config::tokenEvalFuncName, evalFuncName);

  // block substitutions
  tmpl.addBlock(Config::tokenUDataSnippet, uDataLines);
  tmpl.addBlock(Config::tokenQSymGen3Snippet, cppSnippet);

  if (!tmpl.apply(parser.outPathCpp, std::cout)) {
    cout << "Unable to write file: " << parser.outPathCpp
         << ".  Operation aborted.\n";
    return -16;
  }
  cout << "File written successfully: " << parser.outPathCpp << "\n";

  cout << "\nAll operations completed successfully.\n";
  return 0;
}
