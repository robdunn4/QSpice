#include "QItemJunction.h"
#include "StrUtils.h"

QItemJunction::QItemJunction(std::string typeStr, std::string argStr)
    : QItemBase(typeStr, argStr) {}

QItemJunction::QItemJunction(const QItemJunction &other)
    : QItemBase(other), p1(other.p1) {}

QItemBasePtr QItemJunction::clone() const {
  return std::make_shared<QItemJunction>(*this);
}

void QItemJunction::parseItem() {
  StrUtils::StrList strList = StrUtils::tokenize(argStr);

  if (strList.size() != 1) {
    std::string str = "Unexpected content in " + typeStr + " " + argStr;
    throw std::invalid_argument(str);
  }

  p1 = ArgPoint(strList[0]);
}

std::string QItemJunction::toString() const {
  std::string str = typeStr + " " + p1.toString();
  return str;
}
