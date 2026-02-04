//-----------------------------------------------------------------------------
// This file is part of the the QParser2 project.  You can find the complete
// project here:  https://github.com/robdunn4/QSpice/
//-----------------------------------------------------------------------------
/*
 * QItemText.h -- Text/Comment item class.
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

  ArgPoint pt1;
  ArgFontSize fontSize;
  ArgRotAlign rotateAlign;
  ArgTextFlags textFlags;
  ArgColor textColor;
  ArgLookupNdx lookupNdx;
  ArgPinNdx pinNdx;
  ArgString text;
};

typedef std::shared_ptr<QItemText> QItemTextPtr;
