/*
 * QItemSch.h -- Schematic item class.
 *
 * Expecting: "schematic"
 *
 * Analysis:
 *   No parameters?
 */
#pragma once
#include "QItemBase.h"

class QItemSch : public QItemBase {
public:
  QItemSch(std::string typeStr, std::string argStr);
  QItemSch(const QItemSch &other);
  QItemBasePtr clone() const override;

  void parseItem() override;
  std::string toString() const override;

protected:
  // Add schematic-specific member variables as needed
};

typedef std::shared_ptr<QItemSch> QItemSchPtr;
