#include "codegenerator.h"
#include "aboutdlg.h"
#include "prefsdlg.h"
#include <QDateTime>

const char *const CodeGenerator::KeyWords[] = {
    "AppName",           // embedded
    "AppVersion",        // embedded
    "Author",            // embedded
    "User1",             // embedded
    "User2",             // embedded
    "User3",             // embedded
    "DateShort",         // embedded
    "DateLong",          // embedded
    "DateTimeShort",     // embedded
    "DateTimeLong",      // embedded
    "ComponentName",     // embedded
    "ComponentNameLC",   // embedded
    "ComponentNameUC",   // embedded
    "ComponentDesc",     // embedded
    "UdataSize",         // embedded
    "Undef",             // block
    "UdataAll",          // block
    "UdataOutput",       // block
    "UdataSaveOutput",   // block
    "UdataRestoreOutput" // block
};

enum CodeGenerator::KeyWordTypes : int {
  KWT_AppName,       // embedded
  KWT_AppVersion,    // embedded
  KWT_Author,        // embedded
  KWT_User1,         // embedded
  KWT_User2,         // embedded
  KWT_User3,         // embedded
  KWT_DateShort,     // embedded
  KWT_DateLong,      // embedded
  KWT_DateTimeShort, // embedded
  KWT_DateTimeLong,  // embedded
  KWT_CmpName,       // embedded
  KWT_CmpNameLC,     // embedded
  KWT_CmpNameUC,     // embedded
  KWT_CmpDesc,       // embedded
  KWT_UdataSize,     // embeded
  KWT_Undef,         // block
  KWT_UdataAll,      // block
  KWT_UdataOutput,   // block
  KWT_UdataSave,     // block
  KWT_UdataRestore,  // block
  KWT_NoMatch
};

CodeGenerator::CodeGenerator() { makeRegexPats(); }

void CodeGenerator::loadTmplFile(const StrList &inStrings) {
  tmplStrList = inStrings;
}

StrList CodeGenerator::makeCode(CBlockData &cblk) {
  StrList strList;

  for (String &str : tmplStrList) {
    StrList strs = reparseText(cblk, str);

    for (String &str : strs) strList.push_back(str);
  }

  return strList;
}

StrList CodeGenerator::getStrList() { return tmplStrList; }

void CodeGenerator::makeRegexPats() {
  // create regex patterns to capture any string before a keywork,
  // the keyword (needed?), and subsequent text to eol
  String pat;

  for (int i = 0; i < sizeof(KeyWords) / sizeof(KeyWords[0]); i++) {
    switch (KeyWordTypes(i)) {
      // embedded codes
    case KWT_AppName:
    case KWT_AppVersion:
    case KWT_Author:
    case KWT_User1:
    case KWT_User2:
    case KWT_User3:
    case KWT_DateShort:
    case KWT_DateLong:
    case KWT_DateTimeShort:
    case KWT_DateTimeLong:
    case KWT_CmpName:
    case KWT_CmpNameLC:
    case KWT_CmpNameUC:
    case KWT_CmpDesc:
    case KWT_UdataSize:
      // embedded pattern
      pat = String(R"(^(.*)%%()") + KeyWords[i] + R"()%%(.*)$)";
      break;

    // block codes
    case KWT_Undef:
    case KWT_UdataAll:
    case KWT_UdataOutput:
    case KWT_UdataSave:
    case KWT_UdataRestore:
      // block pattern
      pat = String(R"(^(\s*)%%()") + KeyWords[i] + R"()%%(\s*)$)";
      break;
    case KWT_NoMatch: break;
    }

    Regex regex = Regex(pat,
        std::regex_constants::ECMAScript | std::regex_constants::icase);
    regexList.push_back(regex);
  }
}

CodeGenerator::KeyWordTypes CodeGenerator::parseText(String &text, String &pfx,
    String &sfx) {
  int kwdNdx = 0;
  std::smatch sm;

  for (Regex &regex : regexList) {
    if (std::regex_match(text, sm, regex)) break;
    kwdNdx++;
  }

  pfx = sm[1];
  sfx = sm[3];
  return KeyWordTypes(kwdNdx);
}

