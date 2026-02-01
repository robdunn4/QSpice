#include "QItemSch.h"

QItemSch::QItemSch(std::string typeStr, std::string argStr)
    : QItemBase(typeStr, argStr) {}

QItemSch::QItemSch(const QItemSch &other) : QItemBase(other) {}

QItemBasePtr QItemSch::clone() const {
  return std::make_shared<QItemSch>(*this);
}

void QItemSch::parseItem() {
  if (!argStr.empty()) {
    std::string str = "Unexpected content in " + typeStr + " " + argStr;
    throw std::invalid_argument(str);
  }

  // nothing more to do?
}

std::string QItemSch::toString() const { return typeStr; }
