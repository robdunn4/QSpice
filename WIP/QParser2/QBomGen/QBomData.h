//-----------------------------------------------------------------------------
// This file is part of the the QParser2 project.  You can find the complete
// project here:  https://github.com/robdunn4/QSpice/
//-----------------------------------------------------------------------------
/*
 * QBomData.h -- BOM class and vector.
 */
#pragma once
#include "QSchTree.h"
#include <ostream>
#include <string>
#include <vector>

class QBomData {
public:
  QBomData() {}

  bool parseData(const QSchTreePtr symItem);

  static void writeHeader(std::ostream &ostrm);
  void        writeData(std::ostream &ostrm) const;

public:
  std::string name;    // from ItemSym
  std::string type;    // from ItemType
  std::string shorted; // from ItemShort
  std::string desc;    // from ItemDesc
  std::string refID;   // from ItemText
  std::string value;   // from ItemText
};

class QBomList : public std::vector<QBomData> {
public:
  QBomList() {};

  void sort1();
};

