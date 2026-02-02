//-----------------------------------------------------------------------------
// This file is part of the the QParser2 project.  You can find the complete
// project here:  https://github.com/robdunn4/QSpice/
//-----------------------------------------------------------------------------
#include "QItemWire.h"
#include "StrUtils.h"

QItemWire::QItemWire(std::string typeStr, std::string argStr)
    : QItemBase(typeStr, argStr) {}

QItemWire::QItemWire(const QItemWire &other)
    : QItemBase(other), pt1(other.pt1), pt2(other.pt2), netName(other.netName) {}

QItemBasePtr QItemWire::clone() const {
  return std::make_shared<QItemWire>(*this);
}

void QItemWire::parseItem() {
  StrUtils::StrList strList = StrUtils::tokenize(argStr);

  if (strList.size() != 3) {
    std::string str = "Unexpected content in " + typeStr + " " + argStr;
    throw std::invalid_argument(str);
  }

  pt1 = ArgPoint(strList[0]);
  pt2 = ArgPoint(strList[1]);
  netName = ArgString(strList[2]);
}

std::string QItemWire::toString() const {
  std::string str =
      typeStr + " " + pt1.toString() + " " + pt2.toString() + " " + netName.toString();
  return str;
}
