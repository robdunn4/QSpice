//-----------------------------------------------------------------------------
// This file is part of the the QParser2 project.  You can find the complete
// project here:  https://github.com/robdunn4/QSpice/
//-----------------------------------------------------------------------------
/*
 * QItemEllipse.h -- Ellipse item class.
 *
 * Expecting: "ellipse" (P1) (P2) P3 P4 P5 P6 P7 P8 P9
 * Example:   "ellipse (-130,130) (130,-130) 0 0 0 0x1000000 0x1000000 -1 -1"
 *
 * Analysis:
 *   P1 -- Endpoint coordinate
 *   P2 -- Endpoint coordinate
 *   P3 -- Rotation
 *   P4 -- Line width
 *   P5 -- Line type
 *   P6 -- Line color
 *   P7 -- Fill color
 *   P8 -- ?
 *   P9 -- ?
 */
#pragma once
#include "QArgUtils2.h"
#include "QItemBase.h"

class QItemEllipse : public QItemBase {
public:
  QItemEllipse(std::string typeStr, std::string argStr);
  QItemEllipse(const QItemEllipse &other);
  QItemBasePtr clone() const override;

  void parseItem() override;
  std::string toString() const override;

  ArgPoint pt1;
  ArgPoint pt2;
  ArgRot rotate;
  ArgLineWidth lineWidth;
  ArgLineType lineType;
  ArgColor lineColor;
  ArgColor fillColor;
  ArgInt p8;
  ArgInt p9;
};

typedef std::shared_ptr<QItemEllipse> QItemEllipsePtr;
