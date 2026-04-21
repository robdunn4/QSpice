//-----------------------------------------------------------------------------
// This file is part of the the QParser2 project.  You can find the complete
// project here:  https://github.com/robdunn4/QSpice/
//-----------------------------------------------------------------------------
/*
 * ItemTap.h -- Bus tap item class.
 *
 * Expecting: "tap" (P1) (P2)
 * Example:   "tap (-1700,-300) (-1700,0)
 *
 * Analysis:
 *   P1 -- Endpoint coordinate
 *   P2 -- Endpoint coordinate
 *
 * Construction: use ItemTap::makePtr(...).  Constructors are protected.
 */
#pragma once
#include "ItemBase.h"
#include "ArgUtils.h"

class ItemTap;
typedef std::shared_ptr<ItemTap> ItemTapPtr;

class ItemTap : public ItemBaseT<ItemTap> {
public:
  static ItemTapPtr makePtr() {
    return ItemTapPtr(new ItemTap());
  }
  static ItemTapPtr makePtr(const ArgPoint &pt1, const ArgPoint &pt2) {
    return ItemTapPtr(new ItemTap(pt1, pt2));
  }

  bool        parseItem(const std::string &argStr) override;
  std::string toString() const override;

  ArgPoint pt1;
  ArgPoint pt2;

protected:
  ItemTap() : ItemBaseT(QPI::TAP) {}
  ItemTap(const ItemTap &other) = default;
  ItemTap(const ArgPoint &pt1, const ArgPoint &pt2)
      : ItemBaseT(QPI::TAP), pt1(pt1), pt2(pt2) {}

  friend class ItemBaseT<ItemTap>;
};
