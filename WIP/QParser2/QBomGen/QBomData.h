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
  void writeData(std::ostream &ostrm) const;

public:
  std::string name;    // from QItemSym
  std::string type;    // from QItemType
  std::string shorted; // from QItemShort
  std::string desc;    // from QItemDesc
  std::string refID;   // from QItemText
  std::string value;   // from QItemText
};

class QBomList : public std::vector<QBomData> {
public:
  QBomList() {};

  void sort1();
};
