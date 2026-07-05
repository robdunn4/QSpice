//-----------------------------------------------------------------------------
// This file is part of the the QParser2 project.  You can find the complete
// project here:  https://github.com/robdunn4/QSpice/
//-----------------------------------------------------------------------------
/*
 * FileUtil.h -- Utility functions for text file I/O.
 */
#pragma once
#include "StrList.h"
#include <fstream>

namespace FileUtil {

// Open an input file stream from a path
// Throws std::runtime_error if the file cannot be opened
[[nodiscard]] std::ifstream openInput(const std::string &path);

// Open an output file stream from a path
// Throws std::runtime_error if the file cannot be opened
[[nodiscard]] std::ofstream openOutput(const std::string &path);

// istream -> vector<string>
// Reads line by line, strips CR & LF, each element has no line endings
[[nodiscard]] StrList readLines(std::istream &stream);

// ostream -> vector<string>
// Writes line by line; uses CRLF if useCRLF is true, otherwise LF
void writeLines(std::ostream &stream, const StrList &lines,
                bool useCRLF = true);

[[nodiscard]] StrList readLines(const std::string &path);

void writeLines(const std::string &path, const StrList &lines,
                bool useCRLF = true);

} // namespace FileUtil