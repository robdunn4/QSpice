//-----------------------------------------------------------------------------
// This file is part of the the QParser2 project.  You can find the complete
// project here:  https://github.com/robdunn4/QSpice/
//-----------------------------------------------------------------------------
/*
 * ItemTree.h -- Tree of ItemBase nodes.
 *
 * Structure:
 *   - TreeNode wraps an ItemBasePtr plus tree links (parent, children).
 *     ItemBase itself is not modified.
 *   - Children are owned by their parent via shared_ptr<TreeNode>; the
 *     parent link is a weak_ptr to avoid ownership cycles.
 *   - Only container nodes (item->isContainer()) may have children.  The
 *     instance-level NodeType is what matters, so a RECT promoted to a
 *     container via setNodeType() is accepted.
 *
 * QPI-sorted invariant:
 *   - A container's children are kept sorted by QPI sort order
 *     (ItemTypes::sortOrder, i.e. the declaration order in QPI_ITEMS).
 *   - Ties are allowed: multiple children of the same QPI type form a
 *     contiguous "run" within the sorted order.
 *   - addFirst() / addLast() preserve this invariant.  addBefore() /
 *     addAfter() do NOT -- see their comments.
 *
 * Usage sketch:
 *   ItemTree tree;
 *   tree.setRoot(ItemBase::makeItem(QPI::SCH));
 *   auto sym = tree.root()->addLast(ItemBase::makeItem(QPI::SYM));
 *   sym->addLast(ItemBase::makeItem(QPI::PIN));
 *   tree.preOrder([](const NodePtr& n, int depth) {
 *     // visit ...
 *   });
 */
#pragma once
#include "ItemBase.h"
#include "ItemTypes.h"
#include <functional>
#include <memory>
#include <optional>
#include <vector>

class TreeNode;
using NodePtr     = std::shared_ptr<TreeNode>;
using NodeWeakPtr = std::weak_ptr<TreeNode>;
using NodeList    = std::vector<NodePtr>;
// typedef std::vector<NodePtr> NodeList;

//-----------------------------------------------------------------------------
// TreeNode -- one node in the tree.  Holds the ItemBasePtr payload plus
// parent/children links.  All navigation and per-node mutation lives here.
//
// Instances must be managed by shared_ptr (created via std::make_shared or
// the ItemTree factory).  Do not stack-allocate -- several methods rely on
// shared_from_this() to hand out owning references to children/parents.
//-----------------------------------------------------------------------------
class TreeNode : public std::enable_shared_from_this<TreeNode> {
public:
  // Factory.  Prefer this over direct construction so the returned node is
  // always owned by a shared_ptr.
  static NodePtr make(ItemBasePtr item) {
    return std::shared_ptr<TreeNode>(new TreeNode(std::move(item)));
  }

  // ---- payload access ----
  const ItemBasePtr &item() const { return item_; }
  ItemBasePtr       &item() { return item_; }
  QPI                getEnumID() const { return item_->getEnumID(); }
  bool               isContainer() const { return item_->isContainer(); }
  bool               isLeaf() const { return item_->isLeaf(); }

  // ---- navigation ----
  NodePtr parent() const { return parent_.lock(); }

  NodePtr firstChild() const {
    return children_.empty() ? nullptr : children_.front();
  }
  NodePtr lastChild() const {
    return children_.empty() ? nullptr : children_.back();
  }

  // Immediate siblings under the same parent.  Returns nullptr at the ends
  // or if this node has no parent.
  NodePtr nextSibling() const;
  NodePtr prevSibling() const;

  // First/last child of the given QPI type, or nullptr if none.  Uses the
  // sorted invariant -- O(log n) via binary search.
  NodePtr findFirstOf(QPI qpi) const;
  NodePtr findLastOf(QPI qpi) const;

  // Find-or-insert.  If a child of type qpi exists, return the first/last
  // such child.  Otherwise construct an empty item of that type via
  // ItemBase::makeItem(qpi), insert it at the correct sorted position, and
  // return the new node.  Fails (returns nullptr) if this node is not a
  // container or if makeItem returns null.
  NodePtr findOrAddFirstOf(QPI qpi);
  NodePtr findOrAddLastOf(QPI qpi);

  // Read-only access to the children vector, for callers that want to
  // iterate without invoking a visitor.
  const std::vector<NodePtr> &children() const { return children_; }
  size_t                      childCount() const { return children_.size(); }
  bool                        hasChildren() const { return !children_.empty(); }

