#include "QItemDesc.h"
#include "StrUtils.h"

QItemDesc::QItemDesc(std::string typeStr, std::string argStr)
    : QItemBase(typeStr, argStr) {}

QItemDesc::QItemDesc(const QItemDesc &other)
    : QItemBase(other), p1Str(other.p1Str) {}

QItemBasePtr QItemDesc::clone() const {
  return std::make_shared<QItemDesc>(*this);
}

void QItemDesc::parseItem() { p1Str = ArgString(argStr); }

std::string QItemDesc::toString() const {
  std::string str = typeStr + " " + p1Str.toString();
  StrUtils::trim(str);
  return str;
}
