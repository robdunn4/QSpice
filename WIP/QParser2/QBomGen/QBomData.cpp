#include "QBomData.h"
#include "QItemBase.h"
#include "QItemDesc.h"
#include "QItemShort.h"
#include "QItemSym.h"
#include "QItemText.h"
#include "QItemType.h"
#include <StrUtils.h>
#include <algorithm>
#include <exception>
#include <tuple>

// symDataPtr must be valid SYM record upon entry; return true if required
// elements are present or false otherwise
//
// we expect the following children in the following order:
//   * QItemType
//   * QItemDesc
//   * QItemShort
//   * QItemText (first one = reference ID)
//   * QItemText (second one is component value)
//
bool QBomData::parseData(const QSchTreePtr symItem) {
  // for simplicity, let's just throw/catch exceptions...
  // that is, rather than testing for invalid pointers, just let C++ throw
  // exceptions
  try {
    QItemSymPtr symPtr = std::dynamic_pointer_cast<QItemSym>(symItem->itemPtr);
    name = symPtr->p1Str;
    if (!name.length()) return false;

    // find type record
    QSchTreePtr childPtr = symItem->getFirstChild();
    if (childPtr->enumID != QPI::TYPE) return false;
    type = std::dynamic_pointer_cast<QItemType>(childPtr->itemPtr)->p1Str;

    // find description record (optional)
    childPtr = childPtr->getNextSibling();
    if (childPtr->enumID == QPI::DESC) {
      desc = std::dynamic_pointer_cast<QItemDesc>(childPtr->itemPtr)->p1Str;
      childPtr = childPtr->getNextSibling();
    }

    // find shorted record
    if (childPtr->enumID != QPI::SHORTED) return false;
    shorted =
        std::dynamic_pointer_cast<QItemShort>(childPtr->itemPtr)->bShorted;

    // find first text record
    childPtr = childPtr->getNextSibling();
    while (childPtr->enumID != QPI::TEXT)
      childPtr = childPtr->getNextSibling();
    refID = std::dynamic_pointer_cast<QItemText>(childPtr->itemPtr)->p8;

    // find second text record
    childPtr = childPtr->getNextSibling();
    if (childPtr->enumID != QPI::TEXT) return false;
    value = std::dynamic_pointer_cast<QItemText>(childPtr->itemPtr)->p8;
  } catch (std::exception e) {
    return false;
  }
  return true;
}

void QBomData::writeHeader(std::ostream &ostrm) {
  const std::string hdr =
      R"("Name","Type","Shorted","Description","Ref ID","Value")"
      "\n";
  ostrm << hdr;
}

void QBomData::writeData(std::ostream &ostrm) const {
  const std::string dlm = ",";
  ostrm << StrUtils::forceQuotes(name) << dlm << StrUtils::forceQuotes(type)
        << dlm << shorted << dlm << StrUtils::forceQuotes(desc) << dlm
        << StrUtils::forceQuotes(refID) << dlm << StrUtils::forceQuotes(value)
        << std::endl;
}

// Lexicographic sort: type, then name, then refID.
// std::tuple::operator< does this automatically.
// To add another sort key, just append it to the tuple.
bool QBomList::compare1(const QBomData &lhs, const QBomData &rhs) {
  return std::make_tuple(lhs.type, lhs.name, lhs.refID)
       < std::make_tuple(rhs.type, rhs.name, rhs.refID);
}

void QBomList::sort1() { std::sort(begin(), end(), compare1); }
