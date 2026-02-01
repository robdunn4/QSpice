#include "QBomData.h"
// #include "QItemSym.h" // is there an include for all types?  should there be?
// #include "QItemType.h"
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

// void getSymItems(std::shared_ptr<QSchTree> parsedTree,
//                  std::shared_ptr<QSchTree> symTree);

int main(int argc, char *argv[]) {
  std::cerr << verIdStr << std::endl << std::endl;

  // Check for command line argument
  if (argc < 2) {
    std::cerr << "Usage: " << argv[0] << " <filename>" << std::endl;
    std::cerr << "Example: " << argv[0] << " myschematic.qsch" << std::endl;
    return 1;
  }

  std::string inputFilename = argv[1];

  // Generate output filename with "_out" suffix before extension
  std::string baseFilename = getFilenameWithoutExtension(inputFilename);
  std::string extension = getFileExtension(inputFilename);
  std::string outputFilename = baseFilename + "_out" + extension;

  std::cout << "=== Parsing Input File ===" << std::endl;
  std::cout << "Input file: " << inputFilename << std::endl;

  try {
    // Open input file
    std::ifstream inputFile(inputFilename, std::ios::binary);
    if (!inputFile.is_open()) {
      throw std::runtime_error("Cannot open file: " + inputFilename);
    }

    // Parse from stream
    // auto parsedTree = QSchTree::parseFromStream(inputFile);
    QSchTreePtr parsedTree = QSchTree::parseFromStream(inputFile);
    inputFile.close();

    // if not a schematic, quick out
    if (parsedTree->enumID != QPI::SCH) {
      std::cout << "Input file is not a schematic?\n";
      return -1;
    }

    // get BOM data
    std::cout << "\n=== Generating BOM ===" << std::endl;
    QBomList bomList;

    QSchTreePtr cmpItem = parsedTree->getFirstChild();
    while (cmpItem) {
      // should be QItemCmp
      if (cmpItem->enumID == QPI::COMP) {
        // next down should be QItemSym
        QSchTreePtr symItem = cmpItem->getFirstChild();
        if (symItem && symItem->enumID != QPI::SYM) break;

        QBomData bomData;
        if (bomData.parseData(symItem)) bomList.push_back(bomData);
      }

      cmpItem = cmpItem->getNextSibling();
    }

    // Write to output file
    std::cout << "Found " << bomList.size() << " BOM items.\n";

    if (bomList.size()) {
      std::cout << std::endl;
      QBomData::writeHeader(std::cout);
      for (QBomData bomItem : bomList)
        bomItem.writeData(std::cout);
    }

    // sort BOM and write output again
    // how to sort vector...
    bomList.sort1();

    // Write to output file
    std::cout << "\n=== Sorted BOM ===\n";

    if (bomList.size()) {
      std::cout << std::endl;
      QBomData::writeHeader(std::cout);
      for (QBomData bomItem : bomList)
        bomItem.writeData(std::cout);
    }

  } catch (const std::exception &e) {
    std::cerr << "Error: " << e.what() << std::endl;
    return 1;
  }

  return 0;
}