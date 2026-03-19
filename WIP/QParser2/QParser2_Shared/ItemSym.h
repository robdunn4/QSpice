//-----------------------------------------------------------------------------
// This file is part of the the QParser2 project.  You can find the complete
// project here:  https://github.com/robdunn4/QSpice/
//-----------------------------------------------------------------------------
/*
 * ItemSym.h -- Symbol item class.
 *
 * Expecting: "symbol" P1
 * Example:   "symbol Vsin"
 *
 * Analysis:
 *   P1 -- Optional string.
 */
#pragma once
#include "QArgUtils.h"
#include "ItemBase.h"

class ItemSym : public ItemBase {
public:
  ItemSym(std::string typeStr, std::string argStr);
  ItemSym(const ItemSym &other);
  ItemBasePtr clone() const override;

  void        parseItem() override;
  std::string toString() const override;

  ArgString text;
};

typedef std::shared_ptr<ItemSym> ItemSymPtr;

