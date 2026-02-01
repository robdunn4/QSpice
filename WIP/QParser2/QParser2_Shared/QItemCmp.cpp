#include "QItemCmp.h"
#include "StrUtils.h"

QItemCmp::QItemCmp(std::string typeStr, std::string argStr)
    : QItemBase(typeStr, argStr) {}

// copy constructor
QItemCmp::QItemCmp(const QItemCmp &other)
    : QItemBase(other), p1(other.p1), p2(other.p2), p3(other.p3) {}

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

  p1 = ArgPoint(strList[0]);
  p2 = ArgRot(strList[1]);
  p3 = ArgStuff(strList[2]);
}

std::string QItemCmp::toString() const {
  std::string str =
      typeStr + " " + p1.toString() + " " + p2.toString() + " " + p3.toString();
  return str;
}
