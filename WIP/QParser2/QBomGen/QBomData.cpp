//-----------------------------------------------------------------------------
// This file is part of the the QParser2 project.  You can find the complete
// project here:  https://github.com/robdunn4/QSpice/
//-----------------------------------------------------------------------------
#include "QBomData.h"
#include "ItemBase.h"
#include "ItemDesc.h"
#include "ItemShort.h"
#include "ItemSym.h"
#include "ItemText.h"
#include "ItemType.h"
#include <StrUtils.h>
#include <algorithm>
#include <exception>
#include <tuple>

// symDataPtr must be valid SYM record upon entry; return true if required
// elements are present or false otherwise
//
// we expect the following children in the following order:
//   * ItemType
//   * ItemDesc (optional?)
//   * ItemLib (optional)
//   * ItemShort
//   * ItemText (first one = reference ID)
//   * ItemText (second one is component value)
//
bool QBomData::parseData(const QSchTreePtr symItem) {
  // that is, rather than testing for invalid pointers, just let C++ throw
  // exceptions for simplicity during development
  try {
    ItemSymPtr symPtr = std::dynamic_pointer_cast<ItemSym>(symItem->itemPtr);
    name               = symPtr->text;
    if (!name.length()) return false;

    // find type record (should be first child)
    QSchTreePtr childPtr = symItem->getFirstChild();
    if (childPtr->enumID != QPI::TYPE) return false;
    type = std::dynamic_pointer_cast<ItemType>(childPtr->itemPtr)->text;

    // find description record (optional)
    childPtr = childPtr->getNextSibling(QPI::DESC);
    // if (childPtr->enumID == QPI::DESC) {
    if (!childPtr) return false;
    desc = std::dynamic_pointer_cast<ItemDesc>(childPtr->itemPtr)->text;

    // find shorted record
    childPtr = childPtr->getNextSibling(QPI::SHORTED);
    if (!childPtr) return false;
    shorted = std::dynamic_pointer_cast<ItemShort>(childPtr->itemPtr)->bShorted
                  ? "true"
                  : "false";

    // find first text record
    childPtr = childPtr->getNextSibling(QPI::TEXT);
    if (!childPtr) return false;
    refID = std::dynamic_pointer_cast<ItemText>(childPtr->itemPtr)->text;

    // find second text record
    childPtr = childPtr->getNextSibling();
    if (childPtr->enumID != QPI::TEXT) return false;
    value = std::dynamic_pointer_cast<ItemText>(childPtr->itemPtr)->text;
  } catch (std::exception e) {
    return false;
  }
  return true;
}

void QBomData::writeHeader(std::ostream &ostrm) {
  const std::string hdr =
      R"("Name","Type","Shorted","Description","Ref ID","Value")";
  ostrm << hdr << std::endl;
}

void QBomData::writeData(std::ostream &ostrm) const {
  const std::string dlm = ",";
  ostrm << StrUtils::forceQuotes(name) << dlm << StrUtils::forceQuotes(type)
        << dlm << StrUtils::forceQuotes(shorted) << dlm
        << StrUtils::forceQuotes(desc) << dlm << StrUtils::forceQuotes(refID)
        << dlm << StrUtils::forceQuotes(value) << std::endl;
}

// Lexicographic sort: type, then name, then refID.
// std::tuple::operator< does this automatically.
// To add another sort key, just append it to the tuple.
void QBomList::sort1() {
  std::sort(begin(), end(),
            [](const QBomData &lhs, const QBomData &rhs) -> bool {
              return std::make_tuple(lhs.type, lhs.name, lhs.refID) <
                     std::make_tuple(rhs.type, rhs.name, rhs.refID);
            });
}
