#ifndef PARSER_H
#define PARSER_H

#include "cblockdata.h"
#include "codegenerator.h"
#include "strings.h"

typedef StrList Block;
typedef std::vector<Block> BlockList;

class Parser {
  enum LineType : int;

public:
  Parser();

  void reset();

  bool loadFile(const StrList &inStrings);

  LineType lineType(String str) const;
  LineType lineType(int index) const { return lineType(strList[index]); }

  String getLine(std::size_t index) const;
  StrList getLines() const;
  StrList getStrList() const { return strList; }
  void parseStrList();

  void genCblkCode(CodeGenerator &generator);

  int getCblkCnt() const { return int(cblkList.size()); }
  CBlockData getCblk(int index) const { return cblkList[index]; }

protected:
  StrList getBlockText(StrList::iterator &begIter,
      const StrList::iterator &endIter) const;
  StrList getCblkText(StrList blk) const;
  CBlockData parseCblk(StrList strList);
  bool getTextParm(const String &str, String &value, bool &isComment) const;
  bool getPinParm(const String &str, String &ioStr, String &pinName,
      bool &isBus, int &busStart, int &busEnd) const;

protected:
  StrList strList;
  BlockList blkList;
  CBlockDataList cblkList;

  bool isStartBlk(char c) const {
    bool b = (c & 0xff) == startHBlk;
    return b;
  }
  bool isEndBlk(char c) const {
    bool b = (c & 0xff) == endHBlk;
    return b;
  }

  const char *idBytes = "\xff\xd8\xff\xdb";

  const unsigned char startHBlk  = 0xab;
  const unsigned char endHBlk    = 0xbb;
  const unsigned char phiChr     = 0xd8;
  const unsigned char overBarChr = 0xac;
};

#endif // PARSER_H
