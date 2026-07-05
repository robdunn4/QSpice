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
 * PREFIX-COLLISION RULE: No type string in QPI_ITEMS may be a prefix of
 * another type string followed by a space.  splitTypeAndArgs() iterates
 * the table in declaration order and first-match-wins; if one name were a
 * prefix of another (e.g. "foo" vs "foo bar"), a line starting with
 * "foo bar ..." could be misclassified as "foo" with args "bar ...".  The
 * current table is collision-free; preserve this when adding entries.
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
#include <utility>

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
      COUNT
};

namespace ItemTypes {

inline constexpr std::array<std::string_view, static_cast<size_t>(QPI::COUNT)>
    names = {
#define X(name, str, kind) str,
        QPI_ITEMS(X)
#undef X
};

inline constexpr std::array<NodeType, static_cast<size_t>(QPI::COUNT)>
    defaultNodeTypes = {
#define X(name, str, kind) kind,
        QPI_ITEMS(X)
#undef X
};

inline std::string_view getStr(QPI id) {
  auto i = static_cast<size_t>(id);
  return (i < names.size()) ? names[i] : std::string_view{};
}

inline QPI getEnum(std::string_view s) {
  for (size_t i = 1; i < names.size(); ++i) {
    if (names[i] == s) return static_cast<QPI>(i);
  }
  return QPI::UNKNOWN;
}

// Identify the QPI type at the start of `payload` and return it along with
// the remaining argument text.  Matching rule: find the first entry in
// `names` (skipping UNKNOWN) that is a prefix of `payload` AND is followed
// by either end-of-string or a space.  On match, the returned args view
// has one leading space consumed (if present).  On no match, returns
// {UNKNOWN, payload}.
//
// Caller must strip framing bytes (BEGIN_NODE, END_NODE, newline, leading
// spaces) before calling.  See the prefix-collision rule in the file
// header comment.
inline std::pair<QPI, std::string_view>
splitTypeAndArgs(std::string_view payload) {
  for (size_t i = 1; i < names.size(); ++i) {
    std::string_view n = names[i];
    if (payload.size() < n.size()) continue;
    if (payload.substr(0, n.size()) != n) continue;
    if (payload.size() == n.size())
      return {static_cast<QPI>(i), std::string_view{}};
    if (payload[n.size()] == ' ')
      return {static_cast<QPI>(i), payload.substr(n.size() + 1)};
  }
  return {QPI::UNKNOWN, payload};
}

inline constexpr NodeType getDefaultNodeType(QPI id) {
  auto i = static_cast<size_t>(id);
  return (i < defaultNodeTypes.size()) ? defaultNodeTypes[i] : NodeType::Leaf;
}

inline constexpr bool isContainer(QPI id) {
  return any(getDefaultNodeType(id) & NodeType::Container);
}
inline constexpr bool isLeaf(QPI id) {
  return any(getDefaultNodeType(id) & NodeType::Leaf);
}
inline constexpr bool isContainer(NodeType v) {
  return any(v & NodeType::Container);
}
inline constexpr bool isLeaf(NodeType v) { return any(v & NodeType::Leaf); }

inline constexpr int sortOrder(QPI id) { return static_cast<int>(id); }

} // namespace ItemTypes
