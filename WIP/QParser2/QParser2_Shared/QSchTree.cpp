// QSchTree.cpp
#include "QSchTree.h"
#include <algorithm>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <stdexcept>

#include "StrUtils.h" // required for StrUtils namespace
using StrUtils::trim;

QSchTree::QSchTree(const std::string &value, int line)
    : data(value), lineNbr(line) {
  parseData();
}

// Copy constructor - creates deep copy without re-parsing
QSchTree::QSchTree(const QSchTree &other)
    : data(other.data), lineNbr(other.lineNbr), typeStr(other.typeStr),
      dataStr(other.dataStr), enumID(other.enumID),
      itemPtr(other.itemPtr ? other.itemPtr->clone() : nullptr) {

  // Deep copy all children recursively
  children.reserve(other.children.size());
  for (const auto &child : other.children) {
    children.push_back(std::make_shared<QSchTree>(*child));
  }
}

// Copy assignment operator - creates deep copy using copy-and-swap idiom
QSchTree &QSchTree::operator=(const QSchTree &other) {
  if (this != &other) {
    // Copy all simple members
    data = other.data;
    lineNbr = other.lineNbr;
    typeStr = other.typeStr;
    dataStr = other.dataStr;
    enumID = other.enumID;

    // Deep copy itemPtr
    itemPtr = other.itemPtr ? other.itemPtr->clone() : nullptr;

    // Deep copy all children
    children.clear();
    children.reserve(other.children.size());
    for (const auto &child : other.children) {
      children.push_back(std::make_shared<QSchTree>(*child));
    }
  }
  return *this;
}

std::string QSchTree::toString() const { return itemPtr->toString(); }

// void QSchTree::addChild(std::shared_ptr<QSchTree> child) {
void QSchTree::addChild(QSchTreePtr child) {
  if (child) {
    children.push_back(child);
    // Set parent pointer using shared_from_this would require
    // enable_shared_from_this Instead, we'll set it when we have a shared_ptr
    // to this node
    child->parent =
        std::weak_ptr<QSchTree>(); // Will be set properly when needed
  }
}

void QSchTree::addChild(const std::string &value) {
  auto child = std::make_shared<QSchTree>(value, 0);
  children.push_back(child);
  // Parent will be set when tree is properly constructed
}

// void QSchTree::insertChild(size_t index, std::shared_ptr<QSchTree> child) {
void QSchTree::insertChild(size_t index, QSchTreePtr child) {
  if (child && index <= children.size()) {
    children.insert(children.begin() + index, child);
    child->parent = std::weak_ptr<QSchTree>();
  }
}

void QSchTree::removeChild(size_t index) {
  if (index < children.size()) {
    children[index]->parent.reset();
    children.erase(children.begin() + index);
  }
}

// void QSchTree::removeChild(const std::shared_ptr<QSchTree> &child) {
void QSchTree::removeChild(const QSchTreePtr &child) {
  auto it = std::find(children.begin(), children.end(), child);
  if (it != children.end()) {
    (*it)->parent.reset();
    children.erase(it);
  }
}

// Navigation methods
// std::shared_ptr<QSchTree> QSchTree::getParent() const { return parent.lock();
// }
QSchTreePtr QSchTree::getParent() const { return parent.lock(); }

QSchTreePtr QSchTree::getFirstChild() const {
  return children.empty() ? nullptr : children.front();
}

QSchTreePtr QSchTree::getLastChild() const {
  return children.empty() ? nullptr : children.back();
}

QSchTreePtr QSchTree::getChild(size_t index) const {
  return (index < children.size()) ? children[index] : nullptr;
}

QSchTreePtr QSchTree::getNextSibling() const {
  auto parentPtr = parent.lock();
  if (!parentPtr) return nullptr;

  size_t myIndex = getSiblingIndex();
  if (myIndex == static_cast<size_t>(-1) ||
      myIndex + 1 >= parentPtr->children.size()) {
    return nullptr;
  }

  return parentPtr->children[myIndex + 1];
}

