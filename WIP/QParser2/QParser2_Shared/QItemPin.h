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
 *   P9 -- (as yet unidentified optional quoted string argument)
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

  ArgPoint p1;
  ArgPoint p2;
  ArgFontSize p3;
  ArgRotAlign p4;
  ArgPinInfo p5;
  ArgColor p6;
  ArgInt p7;
  ArgString p8;
  ArgString p9;

protected:
  // Add schematic-specific member variables as needed
};

typedef std::shared_ptr<QItemPin> QItemPinPtr;
