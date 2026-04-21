//-----------------------------------------------------------------------------
// This file is part of the the QParser2 project.  You can find the complete
// project here:  https://github.com/robdunn4/QSpice/
//-----------------------------------------------------------------------------
/*
 * ItemDesc.h -- Description item class.
 *
 * Expecting: "description:" P1
 * Example:   "description: Independent Voltage Source"
 *
 * Analysis:
 *   P1 -- String value.  Optional?  Non-quoted?
 *
 * Construction: use ItemDesc::makePtr(...).  Constructors are protected.
 */
#pragma once
#include "ItemBase.h"
#include "ArgUtils.h"

class ItemDesc;
typedef std::shared_ptr<ItemDesc> ItemDescPtr;

class ItemDesc : public ItemBaseT<ItemDesc> {
public:
  static ItemDescPtr makePtr() { return ItemDescPtr(new ItemDesc()); }
  static ItemDescPtr makePtr(const ArgString &text) {
    return ItemDescPtr(new ItemDesc(text));
  }

  bool        parseItem(const std::string &argStr) override;
  std::string toString() const override;

  ArgString text;

protected:
  ItemDesc() : ItemBaseT(QPI::DESC) {}
  ItemDesc(const ItemDesc &other) = default;
  ItemDesc(const ArgString &text) : ItemBaseT(QPI::DESC), text(text) {}

  friend class ItemBaseT<ItemDesc>;
};