QSchTreePtr QSchTree::getPrevSibling() const {
  auto parentPtr = parent.lock();
  if (!parentPtr) return nullptr;

  size_t myIndex = getSiblingIndex();
  if (myIndex == static_cast<size_t>(-1) || myIndex == 0) {
    return nullptr;
  }

  return parentPtr->children[myIndex - 1];
}

// Query methods
bool QSchTree::isRoot() const {
  return parent.expired() || parent.lock() == nullptr;
}

bool QSchTree::isLeaf() const { return children.empty(); }

size_t QSchTree::getChildCount() const { return children.size(); }

size_t QSchTree::getSiblingIndex() const {
  auto parentPtr = parent.lock();
  if (!parentPtr) return static_cast<size_t>(-1);

  for (size_t i = 0; i < parentPtr->children.size(); ++i) {
    if (parentPtr->children[i].get() == this) {
      return i;
    }
  }

  return static_cast<size_t>(-1);
}

void QSchTree::setParent(std::weak_ptr<QSchTree> parentPtr) {
  parent = parentPtr;
}

void QSchTree::parseData() {
  std::istringstream iss(data);
  std::string secondWord, remainder;

  // Extract first two words
  iss >> typeStr;
  typeStr = trim(typeStr);
  iss >> secondWord;
  secondWord = trim(secondWord);

  // get remainder of line...
  std::getline(iss, remainder);
  remainder = trim(remainder);

  // Check if second word ends with a colon
  if (!secondWord.empty() && secondWord.back() == ':') {
    typeStr += " " + secondWord;
    dataStr = remainder;
  } else if (!secondWord.empty()) dataStr = secondWord + " " + remainder;
  else dataStr = remainder;

  // for some reason, still have trailing space on some nodeData -- I'm
  // missing something... this works for now...
  dataStr = trim(dataStr);
  enumID = QItemTypes::getEnum(typeStr);

  itemPtr = QItemBase::makeItem(typeStr, dataStr);
}

// Deep clone method - uses copy constructor
QSchTreePtr QSchTree::clone() const {
  auto cloned = std::make_shared<QSchTree>(*this);
  // Note: parent pointer is intentionally not set - cloned tree is orphaned
  // Call rebuildParentPointers() on the cloned tree if you need parent links
  cloned->rebuildParentPointers();
  return cloned;
}

// Rebuild parent pointers for this node and all descendants
void QSchTree::rebuildParentPointers() {
  // Root node has no parent
  parent.reset();

  // Recursively rebuild for all children
  for (auto &child : children) {
    if (child) {
      child->parent = std::weak_ptr<QSchTree>(); // Will be set properly below
      child->rebuildParentPointersRecursive(child);
    }
  }
}

// Protected recursive helper for rebuilding parent pointers
void QSchTree::rebuildParentPointersRecursive(QSchTreePtr self) {
  // Set parent pointers for all children
  for (auto &child : children) {
    if (child) {
      child->parent = self;
      child->rebuildParentPointersRecursive(child);
    }
  }
}

void QSchTree::printTree(int indent) const {
  for (int i = 0; i < indent; ++i) {
    std::cout << "  ";
  }

  for (const auto &child : children) {
    child->printTree(indent + 1);
  }
}

void QSchTree::printBreadthFirst() const {
  std::vector<std::pair<const QSchTree *, int>> queue;
  queue.push_back({this, 0});

  size_t index = 0;
  while (index < queue.size()) {
    auto [node, level] = queue[index++];

    for (int i = 0; i < level; ++i) {
      std::cout << "  ";
    }
    std::cout << "- " << node->toString() << std::endl;

    for (const auto &child : node->children) {
      queue.push_back({child.get(), level + 1});
    }
  }
}

void QSchTree::printWithPrefix(const std::string &prefix, bool isLast) const {
  std::cout << "[" << std::setw(4) << lineNbr << "] ";
  std::cout << prefix;
  std::cout << (isLast ? "+-- " : "+-- ");
  std::cout << toString() << std::endl;

  for (size_t i = 0; i < children.size(); ++i) {
    bool last = (i == children.size() - 1);
    std::string newPrefix = prefix + (isLast ? "    " : "|   ");
    children[i]->printWithPrefix(newPrefix, last);
  }
}

