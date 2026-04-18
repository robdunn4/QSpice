//-----------------------------------------------------------------------------
// This file is part of the the QParser2 project.  You can find the complete
// project here:  https://github.com/robdunn4/QSpice/
//-----------------------------------------------------------------------------
#include "QBomData.h"
#include <ItemBase.h>
#include <ItemDesc.h>
#include <ItemShort.h>
#include <ItemText.h>
#include <ItemTree.h>
#include <ItemType.h>
#include <StrUtils.h>
#include <algorithm>
#include <exception>
#include <tuple>

// symNode must be valid SYM record upon entry; return true if required
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
// for simplicity, we use try/catch to handle any unexpected conditions (missing
// records, wrong types, etc.) -- if any of those occur, we return false to
// indicate this record should be skipped.  We could be more specific about
// error handling, but this is probably good enough for now.

bool QBomData::parseData(const NodePtr &symNode) {
  // get iterator for child nodes of the symbol node
  NodeList::const_iterator iter = symNode->children().begin();

  try {
    // first should be ItemType (not optional)
    ItemTypePtr typePtr = std::dynamic_pointer_cast<ItemType>((*iter)->item());
    type                = typePtr->text;

    // next should be ItemDesc -- not present for DLL blocks so we need to be
    // able to handle missing description records.  may decide later to omit DLL
    // blocks from the BOM...
    iter++;
    ItemBasePtr basePtr = (*iter)->item();
    if (basePtr->getEnumID() == QPI::DESC) {
      ItemDescPtr descPtr =
          std::dynamic_pointer_cast<ItemDesc>((*iter)->item());
      desc = descPtr->text.getValue();
      // advance to next item
      iter++;
    } else desc = "[None]";

    // next should be ItemShorted (not optional)
    ItemShortPtr shortPtr =
        std::dynamic_pointer_cast<ItemShort>((*iter)->item());
    shorted = shortPtr->bShorted ? "true" : "false"; // maybe change to bool?

    // skip until we find first ItemText (reference ID)
    while (iter != symNode->children().end() &&
           (*iter)->getEnumID() != QPI::TEXT) {
      iter++;
    }

    // if eol, no text record found -- skip this symbol
    if (iter == symNode->children().end()) return false;

    // first text record should be reference ID
    refID =
        std::dynamic_pointer_cast<ItemText>((*iter)->item())->text.getValue();

    // second text record should be value
    iter++;
    if (iter == symNode->children().end() || (*iter)->getEnumID() != QPI::TEXT)
      return false;
    value =
        std::dynamic_pointer_cast<ItemText>((*iter)->item())->text.getValue();
  } catch (std::exception e) {
    // for now, just suppress errors...
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
