#include "parser.h"

#include "appexception.h"
#include <cstdio>
#include <format>
#include <regex>

// for RAII/exception handling
class FilePtr {
public:
  FilePtr() : ptr(nullptr) {}
  FilePtr(FILE *ptr) : ptr(ptr) {}
  ~FilePtr() {
    if (ptr)
      fclose(ptr);
  }

  FILE *ptr;
};

// note: requires C++20 for std::format()
String codeFmt(String dataType, String varName, int dataNdx, String uDataSfx,
               String commentText, bool refChar = false) {
  return std::format("{: <24} {}{: <12} = data[{: >2}]{: <5}; // {}", dataType,
                     refChar ? '&' : ' ', varName, dataNdx, uDataSfx,
                     commentText);
}

enum Parser::LineType : int { begBlk = 0, endBlk, itemBlk, emptyLine, unknown };

Parser::Parser() {}

// throws exceptions on errors...
void Parser::loadFile(QString fileName) {
  FilePtr file;
  char buf[2048]; // file read buffer

  // open file for binary read
  file.ptr = fopen(fileName.toStdString().c_str(), "rb");
  if (!file.ptr)
    throw FileError("Error opening file");

  // read first four bytes for ID signature
  if (!fread(buf, 4, 1, file.ptr))
    throw FileError("Error reading QSpice ID bytes");

  // verify signature
  if (memcmp(buf, idBytes, 4))
    throw FileError("File has invalid QSpice ID bytes");

  // read file into stringlist with error checking
  char *s; // start of text
  s = fgets(buf, sizeof(buf), file.ptr);
  while (!feof(file.ptr) && !ferror(file.ptr)) {
    Q_ASSERT(s); // shouldn't happen

    // ensure buffer is null-terminated (not sure about fgets())
    size_t len = strlen(buf);
    if (len >= sizeof(buf))
      throw FileError("Internal buffer overflow on file read");

    // trim leading whitespace
    s = buf;
    while (!isStartBlk(*s) && isspace(*s))
      s++;

    // trim trailing whitespace
    len = strlen(s);
    for (char *end = s + len - 1; end > s; end--) {
      if (!isspace(*end))
        break;
      *end = 0;
    }

    // add to stringlist
    strList.push_back(String(s));

    // get next line
    s = fgets(buf, sizeof(buf), file.ptr);
  }

  // a final (probably unnecessary) error check
  if (ferror(file.ptr))
    throw FileError("Unexpected file error state");
}

int Parser::parseStrList(StrList strList) {
  // parse out blocks
  StrListIter begIter = strList.begin();
  StrListIter endIter = strList.end();

  begIter++;
  while (begIter < endIter) {
    if (lineType(*begIter) == LineType::begBlk) {
      StrList blk = getBlock(begIter, endIter);
      blk = getCmpBlock(blk);
      if (blk.size() > 0) {
        blkList.push_back(blk);
        CBlockData cblk = makeCblk(blk);
        if (cblk.validState)
          cblkList.push_back(cblk);
      }
    } else
      begIter++;
  }

  return 0;
}

QStringList Parser::genCode() {
  QStringList qStrList;
  if (cblkList.size() > 0) {
    StrList strList;
    strList = cblkList[0].makeCode();
    for (int i = 0; i < strList.size(); i++)
      qStrList.append(strList[i].c_str());
  }
  return qStrList;
}

const char *lineTypes[] = {"Start Block", "End Block", "Item Block",
                           "Empty Line", "Unknown"};

const char *blkTypes[] = {"schematic", "component", "other"};

const char *keyWords[] = {"schematic",     "component", "type: \xd8(.DLL)",
                          "description: ", "text ",     "pin "};

Parser::LineType Parser::lineType(int index) {
  return lineType(strList[index]);
}

Parser::LineType Parser::lineType(String str) {
  if (!str.length())
    return LineType::emptyLine;
  if (isEndBlk(str[0]))
    return LineType::endBlk;
  if (isStartBlk(str[0])) {
    if (isEndBlk(str[str.length() - 1]))
      return LineType::itemBlk;
    else
      return LineType::begBlk;
  }

  return LineType::unknown;
}

String Parser::lineTypeText(LineType type) {
  String str(lineTypes[type]);

  return str;
}

QString Parser::getLine(std::size_t index) {
  return QString(strList[index].c_str());
}

// convert strings to QStrings
QStringList Parser::getLines() {
  QStringList list;

  for (int i = 0; i < strList.size(); i++) {
    list.append(getLine(i));
  }
  return list;
}

