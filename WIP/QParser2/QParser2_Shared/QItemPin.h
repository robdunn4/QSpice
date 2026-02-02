//-----------------------------------------------------------------------------
// This file is part of the the QParser2 project.  You can find the complete
// project here:  https://github.com/robdunn4/QSpice/
//-----------------------------------------------------------------------------
/*
 * QItemPin.h -- Pin item class.
 *
 * Expecting: "pin" (P1) (P2) P3 P4 P5 P6 P7 P8 P9
 * Example:   "pin (0,200) (0,0) 1 0 0 0x0 -1 "+""
 *            "pin (-200,0) (10,0) 0.681 7 0 0x1000000 -1 "EN" "¥""
 *
 * Analysis:
 *   P1 -- Endpoint coordinate?
 *   P2 -- Endpoint coordinate?
 *   P3 -- Font size
 *   P4 -- Text rotation/alignment
 *   P5 -- Pin data and type information
 *   P6 -- Text color
 *   P7 -- ?
 *   P8 -- Pin label text
 *   P9 -- Attached to net name -- for example, when pin is attached to a net in
 *         a symbol file?
 *
 * Note: Pin order is determined by order of occurrance in symbol.
 */
#pragma once
#include "QArgUtils2.h"
#include "QItemBase.h"

class QItemPin : public QItemBase {
public:
  QItemPin(std::string typeStr, std::string argStr);
  QItemPin(const QItemPin &other);
  QItemBasePtr clone() const override;

  void parseItem() override;
  std::string toString() const override;

  ArgPoint pt1;
  ArgPoint pt2;
  ArgFontSize fontSize;
  ArgRotAlign rotateAlign;
  ArgPinInfo pinInfo;
  ArgColor textColor;
  ArgInt p7;
  ArgString pinLabel;
  ArgString netName;
};

typedef std::shared_ptr<QItemPin> QItemPinPtr;
