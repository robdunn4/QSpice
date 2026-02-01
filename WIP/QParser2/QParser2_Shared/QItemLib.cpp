#include "QItemLib.h"
#include "StrUtils.h"

QItemLib::QItemLib(std::string typeStr, std::string argStr)
    : QItemBase(typeStr, argStr) {}

QItemLib::QItemLib(const QItemLib &other)
    : QItemBase(other), p1Str(other.p1Str) {}

QItemBasePtr QItemLib::clone() const {
  return std::make_shared<QItemLib>(*this);
}

void QItemLib::parseItem() { p1Str = ArgString(argStr); }

std::string QItemLib::toString() const {
  std::string str = typeStr + " " + p1Str.toString();
  StrUtils::trim(str);
  return str;
}
