//-----------------------------------------------------------------------------
// This file is part of the the QParser2 project.  You can find the complete
// project here:  https://github.com/robdunn4/QSpice/
//-----------------------------------------------------------------------------
/*
 * ItemShort.h -- Shorted Pins item class.
 *
 * Expecting: "shorted pins:" P1
 * Example:   "shorted pins: false"
 *
 * Analysis:
 *   P1 --  boolean shorted pin state ("true" or "false")
 *
 * Construction: use ItemShort::makePtr(...).  Constructors are protected.
 */
#pragma once
#include "ItemBase.h"
#include "QArgUtils.h"

class ItemShort;
typedef std::shared_ptr<ItemShort> ItemShortPtr;

class ItemShort : public ItemBaseT<ItemShort> {
public:
  static ItemShortPtr makePtr() {
    return ItemShortPtr(new ItemShort());
  }
  static ItemShortPtr makePtr(const ArgBool &bShorted) {
    return ItemShortPtr(new ItemShort(bShorted));
  }

  bool        parseItem(const std::string &argStr) override;
  std::string toString() const override;

  ArgBool bShorted;

protected:
  ItemShort() : ItemBaseT(QPI::SHORTED) {}
  ItemShort(const ItemShort &other) = default;
  ItemShort(const ArgBool &bShorted)
      : ItemBaseT(QPI::SHORTED), bShorted(bShorted) {}

  friend class ItemBaseT<ItemShort>;
};
