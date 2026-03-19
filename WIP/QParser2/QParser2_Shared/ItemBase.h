//-----------------------------------------------------------------------------
// This file is part of the the QParser2 project.  You can find the complete
// project here:  https://github.com/robdunn4/QSpice/
//-----------------------------------------------------------------------------
/*
 * ItemBase.h -- Base class for line items.  Contains factory methods for
 * derived classes.
 */
#pragma once
#include "ItemTypes.h"
#include <memory>
#include <string>

// forward decls
class ItemBase;
typedef std::shared_ptr<ItemBase> ItemBasePtr;

class ItemBase {
public:
  virtual ~ItemBase() = default;

  // Factory method - returns appropriate derived class based on typeStr
  // Note: This creates the object AND calls parseItem() on it
  static ItemBasePtr makeItem(std::string typeStr, std::string argStr);

  // Pure virtual methods that derived classes must implement
  virtual void        parseItem()      = 0;
  virtual std::string toString() const = 0;

  // Pure virtual clone method - each derived class must implement
  // Returns a deep copy of the object with all parsed data
  virtual ItemBasePtr clone() const = 0;

  std::string getTypeStr() { return typeStr; }
  QPI         getEnumID() const { return enumID; }

protected:
  // Constructor: initializes base members, does NOT call parseItem()
  // Parsing is done explicitly after construction in makeItem()
  explicit ItemBase(std::string typeStr, std::string argStr);

  // Copy constructor for use by derived classes
  // Copies base members only (typeStr, argStr, enumID)
  ItemBase(const ItemBase &other);

  std::string typeStr = "Base"; // default; derived classes must replace
  std::string argStr  = "";
  QPI         enumID;
};

