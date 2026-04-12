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
#include "ItemBase.h"
#include "QArgUtils.h"

class ItemSym : public ItemBase {
public:
  ItemSym() : ItemBase(QPI::SYM) {}
  ItemSym(const ItemSym &other) = default;

  // Construct with explicit member values
  ItemSym(const ArgString &text) : ItemBase(QPI::SYM), text(text) {}

  ItemBasePtr clone() const override;

  bool        parseItem(const std::string &argStr) override;
  std::string toString() const override;

  ArgString text;
};

typedef std::shared_ptr<ItemSym> ItemSymPtr;
