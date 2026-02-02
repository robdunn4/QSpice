//-----------------------------------------------------------------------------
// This file is part of the the QParser2 project.  You can find the complete
// project here:  https://github.com/robdunn4/QSpice/
//-----------------------------------------------------------------------------
/*
 * QItemLine.h -- Line item class.
 *
 * Expecting: "line" (P1) (P2) P3 P4 P5 P6 P7
 * Example:   "line (0,-130) (0,-200) 0 0 0x1000000 -1 -1"
 *
 * Analysis:
 *   P1 -- Endpoint coordinate
 *   P2 -- Endpoint coordinate
 *   P3 -- Line width
 *   P4 -- Line type
 *   P5 -- Line color
 *   P6 -- ?
 *   P7 -- ?
 */
#pragma once
#include "QArgUtils2.h"
#include "QItemBase.h"

class QItemLine : public QItemBase {
public:
  QItemLine(std::string typeStr, std::string argStr);
  QItemLine(const QItemLine &other);
  QItemBasePtr clone() const override;

  void parseItem() override;
  std::string toString() const override;

  ArgPoint pt1;
  ArgPoint pt2;
  ArgLineWidth lineWidth;
  ArgLineType lineType;
  ArgColor lineColor;
  ArgInt p6;
  ArgInt p7;
};

typedef std::shared_ptr<QItemLine> QItemLinePtr;
