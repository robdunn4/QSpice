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
  StrList getStrList();

  // need to add function to test a string for embedded substitutions for use by
  // preferences/settings dialog to warn against possible infinite recursion...

protected:
  void makeRegexPats(); // could make static/initialize once
  RegexList regexList;  // could make static/initialize once

  StrList tmplStrList;

  enum KeyWordTypes : int;

  KeyWordTypes parseText(String &text, String &pfx, String &sfx);
  StrList reparseText(CBlockData &cblk, String text); // recursive

  String getAuthorText() const;
  String getCmntText(int index) const;
  String getDateShort() const;
  String getDateLong() const;
  String getDateTimeShort() const;
  String getDateTimeLong() const;

  String getAppName();
  String getAppVersion();

  static const char *const KeyWords[];
};

#endif // CODEGENERATOR_H
