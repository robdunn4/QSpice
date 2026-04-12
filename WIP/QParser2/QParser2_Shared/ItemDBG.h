//-----------------------------------------------------------------------------
// This file is part of the the QParser2 project.  You can find the complete
// project here:  https://github.com/robdunn4/QSpice/
//-----------------------------------------------------------------------------
/*
 * ItemDBG.h -- Dummy class for debugging.  Simply uses the line text.
 *
 * Unlike other item classes, this one retains the raw argStr so that the
 * diagnostic message in parseItem() can show what was not parsed.
 */
#pragma once
#include "ItemBase.h"

class ItemDbg : public ItemBase {
public:
  ItemDbg() : ItemBase(QPI::UNKNOWN) {} // dbg has no real QPI
  ItemDbg(const ItemDbg &other) = default;
  ItemBasePtr clone() const override;

  bool        parseItem(const std::string &argStr) override;
  std::string toString() const override;

private:
  std::string argStr; // retained for diagnostic output
};

typedef std::shared_ptr<ItemDbg> ItemDbgPtr;
