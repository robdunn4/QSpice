//-----------------------------------------------------------------------------
// This file is part of the the QParser2 project.  You can find the complete
// project here:  https://github.com/robdunn4/QSpice/
//-----------------------------------------------------------------------------
/*
 * QSchTree.h -- Schematic or symbol tree.
 */
#pragma once
#include "QItemBase.h"
#include <memory>
#include <string>
#include <vector>

// fwd decls
class QSchTree;
typedef std::shared_ptr<QSchTree> QSchTreePtr;

class QSchTree {
public:
  static const unsigned char BEGIN_NODE = 0xAB;
  static const unsigned char END_NODE = 0xBB;

  // File identifier bytes
  static constexpr unsigned char FILE_ID[4] = {0xFF, 0xD8, 0xFF, 0xDB};

  std::string data;
  std::vector<QSchTreePtr> children;
  int lineNbr;

  std::string typeStr;
  std::string dataStr;
  QPI enumID;
  QItemBasePtr itemPtr;

private:
  std::weak_ptr<QSchTree> parent; // Parent node (weak to avoid circular refs)

public:
  // Constructor that takes a string and parses it
  QSchTree(const std::string &value, int line = 0);

  // Copy constructor for cloning (creates deep copy)
  QSchTree(const QSchTree &other);

  // Copy assignment operator (creates deep copy)
  QSchTree &operator=(const QSchTree &other);

  // return node string (without beg/end markers)
  std::string toString() const;

  // Virtual destructor
  virtual ~QSchTree() = default;

  // Add a child node (automatically sets parent pointer)
  void addChild(QSchTreePtr child);

  // Add a child with a string value (automatically sets parent pointer)
  void addChild(const std::string &value);

  // Insert a child at specific index (automatically sets parent pointer)
  void insertChild(size_t index, QSchTreePtr child);

  // Remove a child by index
  void removeChild(size_t index);

  // Remove a specific child
  void removeChild(const QSchTreePtr &child);

  // Parse data string into typeStr and dataStr
  void parseData();

  // Navigation methods
  QSchTreePtr getParent() const;
  QSchTreePtr getFirstChild() const;
  QSchTreePtr getLastChild() const;
  QSchTreePtr getChild(size_t index) const;
  QSchTreePtr getNextSibling() const;
  QSchTreePtr getPrevSibling() const;

  // Query methods
  bool isRoot() const;
  bool isLeaf() const;
  size_t getChildCount() const;
  size_t getSiblingIndex() const;

  // Create a deep copy of this tree and all children
  QSchTreePtr clone() const;

  // Rebuild parent pointers for this node and all descendants
  void rebuildParentPointers();

  // Depth-first traversal with indentation
  void printTree(int indent = 0) const;

  // Breadth-first traversal
  void printBreadthFirst() const;

  // Print with custom prefix using ASCII characters
  void printWithPrefix(const std::string &prefix = "",
                       bool isLast = true) const;

  // Write tree to stream
  void writeToStream(std::ostream &stream) const;

  // Parse tree from stream
  static QSchTreePtr parseFromStream(std::istream &stream);

protected:
  // Set parent pointer (used internally when building tree)
  void setParent(std::weak_ptr<QSchTree> parentPtr);

  // Recursive helper for rebuilding parent pointers
  void rebuildParentPointersRecursive(QSchTreePtr self);

  // Recursive helper to write node to stream
  void writeNodeRecursive(std::ostream &stream, int indent = 0) const;

  // Recursive helper function to parse nodes from a stream
  static QSchTreePtr parseNodeRecursive(std::istream &stream, int &currentLine);
};