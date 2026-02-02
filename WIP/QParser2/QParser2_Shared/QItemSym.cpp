//-----------------------------------------------------------------------------
// This file is part of the the QParser2 project.  You can find the complete
// project here:  https://github.com/robdunn4/QSpice/
//-----------------------------------------------------------------------------
#include "QItemSym.h"
#include "StrUtils.h"

QItemSym::QItemSym(std::string typeStr, std::string argStr)
    : QItemBase(typeStr, argStr) {}

QItemSym::QItemSym(const QItemSym &other)
    : QItemBase(other), text(other.text) {}

QItemBasePtr QItemSym::clone() const {
  return std::make_shared<QItemSym>(*this);
}

void QItemSym::parseItem() {
  StrUtils::StrList strList = StrUtils::tokenize(argStr);
  if (strList.size() > 1) {
    std::string str = "Unexpected content in " + typeStr + " " + argStr;
    throw std::invalid_argument(str);
  }

  if (strList.size()) text = ArgString(strList[0]);
}

std::string QItemSym::toString() const {
  std::string str = StrUtils::trim(typeStr + " " + text.toString());
  return str;
}
