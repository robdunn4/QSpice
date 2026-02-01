#include "QSchTree.h"
#include <fstream>
#include <iostream>
#include <vector>

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

// Binary compare two files and report if identical
bool binaryCompareFiles(const std::string &file1, const std::string &file2) {
  std::ifstream f1(file1, std::ios::binary);
  std::ifstream f2(file2, std::ios::binary);

  if (!f1.is_open()) {
    std::cerr << "Cannot open file: " << file1 << std::endl;
    return false;
  }

  if (!f2.is_open()) {
    std::cerr << "Cannot open file: " << file2 << std::endl;
    return false;
  }

  // Read both files into memory
  std::vector<char> data1((std::istreambuf_iterator<char>(f1)),
                          std::istreambuf_iterator<char>());
  std::vector<char> data2((std::istreambuf_iterator<char>(f2)),
                          std::istreambuf_iterator<char>());

  f1.close();
  f2.close();

  // compare files...
  std::cout << "\n=== Binary Comparison ===\n";
  std::cout << "File 1: " << file1 << " (" << data1.size() << " bytes).\n";
  std::cout << "File 2: " << file2 << " (" << data2.size() << " bytes).\n";

  if (data1.size() != data2.size()) {
    std::cout << "Files are not identical -- sizes differ.\n";
    return false;
  }

  for (size_t i = 0; i < data1.size(); i++)
    if (data1[i] != data2[i]) {
      std::cout << "Files are not identical -- same size, different content.\n";
      return false;
    }

  std::cout << "Files are identical.\n";
  return true;
}

int main(int argc, char *argv[]) {
  // Check for command line argument
  if (argc < 2) {
    std::cerr << "Usage: " << argv[0] << " <filename>\n";
    std::cerr << "Example: " << argv[0] << " myschematic.qsch\n";
    return 1;
  }

  std::string inputFilename = argv[1];

  // Generate output filename with "_out" suffix before extension
  std::string baseFilename = getFilenameWithoutExtension(inputFilename);
  std::string extension = getFileExtension(inputFilename);
  std::string outputFilename = baseFilename + "_out" + extension;

  std::cout << "=== Parsing Input File ===\n";
  std::cout << "Input file: " << inputFilename << std::endl;

  try {
    // Open input file
    std::ifstream inputFile(inputFilename, std::ios::binary);
    if (!inputFile.is_open()) {
      throw std::runtime_error("Cannot open file: " + inputFilename);
    }

    // Parse from stream
    auto parsedTree = QSchTree::parseFromStream(inputFile);
    inputFile.close();

    std::cout << "\n=== Parsed Tree Structure ===\n";
    parsedTree->printWithPrefix();

    // std::cout << "\n=== Parsed Tree Structure -- Breadth First ==="
    //           << std::endl;
    // parsedTree->printBreadthFirst();

    // Write to output file
    std::cout << "\n=== Writing Output File ===\n";
    std::cout << "Output file: " << outputFilename << std::endl;

    std::ofstream outputFile(outputFilename, std::ios::binary);
    if (!outputFile.is_open()) {
      throw std::runtime_error("Cannot create file: " + outputFilename);
    }

    parsedTree->writeToStream(outputFile);
    outputFile.close();

    // Binary compare input and output files
    binaryCompareFiles(inputFilename, outputFilename);
  } catch (const std::exception &e) {
    std::cerr << "Error: " << e.what() << std::endl;
    return 1;
  }

  return 0;
}