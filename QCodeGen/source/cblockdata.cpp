/*
 * There is much ugliness here due to early decision to have pin/attribute
 * classes format themselves.  Need to move into CBlockData class.
 */

#include "cblockdata.h"
#include <algorithm>

#include <format> // requires C++20
#include <regex>

enum IOType : int { IO_INVALID = 0, IO_Input, IO_Output, IO_Gnd };

const char *const IOTypeText[] = {"INVALID", "Input", "Output", "GND"};

enum DataType : int {
  DT_INVALID = 0,  /* invalid value */
  DT_B       = 1,  /* bool */
  DT_C       = 2,  /* char */
  DT_UC      = 3,  /* unsigned char */
  DT_S       = 4,  /* short */
  DT_US      = 5,  /* unsigned short */
  DT_I       = 6,  /* int */
  DT_UI      = 7,  /* unsigned int */
  DT_F32     = 8,  /* float */
  DT_D       = 9,  /* double */
  DT_I64     = 10, /* long long int */
  DT_UI64    = 11, /* unsigned long long int */
  DT_BV      = 12, /* bit vector -- not supported yet... */
  DT_ATT_STR = 13  /* valid only for char* attributes */
};

const char *const DataTypeText[] = {
    "INVALID DATA TYPE", /* matching DataType enum index */
    "bool", "char", "unsigned char", "short", "unsigned short", "int",
    "unsigned int", "float", "double", "long long int",
    "unsigned long long int", "*** bit vector (unsupported) ***",
    "const char *"};

const char *const UDataSfx[] = {
    //
    ".!!!",   /* Not A Valid Data Type */
    ".b",     /* bool */
    ".c",     /* char */
    ".uc",    /* unsigned char */
    ".s",     /* short */
    ".us",    /* unsigned short */
    ".i",     /* int */
    ".ui",    /* unsigned int */
    ".f",     /* float */
    ".d",     /* double */
    ".i64",   /* long long int */
    ".ui64",  /* unsigned long long int */
    ".bytes", /* *** bit vector (unsupported) *** */
    ".str"    /* char* */
};

//  note: requires C++20 for std::format()
String codeFmt(String dataType, String varName, int dataNdx, String uDataSfx,
    String commentText, int varNameLen, int dataTypeLen, bool refChar = false) {
  return std::format("{: <{}} {}{: <{}} = data[{: >2}]{: <5} ; // {}", dataType,
      dataTypeLen, refChar ? '&' : ' ', varName, varNameLen, dataNdx, uDataSfx,
      commentText);
}

String PinItem::makeCode(int varNameLen, int dataTypeLen) const {
  if (!validState)
    return "*** Invalid Pin Item @ [" + std::to_string(itemNdx) + "]: [" +
           varName + "]";

  String dataType    = DataTypeText[dType];
  String uDataSfx    = UDataSfx[dType];
  String commentText = ioType == IO_Input ? "input" : "output";

  return codeFmt(dataType, varName, itemNdx - 1, uDataSfx, commentText,
      varNameLen, dataTypeLen, (ioType == IO_Output));
}

String PinItem::makeTruncSaveCode(int varNameLen, int dataTypeLen) {
  if (!validState)
    return "*** Invalid Pin Item @ [" + std::to_string(itemNdx) + "]: [" +
           varName + "]";

  String dataType = DataTypeText[dType];
  return std::format("const {: <{}} _{: <{}} = {: <{}} ;", dataType,
      dataTypeLen, varName, varNameLen, varName, varNameLen);
}

String PinItem::makeTruncRestoreCode(int varNameLen) {
  if (!validState)
    return "*** Invalid Pin Item @ [" + std::to_string(itemNdx) + "]: [" +
           varName + "]";

  String dataType = DataTypeText[dType];
  return std::format("{: <{}} = _{: <{}} ;", varName, varNameLen, varName,
      varNameLen);
}

int PinItem::parseItem() {
  // note:  casting int to enum might be questionable???
  int ioInt = std::stoi(strRaw);

  // low nibble determines I/O type; high nibble determines data type
  ioType = IOType(ioInt & 0x0f);
  dType  = DataType((ioInt >> 4) & 0x0f);

  if (ioType < IO_Input || ioType > IO_Gnd) return false;
  if (dType < DT_B || dType > DT_BV) return false;

  return true;
}

/* parsing attributes is going to be a bit iffy...
 * there might or might not be an "=".  there might or might not be
 * a data type.  need to check out all of the possibilites....
 *
 * in particular:  "char* attr", "char *attr", and "char*attr" are possible.
 * the first two produce c-block code.  the third is treated as if no data type
 * was specified and is ignored (i.e., no uData[] element is generated).
 *
 * also, it appears that, once QSpice silently fails to parse an attribute, it
 * stops trying.  (Need to verify that an invalid but commented out attribute
 * kills subsequent attributes...)
 */

