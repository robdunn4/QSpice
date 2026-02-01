#include "QItemWire.h"
#include "StrUtils.h"

QItemWire::QItemWire(std::string typeStr, std::string argStr)
    : QItemBase(typeStr, argStr) {}

QItemWire::QItemWire(const QItemWire &other)
    : QItemBase(other), p1(other.p1), p2(other.p2), p3(other.p3) {}

QItemBasePtr QItemWire::clone() const {
  return std::make_shared<QItemWire>(*this);
}

void QItemWire::parseItem() {
  // we expect:  "component (-1700,-500) 0 0"
  StrUtils::StrList strList = StrUtils::tokenize(argStr);

  if (strList.size() != 3) {
    std::string str = "Unexpected content in " + typeStr + " " + argStr;
    throw std::invalid_argument(str);
  }

  p1 = ArgPoint(strList[0]);
  p2 = ArgPoint(strList[1]);
  p3 = ArgString(strList[2]);
}

std::string QItemWire::toString() const {
  std::string str =
      typeStr + " " + p1.toString() + " " + p2.toString() + " " + p3.toString();
  return str;
}
