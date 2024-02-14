#include "codegenerator.h"

#include "appexception.h"

const char *const CodeGenerator::KeyWords[] = {
    // {"author_name"},               // embedded
    // {"date_generated"},            // embedded
    // {"copyright_text"},            // embedded
    // {"dll_name"},  // embedded
    // {"template_name"}, // embedded
    {"inst_struct"},               // embedded
    {"eval_func"},                 // embedded
    {"undef"},                     // block
    {"udata_all"},                 // block
    {"trunc_udata_output"},        // block
    {"trunc_udata_save_output"},   // block
    {"trunc_udata_restore_output"} // block
};

enum CodeGenerator::KeyWordTypes : int {
  // KWT_Author,       // embedded
  // KWT_Date,         // embedded
  // KWT_Copyright,    // embedded
  // KWT_DllName,      // embedded
  // KWT_TemplateName, // embedded
  KWT_InstStruct,   // embedded
  KWT_EvalFunc,     // embedded
  KWT_Undef,        // block
  KWT_UdataAll,     // block
  KWT_UdataOutput,  // block
  KWT_UdataSave,    // block
  KWT_UdataRestore, // block
  KWT_NoMatch
};

CodeGenerator::CodeGenerator() { makeRegexPats(); }

// should rename now that it's only a setXX() method
void CodeGenerator::loadTmplFile(const StrList &inStrings) {
  tmplStrList = inStrings;
}

StrList CodeGenerator::makeCode(CBlockData &cblk) {
  StrList strList;

  for (String &str : tmplStrList) {
    StrList strs = reparseText(cblk, str);

    for (String &str : strs)
      strList.push_back(str);
  }

  return strList;
}

void CodeGenerator::makeRegexPats() {
  // create regex patterns to capture any string before a keywork,
  // the keyword (needed?), and subsequent text to eol
  String pat;

  for (int i = 0; i < sizeof(KeyWords) / sizeof(KeyWords[0]); i++) {
    switch (KeyWordTypes(i)) {
    case KWT_InstStruct:
    case KWT_EvalFunc:
      // embedded pattern
      pat = String(R"(^(.*)%%()") + KeyWords[i] + R"()%%(.*)$)";
      break;
    case KWT_Undef:
    case KWT_UdataAll:
    case KWT_UdataOutput:
    case KWT_UdataSave:
    case KWT_UdataRestore:
      // block pattern
      pat = String(R"(^(\s*)%%()") + KeyWords[i] + R"()%%(\s*)$)";
      break;
    }

    Regex regex = Regex(pat);
    regexList.push_back(regex);
  }
}

CodeGenerator::KeyWordTypes CodeGenerator::parseText(String &text, String &pfx,
                                                     String &sfx) {
  int kwdNdx = 0;
  std::smatch sm;

  for (Regex &regex : regexList) {
    if (std::regex_match(text, sm, regex))
      break;
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

  kwt = parseText(text, pfx, sfx);

  switch (kwt) {
  case KWT_InstStruct:
    strs = reparseText(cblk, pfx + "s" + cblk.getCblkName() + sfx);
    break;
  case KWT_Undef:
    return cblk.makeUndefCode();
    break;
  case KWT_EvalFunc:
    strs = reparseText(cblk, pfx + cblk.getCblkNameLC() + sfx);
    break;
  case KWT_UdataAll:
    return cblk.makeUdataCode();
    break;
  case KWT_UdataOutput:
    return cblk.makeTruncOutCode();
    break;
  case KWT_UdataSave:
    return cblk.makeTruncSaveCode();
    break;
  case KWT_UdataRestore:
    return cblk.makeTruncRestoreCode();
    break;
  default:
    strs.push_back(text);
    break;
  }

  return strs;
}
