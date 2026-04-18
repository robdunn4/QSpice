//-----------------------------------------------------------------------------
// This file is part of the the QParser2 project.  You can find the complete
// project here:  https://github.com/robdunn4/QSpice/
//-----------------------------------------------------------------------------
#include "ItemTree.h"
#include <algorithm>
#include <cassert>
#include <deque>


//-----------------------------------------------------------------------------
// TreeNode -- navigation
//-----------------------------------------------------------------------------

std::optional<size_t> TreeNode::indexInParent() const {
  auto p = parent_.lock();
  if (!p) return std::nullopt;
  for (size_t i = 0; i < p->children_.size(); ++i) {
    if (p->children_[i].get() == this) return i;
  }
  return std::nullopt; // shouldn't happen if links are consistent
}

NodePtr TreeNode::nextSibling() const {
  auto p = parent_.lock();
  if (!p) return nullptr;
  auto idx = indexInParent();
  if (!idx || *idx + 1 >= p->children_.size()) return nullptr;
  return p->children_[*idx + 1];
}

NodePtr TreeNode::prevSibling() const {
  auto p = parent_.lock();
  if (!p) return nullptr;
  auto idx = indexInParent();
  if (!idx || *idx == 0) return nullptr;
  return p->children_[*idx - 1];
}

//-----------------------------------------------------------------------------
// TreeNode -- sorted-search helpers
//-----------------------------------------------------------------------------

size_t TreeNode::lowerBoundFor(QPI qpi) const {
  const int key = ItemTypes::sortOrder(qpi);
  auto      it  = std::lower_bound(
      children_.begin(), children_.end(), key,
      [](const NodePtr &n, int k) {
        return ItemTypes::sortOrder(n->getEnumID()) < k;
      });
  return static_cast<size_t>(it - children_.begin());
}

size_t TreeNode::upperBoundFor(QPI qpi) const {
  const int key = ItemTypes::sortOrder(qpi);
  auto      it  = std::upper_bound(
      children_.begin(), children_.end(), key,
      [](int k, const NodePtr &n) {
        return k < ItemTypes::sortOrder(n->getEnumID());
      });
  return static_cast<size_t>(it - children_.begin());
}

NodePtr TreeNode::findFirstOf(QPI qpi) const {
  size_t i = lowerBoundFor(qpi);
  if (i < children_.size() && children_[i]->getEnumID() == qpi)
    return children_[i];
  return nullptr;
}

NodePtr TreeNode::findLastOf(QPI qpi) const {
  size_t i = upperBoundFor(qpi);
  if (i == 0) return nullptr;
  const NodePtr &candidate = children_[i - 1];
  if (candidate->getEnumID() == qpi) return candidate;
  return nullptr;
}

//-----------------------------------------------------------------------------
// TreeNode -- insertion back end
//-----------------------------------------------------------------------------

NodePtr TreeNode::insertAt(size_t index, NodePtr newChild) {
  if (!newChild) return nullptr;
  if (!isContainer()) return nullptr;

  // Detach newChild from any previous parent.  If it was already somewhere,
  // remove it cleanly so we don't end up with stale links.
  if (auto oldParent = newChild->parent_.lock()) {
    auto &sibs = oldParent->children_;
    sibs.erase(
        std::remove_if(sibs.begin(), sibs.end(),
                       [&](const NodePtr &n) { return n == newChild; }),
        sibs.end());
  }

  if (index > children_.size()) index = children_.size();
  children_.insert(children_.begin() + index, newChild);
  newChild->parent_ = weak_from_this();

  assertSortedInvariant();
  return newChild;
}

//-----------------------------------------------------------------------------
// TreeNode -- QPI-sorted inserts
//-----------------------------------------------------------------------------

NodePtr TreeNode::addFirst(NodePtr newChild) {
  if (!newChild) return nullptr;
  if (!isContainer()) return nullptr;
  size_t idx = lowerBoundFor(newChild->getEnumID());
  return insertAt(idx, std::move(newChild));
}

NodePtr TreeNode::addFirst(ItemBasePtr item) {
  if (!item) return nullptr;
  return addFirst(TreeNode::make(std::move(item)));
}

NodePtr TreeNode::addLast(NodePtr newChild) {
  if (!newChild) return nullptr;
  if (!isContainer()) return nullptr;
  size_t idx = upperBoundFor(newChild->getEnumID());
  return insertAt(idx, std::move(newChild));
}

NodePtr TreeNode::addLast(ItemBasePtr item) {
  if (!item) return nullptr;
  return addLast(TreeNode::make(std::move(item)));
}

//-----------------------------------------------------------------------------
// TreeNode -- find-or-insert
//-----------------------------------------------------------------------------

NodePtr TreeNode::findOrAddFirstOf(QPI qpi) {
  if (auto existing = findFirstOf(qpi)) return existing;
  if (!isContainer()) return nullptr;
  auto item = ItemBase::makeItem(qpi);
  if (!item) return nullptr;
  return addFirst(std::move(item));
}

