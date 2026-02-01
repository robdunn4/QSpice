/*
 * QItemWire.h -- Wire item class.
 *
 * Expecting: "wire" (P1) (P2) P3
 * Example:   "wire (-1700,-300) (-1700,0) "Vin""
 *
 * Analysis:
 *   P1 -- Endpoint coordinate
 *   P2 -- Endpoint coordinate
 *   P3 -- Net/port name
 */                                                                            \
#pragma once
#include "QArgUtils.h"
#include "QItemBase.h"

class QItemWire : public QItemBase {
public:
  QItemWire(std::string typeStr, std::string argStr);
  QItemWire(const QItemWire &other);
  QItemBasePtr clone() const override;

  void parseItem() override;
  std::string toString() const override;

  ArgPoint p1;
  ArgPoint p2;
  ArgString p3;

protected:
  // Add schematic-specific member variables as needed
};

typedef std::shared_ptr<QItemWire> QItemWirePtr;
