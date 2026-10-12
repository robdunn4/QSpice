#include "SchTreeBuilder.h"

#include <memory>
#include <utility>

#include "ItemArc3P.h"
#include "ItemCmp.h"
#include "ItemCoil.h"
#include "ItemEllipse.h"
#include "ItemJunction.h"
#include "ItemLine.h"
#include "ItemNet.h"
#include "ItemPin.h"
#include "ItemRect.h"
#include "ItemSym.h"
#include "ItemTap.h"
#include "ItemText.h"
#include "ItemWire.h"
#include "ItemZigzag.h"
#include "SchItemArc3P.h"
#include "SchItemArc4P.h"
#include "SchItemCoil.h"
#include "SchItemComp.h"
#include "SchItemEllipse.h"
#include "SchItemJunction.h"
#include "SchItemLine.h"
#include "SchItemNet.h"
#include "SchItemPin.h"
#include "SchItemRect.h"
#include "SchItemTap.h"
#include "SchItemText.h"
#include "SchItemTriangle.h"
#include "SchItemWire.h"
#include "SchItemZigzag.h"
#include "SelHiLiteItem.h"

namespace sch::render {

namespace {

// Constructs a T with args, wrapped in SelHiLiteItem<T> when topLevel is
// true. Only top-level items are ever independently selectable (see
// each wrapper's own constructor) -- children of a SchItemComp never
// are, so wrapping them would add the highlight machinery for no
// visual effect, since isSelected() can never become true for them.
template <typename T, typename... Args>
QGraphicsItem *makeLeafItem(bool topLevel, Args &&...args) {
  if (topLevel) return new SelHiLiteItem<T>(std::forward<Args>(args)...);
  return new T(std::forward<Args>(args)...);
}

// Builds a leaf wrapper for node, parented under parentItem (nullptr for
// a top-level, scene-direct child). Returns nullptr if there's no
// wrapper class for this element type yet.
QGraphicsItem *buildLeafForType(const NodePtr &node,
                                QGraphicsItem *parentItem) {
  const bool topLevel = (parentItem == nullptr);
  switch (node->getEnumID()) {
  case QPI::RECT:
    return makeLeafItem<SchItemRect>(
        topLevel, std::static_pointer_cast<ItemRect>(node->item()), parentItem);
  case QPI::LINE:
    return makeLeafItem<SchItemLine>(
        topLevel, std::static_pointer_cast<ItemLine>(node->item()), parentItem);
  case QPI::WIRE:
    return makeLeafItem<SchItemWire>(
        topLevel, std::static_pointer_cast<ItemWire>(node->item()), parentItem);
  case QPI::TAP:
    return makeLeafItem<SchItemTap>(
        topLevel, std::static_pointer_cast<ItemTap>(node->item()), parentItem);
  case QPI::ELLIPSE:
    return makeLeafItem<SchItemEllipse>(
        topLevel, std::static_pointer_cast<ItemEllipse>(node->item()),
        parentItem);
  case QPI::ARC3P:
    return makeLeafItem<SchItemArc3P>(
        topLevel, std::static_pointer_cast<ItemArc3P>(node->item()), parentItem);
  case QPI::ARC4P:
    return makeLeafItem<SchItemArc4P>(
        topLevel, std::static_pointer_cast<ItemArc4P>(node->item()),
        parentItem);
  case QPI::TRIANGLE:
    return makeLeafItem<SchItemTriangle>(
        topLevel, std::static_pointer_cast<ItemTriangle>(node->item()),
        parentItem);

  case QPI::ZIGZAG:
    return makeLeafItem<SchItemZigzag>(
        topLevel, std::static_pointer_cast<ItemZigzag>(node->item()),
        parentItem);
  case QPI::JUNCTION:
    return makeLeafItem<SchItemJunction>(
        topLevel, std::static_pointer_cast<ItemJunction>(node->item()),
        parentItem);
  case QPI::COIL:
    return makeLeafItem<SchItemCoil>(
        topLevel, std::static_pointer_cast<ItemCoil>(node->item()), parentItem);
  case QPI::PIN:
    return makeLeafItem<SchItemPin>(
        topLevel, std::static_pointer_cast<ItemPin>(node->item()), parentItem);
  case QPI::TEXT:
    return makeLeafItem<SchItemText>(
        topLevel, std::static_pointer_cast<ItemText>(node->item()), parentItem);
  case QPI::NET:
    return makeLeafItem<SchItemNet>(
        topLevel, std::static_pointer_cast<ItemNet>(node->item()), parentItem);
  default:
    return nullptr;
  }
}

// Builds the wrapper and links it back to the node it came from, so
// later edits can add or remove tree nodes without hunting for them.
QGraphicsItem *buildLeaf(const NodePtr &node, QGraphicsItem *parentItem) {
  QGraphicsItem *item = buildLeafForType(node, parentItem);
  if (SchItemBase *schItem = dynamic_cast<SchItemBase *>(item))
    schItem->setNode(node);
  return item;
}

// Builds a component and its flattened symbol children, WITHOUT adding
// it to any scene -- the caller decides where it goes. Paste needs the
// items before they are in a scene, so it can position them relative to
// the cursor first.
QGraphicsItem *buildComponent(const NodePtr &compNode) {
  SelHiLiteItem<SchItemComp> *compItem = new SelHiLiteItem<SchItemComp>(
      std::static_pointer_cast<ItemCmp>(compNode->item()));
  compItem->setNode(compNode);

  // Flatten: see the design note in SchTreeBuilder.h.
  if (NodePtr symNode = compNode->findFirstOf(QPI::SYM))
    for (const NodePtr &child : symNode->children()) buildLeaf(child, compItem);

  return compItem;
}

} // namespace

namespace SchTreeBuilder {

QList<QGraphicsItem *> buildTopLevelItems(const ItemTree &tree) {
  QList<QGraphicsItem *> items;

  NodePtr root = tree.root();
  if (!root) return items;

  for (const NodePtr &child : root->children()) {
    QGraphicsItem *item = (child->getEnumID() == QPI::COMP)
                              ? buildComponent(child)
                              : buildLeaf(child, nullptr);
    if (item != nullptr) items.append(item);
  }

  return items;
}

void populateScene(const ItemTree &tree, QGraphicsScene *scene) {
  for (QGraphicsItem *item : buildTopLevelItems(tree)) scene->addItem(item);
}

} // namespace SchTreeBuilder

} // namespace sch::render
