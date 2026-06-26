//-----------------------------------------------------------------------------
// This file is part of the the QSymGen3 command-line tool contained in the
// QParser2 project.  You can find the complete project here:
// https://github.com/robdunn4/QSpice/
//-----------------------------------------------------------------------------
#include "TemplateProcessor.h"
#include <fstream>

bool TemplateProcessor::load(const Fs::path &templatePath,
                             std::ostream   &errStrm) {
  templateLines.clear();
  int res = Fs::loadFile(templatePath, templateLines);
  if (res) {
    errStrm << "TemplateProcessor: failed to load template: "
            << templatePath.string() << "\n";
    return false;
  }
  return true;
}

void TemplateProcessor::addScalar(std::string_view token,
                                  std::string_view value) {
  scalars.push_back({std::string(token), std::string(value)});
}

void TemplateProcessor::addBlock(std::string_view token,
                                 const StrList   &lines) {
  blocks.push_back({std::string(token), lines});
}

bool TemplateProcessor::apply(const Fs::path &outPath,
                              std::ostream   &errStrm) {
  StrList output;

  for (const std::string &line : templateLines) {
    std::string indent  = getIndent(line);
    std::string trimmed = trimLeft(line);

    // check if trimmed line is a whole-line block token
    bool matchedBlock = false;
    for (const BlockSub &bs : blocks) {
      if (trimmed == bs.token) {
        // emit each replacement line prefixed with the token line's indentation
        for (const std::string &replaceLine : bs.lines)
          output.push_back(indent + replaceLine);
        matchedBlock = true;
        break;
      }
    }
    if (matchedBlock) continue;

    // not a block token -- apply scalar substitutions in-place
    std::string outLine = line;
    for (const ScalarSub &ss : scalars)
      replaceAll(outLine, ss.token, ss.value);

    output.push_back(std::move(outLine));
  }

  int res = Fs::writeFile(outPath, output);
  if (res) {
    errStrm << "TemplateProcessor: failed to write output: "
            << outPath.string() << "\n";
    return false;
  }

  return true;
}

// private helpers

std::string TemplateProcessor::getIndent(const std::string &line) {
  std::string indent;
  for (char c : line) {
    if (c == ' ' || c == '\t') indent += c;
    else break;
  }
  return indent;
}

std::string TemplateProcessor::trimLeft(const std::string &line) {
  std::size_t pos = line.find_first_not_of(" \t");
  return (pos != std::string::npos) ? line.substr(pos) : "";
}

void TemplateProcessor::replaceAll(std::string &str, std::string_view from,
                                   std::string_view to) {
  if (from.empty()) return;
  std::size_t pos = 0;
  while ((pos = str.find(from, pos)) != std::string::npos) {
    str.replace(pos, from.size(), to);
    pos += to.size();
  }
}
