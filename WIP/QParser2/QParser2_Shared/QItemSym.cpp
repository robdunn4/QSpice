#include "QItemSym.h"
#include "StrUtils.h"

QItemSym::QItemSym(std::string typeStr, std::string argStr)
    : QItemBase(typeStr, argStr) {}

QItemSym::QItemSym(const QItemSym &other)
    : QItemBase(other), p1Str(other.p1Str) {}

QItemBasePtr QItemSym::clone() const {
  return std::make_shared<QItemSym>(*this);
}

void QItemSym::parseItem() {
  StrUtils::StrList strList = StrUtils::tokenize(argStr);
  if (strList.size() > 1) {
    std::string str = "Unexpected content in " + typeStr + " " + argStr;
    throw std::invalid_argument(str);
  }

  if (strList.size()) p1Str = ArgString(strList[0]);
}

std::string QItemSym::toString() const {
  std::string str = StrUtils::trim(typeStr + " " + p1Str.toString());
  return str;
}
