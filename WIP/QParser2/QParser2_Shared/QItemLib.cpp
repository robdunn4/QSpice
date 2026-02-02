//-----------------------------------------------------------------------------
// This file is part of the the QParser2 project.  You can find the complete
// project here:  https://github.com/robdunn4/QSpice/
//-----------------------------------------------------------------------------
#include "QItemLib.h"
#include "StrUtils.h"

QItemLib::QItemLib(std::string typeStr, std::string argStr)
    : QItemBase(typeStr, argStr) {}

QItemLib::QItemLib(const QItemLib &other)
    : QItemBase(other), libText(other.libText) {}

QItemBasePtr QItemLib::clone() const {
  return std::make_shared<QItemLib>(*this);
}

void QItemLib::parseItem() { libText = ArgString(argStr); }

std::string QItemLib::toString() const {
  std::string str = typeStr + " " + libText.toString();
  StrUtils::trim(str);
  return str;
}
