/*
 * QItemZigzag.h -- Zigzag item class.
 *
 * Expecting: "zigzag" (P1) (P2) P3 P4 P5 P6 P7 P8
 * Example:   "zigzag (-80,180) (80,-180) 0 0 0 0x1000000 -1 -1"
 *
 * Analysis:
 *   P1 -- Endpoint coordinate
 *   P2 -- Endpoint coordinate
 *   P3 -- Rotation
 *   P4 -- Line width
 *   P5 -- Line type
 *   P6 -- Line color
 *   P7 -- ?
 *   P8 -- ?
 */

#pragma once
#include "QArgUtils2.h"
#include "QItemBase.h"

class QItemZigzag : public QItemBase {
public:
  QItemZigzag(std::string typeStr, std::string argStr);
  QItemZigzag(const QItemZigzag &other);
  QItemBasePtr clone() const override;

  void parseItem() override;
  std::string toString() const override;

  ArgPoint p1;
  ArgPoint p2;
  ArgRot p3;
  ArgLineWidth p4;
  ArgLineType p5;
  ArgColor p6;
  ArgInt p7;
  ArgInt p8;

protected:
  // Add schematic-specific member variables as needed
};

typedef std::shared_ptr<QItemZigzag> QItemZigzagPtr;