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
  ItemSch() : ItemBase(QPI::SCH) {}
  ItemSch(const ItemSch &other) = default;

  ItemBasePtr clone() const override;

  bool        parseItem(const std::string &argStr) override;
  std::string toString() const override;
};

typedef std::shared_ptr<ItemSch> ItemSchPtr;
