//-----------------------------------------------------------------------------
// This file is part of the the QParser2 project.  You can find the complete
// project here:  https://github.com/robdunn4/QSpice/
//-----------------------------------------------------------------------------
/*
 * ItemLib.h -- Library file item class.
 *
 * Expecting: "library file:" P1
 * Example:   "library file: xxx" (xxx is the subcircuit file name)
 *            "library file: |xxx" (xxx is the subcircuit text with newlines
 *            converted to "\n")
 *
 * Analysis:
 *   P1 --  String value.  Optional?  Non-quoted?
 *
 * Construction: use ItemLib::makePtr(...).  Constructors are protected.
 */
#pragma once
#include "ArgUtils2.h"
#include "ItemBase.h"
#include <memory>
#include <string>

class ItemLib;
typedef std::shared_ptr<ItemLib> ItemLibPtr;

class ItemLib : public ItemBaseT<ItemLib> {
public:
  static ItemLibPtr makePtr() { return ItemLibPtr(new ItemLib()); }
  static ItemLibPtr makePtr(const ArgLibString &libText) {
    return ItemLibPtr(new ItemLib(libText));
  }

  bool        parseItem(const std::string &argStr) override;
  std::string toString() const override;

  ArgLibString libText;

protected:
  ItemLib() : ItemBaseT(QPI::LIB) {}
  ItemLib(const ItemLib &other) = default;
  ItemLib(const ArgLibString &libText)
      : ItemBaseT(QPI::LIB), libText(libText) {}

  friend class ItemBaseT<ItemLib>;
};
