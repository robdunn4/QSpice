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
 */
#pragma once
#include "ItemBase.h"
#include "QArgUtils.h"

class ItemTap : public ItemBase {
public:
  ItemTap() : ItemBase(QPI::TAP) {}
  ItemTap(const ItemTap &other) = default;
  ItemTap(const ArgPoint &pt1, const ArgPoint &pt2)
      : ItemBase(QPI::TAP), pt1(pt1), pt2(pt2) {}

  ItemBasePtr clone() const override;

  bool        parseItem(const std::string &argStr) override;
  std::string toString() const override;

  ArgPoint pt1;
  ArgPoint pt2;
};

typedef std::shared_ptr<ItemTap> ItemTapPtr;