StrList Parser::getBlock(StrListIter &begIter, const StrListIter &endIter) {
  StrList strList;
  int depth = 0;

  do {
    if (lineType(*begIter) == LineType::begBlk)
      depth++;
    else if (lineType(*begIter) == LineType::endBlk)
      depth--;
    strList.push_back(*begIter);
  } while ((++begIter < endIter) && depth > 0);

  return strList;
}

// parse/validate first level of block is a DLL component block
StrList Parser::getCmpBlock(StrList blk) {
  StrList strList;
  StrListIter begIter = blk.begin();
  StrListIter endIter = blk.end() - 1;

  if (strncmp(begIter->c_str() + 1, "component", 9))
    return strList;
  begIter++;
  if (strncmp(begIter->c_str() + 1, "symbol", 6))
    return strList;
  strList = getBlock(begIter, endIter);

  // need better error check here...
  if (strList.size() < 3) {
    strList.clear();
    return strList;
  }
  // remove first/last elements
  strList.erase(strList.begin());
  strList.pop_back();

  // verify dll entry («type: Ø(.DLL)»)
  const char *litText = "\xabtype: \xd8(.DLL)\xbb\0"; // move to *.h???
  if (strcmp(strList.begin()->c_str(), litText)) {
    strList.clear();
    return strList;
  }

  // delete the first entry («type: Ø(.DLL)»)
  strList.erase(strList.begin());

  return strList;
}

/*
parse a block of text into a CBlockData instance
 we expect:
  * text items to precede pin items
  * at least two text items (symbol symName then c-block symName)
  * zero or more text items (attribute parameters)
  * pin items -- inputs & outputs intermixed but in proper parameter order
  * we'll skip any items that aren't text or pin

 Note:  There's an optional description item that we might want to pick up
        later.
*/
CBlockData Parser::makeCblk(StrList strList) {
  CBlockData cblk;
  StrListIter begIter = strList.begin();
  StrListIter endIter = strList.end();

  // skip until we have text items
  while (begIter < endIter) {
    if (!strncmp(begIter->c_str(), "\xabtext", 5))
      break;
    begIter++;
  }

  if (begIter >= endIter) {
    // error
    return cblk;
  }

  // symbol name item
  String symName;
  if (!getTextParm(begIter->c_str(), symName)) {
    // error
    return cblk;
  }

  if (++begIter >= endIter) {
    // error
    return cblk;
  }

  // cblock name item
  String cblkName;
  if (!getTextParm(begIter->c_str(), cblkName)) {
    // error
    return cblk;
  }
  cblk = CBlockData(symName, cblkName);

  // get optional attribute items
  String parm;
  while ((++begIter < endIter) && getTextParm(*begIter, parm)) {
    cblk.addAttrItem(AttrItem(parm));
  }

  // we expect pin items from here on out...
  while (begIter < endIter) {
    String ioStr;
    String pinName;
    if (!getPinParm(*begIter, ioStr, pinName)) {
      return cblk; // error
    }

    cblk.addPinItem(PinItem(ioStr, pinName));
    begIter++;
  }

  cblk.validState = true; // valid CBlockData
  return cblk;
}

bool Parser::getTextParm(const String &str, String &value) {
  // maybe need parse for comment flag and warn user if "attribute is a
  // comment"???
  const char *reTextPat = R"^(\xabtext [^\"]*\"\s*(.+)\s*\"\xbb)^";

  std::smatch sm;
  if (!std::regex_match(str, sm, std::regex(reTextPat)) || sm.size() != 2)
    return false;
  value = sm[1].str();
  return true;
}

bool Parser::getPinParm(const String &str, String &ioStr, String &pinName) {
  // note:  the below is fragile, expecting " 0x0 -1 " between io type string
  // and name
  const char *rePinPat = R"^(\xabpin .* ([0-9]+) .+ .+ \"(.+)\"\xbb)^";

  std::smatch sm;
  if (!std::regex_match(str, sm, std::regex(rePinPat)) || sm.size() != 3)
    return false;

  ioStr = sm[1];
  pinName = sm[2].str();

  return true;
}

String PinItem::makeCode(int dataNdx) {
  String str;

  if (!validState) {
    str = "*** Invalid Pin Item: [" + strName + "]";
    return str;
  }

  String varName = this->strName;
  String::iterator begIter = varName.begin();
  String::iterator endIter = varName.end();

  // replace "overscore" characters with underscores ala QSpice
  // should add an option to simply remove them
  while (begIter < endIter) {
    if (*begIter == '\xac')
      *begIter = '_';
    begIter++;
  }

  String dataType = DataTypeText[dType];
  String uDataSfx = UDataSfx[dType];
  String commentText = ioType == IO_Input ? "input" : "output";

  str = codeFmt(dataType, varName, dataNdx, uDataSfx, commentText,
                (ioType == IO_Output));

  return str;
}

