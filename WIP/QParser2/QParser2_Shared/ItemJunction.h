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
 */
#pragma once
#include "ItemBase.h"
#include "QArgUtils.h"

class ItemJunction : public ItemBase {
public:
  ItemJunction() : ItemBase(QPI::JUNCTION) {}
  ItemJunction(const ItemJunction &other) = default;
  ItemJunction(const ArgPoint &pt1) : ItemBase(QPI::JUNCTION), pt1(pt1) {}

  ItemBasePtr clone() const override;

  bool        parseItem(const std::string &argStr) override;
  std::string toString() const override;

  ArgPoint pt1;
};

typedef std::shared_ptr<ItemJunction> ItemJunctionPtr;
