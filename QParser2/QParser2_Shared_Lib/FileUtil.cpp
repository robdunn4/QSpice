//-----------------------------------------------------------------------------
// This file is part of the the QParser2 project.  You can find the complete
// project here:  https://github.com/robdunn4/QSpice/
//-----------------------------------------------------------------------------
#include "FileUtil.h"
#include "StrList.h"
#include <stdexcept>

namespace FileUtil {

std::ifstream openInput(const std::string &path) {
  std::ifstream stream(path, std::ios::binary); // binary?
  if (!stream)
    throw std::runtime_error("FileUtil::openInput: cannot open '" + path + "'");
  return stream;
}

std::ofstream openOutput(const std::string &path) {
  std::ofstream stream(path, std::ios::binary); // binary?
  if (!stream)
    throw std::runtime_error("FileUtil::openOutput: cannot open '" + path +
                             "'");
  return stream;
}

StrList readLines(std::istream &stream) {
  StrList     lines;
  std::string line;

  while (std::getline(stream, line)) {
    if (!line.empty() && line.back() == '\r') line.pop_back();
    lines.push_back(std::move(line));
  }
  return lines;
}

void writeLines(std::ostream &stream, const StrList &strList, bool useCRLF) {
  for (const auto &line : strList) {
    stream << line;
    if (useCRLF) stream.put('\r');
    stream.put('\n');
  }
}

StrList readLines(const std::string &path) {
  std::ifstream stream = openInput(path);
  return readLines(stream);
}

void writeLines(const std::string &path, const StrList &strList, bool useCRLF) {
  std::ofstream stream = openOutput(path);
  writeLines(stream, strList, useCRLF);
}

} // namespace FileUtil