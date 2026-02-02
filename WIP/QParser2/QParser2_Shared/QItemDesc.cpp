//-----------------------------------------------------------------------------
// This file is part of the the QParser2 project.  You can find the complete
// project here:  https://github.com/robdunn4/QSpice/
//-----------------------------------------------------------------------------
#include "QItemDesc.h"
#include "StrUtils.h"

QItemDesc::QItemDesc(std::string typeStr, std::string argStr)
    : QItemBase(typeStr, argStr) {}

QItemDesc::QItemDesc(const QItemDesc &other)
    : QItemBase(other), text(other.text) {}

QItemBasePtr QItemDesc::clone() const {
  return std::make_shared<QItemDesc>(*this);
}

void QItemDesc::parseItem() { text = ArgString(argStr); }

std::string QItemDesc::toString() const {
  std::string str = typeStr + " " + text.toString();
  StrUtils::trim(str);
  return str;
}
