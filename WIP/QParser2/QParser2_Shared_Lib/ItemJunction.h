//-----------------------------------------------------------------------------
// This file is part of the the QParser2 project.  You can find the complete
// project here:  https://github.com/robdunn4/QSpice/
//-----------------------------------------------------------------------------
/*
 * ItemJunction.h -- Junction item class.
 *
 * Expecting: "junction" (P1)
 * Example:   "junction (1500,0)"
 *
 * Analysis:
 *   P1 -- Endpoint coordinate
 *
 * Construction: use ItemJunction::makePtr(...).  Constructors are protected.
 */
#pragma once
#include "ItemBase.h"
#include "ArgUtils.h"

class ItemJunction;
typedef std::shared_ptr<ItemJunction> ItemJunctionPtr;

class ItemJunction : public ItemBaseT<ItemJunction> {
public:
  static ItemJunctionPtr makePtr() {
    return ItemJunctionPtr(new ItemJunction());
  }
  static ItemJunctionPtr makePtr(const ArgPoint &pt1) {
    return ItemJunctionPtr(new ItemJunction(pt1));
  }

  bool        parseItem(const std::string &argStr) override;
  std::string toString() const override;

  ArgPoint pt1;

protected:
  ItemJunction() : ItemBaseT(QPI::JUNCTION) {}
  ItemJunction(const ItemJunction &other) = default;
  ItemJunction(const ArgPoint &pt1) : ItemBaseT(QPI::JUNCTION), pt1(pt1) {}

  friend class ItemBaseT<ItemJunction>;
};