AttrItem::AttrItem(String str, bool isComment)
    : ioType(IOType::IO_INVALID), attrNbr(-1), strRaw(str),
      bComment(isComment) {
  parseItem();
}

String AttrItem::makeCode(int varNameLen, int dataTypeLen) const {
  // no need for stack copies for most of these...
  String dataType    = DataTypeText[dType];
  String uDataSfx    = UDataSfx[dType];
  String commentText = "input parameter";

  if (!validState)
    return "*** Invalid Attribute Item @ [" + std::to_string(itemNdx) + "]: [" +
           this->strRaw + "]";

  return codeFmt(dataType, varName, itemNdx - 1, uDataSfx, commentText,
      varNameLen, dataTypeLen);
}

// should we do this?
String AttrItem::getVarName() const {
  if (!validState)
    return "*** Invalid attribute @ [" + std::to_string(itemNdx) + "] [" +
           strRaw + "]";

  return varName;
}

// valid patterns are:
// • “bool Attr=[bval]”
// • “char Attr=[cval]”
// • “int Attr=[ival]”
// • “float Attr=[fval]” (promoted to “double Attr=[fval]”)
// • “double Attr=[fval]”
// • “char* Attr=[sval]”
// • “char *Attr=[sval]”
// • “char * Attr=[sval]”

void AttrItem::parseItem() {
  static const char *reBoolPat   = R"(bool +([_a-zA-Z0-9]+)=(.+)$)";
  static const char *reCharPat   = R"(char +([_a-zA-Z0-9]+)=(.+)$)";
  static const char *reIntPat    = R"(int +([_a-zA-Z0-9]+)=(.+)$)";
  static const char *reFloatPat  = R"(float +([_a-zA-Z0-9]+)=(.+)$)";
  static const char *reDoublePat = R"(double +([_a-zA-Z0-9]+)=(.+)$)";
  static const char *reStrPat1   = R"(char +\* *([_a-zA-Z0-9]+)=(.+)$)";
  static const char *reStrPat2   = R"(char\* +([_a-zA-Z0-9]+)=(.+)$)";

  static const std::regex reBool(reBoolPat);
  static const std::regex reChar(reCharPat);
  static const std::regex reInt(reIntPat);
  static const std::regex reFloat(reFloatPat);
  static const std::regex reDouble(reDoublePat);
  static const std::regex reStr1(reStrPat1);
  static const std::regex reStr2(reStrPat2);

  if (reMatch(reBool, DataType::DT_B)) return;
  if (reMatch(reChar, DataType::DT_C)) return;
  if (reMatch(reInt, DataType::DT_I)) return;
  if (reMatch(reFloat, DataType::DT_D)) // note conversion to double
    return;
  if (reMatch(reDouble, DataType::DT_D)) return;
  if (reMatch(reStr1, DataType::DT_ATT_STR)) return;
  if (reMatch(reStr2, DataType::DT_ATT_STR)) return;
}

// AttrItem::parseItem() calls this after basic parsing setup
// additional parameter value (stuff after "=") is done here
bool AttrItem::reMatch(const std::regex &regex, DataType dType) {
  std::smatch sm;
  if (!std::regex_match(strRaw, sm, regex) || sm.size() != 3) return false;

  // first sub-match is attribute name
  varName     = sm[1].str();
  this->dType = dType;

  // second sub-match is attribute value (after "=")
  // we'll do additional testing for validity by type...
  String str = sm[2].str();

  // handle possible .param literal
  // note:  allowing .param literal for char* attribute doesn't
  //        really make sense, i.e., not allowed when running
  //        the simulation but I think QSpice allows it when
  //        generating code...
  static const char *reParmPat = R"([a-zA-Z][_[:alnum:]]*)";
  static const std::regex reParm(reParmPat);

  if (std::regex_match(str, sm, reParm)) return validState = true;

  // validate (possibly signed) integral value (bool, char, int)
  if (dType == DT_B || dType == DT_C || dType == DT_I) {
    static const char *reIntPat = R"(-?[0-9]+)";
    static const std::regex reInt(reIntPat);
    if (std::regex_match(str, sm, reInt)) return validState = true;
    else if (dType != DT_C) return false;
    // fallthrough to quoted char
  }

  // validate single-quoted char
  if (dType == DT_C) {
    static const char *reCharPat = R"('.')";
    static const std::regex reChar(reCharPat);
    return validState = std::regex_match(str, sm, reChar);
  }

  // validate float/double -- it's a bit more complex
  if (dType == DT_F32 || dType == DT_D) {
    return validState = reMatchDblFlt(str);
  }

  // final case -- quoted char string; ensure outer quotes are present
  // and no embedded quotes
  static const char *reStrPat = R"("[^"]*")";
  static const std::regex reStr(reStrPat);
  return validState = std::regex_match(str, sm, reStr);
}

