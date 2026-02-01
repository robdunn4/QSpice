/*
 * QItemCmp.h -- Component item class.
 *
 * Expecting: "component" (P1) P2 P3
 * Example:   "component (-1700,-500) 0 0"
 *
 * Analysis:
 *   P1 -- Endpoint coordinate
 *   P2 -- Rotation
 *   P3 -- Stuff/short
 */
#pragma once
#include "QArgUtils2.h"
#include "QItemBase.h"

class QItemCmp : public QItemBase {
public:
  QItemCmp(std::string typeStr, std::string argStr);
  QItemCmp(const QItemCmp &other);
  QItemBasePtr clone() const override;

  void parseItem() override;
  std::string toString() const override;

  ArgPoint p1;
  ArgRot p2;
  ArgStuff p3;

protected:
  // Add schematic-specific member variables as needed
};

typedef std::shared_ptr<QItemCmp> QItemCmpPtr;
