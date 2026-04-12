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
 */
#pragma once
#include "ItemBase.h"
#include "QArgUtils.h"

class ItemShort : public ItemBase {
public:
  ItemShort() : ItemBase(QPI::SHORTED) {}
  ItemShort(const ItemShort &other) = default;

  ItemShort(const ArgBool &bShorted)
      : ItemBase(QPI::SHORTED), bShorted(bShorted) {}

  ItemBasePtr clone() const override;

  bool        parseItem(const std::string &argStr) override;
  std::string toString() const override;

  ArgBool bShorted;
};

typedef std::shared_ptr<ItemShort> ItemShortPtr;
