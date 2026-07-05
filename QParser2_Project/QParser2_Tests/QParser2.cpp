//-----------------------------------------------------------------------------
// This file is part of the the QParser2 project.  You can find the complete
// project here:  https://github.com/robdunn4/QSpice/
//-----------------------------------------------------------------------------
/*
 * QParser2.cpp -- Test program for QParser2 library.  This program reads a
 * schematic file, parses it into an ItemTree, writes the tree back out to a
 * new file, and then compares the two files to verify they are identical.
 */
#include <ItemAll.h>
#include <ItemTreeIO.h>
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
    std::cout << "Cannot open file: " << file1 << std::endl;
    return false;
  }

  if (!f2.is_open()) {
    std::cout << "Cannot open file: " << file2 << std::endl;
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

  // trim trailing newlines
  bool sizeDiffers = (data1.size() != data2.size());
  if (sizeDiffers) {
    std::cout << "File sizes differ.  Stripping trailing whitespace...\n";
    data1.erase(std::find_if(data1.rbegin(), data1.rend(),
                             [](char ch) { return ch != '\n'; })
                    .base(),
                data1.end());
    data2.erase(std::find_if(data2.rbegin(), data2.rend(),
                             [](char ch) { return ch != '\n'; })
                    .base(),
                data2.end());
  }

  if (data1.size() != data2.size()) {
    std::cout << "Files are not identical -- size difference remains after "
                 "stripping trailing whitespace.\n";
    return false;
  }

  for (size_t i = 0; i < data1.size(); i++) {
    if (data1[i] != data2[i]) {
      std::cout << "Files are not identical -- contents differ after stripping "
                   "trailing whitespace.\n ";
      return false;
    }
  }

  std::cout << "Files are " << (sizeDiffers ? "functionally " : "")
            << "identical.\n";
  return true;
}

// main() -- returns non-zero on error, zero on success.
int main(int argc, char *argv[]) {
  // Check for command line argument
  if (argc < 2) {
    std::cout << "Usage: " << argv[0] << " <filename>\n";
    std::cout << "Example: " << argv[0] << " myschematic.qsch\n";
    return -1;
  }

  std::string inputFilename = argv[1];

  // Generate output filename with "_out" suffix before extension
  std::string baseFilename   = getFilenameWithoutExtension(inputFilename);
  std::string extension      = getFileExtension(inputFilename);
  std::string outputFilename = baseFilename + "_out" + extension;

  std::cout << "=== Parsing Input File ===\n";
  std::cout << "Input file: " << inputFilename << std::endl;

  try {
    // Parse from file into an ItemTree
    ItemTreeIO::ParseResult res = ItemTreeIO::readFile(inputFilename);

    if (res.error.size()) {
      std::cout << "Parse error: " << res.error;
      if (res.line > 0) std::cout << " (line " << res.line << ")";
      std::cout << std::endl;
      return -2;
    }

    // Successfully parsed tree; write it back out and compare to input.
    bool writeResult = ItemTreeIO::writeFile(res.tree, outputFilename);
    if (!writeResult) {
      throw std::runtime_error("Failed to write output file: " +
                               outputFilename);
    }

    // Binary compare input and output files
    binaryCompareFiles(inputFilename, outputFilename);
  } catch (const std::exception &e) {
    std::cout << "Error: " << e.what() << std::endl;
    return -3;
  }

  return 0;
}