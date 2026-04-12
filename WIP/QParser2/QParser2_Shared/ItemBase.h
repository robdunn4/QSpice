//-----------------------------------------------------------------------------
// This file is part of the the QParser2 project.  You can find the complete
// project here:  https://github.com/robdunn4/QSpice/
//-----------------------------------------------------------------------------
/*
 * ItemBase.h -- Base class for line items.  Contains factory methods for
 * derived classes.
 *
 * Derived classes MUST forward their QPI enum value to the ItemBase(QPI)
 * constructor.  This is compiler-enforced -- there is no default constructor.
 * The instance-level nodeType is initialized automatically from the ItemTypes
 * table and may be overwritten afterwards via setNodeType().
 *
 * Usage:
 *   auto item = ItemBase::makeItem(QPI::COIL);     // or makeItem("coil")
 *   if (!item->parseItem(argStr)) { ...error... }
 */

// TODO:  reconsider what the derived class clone() methods return.  Currently
// returns a shared_ptr<ItemBase> to the object.  Shouldn't it return an
// independent, unshared copy?  Does make_unique make more sense?  Or should the
// caller decide whether to share or not?

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

  // Factory methods.  Construct an empty item of the requested type.
  // Parsing is a separate step -- call parseItem(argStr) afterwards.
  // The string overload returns nullptr if typeStr is not recognized.
  static ItemBasePtr makeItem(QPI enumID);
  static ItemBasePtr makeItem(const std::string &typeStr);

  // Parse argStr into this item's fields.  Returns true on success, false
  // on failure.  On failure, field contents are unspecified -- the caller
  // should discard the object.
  virtual bool parseItem(const std::string &argStr) = 0;

  virtual std::string toString() const = 0;

  // Deep copy of the object with all parsed data.
  virtual ItemBasePtr clone() const = 0;

  QPI              getEnumID() const { return enumID; }
  std::string_view getTypeStr() const { return ItemTypes::getStr(enumID); }

  // True if the item has a recognized type (i.e. not UNKNOWN).
  bool isValid() const { return enumID != QPI::UNKNOWN; }

  // Instance-level node type.  Initialized from the ItemTypes table default
  // by the ItemBase(QPI) constructor.  Derived classes or parsing logic may
  // overwrite it to a specific value (e.g. a hierarchical-block rect
  // switching its default "Either" to "Container").
  NodeType getNodeType() const { return nodeType; }
  void     setNodeType(NodeType nt) { nodeType = nt; }

  bool isContainer() const { return ItemTypes::isContainer(nodeType); }
  bool isLeaf() const { return ItemTypes::isLeaf(nodeType); }

protected:
  // Derived classes MUST pass their QPI enum value here.  The compiler
  // enforces this -- there is no default constructor.
  explicit ItemBase(QPI id)
      : enumID(id), nodeType(ItemTypes::getDefaultNodeType(id)) {}
  ItemBase(const ItemBase &other) = default;

  QPI      enumID;
  NodeType nodeType;
};