  // Index of this node within its parent's children vector, or nullopt if
  // this node has no parent.
  std::optional<size_t> indexInParent() const;

  // ---- mutation: QPI-sorted inserts ----

  // Insert newChild as the FIRST child of its QPI-type run within this
  // container.  Equivalent to "put it before any existing same-type
  // siblings, still respecting overall QPI sort order."  Returns the
  // inserted node, or nullptr on failure (this is not a container, or
  // newChild is null).
  NodePtr addFirst(ItemBasePtr item);
  NodePtr addFirst(NodePtr newChild);

  // Insert newChild as the LAST child of its QPI-type run.  Same failure
  // modes as addFirst.
  NodePtr addLast(ItemBasePtr item);
  NodePtr addLast(NodePtr newChild);

  // ---- mutation: positional inserts (trust-the-caller) ----

  // Insert newNode immediately before/after this node, under this node's
  // parent.  Returns the inserted node, or nullptr on failure.
  //
  // WARNING: these do NOT preserve the QPI-sorted invariant.  If the
  // caller inserts a node of the wrong sort order relative to its
  // neighbors, subsequent findFirstOf/findLastOf and addFirst/addLast
  // calls on the parent may return incorrect results (they rely on
  // binary search over sorted children).  In debug builds the tree
  // asserts the invariant after every mutation -- if you break it, you
  // will find out quickly.  The caller is responsible for maintaining
  // correctness.
  NodePtr addBefore(ItemBasePtr item);
  NodePtr addBefore(NodePtr newNode);
  NodePtr addAfter(ItemBasePtr item);
  NodePtr addAfter(NodePtr newNode);

  // ---- mutation: removal ----

  // Detach this node from its parent and return it (still valid, now
  // parentless, caller may reattach it elsewhere).  Returns nullptr if
  // this node has no parent (e.g. it is the tree root -- use
  // ItemTree::clear() for that).
  NodePtr detach();

  // Remove all children of this container.  No-op on a leaf or an already
  // empty container.
  void clearChildren();

private:
  explicit TreeNode(ItemBasePtr item) : item_(std::move(item)) {}

  // Binary-search helpers keyed on QPI sort order.
  size_t lowerBoundFor(QPI qpi) const;
  size_t upperBoundFor(QPI qpi) const;

  // Insert at a specific index, fixing up parent links.  Shared back end
  // for all the add* methods.
  NodePtr insertAt(size_t index, NodePtr newChild);

  // Debug-only: verify children_ are sorted by QPI sort order.  Compiled
  // out in release builds.
  void assertSortedInvariant() const;

  ItemBasePtr          item_;
  NodeWeakPtr          parent_;
  std::vector<NodePtr> children_;

  friend class ItemTree;
};

//-----------------------------------------------------------------------------
// ItemTree -- thin owner of the root node plus whole-tree operations.
//-----------------------------------------------------------------------------
class ItemTree {
public:
  // Visitor signature used by all three traversals.  Depth of the root is
  // 0.  Callers that don't care about depth can take it as an unused
  // parameter: `[](const NodePtr& n, int) { ... }`.
  using Visitor = std::function<void(const NodePtr &, int depth)>;

  ItemTree() = default;

  // Root management.
  NodePtr root() const { return root_; }
  NodePtr setRoot(ItemBasePtr item);
  NodePtr setRoot(NodePtr node);
  void    clear() { root_.reset(); }
  bool    empty() const { return !root_; }

  // Deep copy of the whole tree.  Each node's item is cloned via
  // ItemBase::clone().
  ItemTree clone() const;

  // ---- whole-tree traversals ----
  // All three are no-ops on an empty tree.

  // Pre-order DFS: visit node, then recurse children left-to-right.
  void preOrder(const Visitor &v) const;

  // Post-order DFS: recurse children left-to-right, then visit node.
  void postOrder(const Visitor &v) const;

  // Breadth-first / level-order.
  void breadthFirst(const Visitor &v) const;

private:
  static void    preOrderImpl(const NodePtr &n, int depth, const Visitor &v);
  static void    postOrderImpl(const NodePtr &n, int depth, const Visitor &v);
  static NodePtr cloneSubtree(const NodePtr &src);

  NodePtr root_;
};
