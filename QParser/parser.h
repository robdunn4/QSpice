#ifndef PARSER_H
#define PARSER_H

#include <QString>
#include <QStringList>
#include <regex>
#include <string>
#include <vector>

// Some people, when confronted with a problem, think "I know, I'll use regular
// expressions. Now they have two problems."

typedef std::string String;
typedef std::vector<String> StrList;
typedef StrList::iterator StrListIter;
typedef StrList Block;
typedef std::vector<Block> BlockList;

enum IOType : int { IO_Input = 1, IO_Output = 2, IO_Gnd = 3 };

const char *const IOTypeText[] = {"Input", "Output", "GND"};

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
    "Not A Valid Data Type", /* matching DataType enum index */
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

class ItemState {
public:
  ItemState() : validState(false) {}
  bool validState;
};

class PinItem : public ItemState {
public:
  PinItem(const String &strType, const String &strName)
      : strType(strType), strName(strName) {
    validState = parseItem();
  }

  String makeCode(int dataNdx);

  IOType ioType;
  DataType dType;

protected:
  int parseItem();

protected:
  String strType;
  String strName;
};
typedef std::vector<PinItem> PinList;

class AttrItem : public ItemState {
public:
  AttrItem(String str) : str(str), ioType(IOType::IO_Input) { parseItem(); }

  String makeCode(int dataNdx);

  const IOType ioType; // always IO_Input
  DataType dType;

protected:
  void parseItem();
  bool reMatch(const std::regex &regex, DataType dType);

protected:
  String str;
  String varName;
};
typedef std::vector<AttrItem> AttrList;

class CBlockData : public ItemState {
public:
  CBlockData() {}
  CBlockData(String symName, String cblkName)
      : symName(symName), cblkName(cblkName) {}

  void addPinItem(PinItem item) { pins.push_back(item); }

  void addAttrItem(AttrItem item) { attrs.push_back(item); }

  StrList makeCode();

protected:
  String symName;
  // String desc;
  String cblkName;
  AttrList attrs;
  PinList pins;
};
typedef std::vector<CBlockData> CBlockDataList;

class Parser {
  enum LineType : int;

public:
  Parser();

  void loadFile(QString fileName);
  LineType lineType(int index);
  LineType lineType(String str);
  String lineTypeText(LineType type);

  QString getLine(std::size_t index);
  QStringList getLines();
  StrList getStrList() { return strList; }
  int parseStrList(StrList strList);

  QStringList genCode();

protected:
  StrList getBlock(StrListIter &begIter, const StrListIter &endIter);
  StrList getCmpBlock(StrList blk);
  CBlockData makeCblk(StrList strList);
  bool getTextParm(const String &str, String &value);
  bool getPinParm(const String &str, String &ioStr, String &pinName);

protected:
  StrList strList;
  BlockList blkList;
  CBlockDataList cblkList;

  // ensure that isspace(), isprint(), etc., don't promote to out of range (for
  // debugger)
  int isspace(int c) { return ::isspace(c & 0x7f); }
  int isprint(int c) { return ::isprint(c & 0x7f); }

  bool isStartBlk(char c) {
    bool b = (c & 0xff) == startHBlk;
    return b;
  }
  bool isEndBlk(char c) {
    bool b = (c & 0xff) == endHBlk;
    return b;
  }

  const char *idBytes = "\xff\xd8\xff\xdb";

  const unsigned char startHBlk = 0xab;
  const unsigned char endHBlk = 0xbb;
  const unsigned char phiChr = 0xd8;
  const unsigned char overBarChr = 0xac;
};

#endif // PARSER_H
