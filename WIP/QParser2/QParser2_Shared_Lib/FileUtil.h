//-----------------------------------------------------------------------------
// This file is part of the the QParser2 project.  You can find the complete
// project here:  https://github.com/robdunn4/QSpice/
//-----------------------------------------------------------------------------
/*
 * FileUtil.h -- Utility functions for text file I/O.
 */
#pragma once
#include <fstream>
#include <string>
#include <vector>

namespace FileUtil {

// Open an input file stream from a path
// Throws std::runtime_error if the file cannot be opened
[[nodiscard]] std::ifstream openInput(const std::string &path);

// Open an output file stream from a path
// Throws std::runtime_error if the file cannot be opened
[[nodiscard]] std::ofstream openOutput(const std::string &path);

// istream -> vector<string>
// Reads line by line, strips CR & LF, each element has no line endings
[[nodiscard]] std::vector<std::string> readLines(std::istream &stream);

// ostream -> vector<string>
// Writes line by line; uses CRLF if useCRLF is true, otherwise LF
void writeLines(std::ostream &stream, const std::vector<std::string> &lines,
                bool useCRLF = true);

[[nodiscard]] std::vector<std::string> readLines(const std::string &path);

void writeLines(const std::string &path, const std::vector<std::string> &lines,
                bool useCRLF = true);

} // namespace FileUtil