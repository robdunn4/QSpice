//-----------------------------------------------------------------------------
// This file is part of the the QParser2 project.  You can find the complete
// project here:  https://github.com/robdunn4/QSpice/
//-----------------------------------------------------------------------------
/*
 * ItemNet.h -- Net item class.
 *
 * Expecting: "net" (P1) P2 P3 P4 P5 P6
 * Example:   "net (-1700,-800) 1 13 0 "GND" "Description""
 *
 * Analysis:
 *   P1 -- Endpoint coordinate
 *   P2 -- Font size (different values from text sizes -- see GUI menu)
 *   P3 -- Alignment only; rotation appears to be disallowed in GUI
 *   P4 -- Net type (net = 0, port = 1)
 *   P5 -- Net name
 *   P6 -- Descriptive text (optional)
 */
#pragma once
#include "QArgUtils2.h"
#include "ItemBase.h"

class ItemNet : public ItemBase {
public:
  ItemNet(std::string typeStr, std::string argStr);
  ItemNet(const ItemNet &other);
  ItemBasePtr clone() const override;

  void        parseItem() override;
  std::string toString() const override;

  ArgPoint    pt1;
  ArgFontSize fontSize;
  ArgRotAlign rotateAlign;
  ArgInt      netType;
  ArgString   netName;
  ArgString   netDesc; // optional description
};

typedef std::shared_ptr<ItemNet> ItemNetPtr;

