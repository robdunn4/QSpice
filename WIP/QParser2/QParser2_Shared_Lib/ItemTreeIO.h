//-----------------------------------------------------------------------------
// This file is part of the the QParser2 project.  You can find the complete
// project here:  https://github.com/robdunn4/QSpice/
//-----------------------------------------------------------------------------
/*
 * ItemTreeIO.h -- Read and write .qsch-format files into/from an ItemTree.
 *
 * File format:
 *   file      := FILE_ID item
 *   item      := leaf | container
 *   leaf      := INDENT AB type (SP args)? BB LF
 *   container := INDENT AB type (SP args)? LF  item*  INDENT BB LF
 *
 * INDENT is 2 * depth spaces on write, ignored on read.  Lines are LF
 * terminated on write; CRLF is tolerated on read.  The writer always
 * produces canonical output; parsing an out-of-order file silently
 * re-sorts it to the QPI-sorted invariant.
 */
#pragma once
#include "ItemTree.h"
#include <iosfwd>
#include <string>
#include <string_view>

namespace ItemTreeIO {

inline constexpr unsigned char BEGIN_NODE  = 0xAB;
inline constexpr unsigned char END_NODE    = 0xBB;
inline constexpr unsigned char FILE_ID[4]  = {0xFF, 0xD8, 0xFF, 0xDB};

struct ParseResult {
  ItemTree    tree;
  bool        ok   = false;
  std::string error;
  int         line = 0; // 1-based; 0 if not line-specific
};

// Read.  On failure, `tree` is empty and `error`/`line` describe the problem.
ParseResult read(std::istream &in);
ParseResult readFile(const std::string &path);
ParseResult readString(std::string_view text);

// Write.  Returns false on I/O error.  Output is always well-formed.
bool        write(const ItemTree &tree, std::ostream &out);
bool        writeFile(const ItemTree &tree, const std::string &path);
std::string writeString(const ItemTree &tree);

} // namespace ItemTreeIO
