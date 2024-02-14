#include "cblockdata.h"

#include "appexception.h"
#include <cstdio>
#include <format> // requires C++20
#include <regex>

enum IOType : int { IO_INVALID = 0, IO_Input = 1, IO_Output = 2, IO_Gnd = 3 };

const char *const IOTypeText[] = {"INVALID", "Input", "Output", "GND"};

enum DataType : int {
  DT_INVALID = 0, /* invalid value */
  DT_B = 1,       /* bool */
  DT_C = 2,       /* char */
  DT_UC = 3,      /* unsigned char */
  DT_S = 4,       /* short */
  DT_US = 5,      /* unsigned short */
  DT_I = 6,       /* int */
  DT_UI = 7,      /* unsigned int */
  DT_F32 = 8,     /* float */
  DT_D = 9,       /* double */
  DT_I64 = 10,    /* long long int */
  DT_UI64 = 11,   /* unsigned long long int */
  DT_BV = 12,     /* bit vector -- not supported yet... */
  DT_ATT_STR = 13 /* valid only for char* attributes */
};

const char *const DataTypeText[] = {
    "INVALID DATA TYPE", /* matching DataType enum index */
    "bool",
    "char",
    "unsigned char",
    "short",
    "unsigned short",
    "int",
    "unsigned int",
    "float",
    "double",
    "long long int",
    "unsigned long long int",
    "*** bit vector (unsupported) ***",
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
    ".str" /* char* */};

//  note: requires C++20 for std::format()
String codeFmt(String dataType, String varName, int dataNdx, String uDataSfx,
               String commentText, bool refChar = false) {
  return std::format("{: <24} {}{: <12} = data[{: >2}]{: <5}; // {}", dataType,
                     refChar ? '&' : ' ', varName, dataNdx, uDataSfx,
                     commentText);
}

String PinItem::makeCode() {
  if (!validState)
    return "*** Invalid Pin Item @ [" + std::to_string(itemNdx) + "]: [" +
           varName + "]";

  String dataType = DataTypeText[dType];
  String uDataSfx = UDataSfx[dType];
  String commentText = ioType == IO_Input ? "input" : "output";

  return codeFmt(dataType, varName, itemNdx, uDataSfx, commentText,
                 (ioType == IO_Output));
}

String PinItem::makeTruncSaveCode() {
  if (!validState)
    return "*** Invalid Pin Item @ [" + std::to_string(itemNdx) + "]: [" +
           varName + "]";

  String dataType = DataTypeText[dType];
  return std::format("const {: <30} _{: <12} = {: <12};", dataType, varName,
                     varName);
}

String PinItem::makeTruncRestoreCode() {
  if (!validState)
    return "*** Invalid Pin Item @ [" + std::to_string(itemNdx) + "]: [" +
           varName + "]";

  String dataType = DataTypeText[dType];
  return std::format("{: <12} = _{: <12};", varName, varName);
}

