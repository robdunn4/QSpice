#ifndef CODEGENERATOR_H
#define CODEGENERATOR_H

#include "cblockdata.h"
#include <regex>

typedef std::regex Regex;
typedef std::vector<Regex> RegexList;

class CodeGenerator {
public:
  CodeGenerator();

  void loadTmplFile(const StrList &inStrings);
  StrList makeCode(CBlockData &cblk);

protected:
  void makeRegexPats(); // could make static/initialize once
  RegexList regexList;  // could make static/initialize once

  StrList tmplStrList;

  enum KeyWordTypes : int;

  KeyWordTypes parseText(String &text, String &pfx, String &sfx);
  StrList reparseText(CBlockData &cblk, String text); // recursive

  static const char *const KeyWords[];
};

#endif // CODEGENERATOR_H
