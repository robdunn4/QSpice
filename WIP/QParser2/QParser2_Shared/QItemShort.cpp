//-----------------------------------------------------------------------------
// This file is part of the the QParser2 project.  You can find the complete
// project here:  https://github.com/robdunn4/QSpice/
//-----------------------------------------------------------------------------
#include "QItemShort.h"

QItemShort::QItemShort(std::string typeStr, std::string argStr)
    : QItemBase(typeStr, argStr) {}

QItemShort::QItemShort(const QItemShort &other)
    : QItemBase(other), bShorted(other.bShorted) {}

QItemBasePtr QItemShort::clone() const {
  return std::make_shared<QItemShort>(*this);
}

void QItemShort::parseItem() { bShorted = ArgBool(argStr); }

std::string QItemShort::toString() const {
  return typeStr + " " + bShorted.toString();
}
