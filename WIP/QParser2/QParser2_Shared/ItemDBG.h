//-----------------------------------------------------------------------------
// This file is part of the the QParser2 project.  You can find the complete
// project here:  https://github.com/robdunn4/QSpice/
//-----------------------------------------------------------------------------
/*
 * ItemDBG.h -- Dummy class for debugging.  Simply uses the line text.
 */
#pragma once
#include "ItemBase.h"

class ItemDbg : public ItemBase {
public:
  ItemDbg(std::string typeStr, std::string argStr);
  ItemDbg(const ItemDbg &other);
  ItemBasePtr clone() const override;

  void        parseItem() override;
  std::string toString() const override;
};

typedef std::shared_ptr<ItemDbg> ItemDbgPtr;

