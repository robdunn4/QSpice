//-----------------------------------------------------------------------------
// This file is part of the the QParser2 project.  You can find the complete
// project here:  https://github.com/robdunn4/QSpice/
//-----------------------------------------------------------------------------
/*
 * ItemCmp.h -- Component item class.
 *
 * Expecting: "component" (P1) P2 P3
 * Example:   "component (-1700,-500) 0 0"
 *
 * Analysis:
 *   P1 -- Endpoint coordinate
 *   P2 -- Rotation
 *   P3 -- Stuff/short
 */
#pragma once
#include "ItemBase.h"
#include "QArgUtils2.h"

class ItemCmp : public ItemBase {
public:
  ItemCmp() : ItemBase(QPI::COMP) {}
  ItemCmp(const ItemCmp &other) = default;

  // Construct with explicit member values
  ItemCmp(const ArgPoint &pt1, const ArgRot &rotate, const ArgStuff &stuff)
      : ItemBase(QPI::COMP), pt1(pt1), rotate(rotate), stuffShort(stuff) {}

  ItemBasePtr clone() const override;

  bool        parseItem(const std::string &argStr) override;
  std::string toString() const override;

  ArgPoint pt1;
  ArgRot   rotate;
  ArgStuff stuffShort;
};

typedef std::shared_ptr<ItemCmp> ItemCmpPtr;
