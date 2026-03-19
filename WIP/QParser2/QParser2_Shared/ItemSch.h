//-----------------------------------------------------------------------------
// This file is part of the the QParser2 project.  You can find the complete
// project here:  https://github.com/robdunn4/QSpice/
//-----------------------------------------------------------------------------
/*
 * ItemSch.h -- Schematic item class.
 *
 * Expecting: "schematic"
 *
 * Analysis:
 *   No parameters?
 */
#pragma once
#include "ItemBase.h"

class ItemSch : public ItemBase {
public:
  ItemSch(std::string typeStr, std::string argStr);
  ItemSch(const ItemSch &other);
  ItemBasePtr clone() const override;

  void        parseItem() override;
  std::string toString() const override;
};

typedef std::shared_ptr<ItemSch> ItemSchPtr;

