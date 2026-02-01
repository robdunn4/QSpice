#include "QItemTap.h"
#include "StrUtils.h"

QItemTap::QItemTap(std::string typeStr, std::string argStr)
    : QItemBase(typeStr, argStr) {}

QItemTap::QItemTap(const QItemTap &other)
    : QItemBase(other), p1(other.p1), p2(other.p2) {}

QItemBasePtr QItemTap::clone() const {
  return std::make_shared<QItemTap>(*this);
}

void QItemTap::parseItem() {
  StrUtils::StrList strList = StrUtils::tokenize(argStr);

  if (strList.size() != 2) {
    std::string str = "Unexpected content in " + typeStr + " " + argStr;
    throw std::invalid_argument(str);
  }

  p1 = ArgPoint(strList[0]);
  p2 = ArgPoint(strList[1]);
}

std::string QItemTap::toString() const {
  std::string str = typeStr + " " + p1.toString() + " " + p2.toString();
  return str;
}
