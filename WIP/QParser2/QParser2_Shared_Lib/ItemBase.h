//-----------------------------------------------------------------------------
// This file is part of the the QParser2 project.  You can find the complete
// project here:  https://github.com/robdunn4/QSpice/
//-----------------------------------------------------------------------------
/*
 * ItemBase.h -- Base class for line items.
 *
 * Ownership model: derived classes have no public constructors.  All
 * creation goes through static ItemXxx::makePtr(...) factories that
 * return shared_ptr<ItemXxx>.  Stack instances are a compile error.
 *
 * CRTP layer: derived classes inherit from ItemBaseT<Derived> (not
 * ItemBase directly).  The CRTP base generates clonePtr() -- which
 * returns a typed shared_ptr<Derived> -- and implements the virtual
 * clone() in terms of it.  Derived classes never write clone() by hand.
 *
 * Usage:
 *   auto sym = ItemSym::makePtr(text);           // typed factory
 *   auto copy = sym->clonePtr();                 // typed copy
 *   ItemBasePtr erased = ItemBase::makeItem(QPI::SYM);  // parse path
 */
#pragma once
#include "ItemTypes.h"
#include <memory>
#include <string>

class ItemBase;
typedef std::shared_ptr<ItemBase> ItemBasePtr;

class ItemBase {
public:
  virtual ~ItemBase() = default;

  // Factory for the parse path.  Constructs an empty item of the requested
  // type via the corresponding derived class's makePtr() factory.
  // Returns nullptr for UNKNOWN/COUNT/unrecognized input.
  static ItemBasePtr makeItem(QPI enumID);
  static ItemBasePtr makeItem(const std::string &typeStr);

  // Parse argStr into this item's fields.  Returns true on success, false
  // on failure.  On failure, field contents are unspecified -- the caller
  // should discard the object.
  virtual bool parseItem(const std::string &argStr) = 0;

  // Serialize this item as a full formatted line (type string + args,
  // no framing bytes, no indent, no newline).
  virtual std::string toString() const = 0;

  // Type-erased deep copy.  Implemented by ItemBaseT<Derived>; derived
  // classes do not override this directly.
  virtual ItemBasePtr clone() const = 0;

  QPI              getEnumID() const { return enumID; }
  std::string_view getTypeStr() const { return ItemTypes::getStr(enumID); }

  // True if the item has a recognized type (i.e. not UNKNOWN).
  bool isValid() const { return enumID != QPI::UNKNOWN; }

  // Instance-level node type.  Initialized from the ItemTypes table default
  // by the ItemBase(QPI) constructor.  Derived classes or parsing logic may
  // overwrite it to a specific value (e.g. a hierarchical-block rect
  // switching its default to "Container").
  NodeType getNodeType() const { return nodeType; }
  void     setNodeType(NodeType nt) { nodeType = nt; }

  bool isContainer() const { return ItemTypes::isContainer(nodeType); }
  bool isLeaf() const { return ItemTypes::isLeaf(nodeType); }

protected:
  // Derived classes reach this through ItemBaseT<Derived>.  Direct
  // inheritance from ItemBase is still possible but discouraged -- use
  // the CRTP layer so clone()/clonePtr() are generated for you.
  explicit ItemBase(QPI id)
      : enumID(id), nodeType(ItemTypes::getDefaultNodeType(id)) {}
  ItemBase(const ItemBase &other) = default;

  QPI      enumID;
  NodeType nodeType;
};

//-----------------------------------------------------------------------------
// ItemBaseT<Derived> -- CRTP layer.
//
// Every concrete item class should derive from ItemBaseT<Itself> rather
// than directly from ItemBase.  This generates:
//   - clonePtr()  returning std::shared_ptr<Derived>
//   - clone()     implemented in terms of clonePtr()
//
// Derived classes keep their constructors protected and expose static
// makePtr(...) factories that match each constructor.  Because
// clonePtr() calls `new Derived(...)` from this template body, every
// derived class needs `friend class ItemBaseT<ItemXxx>;` so the
// protected copy constructor is reachable.
//-----------------------------------------------------------------------------
template <typename Derived>
class ItemBaseT : public ItemBase {
public:
  using ItemBase::ItemBase;

  // Typed deep copy -- returns shared_ptr<Derived>, no downcast needed.
  std::shared_ptr<Derived> clonePtr() const {
    return std::shared_ptr<Derived>(
        new Derived(static_cast<const Derived &>(*this)));
  }

  // Satisfies the virtual contract.  Implicit upcast from
  // shared_ptr<Derived> to ItemBasePtr.
  ItemBasePtr clone() const override { return clonePtr(); }
};
