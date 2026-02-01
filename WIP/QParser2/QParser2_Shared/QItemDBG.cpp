#include "QItemDBG.h"
#include "iostream"

// this is a dummy class to use during development -- it does no actual parsing
// and throws no execptions

QItemDbg::QItemDbg(std::string typeStr, std::string argStr)
    : QItemBase(typeStr, argStr) {}

QItemDbg::QItemDbg(const QItemDbg &other) : QItemBase(other) {}

QItemBasePtr QItemDbg::clone() const {
  return std::make_shared<QItemDbg>(*this);
}

void QItemDbg::parseItem() {
  // show error message for debugging
  std::cout << "*** Unhandled record type in QItemDBG: " << toString()
            << std::endl;
}

std::string QItemDbg::toString() const {
  std::string str = typeStr;
  if (argStr.length()) str += " " + argStr;
  return str;
}