// parsing double/float values is complicated...
// might be in scientific format (e.g., 1e-6), decimal (e.g., 0.000001),
// decimal with suffix (0.001m), etc.  trying to be flexible but
// probably not handling all cases perfectly, definitely not well-tested...
// note: u suffix (Mu) = 0xb5
bool AttrItem::reMatchDblFlt(const String str) {
  static const char *reExpPat =
      R"(\s*[+-]?[0-9]+(\.[0-9]*)?[eE]([+-]?[0-9]+)?\s*)";
  static const std::regex reExp(reExpPat);
  static const char *reDecPat =
      R"(\s*([+-]?[0-9]+)(\.[0-9]*)?([fpnmukgt\xb5]?|(meg)?)\s*)";
  static const std::regex reDec(reDecPat, std::regex::icase);
  std::smatch sm;

  // try both formats
  bool res = false;
  if (std::regex_match(str, sm, reExp)) res = true;
  else if (std::regex_match(str, sm, reDec)) res = true;
  return res;
}

void CBlockData::addPinItem(PinItem item) {
  switch (item.ioType) {
  case IO_Input: pinsIn.push_back(item); break;
  case IO_Output: pinsOut.push_back(item); break;
  case IO_Gnd: // do nothing
    break;
  default: // add warning
    String text =
        std::format("*** Invalid Pin I/O Port Type  : {}", item.getVarName());
    addWarning(text);
  }

  if (item.dType == DT_INVALID) {
    String text =
        std::format("*** Invalid Pin Port Data Type : {}", item.getVarName());
    addWarning(text);
  }
}

// call after parsing pins/attributes to fix item indexes
// also captures lengths for generating "pretty" formatted code
void CBlockData::indexItems() {
  // clear cblk item counter
  itemCnt     = 1;
  int pinNbr  = 0;
  int attrNbr = 1;

  String warning;

  // first input pins
  for (PinItem &item : pinsIn) {
    item.itemNdx = itemCnt++;
    item.pinNbr  = pinNbr++;

    if (!item.validState) {
      warning = std::format("*** Pin Possibly Invalid: {} (Pin Index = {})",
          item.getVarName(), item.pinNbr);
      addWarning(warning);
    } else {
      maxVarNameLen = std::max(maxVarNameLen, (int)item.getVarName().length());
      maxDataTypeLen =
          std::max(maxDataTypeLen, (int)strlen(DataTypeText[item.dType]));
    }
  }

  // then input attributes
  for (AttrItem &item : attrs) {
    item.itemNdx = itemCnt++;
    item.attrNbr = attrNbr++;

    if (!item.validState) {
      warning = std::format(
          "*** Attribute Possibly Invalid : @Index={: >2}, Text=[{}]",
          item.attrNbr + 1, item.getRawText());
      addWarning(warning);
    } else {
      maxVarNameLen = std::max(maxVarNameLen, (int)item.getVarName().length());
      maxDataTypeLen =
          std::max(maxDataTypeLen, (int)strlen(DataTypeText[item.dType]));
    }
  }

  // then output pins
  for (PinItem &item : pinsOut) {
    item.itemNdx = itemCnt++;
    item.pinNbr  = pinNbr++;

    if (!item.validState) {
      warning = std::format("*** Pin Possibly Invalid: {} (Pin Index = {})",
          item.getVarName(), item.pinNbr);
      addWarning(warning);
    } else {
      maxVarNameLen = std::max(maxVarNameLen, (int)item.getVarName().length());
      maxDataTypeLen =
          std::max(maxDataTypeLen, (int)strlen(DataTypeText[item.dType]));
    }
  }
}

StrList CBlockData::makeUdataCode(const String &blkKeyword, const String &pfx,
    int varNameLen, int dataTypeLen) const {
  StrList code;

  // add commented out block keyword
  code.push_back(pfx + "/* %%" + blkKeyword + "%% */");

  // first input pins
  for (const PinItem &item : pinsIn) {
    code.push_back(pfx + item.makeCode(varNameLen, dataTypeLen));
  }
  // then input attributes
  for (const AttrItem &item : attrs) {
    code.push_back(pfx + item.makeCode(varNameLen, dataTypeLen));
  }
  // then output pins
  for (const PinItem &item : pinsOut) {
    code.push_back(pfx + item.makeCode(varNameLen, dataTypeLen));
  }

  return code;
}

