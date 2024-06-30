#include "parser.h"

#include <QtGlobal>
#include <cstdio>
#include <format>
#include <regex>

enum Parser::LineType : int { begBlk = 0, endBlk, itemBlk, emptyLine, unknown };

Parser::Parser() {}

void Parser::reset() {
  strList.clear();
  blkList.clear();
  cblkList.clear();
}

String trim(const String &str, const String &whitespace = " \t\n\r") {
  const auto strBegin = str.find_first_not_of(whitespace);
  if (strBegin == String::npos) return ""; // no content

  const auto strEnd   = str.find_last_not_of(whitespace);
  const auto strRange = strEnd - strBegin + 1;

  return str.substr(strBegin, strRange);
}

bool Parser::loadFile(const StrList &inStrings) {
  StrList outStrings;

  for (const String &cStr : inStrings) {
    String s = trim(cStr);
    outStrings.push_back(s);
  }

  // check for ID bytes and remove
  String s = outStrings[0];
  if (!s.starts_with(idBytes)) return false;

  s             = s.substr(4);
  outStrings[0] = s;
  strList       = outStrings;

  parseStrList();
  return true;
}

void Parser::parseStrList() {
  // parse out blocks
  StrList::iterator begIter = strList.begin();
  StrList::iterator endIter = strList.end();

  begIter++;
  while (begIter < endIter) {
    if (lineType(*begIter) == LineType::begBlk) {
      StrList blk = getBlockText(begIter, endIter);
      blk         = getCblkText(blk);
      if (blk.size() > 0) {
        blkList.push_back(blk);
        CBlockData cblk = parseCblk(blk);
        if (cblk.validState) cblkList.push_back(cblk);
      }
    } else begIter++;
  }
}

void Parser::genCblkCode(CodeGenerator &generator) {
  // generate code for all cblocks
  for (CBlockData &cblk : cblkList) {
    // generate code
    StrList code = generator.makeCode(cblk);
    cblk.setCblkCode(code);
  }
}

Parser::LineType Parser::lineType(String str) const {
  if (!str.length()) return LineType::emptyLine;
  if (isEndBlk(str[0])) return LineType::endBlk;
  if (isStartBlk(str[0])) {
    if (isEndBlk(str[str.length() - 1])) return LineType::itemBlk;
    else return LineType::begBlk;
  }

  return LineType::unknown;
}

String Parser::getLine(std::size_t index) const { return strList[index]; }

StrList Parser::getLines() const { return strList; }

// parse a block (of any type)
StrList Parser::getBlockText(StrList::iterator &begIter,
    const StrList::iterator &endIter) const {
  StrList strList;
  int depth = 0;

  do {
    if (lineType(*begIter) == LineType::begBlk) depth++;
    else if (lineType(*begIter) == LineType::endBlk) depth--;
    strList.push_back(*begIter);
  } while ((++begIter < endIter) && depth > 0);

  return strList;
}