int PinItem::parseItem() {
  // note:  casting int to enum might be questionable???
  int ioInt = std::stoi(strType);

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

String AttrItem::makeCode(int dataNdx) {
  // no need for stack copies for most of these...
  String dataType = DataTypeText[dType];
  String uDataSfx = UDataSfx[dType];
  String commentText = "input parameter";
  String str;

  if (!validState) {
    str = "*** Invalid Attribute Item: [" + this->str + "]";
    return str;
  }

  str = codeFmt(dataType, varName, dataNdx, uDataSfx, commentText);

  return str;
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
  const char *reBoolPat = R"(bool +([_a-zA-Z0-9]+)=(.+)$)";
  const char *reCharPat = R"(char +([_a-zA-Z0-9]+)=(.+)$)";
  const char *reIntPat = R"(int +([_a-zA-Z0-9]+)=(.+)$)";
  const char *reFloatPat = R"(float +([_a-zA-Z0-9]+)=(.+)$)";
  const char *reDoublePat = R"(double +([_a-zA-Z0-9]+)=(.+)$)";
  const char *reStrPat1 = R"(char +\* *([_a-zA-Z0-9]+)=(.+)$)";
  const char *reStrPat2 = R"(char\* +([_a-zA-Z0-9]+)=(.+)$)";

  const std::regex reBool(reBoolPat);
  const std::regex reChar(reCharPat);
  const std::regex reInt(reIntPat);
  const std::regex reFloat(reFloatPat);
  const std::regex reDouble(reDoublePat);
  const std::regex reStr1(reStrPat1);
  const std::regex reStr2(reStrPat2);

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
  if (!std::regex_match(str, sm, regex) || sm.size() != 3)
    return false;

  // first sub-match is attribute name
  varName = sm[1].str();
  this->dType = dType;

  // second sub-match is attribute value (after "=")
  // we'll do additional testing for validity by type...
  String str = sm[2].str();

  // handle possible .param literal
  const char *reParmPat = R"([a-zA-Z][_[:alnum:]]*)";
  const std::regex reParm(reParmPat);
  if (std::regex_match(str, sm, reParm)) {
    validState = true;
    return validState;
  }

  // validate (possibly signed) integral value (bool, char, int???)
  if (dType == DT_B || dType == DT_C || dType == DT_I) {
    const char *reIntPat = R"(-?[0-9]+)";
    const std::regex reInt(reIntPat);
    if (std::regex_match(str, sm, reInt)) {
      validState = true;
      return validState;
    } else if (dType != DT_C) {
      return false;
    }
  }

  // validate single-quoted char
  if (dType == DT_C) {
    const char *reCharPat = R"('.')";
    const std::regex reChar(reCharPat);
    if (std::regex_match(str, sm, reChar)) {
      validState = true;
      return validState;
    } else
      return false;
  }

  // validate float/double
  // does QSpice allow leading +/- and permit "." with no trailing digits?
  if (dType == DT_F32 || dType == DT_D) {
    const char *reDblPat = R"([+-]?[0-9]+(\.[0-9]+))";
    const std::regex reDbl(reDblPat);
    if (std::regex_match(str, sm, reDbl)) {
      validState = true;
      return validState;
    } else
      return false;
  }

  // final case -- quoted char string; ensure outer quotes are present
  // and no embedded quotes
  const char *reStrPat = R"("[^"]*")";
  const std::regex reStr(reStrPat);
  if (std::regex_match(str, sm, reStr)) {
    validState = true;
    return validState;
  }

  validState = false;
  return validState;
}

StrList CBlockData::makeCode() {
  StrList code;
  String codeItem;
  int ndx = 0;

  // first input pins
  PinList::iterator begPinIter = pins.begin();
  PinList::iterator endPinIter = pins.end();

  while (begPinIter < endPinIter) {
    if (begPinIter->ioType == IO_Input) {
      codeItem = begPinIter->makeCode(ndx);
      code.push_back(codeItem);
      ndx++;
    }
    begPinIter++;
  }

  // next attributes
  AttrList::iterator begAttrIter = attrs.begin();
  AttrList::iterator endAttrIter = attrs.end();
  while (begAttrIter < endAttrIter) {
    if (begAttrIter->ioType == IO_Input) {
      codeItem = begAttrIter->makeCode(ndx);
      code.push_back(codeItem);
      ndx++;
    }
    begAttrIter++;
  }

  // finally, output pins
  begPinIter = pins.begin();
  while (begPinIter < endPinIter) {
    if (begPinIter->ioType == IO_Output) {
      codeItem = begPinIter->makeCode(ndx);
      code.push_back(codeItem);
      ndx++;
    }
    begPinIter++;
  }

  return code;
}
