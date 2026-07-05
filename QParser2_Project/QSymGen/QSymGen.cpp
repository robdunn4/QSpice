//-----------------------------------------------------------------------------
// This file is part of the the QParser2 project.  You can find the complete
// project here:  https://github.com/robdunn4/QSpice/
//-----------------------------------------------------------------------------
#include "PinItems.h"
#include "SymData.h"
#include <ItemAll.h>
#include <ItemTreeIO.h>
#include <iostream>

const char *ProgName = "QSymGen";
const char *Version  = "0.1.0";
const char *DateTime = __DATE__ " " __TIME__;

// TODO:  relocate this later...
const char *DLL_TYPE_STRING = "\xD8(.DLL)";

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

// main() -- returns zero on success, non-zero on failure
int main(int argc, char *argv[]) {
  // display version information
  std::cout << ProgName << " v" << Version << "  " << DateTime;
#ifdef _DEBUG
  std::cout << "  *** Debugging Build ***";
#endif
  std::cout << std::endl << std::endl;

  // Check for command line argument
  if (argc < 2) {
    std::cerr << "Usage: " << argv[0] << " <filename>\n";
    std::cerr << "Example: " << argv[0]
              << " PinDefs.txt [generates PinDefs.qsym]\n";
    return -1;
  }

  std::string inputFilename  = argv[1];
  std::string baseFilename   = getFilenameWithoutExtension(inputFilename);
  std::string outputFilename = baseFilename + ".qsym";

  std::cout << "=== Parsing Input File ===\n";
  std::cout << "Input file: " << inputFilename << std::endl;

  try {
    PinItemList pinItems;
    try {
      pinItems.parseFile(inputFilename);
    } catch (const std::exception &e) {
      std::cerr << "Error parsing pin definitions file: " << e.what()
                << std::endl;
      return -2;
    }

    SymData symData(pinItems);

    symData.fixCoords();
    Point topLeft  = symData.topLeft;
    Point botRight = symData.botRight;

    ItemTree   tree;
    ItemSymPtr sym  = ItemSym::makePtr("MySymbol");
    auto       root = tree.setRoot(sym);

    if (!root) {
      std::cout << "Failed to create root node.\n";
      return -3;
    }

    NodePtr nodePtr = root;
    nodePtr->addLast(ItemType::makePtr(DLL_TYPE_STRING));
    nodePtr->addLast(ItemDesc::makePtr("Symbol Generated with QSymGen"));
    nodePtr->addLast(ItemShort::makePtr(false));
    nodePtr->addLast(ItemRect::makePtr(topLeft, botRight, 0, 0, 0, 0xff0000,
                                       0x4000000, -1, 0, -1));
    nodePtr->addLast(ItemText::makePtr(ArgPoint(0, topLeft.y + 250), 1, 15, 0,
                                       0x1000000, -1, -1, "X1"));
    nodePtr->addLast(ItemText::makePtr(ArgPoint(0, topLeft.y + 100), 1, 15, 0,
                                       0x1000000, -1, -1, "DllFileName"));

    for (SymPin &symPin : symData) {
      nodePtr->addLast(symPin.itemPin);
    }

    // Write to output file
    std::cout << "\n=== Writing Output File ===\n";
    std::cout << "Output file: " << outputFilename << std::endl;
    ItemTreeIO::writeFile(tree, outputFilename);
  } catch (const std::exception &e) {
    std::cerr << "Error: " << e.what() << std::endl;
    return -4;
  }

  return 0;
}