// recursive parsing...
StrList CodeGenerator::reparseText(CBlockData &cblk, String text) {
  KeyWordTypes kwt;
  StrList strs;
  String pfx;
  String sfx;
  String s;
  int vnLen = cblk.maxVarNameLen;
  int dtLen = cblk.maxDataTypeLen;

  kwt = parseText(text, pfx, sfx);

  switch (kwt) {
  // embedded keywords
  case KWT_AppName: strs = reparseText(cblk, pfx + getAppName() + sfx); break;
  case KWT_AppVersion:
    strs = reparseText(cblk, pfx + getAppVersion() + sfx);
    break;
  case KWT_Author: strs = reparseText(cblk, pfx + getAuthorText() + sfx); break;
  case KWT_User1: strs = reparseText(cblk, pfx + getCmntText(0) + sfx); break;
  case KWT_User2: strs = reparseText(cblk, pfx + getCmntText(1) + sfx); break;
  case KWT_User3: strs = reparseText(cblk, pfx + getCmntText(2) + sfx); break;
  case KWT_DateShort:
    strs = reparseText(cblk, pfx + getDateShort() + sfx);
    break;
  case KWT_DateLong: strs = reparseText(cblk, pfx + getDateLong() + sfx); break;
  case KWT_DateTimeShort:
    strs = reparseText(cblk, pfx + getDateTimeShort() + sfx);
    break;
  case KWT_DateTimeLong:
    strs = reparseText(cblk, pfx + getDateTimeLong() + sfx);
    break;
  case KWT_CmpName:
    strs = reparseText(cblk, pfx + cblk.getCblkName() + sfx);
    break;
  case KWT_CmpNameUC:
    strs = reparseText(cblk, pfx + cblk.getCblkNameUC() + sfx);
    break;
  case KWT_CmpNameLC:
    strs = reparseText(cblk, pfx + cblk.getCblkNameLC() + sfx);
    break;
  case KWT_CmpDesc:
    strs = reparseText(cblk, pfx + cblk.getCblkDesc() + sfx);
    break;
  case KWT_UdataSize:
    strs = reparseText(cblk, pfx + cblk.getUdataSize() + sfx);
    break;

  // block keywords
  case KWT_Undef: return cblk.makeUndefCode(KeyWords[kwt], pfx);
  case KWT_UdataAll:
    return cblk.makeUdataCode(KeyWords[kwt], pfx, vnLen, dtLen);
  case KWT_UdataOutput: return cblk.makeTruncOutCode(KeyWords[kwt], pfx);
  case KWT_UdataSave: return cblk.makeTruncSaveCode(KeyWords[kwt], pfx);
  case KWT_UdataRestore: return cblk.makeTruncRestoreCode(KeyWords[kwt], pfx);

  // no keywords
  case KWT_NoMatch:
  default: strs.push_back(text); break;
  }

  return strs;
}

String CodeGenerator::getAuthorText() const {
  return prefsSettings.getAuthor().toStdString();
}

String CodeGenerator::getCmntText(int index) const {
  return prefsSettings.getUserField(index).toStdString();
}

String CodeGenerator::getDateShort() const {
  QString dt = QDateTime::currentDateTime().toString("yyyy.MM.dd");
  return dt.toStdString();
}

String CodeGenerator::getDateLong() const {
  return QDateTime::currentDateTime().toString("ddd MMMM d yyyy").toStdString();
}

String CodeGenerator::getDateTimeShort() const {
  return QDateTime::currentDateTime()
      .toString("yyyy.MM.dd hh:mm:ss")
      .toStdString();
}

String CodeGenerator::getDateTimeLong() const {
  return QDateTime::currentDateTime().toString().toStdString();
}

String CodeGenerator::getAppName() { return AboutDlg::appName.toStdString(); }

String CodeGenerator::getAppVersion() {
  return AboutDlg::appVersion.toStdString();
}
