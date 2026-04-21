//-----------------------------------------------------------------------------
// This file is part of the the QParser2 project.  You can find the complete
// project here:  https://github.com/robdunn4/QSpice/
//-----------------------------------------------------------------------------
/*
 * ItemLib.h -- Library file item class.
 *
 * Expecting: "library file:" P1
 * Example:   "library file: xxx"
 *
 * Analysis:
 *   P1 --  String value.  Optional?  Non-quoted?
 *
 * Construction: use ItemLib::makePtr(...).  Constructors are protected.
 */
#pragma once
#include "ItemBase.h"
#include "ArgUtils.h"
#include <memory>

class ItemLib;
typedef std::shared_ptr<ItemLib> ItemLibPtr;

class ItemLib : public ItemBaseT<ItemLib> {
public:
  static ItemLibPtr makePtr() { return ItemLibPtr(new ItemLib()); }
  static ItemLibPtr makePtr(const ArgString &libText) {
    return ItemLibPtr(new ItemLib(libText));
  }

  bool        parseItem(const std::string &argStr) override;
  std::string toString() const override;

  ArgString libText;

protected:
  ItemLib() : ItemBaseT(QPI::LIB) {}
  ItemLib(const ItemLib &other) = default;
  ItemLib(const ArgString &libText)
      : ItemBaseT(QPI::LIB), libText(libText) {}

  friend class ItemBaseT<ItemLib>;
};