NodePtr TreeNode::findOrAddLastOf(QPI qpi) {
  if (auto existing = findLastOf(qpi)) return existing;
  if (!isContainer()) return nullptr;
  auto item = ItemBase::makeItem(qpi);
  if (!item) return nullptr;
  return addLast(std::move(item));
}

//-----------------------------------------------------------------------------
// TreeNode -- positional inserts (trust-the-caller)
//-----------------------------------------------------------------------------

NodePtr TreeNode::addBefore(NodePtr newNode) {
  if (!newNode) return nullptr;
  auto p = parent_.lock();
  if (!p) return nullptr;
  auto idx = indexInParent();
  if (!idx) return nullptr;
  return p->insertAt(*idx, std::move(newNode));
}

NodePtr TreeNode::addBefore(ItemBasePtr item) {
  if (!item) return nullptr;
  return addBefore(TreeNode::make(std::move(item)));
}

NodePtr TreeNode::addAfter(NodePtr newNode) {
  if (!newNode) return nullptr;
  auto p = parent_.lock();
  if (!p) return nullptr;
  auto idx = indexInParent();
  if (!idx) return nullptr;
  return p->insertAt(*idx + 1, std::move(newNode));
}

NodePtr TreeNode::addAfter(ItemBasePtr item) {
  if (!item) return nullptr;
  return addAfter(TreeNode::make(std::move(item)));
}

//-----------------------------------------------------------------------------
// TreeNode -- removal
//-----------------------------------------------------------------------------

NodePtr TreeNode::detach() {
  auto p = parent_.lock();
  if (!p) return nullptr;
  auto idx = indexInParent();
  if (!idx) return nullptr;

  // Hold a strong ref so we can safely return after erase.
  NodePtr self = p->children_[*idx];
  p->children_.erase(p->children_.begin() + *idx);
  self->parent_.reset();
  p->assertSortedInvariant();
  return self;
}

void TreeNode::clearChildren() {
  if (children_.empty()) return;
  for (auto &c : children_) c->parent_.reset();
  children_.clear();
}

//-----------------------------------------------------------------------------
// TreeNode -- debug invariant
//-----------------------------------------------------------------------------

void TreeNode::assertSortedInvariant() const {
#ifndef NDEBUG
  for (size_t i = 1; i < children_.size(); ++i) {
    int a = ItemTypes::sortOrder(children_[i - 1]->getEnumID());
    int b = ItemTypes::sortOrder(children_[i]->getEnumID());
    assert(a <= b && "TreeNode children not in QPI sort order -- "
                     "addBefore/addAfter misuse?");
  }
#endif
}

//-----------------------------------------------------------------------------
// ItemTree -- root management
//-----------------------------------------------------------------------------

NodePtr ItemTree::setRoot(ItemBasePtr item) {
  if (!item) { root_.reset(); return nullptr; }
  root_ = TreeNode::make(std::move(item));
  return root_;
}

NodePtr ItemTree::setRoot(NodePtr node) {
  root_ = std::move(node);
  if (root_) root_->parent_.reset();
  return root_;
}

//-----------------------------------------------------------------------------
// ItemTree -- traversals
//-----------------------------------------------------------------------------

void ItemTree::preOrderImpl(const NodePtr &n, int depth, const Visitor &v) {
  v(n, depth);
  for (const auto &c : n->children_) preOrderImpl(c, depth + 1, v);
}

void ItemTree::postOrderImpl(const NodePtr &n, int depth, const Visitor &v) {
  for (const auto &c : n->children_) postOrderImpl(c, depth + 1, v);
  v(n, depth);
}

void ItemTree::preOrder(const Visitor &v) const {
  if (root_) preOrderImpl(root_, 0, v);
}

void ItemTree::postOrder(const Visitor &v) const {
  if (root_) postOrderImpl(root_, 0, v);
}

void ItemTree::breadthFirst(const Visitor &v) const {
  if (!root_) return;
  std::deque<std::pair<NodePtr, int>> q;
  q.emplace_back(root_, 0);
  while (!q.empty()) {
    auto [n, depth] = q.front();
    q.pop_front();
    v(n, depth);
    for (const auto &c : n->children_) q.emplace_back(c, depth + 1);
  }
}

//-----------------------------------------------------------------------------
// ItemTree -- clone
//-----------------------------------------------------------------------------

NodePtr ItemTree::cloneSubtree(const NodePtr &src) {
  if (!src) return nullptr;
  auto copy = TreeNode::make(src->item_->clone());
  copy->children_.reserve(src->children_.size());
  for (const auto &c : src->children_) {
    auto childCopy     = cloneSubtree(c);
    childCopy->parent_ = copy;
    copy->children_.push_back(std::move(childCopy));
  }
  return copy;
}

ItemTree ItemTree::clone() const {
  ItemTree out;
  out.root_ = cloneSubtree(root_);
  return out;
}
