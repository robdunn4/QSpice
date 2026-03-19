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
#include "QArgUtils2.h"
#include "ItemBase.h"

class ItemCmp : public ItemBase {
public:
  ItemCmp(std::string typeStr, std::string argStr);
  ItemCmp(const ItemCmp &other);
  ItemBasePtr clone() const override;

  void        parseItem() override;
  std::string toString() const override;

  ArgPoint pt1;
  ArgRot   rotate;
  ArgStuff stuffShort;
};

typedef std::shared_ptr<ItemCmp> ItemCmpPtr;