void QSchTree::writeToStream(std::ostream &stream) const {
  // Write file identifier
  stream.write(reinterpret_cast<const char *>(FILE_ID), 4);

  // Write tree content recursively
  writeNodeRecursive(stream);

  // Add final newline
  stream << "\n";
}

void QSchTree::writeNodeRecursive(std::ostream &stream, int indent) const {
  // Write indentation for readability
  for (int i = 0; i < indent; ++i) {
    stream << "  ";
  }

  // Write BEGIN_NODE marker
  stream.put(BEGIN_NODE);

  // Write node name
  stream << toString();

  if (children.empty()) {
    // Leaf node: close on same line
    stream.put(END_NODE);
    stream << "\n";
  } else {
    // Parent node: write on separate lines
    stream << "\n";

    // Recursively write all children
    for (const auto &child : children) {
      child->writeNodeRecursive(stream, indent + 1);
    }

    // Write closing marker with indentation
    for (int i = 0; i < indent; ++i) {
      stream << "  ";
    }
    stream.put(END_NODE);
    stream << "\n";
  }
}

QSchTreePtr QSchTree::parseNodeRecursive(std::istream &stream,
                                         int &currentLine) {
  std::string line;

  // Read the next non-empty line
  while (std::getline(stream, line)) {
    currentLine++;
    line = trim(line);
    if (!line.empty()) {
      break;
    }
  }

  if (line.empty()) {
    return nullptr; // End of file
  }

  // Check for closing tag (just the END_NODE marker)
  if (line.length() == 1 && static_cast<unsigned char>(line[0]) == END_NODE) {
    return nullptr; // Signals end of current parent
  }

  // Parse leaf node: BEGIN_NODE + name + END_NODE
  if (line.length() > 2 && static_cast<unsigned char>(line[0]) == BEGIN_NODE &&
      static_cast<unsigned char>(line[line.length() - 1]) == END_NODE) {
    std::string nodeName = trim(line.substr(1, line.length() - 2));
    return std::make_shared<QSchTree>(nodeName, currentLine);
  }

  // Parse parent node: BEGIN_NODE + name (no END_NODE)
  if (!line.empty() && static_cast<unsigned char>(line[0]) == BEGIN_NODE) {
    std::string nodeName = trim(line.substr(1));
    auto node = std::make_shared<QSchTree>(nodeName, currentLine);

    // Recursively parse children until we hit END_NODE
    while (true) {
      auto child = parseNodeRecursive(stream, currentLine);
      if (child == nullptr) {
        break; // Hit END_NODE or end of file
      }
      node->addChild(child);
      // Set parent pointer for the child
      child->setParent(node);
    }

    return node;
  }

  throw std::runtime_error("Parse error: invalid line format: " + line);
}

QSchTreePtr QSchTree::parseFromStream(std::istream &stream) {
  // Read and verify file identifier (first 4 bytes)
  unsigned char header[4];
  stream.read(reinterpret_cast<char *>(header), 4);

  if (!stream.good() || stream.gcount() != 4) {
    throw std::runtime_error("File too small or read error");
  }

  if (::memcmp(header, FILE_ID, 4)) {
    std::ostringstream oss;
    oss << "Invalid file identifier. Expected: 0x" << std::hex << std::uppercase
        << static_cast<int>(FILE_ID[0]) << " 0x" << static_cast<int>(FILE_ID[1])
        << " 0x" << static_cast<int>(FILE_ID[2]) << " 0x"
        << static_cast<int>(FILE_ID[3]) << ", Got: " << std::hex
        << std::uppercase << "0x" << static_cast<int>(header[0]) << " 0x"
        << static_cast<int>(header[1]) << " 0x" << static_cast<int>(header[2])
        << " 0x" << static_cast<int>(header[3]);
    throw std::invalid_argument(oss.str());
  }

  // Parse the tree recursively
  int currentLine = 0;
  auto root = parseNodeRecursive(stream, currentLine);

  if (root == nullptr) {
    throw std::runtime_error("Parse error: empty file or no valid root node");
  }

  return root;
}