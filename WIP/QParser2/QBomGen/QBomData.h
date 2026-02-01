// QBomData.h -- BOM class.
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
  std::string name;     // from QItemSym
  std::string type;     // from QItemType
  bool shorted = false; // from QItemShort
  std::string desc;     // from QItemDesc
  std::string refID;    // from QItemText
  std::string value;    // from QItemText
};

class QBomList : public std::vector<QBomData> {
public:
  QBomList() {};

  static bool compare1(const QBomData &lhs, const QBomData &rhs);

  void sort1();
};
