//-----------------------------------------------------------------------------
// This file is part of the the QParser2 project.  You can find the complete
// project here:  https://github.com/robdunn4/QSpice/
//-----------------------------------------------------------------------------
/*
 * QBomGen.cpp -- Bill of materials generator for QSpice schematic files.
 *
 * This program is part of the the QParser2 project.  You can find the complete
 * project here:  https://github.com/robdunn4/QSpice/
 */
#include "QBomData.h"
#include "QSchTree.h"
#include <fstream>
#include <iostream>
#include <memory>
#include <string>

const std::string verIdStr = std::string("QBomGen v0.1 ") +
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

int main(int argc, char *argv[]) {
  std::cerr << verIdStr << std::endl << std::endl;

  // Check for command line argument
  if (argc < 2) {
    std::cerr << "Usage: " << argv[0] << " <filename>" << std::endl;
    std::cerr << "Example: " << argv[0] << " myschematic.qsch" << std::endl;
    return 1;
  }

  std::string inputFilename = argv[1];

  // Generate output filename with "_out.csv" suffix
  std::string baseFilename   = getFilenameWithoutExtension(inputFilename);
  std::string extension      = getFileExtension(inputFilename);
  std::string outputFilename = baseFilename + "_out.csv";

  std::cout << "=== Parsing Input File ===" << std::endl;
  std::cout << "Input file: " << inputFilename << std::endl;

  try {
    // Open input file
    std::ifstream inputFile(inputFilename, std::ios::binary);
    if (!inputFile.is_open()) {
      throw std::runtime_error("Cannot open file: " + inputFilename);
    }

    // Parse from stream
    QSchTreePtr parsedTree = QSchTree::parseFromStream(inputFile);
    inputFile.close();

    // if not a schematic, quick out
    if (parsedTree->enumID != QPI::SCH) {
      std::cout << "Input file is not a schematic.\n";
      return -1;
    }

    // get BOM data
    std::cout << "\n=== Generating BOM ===" << std::endl;
    QBomList bomList;

    // parse first-level ItemCmp elements (skip others)
    QSchTreePtr cmpItem = parsedTree->getFirstChild();
    while (cmpItem) {
      if (cmpItem->enumID == QPI::COMP) {
        // next down should be ItemSym
        QSchTreePtr symItem = cmpItem->getFirstChild();
        if (symItem && symItem->enumID != QPI::SYM) break;

        // parse and add to BOM list if valid
        QBomData bomData;
        if (bomData.parseData(symItem)) bomList.push_back(bomData);
      }

      cmpItem = cmpItem->getNextSibling();
    }

    if (!bomList.size()) {
      std::cout << "BOM list is empty.\n";
      return 0;
    }
    std::cout << "Found " << bomList.size() << " BOM items.\n";

    // sort BOM...
    bomList.sort1();

    // Open output file
    std::ofstream outputFile(outputFilename, std::ios::binary);
    if (!outputFile.is_open()) {
      throw std::runtime_error("Cannot open file: " + outputFilename);
    }

    std::cout << "Writing BOM to " << outputFilename << "...";
    QBomData::writeHeader(outputFile);
    for (QBomData bomItem : bomList)
      bomItem.writeData(outputFile);
    outputFile.close();
    std::cout << "  Done.\n";

  } catch (const std::exception &e) {
    std::cerr << "Error: " << e.what() << std::endl;
    return 1;
  }

  return 0;
}
