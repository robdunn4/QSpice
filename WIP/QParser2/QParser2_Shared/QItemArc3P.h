//-----------------------------------------------------------------------------
// This file is part of the the QParser2 project.  You can find the complete
// project here:  https://github.com/robdunn4/QSpice/
//-----------------------------------------------------------------------------
/*
 * QItemArc3P.h -- Arc3P item class.
 *
 * Expecting: "arc3p" (P1) (P2) (P3) P4 P5 P6 P7 P8
 * Example:   "arc3p (0,0) (60,0) (30,0) 0 0 0x1000000 -1 -1"
 *
 * Analysis:
 *   P1 -- Endpoint coordinate
 *   P2 -- Endpoint coordinate
 *   P3 -- Endpoint coordinate
 *   P4 -- Line width
 *   P5 -- Line type
 *   P6 -- Line color
 *   P7 -- ?
 *   P8 -- ?
 */
#pragma once
#include "QArgUtils2.h"
#include "QItemBase.h"

class QItemArc;
typedef std::shared_ptr<QItemArc> QItemArcPtr;

class QItemArc : public QItemBase {
public:
  QItemArc(std::string typeStr, std::string argStr);
  QItemArc(const QItemArc &other);
  QItemBasePtr clone() const override;

  void parseItem() override;
  std::string toString() const override;

  ArgPoint pt1;
  ArgPoint pt2;
  ArgPoint pt3;
  ArgLineWidth lineWidth;
  ArgLineType lineType;
  ArgColor lineColor;
  ArgInt p7;
  ArgInt p8;
};
