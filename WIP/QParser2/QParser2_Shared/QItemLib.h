/*
 * QItemLib.h -- Library file item class.
 *
 * Expecting: "library file:" P1
 * Example:   "library file: xxx"
 *
 * Analysis:
 *   P1 --  String value.  Optional?  Non-quoted?
 */
#pragma once
#include "QArgUtils.h"
#include "QItemBase.h"

class QItemLib : public QItemBase {
public:
  QItemLib(std::string typeStr, std::string argStr);
  QItemLib(const QItemLib &other);
  QItemBasePtr clone() const override;

  void parseItem() override;
  std::string toString() const override;

  ArgString p1Str;

protected:
  // Add schematic-specific member variables as needed
};

typedef std::shared_ptr<QItemLib> QItemLibPtr;