StrList CBlockData::makeUndefCode(const String &blkKeyword, const String &pfx) {
  StrList code;

  // add commented out block keyword
  code.push_back(pfx + "/* %%" + blkKeyword + "%% */");

  for (PinItem &item : pinsIn) {
    code.push_back(pfx + "#undef " + item.getVarName());
  }

  // apparently, QSpice doesn't undef attributes so removed for now
  // for (AttrItem &item : attrs) {
  //   code.push_back(pfx + "#undef " + item.getVarName());
  // }

  for (PinItem &item : pinsOut) {
    code.push_back(pfx + "#undef " + item.getVarName());
  }

  return code;
}

StrList CBlockData::makeTruncOutCode(const String &blkKeyword,
    const String &pfx) {
  StrList code;

  // add commented out block keyword
  code.push_back(pfx + "/* %%" + blkKeyword + "%% */");

  // just the output pins
  for (PinItem &item : pinsOut) {
    code.push_back(pfx + item.makeCode(maxVarNameLen, maxDataTypeLen));
  }

  return code;
}

StrList CBlockData::makeTruncSaveCode(const String &blkKeyword,
    const String &pfx) {
  StrList code;

  // add commented out block keyword
  code.push_back(pfx + "/* %%" + blkKeyword + "%% */");

  // just the output pins
  for (PinItem &item : pinsOut) {
    code.push_back(pfx + item.makeTruncSaveCode(maxVarNameLen, maxDataTypeLen));
  }

  return code;
}

StrList CBlockData::makeTruncRestoreCode(const String &blkKeyword,
    const String &pfx) {
  StrList code;

  // add commented out block keyword
  code.push_back(pfx + "/* %%" + blkKeyword + "%% */");

  // just the output pins
  for (PinItem &item : pinsOut) {
    code.push_back(pfx + item.makeTruncRestoreCode(maxVarNameLen));
  }

  return code;
}

String CBlockData::getCblkNameLC() const {
  // apparently, no std::string function to do this...
  String s = getCblkName();
  std::transform(s.begin(), s.end(), s.begin(),
      [](unsigned char c) { return std::tolower(c); });
  return s;
}

String CBlockData::getCblkNameUC() const {
  // apparently, no std::string function to do this...
  String s = getCblkName();
  std::transform(s.begin(), s.end(), s.begin(),
      [](unsigned char c) { return std::toupper(c); });
  return s;
}

String CBlockData::getUdataSize() const {
  int dsize = int(pinsIn.size()) + int(attrs.size()) + int(pinsOut.size());
  return std::format("{}", dsize);
}

StrList CBlockData::getCblkSummary() {
  StrList strList;
  String str;

  strList.push_back("*** Component Summary ***");

  strList.push_back("");
  strList.push_back(std::format("Schematic Symbol:      {}", symName));
  strList.push_back(std::format("Component/DLL Name:    {}", cblkName));
  strList.push_back(std::format("Component Description: {}", getCblkDesc()));

  strList.push_back("");
  strList.push_back(
      std::format("Warnings:              {}", warnings.size() ? "" : "None"));
  strList.insert(strList.end(), warnings.begin(), warnings.end());

  strList.push_back("");
  strList.push_back(std::format("Input Pin Count:       {}", pinsIn.size()));
  strList.push_back(std::format("Input Attribute Count: {}", attrs.size()));
  strList.push_back(std::format("Output Pin Count:      {}", pinsOut.size()));

  // input pin details
  strList.push_back("");
  strList.push_back("Input Pin Details:");
  for (PinItem &pin : pinsIn) {
    str = std::format(
        "PinOrder: {: >2}, PinName: {: <10}, PortType: {}, DataType: {}",
        pin.pinNbr, pin.getVarName(), IOTypeText[pin.ioType],
        DataTypeText[pin.dType]);
    strList.push_back(str);
  }

  // output pin details
  strList.push_back("");
  strList.push_back("Output Pin Details:");
  for (PinItem &pin : pinsOut) {
    str = std::format(
        "PinOrder: {: >2}, PinName: {: <10}, PortType: {}, DataType: {}",
        pin.pinNbr, pin.getVarName(), IOTypeText[pin.ioType],
        DataTypeText[pin.dType]);
    strList.push_back(str);
  }

  // attribute string details
  strList.push_back("");
  strList.push_back("Input Attribute Details:");
  for (AttrItem &attr : attrs) {
    str = std::format("TextOrder: {: >2}, Content: {}, DataType: {}",
        attr.attrNbr + 1, attr.getRawText().c_str(), DataTypeText[attr.dType]);
    strList.push_back(str);
  }

  return strList;
}

StrList CBlockData::getCblkCode() { return code; }
