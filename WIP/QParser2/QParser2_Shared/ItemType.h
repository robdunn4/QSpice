//-----------------------------------------------------------------------------
// This file is part of the the QParser2 project.  You can find the complete
// project here:  https://github.com/robdunn4/QSpice/
//-----------------------------------------------------------------------------
/*
 * ItemType.h -- Type item class.
 *
 * Expecting: "type:" P1
 * Example:   "type: V"
 *
 * Analysis:
 *   P1 --  Type string.  Optional?  Unquoted?
 */
#pragma once
#include "QArgUtils.h"
#include "ItemBase.h"

class ItemType : public ItemBase {
public:
  ItemType(std::string typeStr, std::string argStr);
  ItemType(const ItemType &other);
  ItemBasePtr clone() const override;

  void        parseItem() override;
  std::string toString() const override;

  ArgString text;
};

typedef std::shared_ptr<ItemType> ItemTypePtr;
