//-----------------------------------------------------------------------------
// This file is part of the the QParser2 project.  You can find the complete
// project here:  https://github.com/robdunn4/QSpice/
//-----------------------------------------------------------------------------
/*
 * QBomGen.cpp -- Bill of materials generator for QSpice schematic files.
 */
#include "QBomData.h"
#include <ItemAll.h>
#include <ItemTree.h>
#include <ItemTreeIO.h>
#include <fstream>
#include <iostream>
#include <memory>
#include <string>

const std::string verIdStr = std::string("QBomGen v0.5 ") +
                             std::string(__DATE__) + std::string(" ") +
                             std::string(__TIME__);

// Helper function to extract filename without extension
std::string getFilenameWithoutExtension(const std::string &filename) {
  size_t lastDot = filename.find_last_of('.');
  if (lastDot == std::string::npos) return filename;
  return filename.substr(0, lastDot);
}

// Helper function to get file extension
std::string getFileExtension(const std::string &filename) {
  size_t lastDot = filename.find_last_of('.');
  if (lastDot == std::string::npos) return "";
  return filename.substr(lastDot);
}

// main() -- returns non-zero on error, zero on success.
int main(int argc, char *argv[]) {
  // display version info
  std::cout << verIdStr << std::endl << std::endl;

  // Check for command line argument
  if (argc < 2) {
    std::cout << "Usage: " << argv[0] << " <filename>" << std::endl;
    std::cout << "Example: " << argv[0] << " myschematic.qsch" << std::endl;
    return -1;
  }

  std::string inputFilename = argv[1];

  // Generate output filename with "_out.csv" suffix
  std::string baseFilename   = getFilenameWithoutExtension(inputFilename);
  std::string extension      = getFileExtension(inputFilename);
  std::string outputFilename = baseFilename + "_out.csv";

  std::cout << "=== Parsing Input File ===" << std::endl;
  std::cout << "Input file: " << inputFilename << std::endl;

  try {
    // parse input file into an ItemTree
    ItemTreeIO::ParseResult res = ItemTreeIO::readFile(inputFilename);
    if (res.error.length()) {
      std::cout << "Error parsing file: " << res.error;
      if (res.line) std::cout << " (line " << res.line << ")";
      std::cout << std::endl;
      return -2;
    }
    ItemTree &tree = res.tree;

    if (tree.empty()) {
      std::cout << "Parsed tree is empty.\n";
      return -2;
    }

    // if not a schematic, quick out
    if (tree.root()->getEnumID() != QPI::SCH) {
      std::cout << "Input file is not a schematic.\n";
      return -3;
    }

    // get BOM data
    std::cout << "\n=== Generating BOM ===" << std::endl;
    QBomList bomList;

    // parse first-level ItemCmp elements (skip others)
    NodeList cmpNodes = tree.root()->children();
    for (NodePtr cmpNode : cmpNodes) {
      if (cmpNode->getEnumID() != QPI::COMP) continue;

      // next down should be ItemSym; we expect only one symbol per component
      NodePtr symNode = cmpNode->firstChild();
      if (!symNode || symNode->getEnumID() != QPI::SYM) continue;

      QBomData bomData;

      ItemSymPtr symPtr = std::dynamic_pointer_cast<ItemSym>(symNode->item());
      bomData.name      = symPtr->text;

      // some symbols (e.g. DLL components) may not have a name
      if (!bomData.name.length()) bomData.name = "[Unnamed]";

      // parse and add to BOM list if valid
      if (bomData.parseData(symNode)) bomList.push_back(bomData);
    }

    // handle empty BOM list
    if (!bomList.size()) {
      std::cout << "BOM list is empty.\n";
      return -4;
    }

    std::cout << "Found " << bomList.size() << " BOM items.\n";

    // sort BOM... other sorting methods could be implemented...
    bomList.sort1();

    // Open output file
    std::ofstream outputFile(outputFilename, std::ios::binary);
    if (!outputFile.is_open()) {
      throw std::runtime_error("Cannot open file: " + outputFilename);
    }

    // write BOM to output file
    std::cout << "Writing BOM to " << outputFilename << "...";
    QBomData::writeHeader(outputFile);
    for (QBomData bomItem : bomList)
      bomItem.writeData(outputFile);
    outputFile.close();
  } catch (const std::exception &e) {
    std::cerr << "Error: " << e.what() << std::endl;
    return -5;
  }

  // we're done
  std::cout << "  Done.\n";
  return 0;
}