// parse/validate first level of block is a DLL component block
StrList Parser::getCblkText(StrList blk) const {
  StrList strList;
  StrList::iterator begIter = blk.begin();
  StrList::iterator endIter = blk.end() - 1;

  if (strncmp(begIter->c_str() + 1, "component", 9)) return strList;
  begIter++;
  if (strncmp(begIter->c_str() + 1, "symbol", 6)) return strList;
  strList = getBlockText(begIter, endIter);

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
    // not a c-block
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
  * a description record (possibly empty, maybe optional?) before text items
  * text items to precede pin items
  * at least two text items (symbol symName then c-block symName)
  * zero or more text items (attribute parameters)
  * pin items -- inputs & outputs intermixed but in proper parameter order
  * we'll skip any items that aren't text or pin
*/
CBlockData Parser::parseCblk(StrList strList) {
  CBlockData cblk;
  StrList::iterator begIter       = strList.begin();
  StrList::const_iterator endIter = strList.end();

  static const char *reDescPat =
      R"(\xab)" /* break required to prevent "\xabde" */
      R"(description:\s*(\S.*)\xbb)";
  static const std::regex reDesc(reDescPat);
  std::smatch sm;

  // skip until we have text items; catch description record on the way
  String descText;
  while (begIter < endIter) {
    if (regex_match(*begIter, sm, reDesc)) {
      descText = sm[1].str();
      begIter++;
      continue;
    }

    if (!strncmp(begIter->c_str(), "\xabtext", 5)) break;
    begIter++;
  }

  if (begIter >= endIter) return cblk; // error

  // symbol name item
  String symName;
  bool isComment;
  if (!getTextParm(begIter->c_str(), symName, isComment)) return cblk; // error

  if (++begIter >= endIter) return cblk; // error

  // cblock name item
  String cblkName;
  if (!getTextParm(begIter->c_str(), cblkName, isComment)) return cblk; // error

  cblk = CBlockData(symName, cblkName, descText);

  // get optional attribute items
  String parm;
  while ((++begIter < endIter) && getTextParm(*begIter, parm, isComment)) {
    cblk.addAttrItem(AttrItem(parm, isComment));

    if (isComment) {
      String text =
          std::format("*** Attribute marked as comment: \"{}\"", parm);
      cblk.addWarning(text);
    }
  }

  // we expect pin items from here on out...
  while (begIter < endIter) {
    String ioStr;
    String pinName;
    bool isBus   = false;
    int startBus = 0;
    int endBus   = 0;

    if (!getPinParm(*begIter, ioStr, pinName, isBus, startBus, endBus))
      return cblk; // error

    if (!isBus) {
      PinItem pinItem(ioStr, pinName);
      cblk.addPinItem(pinItem);
    } else {
      // check for error
      if (startBus < 0 || endBus < 0) {
        PinItem pinItem(ioStr, pinName);
        cblk.validState    = false;
        pinItem.validState = false;
        cblk.addPinItem(pinItem);
      } else {
        int incr = startBus > endBus ? -1 : 1;
        int cnt  = abs(startBus - endBus) + 1;
        do {
          String varName = pinName + "_" + std::to_string(startBus) + "_";
          PinItem pinItem(ioStr, varName);
          cblk.addPinItem(pinItem);

          startBus += incr;
        } while (--cnt > 0);
      }
    }
    begIter++;
  }

  cblk.validState = true; // valid CBlockData
  cblk.indexItems();      // fix up item indexes

  return cblk;
}

// «text (900,-150) 0.681 13 0 0x1000000 -1 -1 "char *Attr3=Attr3Txt"»
// «text (900,-300) 0.681 13 1 0x1000000 -1 -1 "char * Attr4="Attr4 Txt""»
//                           ^
// attr4 is commented -- QSpice parses anyway...
//
// text (900,-300) 0.681 13 1 0x1000000 -1 -1  "char * Attr4="Attr4Txt""
//      ^        ^ ^   ^ ^  ^ ^       ^ ^^ ^^  ^                       ^
//      |________| |___| |  | |_______| || ||  |_______________________|
//      arg1       arg2  |  | arg5      |  |   arg8: attribute text
//                    arg3  |        arg6  arg7
//                          arg4: 0=normal, 1=commented out???

bool Parser::getTextParm(const String &str, String &value,
    bool &isComment) const {
  static const char *deepParsePat =
      R"(\xabtext (\(\S+\)) (\S+) (\S+) (\S+) (\S+) (\S+) (\S+) \"(.+)\"\xbb)";
  static const std::regex reDeepParse(deepParsePat);
  std::smatch sm;

  std::regex_match(str, sm, reDeepParse);
  if (sm.size() != 9) return false;

  value     = sm[8].str();
  isComment = sm[4].str() != "0";

  return true;
}

// parse pin parameters
// Note: This took a while to debug....  When you call
// std::regex_match(std::string str, std::smatch sm...), the values returned in
// sm are pointers into str.  If you change str, do not subsequently use
// sm[x].  You will likely create bugs.
//
bool Parser::getPinParm(const String &str, String &ioStr, String &pinName,
    bool &isBus, int &busStart, int &busEnd) const {
  // note:  the below is fragile???
  isBus    = false;
  busStart = busEnd = -1; // use -1 as error upstream

  // get ioStr and pin name
  static const char *rePinPat = R"^(\xabpin .* ([0-9]+) .+ .+ \"(.+)\"\xbb)^";
  static const std::regex rePin(rePinPat);

  std::smatch sm;
  bool bMat;

  bMat = std::regex_match(str, sm, rePin);
  if (!bMat || sm.size() != 3) return false; // error

  ioStr   = sm[1].str();
  pinName = sm[2].str();

  // parse into a valid variable name possibly followed by more text
  // note: special character (0xac = ) for overbar is embedded in pin name
  static const char *rePinNamePartsPat =
      R"^((\xac?_?[a-zA-Z\xac]+[a-zA-Z0-9_\xac]*)(.*))^";
  static const std::regex rePinNameParts(rePinNamePartsPat);

  bMat = std::regex_match(pinName, sm, rePinNameParts);
  Q_ASSERT(bMat);         // this shouldn't fail?
  if (!bMat) return true; // this shouldn't happen?

  // there was more text; is it a valid range value ("[n:n]")?
  String sRange = sm[2].str();
  pinName       = sm[1].str();

  if (!sRange.length()) return true; // no range text

  // assume some attempt to make bus range
  isBus = true;

  static const char *reBusRangePat = R"^(\[([0-9])+:([0-9])+\])^";
  static const std::regex reBusRange(reBusRangePat);

  bMat = std::regex_match(sRange, sm, reBusRange);
  if (!bMat) return true; // not a valid range

  // make range valid
  busStart = stoi(sm[1]);
  busEnd   = stoi(sm[2]);
  return true;
}
