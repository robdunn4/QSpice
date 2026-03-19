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
#include "QArgUtils.h"
#include "ItemBase.h"

class ItemDesc : public ItemBase {
public:
  ItemDesc(std::string typeStr, std::string argStr);
  ItemDesc(const ItemDesc &other);
  ItemBasePtr clone() const override;

  void        parseItem() override;
  std::string toString() const override;

  ArgString text;
};

typedef std::shared_ptr<ItemDesc> ItemDescPtr;

