#ifndef CBLOCKDATA_H
#define CBLOCKDATA_H

#include "types.h"
#include <regex>

// Some people, when confronted with a problem, think "I know, I'll use regular
// expressions. Now they have two problems."

enum IOType : int;
enum DataType : int;

class ItemState {
public:
  ItemState() : validState(false) {}
  bool validState;
  int itemNdx;
};

class PinItem : public ItemState {
public:
  PinItem(const String &strType, const String &strName)
      : strRaw(strType), varName(strName), pinNbr(-1) {
    validState = parseItem();

    // replace "overscore" characters with underscores ala QSpice
    // should add an option to simply remove them
    String::iterator begIter = this->varName.begin();
    String::iterator endIter = this->varName.end();
    while (begIter < endIter) {
      if (*begIter == '\xac')
        *begIter = '_';
      begIter++;
    }
  }

  String makeCode();
  String makeTruncSaveCode();
  String makeTruncRestoreCode();

  String getVarName() const { return varName; }
  String getRawText() const { return strRaw; }

  IOType ioType;
  DataType dType;
  int pinNbr; // pin index corresponding to schematic/port pin order...

protected:
  int parseItem();

  String strRaw;
  String varName;
};
typedef std::vector<PinItem> PinList;

class AttrItem : public ItemState {
public:
  AttrItem(String str, bool isComment = false);

  String makeCode();

  String getVarName() const;
  String getRawText() const { return strRaw; }
  bool isComment() { return bComment; }

  const IOType ioType; // always IO_Input
  DataType dType;
  int attrNbr; // string attributes order in symbol properties

protected:
  void parseItem();
  bool reMatch(const std::regex &regex, DataType dType);

protected:
  String strRaw;
  String varName;
  bool bComment;
};
typedef std::vector<AttrItem> AttrList;

class CBlockData : public ItemState {
public:
  CBlockData() {}
  CBlockData(String symName, String cblkName, String descText)
      : symName(symName), cblkName(cblkName), descText(descText) {
    if (!descText.length())
      this->descText = "[no description]";
  }

  void addPinItem(PinItem item);
  void addAttrItem(AttrItem item) { attrs.push_back(item); }

  void indexItems();

  StrList makeUdataCode();
  StrList makeUndefCode();

  StrList makeTruncOutCode();
  StrList makeTruncSaveCode();
  StrList makeTruncRestoreCode();

  String getSymName() const { return symName; }
  String getCblkName() const { return cblkName; }
  String getCblkNameLC() const;
  String getCblkDesc() const { return descText; }

  StrList getCblkSummary();

  void setCblkCode(StrList &cblkCode) { code = cblkCode; }
  StrList getCblkCode();

  void addWarning(String &text) { warnings.push_back(text); }
  const StrList &getWarnings() const { return warnings; }

protected:
  String symName;
  String descText;
  String cblkName;
  AttrList attrs;
  PinList pinsIn;
  PinList pinsOut;
  int itemCnt;

  StrList code; // generated code
  StrList warnings;
};
typedef std::vector<CBlockData> CBlockDataList;

#endif // CBLOCKDATA_H