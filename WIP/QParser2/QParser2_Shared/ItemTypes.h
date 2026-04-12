//-----------------------------------------------------------------------------
// This file is part of the the QParser2 project.  You can find the complete
// project here:  https://github.com/robdunn4/QSpice/
//-----------------------------------------------------------------------------
/*
 * ItemTypes.h -- Enum/string/nodeType mapping for QPI item types.
 *
 * Single source of truth is the QPI_ITEMS X-macro below.  The enum, the
 * string table, and the default NodeType table are all generated from it,
 * so they cannot drift out of sync.  To add, remove, or reorder an item
 * type, edit QPI_ITEMS only.
 *
 * The declaration order of QPI_ITEMS defines both the enum value and the
 * sort order used elsewhere in the program (via sortOrder() or a direct
 * static_cast<int>).
 *
 * NodeType is a bitfield.  At the type level (the default stored in the
 * table) a value may have both bits set to indicate "could be either."
 * At the instance level (stored on ItemBase) exactly one bit should be
 * set once the role is known.
 */
#pragma once
#include <array>
#include <string_view>
#include <type_traits>

//-----------------------------------------------------------------------------
// NodeType -- bitfield describing whether an item is a container, a leaf,
// or either.
//-----------------------------------------------------------------------------
enum class NodeType : unsigned
{
  None      = 0,
  Container = 1 << 0,
  Leaf      = 1 << 1,
  Either    = Container | Leaf,
};

// Bitwise operators for NodeType (scoped enums don't get these for free).
inline constexpr NodeType operator|(NodeType a, NodeType b) {
  using U = std::underlying_type_t<NodeType>;
  return static_cast<NodeType>(static_cast<U>(a) | static_cast<U>(b));
}
inline constexpr NodeType operator&(NodeType a, NodeType b) {
  using U = std::underlying_type_t<NodeType>;
  return static_cast<NodeType>(static_cast<U>(a) & static_cast<U>(b));
}
inline constexpr NodeType &operator|=(NodeType &a, NodeType b) {
  return a = a | b;
}
inline constexpr NodeType &operator&=(NodeType &a, NodeType b) {
  return a = a & b;
}
inline constexpr bool any(NodeType v) {
  return static_cast<std::underlying_type_t<NodeType>>(v) != 0;
}

//-----------------------------------------------------------------------------
// Single source of truth.  Order matters: it defines enum values and sort
// order.  UNKNOWN must remain first.
//
// Columns:
//   1. Enum name
//   2. String representation
//   3. Default NodeType (Container, Leaf, or Either)
//-----------------------------------------------------------------------------
#define QPI_ITEMS(X)                                                           \
  X(UNKNOWN, "base", NodeType::Leaf)                                           \
  X(SCH, "schematic", NodeType::Container)                                     \
  X(COMP, "component", NodeType::Container)                                    \
  X(SYM, "symbol", NodeType::Container)                                        \
  X(TYPE, "type:", NodeType::Leaf)                                             \
  X(DESC, "description:", NodeType::Leaf)                                      \
  X(LIB, "library file:", NodeType::Leaf)                                      \
  X(SHORTED, "shorted pins:", NodeType::Leaf)                                  \
  X(NET, "net", NodeType::Leaf)                                                \
  X(JUNCTION, "junction", NodeType::Leaf)                                      \
  X(WIRE, "wire", NodeType::Leaf)                                              \
  X(TAP, "tap", NodeType::Leaf)                                                \
  X(LINE, "line", NodeType::Leaf)                                              \
  X(RECT, "rect", NodeType::Leaf)                                              \
  X(ELLIPSE, "ellipse", NodeType::Leaf)                                        \
  X(ARC3P, "arc3p", NodeType::Leaf)                                            \
  X(TRIANGLE, "triangle", NodeType::Leaf)                                      \
  X(COIL, "coil", NodeType::Leaf)                                              \
  X(ZIGZAG, "zigzag", NodeType::Leaf)                                          \
  X(TEXT, "text", NodeType::Leaf)                                              \
  X(PIN, "pin", NodeType::Leaf)

enum class QPI : int
{
#define X(name, str, kind) name,
  QPI_ITEMS(X)
#undef X
      COUNT // keep last -- sentinel for table size
};

namespace ItemTypes {

// String table, indexed directly by the enum value.
inline constexpr std::array<std::string_view, static_cast<size_t>(QPI::COUNT)>
    names = {
#define X(name, str, kind) str,
        QPI_ITEMS(X)
#undef X
};

// Default NodeType table, indexed directly by the enum value.
inline constexpr std::array<NodeType, static_cast<size_t>(QPI::COUNT)>
    defaultNodeTypes = {
#define X(name, str, kind) kind,
        QPI_ITEMS(X)
#undef X
};

// enum -> string.  Returns "" for UNKNOWN or out-of-range values.
inline std::string_view getStr(QPI id) {
  auto i = static_cast<size_t>(id);
  return (i < names.size()) ? names[i] : std::string_view{};
}

// string -> enum.  Returns QPI::UNKNOWN if not found.  Linear scan is
// faster than a hash map for a table this size.
inline QPI getEnum(std::string_view s) {
  for (size_t i = 1; i < names.size(); ++i) { // skip UNKNOWN
    if (names[i] == s) return static_cast<QPI>(i);
  }
  return QPI::UNKNOWN;
}

// enum -> default NodeType from the table.
inline constexpr NodeType getDefaultNodeType(QPI id) {
  auto i = static_cast<size_t>(id);
  return (i < defaultNodeTypes.size()) ? defaultNodeTypes[i] : NodeType::Leaf;
}

// Type-level queries.  These answer "can this type be X" -- for Either
// entries both return true.
inline constexpr bool isContainer(QPI id) {
  return any(getDefaultNodeType(id) & NodeType::Container);
}
inline constexpr bool isLeaf(QPI id) {
  return any(getDefaultNodeType(id) & NodeType::Leaf);
}

// Value-level queries, for an already-resolved NodeType.
inline constexpr bool isContainer(NodeType v) {
  return any(v & NodeType::Container);
}
inline constexpr bool isLeaf(NodeType v) { return any(v & NodeType::Leaf); }

// Explicit sort key, for callers that want to document intent.
inline constexpr int sortOrder(QPI id) { return static_cast<int>(id); }

} // namespace ItemTypes
