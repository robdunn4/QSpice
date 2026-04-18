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
 *
 * Construction: use ItemSch::makePtr().  Constructors are protected.
 */
#pragma once
#include "ItemBase.h"

class ItemSch;
typedef std::shared_ptr<ItemSch> ItemSchPtr;

class ItemSch : public ItemBaseT<ItemSch> {
public:
  static ItemSchPtr makePtr() {
    return ItemSchPtr(new ItemSch());
  }

  bool        parseItem(const std::string &argStr) override;
  std::string toString() const override;

protected:
  ItemSch() : ItemBaseT(QPI::SCH) {}
  ItemSch(const ItemSch &other) = default;

  friend class ItemBaseT<ItemSch>;
};
