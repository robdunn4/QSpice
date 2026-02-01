/*
 * QItemText.h -- Text/Comment item class.
 *
 * Expecting: "text" (P1) P2 P3 P4 P5 P6 P7 P8
 * Example:   "text (100,150) 1 7 0 0x1000000 -1 -1 "V1""
 *
 * Analysis:
 *   P1 -- Anchor coordinate?
 *   P2 -- Float - Font Size (5 4 3 2.5 2 1.6 1.25 1 0.8 0.65)
 *   P3 -- Text alignment/rotation
 *   P4 -- Bit 0: clear = text, set = comment
 *         Bit 1: clear = visible, set = hidden
 *   P5 -- RGB color
 *   P6 -- ?
 *   P7 -- ?
 *   P8 -- String value.  Note special characters.  Overbars are embedded
 *         special characters?
 *
 * Note:  Per ME, P7 is a "lookup index."  Didn't ask what it indexes into...
 */
#pragma once
#include "QArgUtils2.h"
#include "QItemBase.h"

class QItemText : public QItemBase {
public:
  QItemText(std::string typeStr, std::string argStr);
  QItemText(const QItemText &other);
  QItemBasePtr clone() const override;

  void parseItem() override;
  std::string toString() const override;

  ArgPoint p1;
  ArgFontSize p2;
  ArgRotAlign p3;
  ArgInt p4;
  ArgColor p5;
  ArgInt p6;
  ArgInt p7;
  ArgString p8;

protected:
  // Add schematic-specific member variables as needed
};

typedef std::shared_ptr<QItemText> QItemTextPtr;
