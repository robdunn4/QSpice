//-----------------------------------------------------------------------------
// This file is part of the the QSymGen3 command-line tool contained in the
// QParser2 project.  You can find the complete project here:
// https://github.com/robdunn4/QSpice/
//-----------------------------------------------------------------------------
#pragma once
#include "FsUtil.h"
#include "NullStream.h"
#include "StrList.h"
#include <iostream>
#include <string>
#include <vector>

class TemplateProcessor {
public:
  TemplateProcessor() = default;

  // Load template lines from disk; returns false on failure
  bool load(const Fs::path &templatePath, std::ostream &errStrm = nullStream);

  // Register an inline scalar substitution: token replaced in-place within
  // any line that contains it (e.g. __EVAL_FUNC_NAME__ mid-line)
  void addScalar(std::string_view token, std::string_view value);

  // Register a block substitution: a line whose trimmed content equals token
  // is replaced by the lines in 'lines', each prefixed with the indentation
  // of the token line (e.g. __UDATA_CODE_SNIPPET__, __QSymGen3_Code_Snippet__)
  void addBlock(std::string_view token, const StrList &lines);

  // Apply all substitutions and write the result to outPath.
  // Returns false on failure; diagnostics go to errStrm.
  bool apply(const Fs::path &outPath, std::ostream &errStrm = nullStream);

private:
  struct ScalarSub {
    std::string token;
    std::string value;
  };

  struct BlockSub {
    std::string token;
    StrList     lines;
  };

  // Extract leading whitespace from a line
  static std::string getIndent(const std::string &line);

  // Return line with all leading whitespace stripped
  static std::string trimLeft(const std::string &line);

  // Replace all occurrences of 'from' with 'to' in 'str' (in-place)
  static void replaceAll(std::string &str, std::string_view from,
                         std::string_view to);

  StrList                  templateLines;
  std::vector<ScalarSub>   scalars;
  std::vector<BlockSub>    blocks;
};
