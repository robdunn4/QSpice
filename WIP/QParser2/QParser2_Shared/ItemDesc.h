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
 */
#pragma once
#include "ItemBase.h"
#include "QArgUtils.h"

class ItemDesc : public ItemBase {
public:
  ItemDesc() : ItemBase(QPI::DESC) {}
  ItemDesc(const ItemDesc &other) = default;
  ItemDesc(const ArgString &text) : ItemBase(QPI::DESC), text(text) {}

  ItemBasePtr clone() const override;

  bool        parseItem(const std::string &argStr) override;
  std::string toString() const override;

  ArgString text;
};

typedef std::shared_ptr<ItemDesc> ItemDescPtr;
