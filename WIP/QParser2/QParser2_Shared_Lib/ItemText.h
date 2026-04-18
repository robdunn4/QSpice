//-----------------------------------------------------------------------------
// This file is part of the the QParser2 project.  You can find the complete
// project here:  https://github.com/robdunn4/QSpice/
//-----------------------------------------------------------------------------
/*
 * ItemText.h -- Text/Comment item class.
 *
 * Expecting: "text" (P1) P2 P3 P4 P5 P6 P7 P8
 * Example:   "text (100,150) 1 7 0 0x1000000 -1 -1 "V1""
 *
 * Analysis:
 *   P1 -- Anchor coordinate
 *   P2 -- Float - Font Size (5 4 3 2.5 2 1.6 1.25 1 0.8 0.65)
 *   P3 -- Text alignment/rotation
 *   P4 -- Bit 0: clear = text, set = comment
 *         Bit 1: clear = visible, set = hidden
 *   P5 -- RGB color
 *   P6 -- A lookup index (per ME)
 *   P7 -- A pin index (per ME)
 *   P8 -- String value.  Note special characters.  Overbars are embedded
 *         special characters.
 *
 * Construction: use ItemText::makePtr(...).  Constructors are protected.
 */
#pragma once
#include "ItemBase.h"
#include "QArgUtils2.h"

class ItemText;
typedef std::shared_ptr<ItemText> ItemTextPtr;

class ItemText : public ItemBaseT<ItemText> {
public:
  static ItemTextPtr makePtr() { return ItemTextPtr(new ItemText()); }
  static ItemTextPtr makePtr(const ArgPoint &pt1, const ArgFontSize &fontSize,
                             const ArgRotAlign  &rotateAlign,
                             const ArgTextFlags &textFlags,
                             const ArgColor     &textColor,
                             const ArgLookupNdx &lookupNdx,
                             const ArgPinNdx &pinNdx, const ArgString &text) {
    return ItemTextPtr(new ItemText(pt1, fontSize, rotateAlign, textFlags,
                                    textColor, lookupNdx, pinNdx, text));
  }

  bool        parseItem(const std::string &argStr) override;
  std::string toString() const override;

  ArgPoint     pt1;
  ArgFontSize  fontSize;
  ArgRotAlign  rotateAlign;
  ArgTextFlags textFlags;
  ArgColor     textColor;
  ArgLookupNdx lookupNdx;
  ArgPinNdx    pinNdx;
  ArgString    text;

protected:
  ItemText() : ItemBaseT(QPI::TEXT) {}
  ItemText(const ItemText &other) = default;
  ItemText(const ArgPoint &pt1, const ArgFontSize &fontSize,
           const ArgRotAlign &rotateAlign, const ArgTextFlags &textFlags,
           const ArgColor &textColor, const ArgLookupNdx &lookupNdx,
           const ArgPinNdx &pinNdx, const ArgString &text)
      : ItemBaseT(QPI::TEXT), pt1(pt1), fontSize(fontSize),
        rotateAlign(rotateAlign), textFlags(textFlags), textColor(textColor),
        lookupNdx(lookupNdx), pinNdx(pinNdx), text(text.quote()) {}

  friend class ItemBaseT<ItemText>;
};
