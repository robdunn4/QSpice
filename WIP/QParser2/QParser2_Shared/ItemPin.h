//-----------------------------------------------------------------------------
// This file is part of the the QParser2 project.  You can find the complete
// project here:  https://github.com/robdunn4/QSpice/
//-----------------------------------------------------------------------------
/*
 * ItemPin.h -- Pin item class.
 *
 * Expecting: "pin" (P1) (P2) P3 P4 P5 P6 P7 P8 P9
 * Example:   "pin (0,200) (0,0) 1 0 0 0x0 -1 "+""
 *            "pin (-200,0) (10,0) 0.681 7 0 0x1000000 -1 "EN" "net""
 *
 * Analysis:
 *   P1 -- Endpoint coordinate?
 *   P2 -- Offset between pin an pin label (per ME)
 *   P3 -- Font size
 *   P4 -- Text rotation/alignment
 *   P5 -- Pin data and type information
 *   P6 -- Text color
 *   P7 -- Lookup index?
 *   P8 -- Pin label text
 *   P9 -- Attached to net name -- for example, when pin is attached to a net in
 *         a symbol file?
 *
 * Note: Pin order is determined by order of occurrance in symbol.
 */
#pragma once
#include "ItemBase.h"
#include "QArgUtils2.h"

class ItemPin : public ItemBase {
public:
  ItemPin() : ItemBase(QPI::PIN) {}
  ItemPin(const ItemPin &other) = default;

  // Construct with explicit member values
  ItemPin(const ArgPoint &pt1, const ArgPoint &pt2, const ArgFontSize &fontSize,
          const ArgRotAlign &rotateAlign, const ArgPinInfo &pinInfo,
          const ArgColor &textColor, const ArgLookupNdx &lookupNdx,
          const ArgString &pinLabel, const ArgString &netName)
      : ItemBase(QPI::PIN), pt1(pt1), pt2(pt2), fontSize(fontSize),
        rotateAlign(rotateAlign), pinInfo(pinInfo), textColor(textColor),
        lookupNdx(lookupNdx), pinLabel(pinLabel), netName(netName) {}

  ItemBasePtr clone() const override;

  bool        parseItem(const std::string &argStr) override;
  std::string toString() const override;

  ArgPoint     pt1;
  ArgPoint     pt2;
  ArgFontSize  fontSize;
  ArgRotAlign  rotateAlign;
  ArgPinInfo   pinInfo;
  ArgColor     textColor;
  ArgLookupNdx lookupNdx;
  ArgString    pinLabel;
  ArgString    netName;
};

typedef std::shared_ptr<ItemPin> ItemPinPtr;