int PinItem::parseItem() {
  // note:  casting int to enum might be questionable???
  int ioInt = std::stoi(strRaw);

  // low nibble determines I/O type; high nibble determines data type
  ioType = IOType(ioInt & 0x0f);
  dType = DataType((ioInt >> 4) & 0x0f);

  if (ioType < IO_Input || ioType > IO_Gnd)
    return false;
  if (dType < DT_B || dType > DT_BV)
    return false;

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
 * kills subsequent attributes...
 */

AttrItem::AttrItem(String str, bool isComment)
    : strRaw(str), ioType(IOType::IO_INVALID), attrNbr(-1),
      bComment(isComment) {
  parseItem();
}

String AttrItem::makeCode() {
  // no need for stack copies for most of these...
  String dataType = DataTypeText[dType];
  String uDataSfx = UDataSfx[dType];
  String commentText = "input parameter";

  if (!validState)
    return "*** Invalid Attribute Item @ [" + std::to_string(itemNdx) + "]: [" +
           this->strRaw + "]";

  return codeFmt(dataType, varName, itemNdx, uDataSfx, commentText);
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

// // «text (900,-150) 0.681 13 0 0x1000000 -1 -1 "char *Attr3=Attr3Txt"»
// // «text (900,-300) 0.681 13 1 0x1000000 -1 -1 "char * Attr4="Attr4 Txt""»
// //                           ^
// // attr4 is commented -- QSpice doesn't honor and parses anyway

// // temporarily implementing as separate function
// bool AttrItem::parseDeep() {
//   // static const char *reBoolPat = R"(bool +([_a-zA-Z0-9]+)=(.+)$)";
//   // static const char *reCharPat = R"(char +([_a-zA-Z0-9]+)=(.+)$)";
//   // static const char *reIntPat = R"(int +([_a-zA-Z0-9]+)=(.+)$)";
//   // static const char *reFloatPat = R"(float +([_a-zA-Z0-9]+)=(.+)$)";
//   // static const char *reDoublePat = R"(double +([_a-zA-Z0-9]+)=(.+)$)";
//   // static const char *reStrPat1 = R"(char +\* *([_a-zA-Z0-9]+)=(.+)$)";
//   // static const char *reStrPat2 = R"(char\* +([_a-zA-Z0-9]+)=(.+)$)";

//   // static const std::regex reBool(reBoolPat);
//   // static const std::regex reChar(reCharPat);
//   // static const std::regex reInt(reIntPat);
//   // static const std::regex reFloat(reFloatPat);
//   // static const std::regex reDouble(reDoublePat);
//   // static const std::regex reStr1(reStrPat1);
//   // static const std::regex reStr2(reStrPat2);

//   static const char *deepParsePat =
//       R"(^(\S+) \((\S+)\) (\S+) (\S+) (.+) (\S+) (\S+)
//       (([_a-zA-Z0-9]+)=(.+))$)";
//   static const std::regex reDeepParse(deepParsePat);

//   std::smatch sm;
//   if (!std::regex_match(strRaw, sm, reDeepParse)) // || sm.size() != 4)
//     return false;
//   return true;
// }

void AttrItem::parseItem() {
  static const char *reBoolPat = R"(bool +([_a-zA-Z0-9]+)=(.+)$)";
  static const char *reCharPat = R"(char +([_a-zA-Z0-9]+)=(.+)$)";
  static const char *reIntPat = R"(int +([_a-zA-Z0-9]+)=(.+)$)";
  static const char *reFloatPat = R"(float +([_a-zA-Z0-9]+)=(.+)$)";
  static const char *reDoublePat = R"(double +([_a-zA-Z0-9]+)=(.+)$)";
  static const char *reStrPat1 = R"(char +\* *([_a-zA-Z0-9]+)=(.+)$)";
  static const char *reStrPat2 = R"(char\* +([_a-zA-Z0-9]+)=(.+)$)";

  static const std::regex reBool(reBoolPat);
  static const std::regex reChar(reCharPat);
  static const std::regex reInt(reIntPat);
  static const std::regex reFloat(reFloatPat);
  static const std::regex reDouble(reDoublePat);
  static const std::regex reStr1(reStrPat1);
  static const std::regex reStr2(reStrPat2);

  if (reMatch(reBool, DataType::DT_B))
    return;
  if (reMatch(reChar, DataType::DT_C))
    return;
  if (reMatch(reInt, DataType::DT_I))
    return;
  if (reMatch(reFloat, DataType::DT_D)) // note conversion to double
    return;
  if (reMatch(reDouble, DataType::DT_D))
    return;
  if (reMatch(reStr1, DataType::DT_ATT_STR))
    return;
  if (reMatch(reStr2, DataType::DT_ATT_STR))
    return;
}

// AttrItem::parseItem() calls this after basic parsing setup
// additional parameter value (stuff after "=") is done here
bool AttrItem::reMatch(const std::regex &regex, DataType dType) {
  std::smatch sm;
  if (!std::regex_match(strRaw, sm, regex) || sm.size() != 3)
    return false;

  // first sub-match is attribute name
  varName = sm[1].str();
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

  if (std::regex_match(str, sm, reParm)) {
    validState = true;
    return validState;
  }

  // validate (possibly signed) integral value (bool, char, int???)
  if (dType == DT_B || dType == DT_C || dType == DT_I) {
    static const char *reIntPat = R"(-?[0-9]+)";
    static const std::regex reInt(reIntPat);
    if (std::regex_match(str, sm, reInt)) {
      validState = true;
      return validState;
    } else if (dType != DT_C) {
      return false;
    }
  }

  // validate single-quoted char
  if (dType == DT_C) {
    static const char *reCharPat = R"('.')";
    static const std::regex reChar(reCharPat);
    if (std::regex_match(str, sm, reChar)) {
      validState = true;
      return validState;
    } else
      return false;
  }

  // validate float/double
  // does QSpice allow leading +/- and permit "." with no trailing digits?
  if (dType == DT_F32 || dType == DT_D) {
    static const char *reDblPat = R"([+-]?[0-9]+(\.[0-9]+))";
    static const std::regex reDbl(reDblPat);
    if (std::regex_match(str, sm, reDbl)) {
      validState = true;
      return validState;
    } else
      return false;
  }

  // final case -- quoted char string; ensure outer quotes are present
  // and no embedded quotes
  static const char *reStrPat = R"("[^"]*")";
  static const std::regex reStr(reStrPat);
  if (std::regex_match(str, sm, reStr)) {
    validState = true;
    return validState;
  }

  validState = false;
  return validState;
}

// call after parsing pins/attributes to fix item indexes
void CBlockData::addPinItem(PinItem item) {
  switch (item.ioType) {
  case IO_Input:
    pinsIn.push_back(item);
    break;
  case IO_Output:
    pinsOut.push_back(item);
    break;
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

void CBlockData::indexItems() {
  // clear cblk item counter
  itemCnt = 1;
  int pinNbr = 0;
  int attrNbr = 1;

  String warning;

  // first input pins
  for (PinItem &item : pinsIn) {
    item.itemNdx = itemCnt++;
    item.pinNbr = pinNbr++;

    if (!item.validState) {
      warning = std::format("*** Pin Possibly Invalid: {} (Pin Index = {})",
                            item.getVarName(), item.pinNbr);
      addWarning(warning);
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
    }
  }

  // then output pins
  for (PinItem &item : pinsOut) {
    item.itemNdx = itemCnt++;
    item.pinNbr = pinNbr++;

    if (!item.validState) {
      warning = std::format("*** Pin Possibly Invalid: {} (Pin Index = {})",
                            item.getVarName(), item.pinNbr);
      addWarning(warning);
    }
  }
}

StrList CBlockData::makeUdataCode() {
  StrList code;

  // first input pins
  for (PinItem &item : pinsIn) {
    code.push_back(item.makeCode());
  }
  // then input attributes
  for (AttrItem &item : attrs) {
    code.push_back(item.makeCode());
  }
  // then output pins
  for (PinItem &item : pinsOut) {
    code.push_back(item.makeCode());
  }

  return code;
}

StrList CBlockData::makeUndefCode() {
  StrList code;
  String str;

  for (PinItem &item : pinsIn) {
    str = "#undef " + item.getVarName();
    code.push_back(str);
  }
  for (AttrItem &item : attrs) {
    str = "#undef " + item.getVarName();
    code.push_back(str);
  }
  for (PinItem &item : pinsOut) {
    str = "#undef " + item.getVarName();
    code.push_back(str);
  }

  return code;
}

StrList CBlockData::makeTruncOutCode() {
  StrList code;

  // just the output pins
  for (PinItem &item : pinsOut) {
    code.push_back(item.makeCode());
  }

  return code;
}

StrList CBlockData::makeTruncSaveCode() {
  StrList code;

  // just the output pins
  for (PinItem &item : pinsOut) {
    code.push_back(item.makeTruncSaveCode());
  }

  return code;
}

StrList CBlockData::makeTruncRestoreCode() {
  StrList code;

  // just the output pins
  for (PinItem &item : pinsOut) {
    code.push_back(item.makeTruncRestoreCode());
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

StrList CBlockData::getCblkSummary() {
  StrList strList;
  String str;

  strList.push_back("*** Component Summary ***");

  strList.push_back("");
  strList.push_back(std::format("Schematic Symbol:      {}", symName));
  strList.push_back(std::format("Component/DLL Name:    {}", cblkName));

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
        "PinOrder: {: <2}, PinName: {: <10}, PortType: {}, DataType: {}",
        pin.pinNbr, pin.getVarName(), IOTypeText[pin.ioType],
        DataTypeText[pin.dType]);
    strList.push_back(str);
  }

  // output pin details
  strList.push_back("");
  strList.push_back("Output Pin Details:");
  for (PinItem &pin : pinsOut) {
    str = std::format(
        "PinOrder: {: <2}, PinName: {: <10}, PortType: {}, DataType: {}",
        pin.pinNbr, pin.getVarName(), IOTypeText[pin.ioType],
        DataTypeText[pin.dType]);
    strList.push_back(str);
  }

  // attribute string details
  strList.push_back("");
  strList.push_back("Input Attribute Details:");
  for (AttrItem &attr : attrs) {
    str = std::format("TextOrder: {: <2}, Content: {: <15}, DataType: {}",
                      attr.attrNbr + 1, attr.getRawText().c_str(),
                      DataTypeText[attr.dType]);
    strList.push_back(str);
  }

  return strList;
}

StrList CBlockData::getCblkCode() { return code; }
