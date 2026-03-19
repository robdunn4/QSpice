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
#include "QArgUtils.h"
#include "ItemBase.h"

class ItemShort : public ItemBase {
public:
  ItemShort(std::string typeStr, std::string argStr);
  ItemShort(const ItemShort &other);
  ItemBasePtr clone() const override;

  void        parseItem() override;
  std::string toString() const override;

  ArgBool bShorted;
};

typedef std::shared_ptr<ItemShort> ItemShortPtr;

