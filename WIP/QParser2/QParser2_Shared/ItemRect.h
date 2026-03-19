//-----------------------------------------------------------------------------
// This file is part of the the QParser2 project.  You can find the complete
// project here:  https://github.com/robdunn4/QSpice/
//-----------------------------------------------------------------------------
// clang-format off
/*
 * ItemRect.h -- Rectangle item class.
 *
 * Expecting: "rect" (P1) (P2) P3 P4 P5 P6 P7 P8 P9 P10
 * Example:   "rect (-25,77) (25,73) 0 0 0 0x1000000 0x3000000 -1 0 -1"
 *
 * With graphic: "rect" (P1) (P2) P3 P4 P5 P6 P7 P8 P9 P10 P11
 * Example:      "rect (3018,68) (3082,132) 0 0 2 0xff0000 0x5000000 -1 0 -1
 *                89504e470d0a1a0a...0000014f0000"
 *
 * Analysis:
 *   P1 -- Endpoint coordinate
 *   P2 -- Endpoint coordinate
 *   P3 -- Rotation
 *   P4 -- Line width
 *   P5 -- Line type
 *   P6 -- Line color
 *   P7 -- Fill color
 *   P8 -- Lookup index
 *   P9 -- Is 0 normally; 1 if hierarchical block???
 *   P10 -- Pin index
 *   P11 -- Appears to be used only for a graphic image, a string of hex values
 *          with no embedded spaces/newlines.  Not included if no graphic.  Given 
 *          that this may be a very long block of text without embedded spaces, it
 *          overrun default buffers....
 * 
 * Some testing:
 *   Default box:            rect (-1550,-150) (-1000,-700) 0 0 2 0x4000000 0x1000000 -1 0 -1
 *   Line Type Change:       rect (-1550,-150) (-1000,-700) 0 0 4 0x4000000 0x1000000 -1 0 -1
 *   Line Width Change:      rect (-1550,-150) (-1000,-700) 0 7 4 0x4000000 0x1000000 -1 0 -1
 *   Line Color Change:      rect (-1550,-150) (-1000,-700) 0 7 4 0xff 0x1000000 -1 0 -1
 *   Fill Color Change1:     rect (-1550,-150) (-1000,-700) 0 7 4 0xff 0x2e9ffff -1 0 -1
 *   Fill Color Change2:     rect (-1550,-150) (-1000,-700) 0 7 4 0xff 0x3ff0000 -1 0 -1
 *   Fill Color Change3:     rect (-1550,-150) (-1000,-700) 0 7 4 0xff 0x400ffff -1 0 -1
 *   Fill Color Change4:     rect (-1550,-150) (-1000,-700) 0 7 4 0xff 0xffff -1 0 -1
 *   In hierarchical block:  rect (-500,400) (500,-400) 0 0 0 0x4000000 0x4000000 -1 1 -1
 * 
 * 
 * Fill        :  Default color    -- 0x01 + cad background color from registry
 * Fill Change1:  Background color -- 0x02 + cad background color from registry
 * Fill Change2:  Foreground color -- 0x03 + one of several possible cad colors from registry
 * Fill Change3:  Solid fill color -- 0x04 + ???
 * Fill Change4:  Custom color     -- 0x00 + custom color
 * Image type:    Use background image  -- 0x05
 * 
 * New Image Info:
 *   SRCCOPY:     rect (-97,-6844)  (97,-6656)   0 0 2 0x4000000 0x5000000 -1 0 -1 [Image Data] 
 *   SRCPAINT:    rect (-347,-7144) (-153,-6956) 0 0 2 0x4000000 0x6000000 -1 0 -1 [Image Data]
 *   SRCAND:      rect (-297,-7594) (-103,-7406) 0 0 2 0x4000000 0x7000000 -1 0 -1 [Image Data]
 *   SRCINVERT:   rect (203,-8094)  (397,-7906)  0 0 2 0x4000000 0x8000000 -1 0 -1 [Image Data]
 *   SRCERASE:    rect (-97,-8294)  (97,-8106)   0 0 2 0x4000000 0x9000000 -1 0 -1 [Image Data]
 *   NOTSRCERASE: rect (-147,-8794) (47,-8606)   0 0 2 0x4000000 0xa000000 -1 0 -1 [Image Data]
 *   MERGEPAINT:  rect (53,-9044)   (247,-8856)  0 0 2 0x4000000 0xb000000 -1 0 -1 [Image Data]
 * 
 * OK, fillColor/P7 high bits are used for image BLT mode....
 * 
 * 
 */
// clang-format on
#pragma once
#include "QArgUtils2.h"
#include "ItemBase.h"

class ItemRect : public ItemBase {
public:
  ItemRect(std::string typeStr, std::string argStr);
  ItemRect(const ItemRect &other);
  ItemBasePtr clone() const override;

  void        parseItem() override;
  std::string toString() const override;

  ArgPoint     pt1;
  ArgPoint     pt2;
  ArgRot       rotate;
  ArgLineWidth lineWidth;
  ArgLineType  lineType;
  ArgColor     lineColor;
  ArgColor     fillColor;
  ArgLookupNdx lookupNdx;
  ArgInt       isHierarchicalBlock;
  ArgPinNdx    pinNdx;
  ArgImage     imageData; // image bitmap
};

typedef std::shared_ptr<ItemRect> ItemRectPtr;

