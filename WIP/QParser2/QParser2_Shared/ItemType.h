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
#include "ItemBase.h"
#include "QArgUtils.h"

class ItemType : public ItemBase {
public:
  ItemType() : ItemBase(QPI::TYPE) {}
  ItemType(const ItemType &other) = default;
  ItemType(const ArgString &text) : ItemBase(QPI::TYPE), text(text) {}

  ItemBasePtr clone() const override;

  bool        parseItem(const std::string &argStr) override;
  std::string toString() const override;

  ArgString text;
};

typedef std::shared_ptr<ItemType> ItemTypePtr;
