/*
 * QItemDesc.h -- Description item class.
 *
 * Expecting: "description:" P1
 * Example:   "description: Independent Voltage Source"
 *
 * Analysis:
 *   P1 -- String value.  Optional?  Non-quoted?
 */
#pragma once
#include "QArgUtils.h"
#include "QItemBase.h"

class QItemDesc : public QItemBase {
public:
  QItemDesc(std::string typeStr, std::string argStr);
  QItemDesc(const QItemDesc &other);
  QItemBasePtr clone() const override;

  void parseItem() override;
  std::string toString() const override;

  ArgString p1Str;

protected:
  // Add schematic-specific member variables as needed
};

typedef std::shared_ptr<QItemDesc> QItemDescPtr;
