//-----------------------------------------------------------------------------
// This file is part of the the QParser2 project.  You can find the complete
// project here:  https://github.com/robdunn4/QSpice/
//-----------------------------------------------------------------------------
#include "QItemType.h"
#include "StrUtils.h"

QItemType::QItemType(std::string typeStr, std::string argStr)
    : QItemBase(typeStr, argStr) {}

QItemType::QItemType(const QItemType &other)
    : QItemBase(other), text(other.text) {}

QItemBasePtr QItemType::clone() const {
  return std::make_shared<QItemType>(*this);
}

void QItemType::parseItem() { text = ArgString(argStr); }

std::string QItemType::toString() const {
  std::string str = typeStr + " " + text.toString();
  StrUtils::trim(str);
  return str;
}
