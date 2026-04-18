//-----------------------------------------------------------------------------
// This file is part of the the QParser2 project.  You can find the complete
// project here:  https://github.com/robdunn4/QSpice/
//-----------------------------------------------------------------------------
/*
 * ItemSym.h -- Symbol item class.
 *
 * Expecting: "symbol" P1
 * Example:   "symbol Vsin"
 *
 * Analysis:
 *   P1 -- Optional string.
 *
 * Construction: use ItemSym::makePtr(...).  Constructors are protected.
 * For a typed deep copy of an existing instance, use clonePtr() (inherited
 * from ItemBaseT<ItemSym>).
 */
#pragma once
#include "ItemBase.h"
#include "QArgUtils.h"

class ItemSym;
typedef std::shared_ptr<ItemSym> ItemSymPtr;

class ItemSym : public ItemBaseT<ItemSym> {
public:
  static ItemSymPtr makePtr() {
    return ItemSymPtr(new ItemSym());
  }
  static ItemSymPtr makePtr(const ArgString &text) {
    return ItemSymPtr(new ItemSym(text));
  }

  bool        parseItem(const std::string &argStr) override;
  std::string toString() const override;

  ArgString text;

protected:
  ItemSym() : ItemBaseT(QPI::SYM) {}
  ItemSym(const ItemSym &other) = default;
  ItemSym(const ArgString &text) : ItemBaseT(QPI::SYM), text(text) {}

  friend class ItemBaseT<ItemSym>;
};
