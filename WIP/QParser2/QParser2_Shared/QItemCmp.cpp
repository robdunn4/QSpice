//-----------------------------------------------------------------------------
// This file is part of the the QParser2 project.  You can find the complete
// project here:  https://github.com/robdunn4/QSpice/
//-----------------------------------------------------------------------------
#include "QItemCmp.h"
#include "StrUtils.h"

QItemCmp::QItemCmp(std::string typeStr, std::string argStr)
    : QItemBase(typeStr, argStr) {}

// copy constructor
QItemCmp::QItemCmp(const QItemCmp &other)
    : QItemBase(other), pt1(other.pt1), rotate(other.rotate), stuffShort(other.stuffShort) {}

// clone method
QItemBasePtr QItemCmp::clone() const {
  return std::make_shared<QItemCmp>(*this);
}
void QItemCmp::parseItem() {
  StrUtils::StrList strList = StrUtils::tokenize(argStr);

  if (strList.size() != 3) {
    std::string str = "Unexpected content in " + typeStr + " " + argStr;
    throw std::invalid_argument(str);
  }

  pt1 = ArgPoint(strList[0]);
  rotate = ArgRot(strList[1]);
  stuffShort = ArgStuff(strList[2]);
}

std::string QItemCmp::toString() const {
  std::string str =
      typeStr + " " + pt1.toString() + " " + rotate.toString() + " " + stuffShort.toString();
  return str;
}
