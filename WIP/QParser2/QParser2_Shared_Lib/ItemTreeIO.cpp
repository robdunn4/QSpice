//-----------------------------------------------------------------------------
// This file is part of the the QParser2 project.  You can find the complete
// project here:  https://github.com/robdunn4/QSpice/
//-----------------------------------------------------------------------------
#include "ItemTreeIO.h"
#include "FileUtil.h"
#include "ItemBase.h"
#include "ItemTypes.h"
#include <sstream>
#include <vector>

namespace ItemTreeIO {

namespace {

// Make a failing ParseResult with a message and line number.
ParseResult fail(std::string msg, int line = 0) {
  ParseResult r;
  r.ok    = false;
  r.error = std::move(msg);
  r.line  = line;
  return r;
}

// Strip one trailing '\r' (tolerate CRLF input).
void stripCR(std::string &s) {
  if (!s.empty() && s.back() == '\r') s.pop_back();
}

// Count and strip leading spaces.  Returns the stripped view.
std::string_view stripLeadingSpaces(std::string_view s) {
  size_t i = 0;
  while (i < s.size() && s[i] == ' ')
    ++i;
  return s.substr(i);
}

// Recursive writer.  Emits a node and (if container) its children + close.
bool writeNode(const NodePtr &n, int depth, std::ostream &out) {
  if (!n) return true;
  // indent
  for (int i = 0; i < depth * 2; ++i)
    out.put(' ');
  out.put(static_cast<char>(BEGIN_NODE));
  // toString() returns the full type+args string (no framing, no indent).
  std::string body = n->item()->toString();
  out.write(body.data(), static_cast<std::streamsize>(body.size()));

  if (n->isContainer()) {
    out.put('\n');
    if (!out) return false;
    for (const auto &c : n->children()) {
      if (!writeNode(c, depth + 1, out)) return false;
    }
    for (int i = 0; i < depth * 2; ++i)
      out.put(' ');
    out.put(static_cast<char>(END_NODE));
    out.put('\n');
  } else {
    out.put(static_cast<char>(END_NODE));
    out.put('\n');
  }
  return static_cast<bool>(out);
}

} // namespace

//-----------------------------------------------------------------------------
// Read
//-----------------------------------------------------------------------------

ParseResult read(std::istream &in) {
  // Header check.
  char hdr[4] = {0};
  in.read(hdr, 4);
  if (in.gcount() != 4) return fail("file too short: missing header");
  for (int i = 0; i < 4; ++i) {
    if (static_cast<unsigned char>(hdr[i]) != FILE_ID[i])
      return fail("bad file header");
  }

  ParseResult          result;
  std::vector<NodePtr> stack; // open containers; top is current parent
  int                  lineNo = 0;
  std::string          rawLine;

  while (std::getline(in, rawLine)) {
    ++lineNo;
    stripCR(rawLine);
    std::string_view line = stripLeadingSpaces(rawLine);

    if (line.empty()) continue; // trailing blank line(s) OK

    unsigned char first = static_cast<unsigned char>(line.front());

    if (first == END_NODE) {
      // Container close.
      if (stack.empty()) return fail("unexpected container close", lineNo);
      stack.pop_back();
      continue;
    }

    if (first != BEGIN_NODE) return fail("expected BEGIN_NODE", lineNo);

    // Strip leading BEGIN_NODE.
    line.remove_prefix(1);

    // Leaf if last byte is END_NODE, else container opener.
    bool isLeaf =
        !line.empty() && static_cast<unsigned char>(line.back()) == END_NODE;
    if (isLeaf) line.remove_suffix(1);

    auto [qpi, args] = ItemTypes::splitTypeAndArgs(line);
    if (qpi == QPI::UNKNOWN) return fail("unknown item type", lineNo);

    ItemBasePtr item = ItemBase::makeItem(qpi);
    if (!item) return fail("makeItem failed", lineNo);

    if (!item->parseItem(std::string(args)))
      return fail("parseItem failed", lineNo);

    NodePtr node = TreeNode::make(std::move(item));

    if (stack.empty()) {
      // This must be the root.  It must be a container opener.
      if (isLeaf) return fail("root must be a container", lineNo);
      result.tree.setRoot(node);
      stack.push_back(node);
    } else {
      NodePtr parent = stack.back();
      if (!parent->addLast(node))
        return fail("addLast failed (parent not a container?)", lineNo);
      if (!isLeaf) stack.push_back(node);
    }
  }

  if (!stack.empty()) return fail("unterminated container at EOF", lineNo);
  if (result.tree.empty()) return fail("empty file: no root item");

  result.ok = true;
  return result;
}

ParseResult readFile(const std::string &path) {
  try {
    std::ifstream f = FileUtil::openInput(path);
    return read(f);
  } catch (const std::exception &e) {
    return fail(e.what());
  }
}

ParseResult readString(std::string_view text) {
  std::istringstream s{std::string(text)};
  return read(s);
}

//-----------------------------------------------------------------------------
// Write
//-----------------------------------------------------------------------------

bool write(const ItemTree &tree, std::ostream &out) {
  if (tree.empty()) return false;
  out.write(reinterpret_cast<const char *>(FILE_ID), 4);
  if (!out) return false;
  if (!writeNode(tree.root(), 0, out)) return false;
  //// Trailing blank line, matching the sample file.
  // out.put('\n');
  return static_cast<bool>(out);
}

bool writeFile(const ItemTree &tree, const std::string &path) {
  try {
    std::ofstream f = FileUtil::openOutput(path);
    return write(tree, f);
  } catch (...) {
    return false;
  }
}

std::string writeString(const ItemTree &tree) {
  std::ostringstream s;
  if (!write(tree, s)) return {};
  return s.str();
}

} // namespace ItemTreeIO