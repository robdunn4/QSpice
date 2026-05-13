//-----------------------------------------------------------------------------
// This file is part of the the QParser2 project.  You can find the complete
// project here:  https://github.com/robdunn4/QSpice/
//-----------------------------------------------------------------------------
#include "FsUtil.h"
#include <fstream>

int Fs::loadFile(std::istream &istrm, StrList &lines) {
  std::string line;
  while (std::getline(istrm, line)) {
    // Strip trailing '\r' to handle CRLF line endings on Windows
    if (!line.empty() && line.back() == '\r') line.pop_back();

    lines.push_back(std::move(line));
  }

  return istrm.bad() ? -1 : 0;
}

int Fs::loadFile(const Fs::path &filePath, StrList &lines) {
  // std::ifstream accepts std::filesystem::path natively in C++17/20,
  // correctly handling Unicode paths on Windows via the wide-string API
  std::ifstream fileStrm(filePath);
  if (!fileStrm.is_open()) return -1;

  return loadFile(fileStrm, lines) ? -1 : 0;
}

int Fs::writeFile(const Fs::path &filePath, const StrList &lines) {
  std::ofstream fileStrm(filePath);
  if (!fileStrm.is_open()) return -1;

  return writeFile(fileStrm, lines);
}

int Fs::writeFile(std::ostream &ostrm, const StrList &lines) {
  for (const auto &line : lines) {
    ostrm << line << "\n";
    if (ostrm.bad()) return -1;
  }

  return 0;
}