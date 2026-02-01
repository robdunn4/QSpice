#include "QItemType.h"
#include "StrUtils.h"

QItemType::QItemType(std::string typeStr, std::string argStr)
    : QItemBase(typeStr, argStr) {}

QItemType::QItemType(const QItemType &other)
    : QItemBase(other), p1Str(other.p1Str) {}

QItemBasePtr QItemType::clone() const {
  return std::make_shared<QItemType>(*this);
}

void QItemType::parseItem() { p1Str = ArgString(argStr); }

std::string QItemType::toString() const {
  std::string str = typeStr + " " + p1Str.toString();
  StrUtils::trim(str);
  return str;
}
