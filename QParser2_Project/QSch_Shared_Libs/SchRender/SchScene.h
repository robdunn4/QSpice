#pragma once

#include <QElapsedTimer>
#include <QGraphicsScene>
#include <QString>

#include <memory>

#include "ItemTree.h"
#include "SchNetModel.h"

class QGraphicsItem;

namespace sch::render {

// The scene a document view shows: a QGraphicsScene that also knows the
// document's record tree and keeps its connectivity (sch::core::NetModel).
//
// The model is CACHED and rebuilt lazily -- on the first request after
// invalidateNetModel(). SchView invalidates it on every input event it
// handles (a drag changes records long before anything reaches the undo
// stack), on every undo-stack change and when the tree is replaced.
// Anything else that changes records outside those paths must call
// invalidateNetModel() itself.
//
// Items find it with SchScene::of(this). A scene that is NOT a SchScene
// -- the symbol preview -- gets null, and the items fall back to their
// scene-searching code.
//
// No new signals or slots, so no Q_OBJECT and no moc.
class SchScene : public QGraphicsScene {
public:
  explicit SchScene(QObject *parent = nullptr);

  // The SchScene an item is in; null if it is in none, or in a plain
  // QGraphicsScene.
  static SchScene *of(const QGraphicsItem *item);

  // The document's sheet root. Invalidates the model.
  void           setRoot(const NodePtr &root);
  const NodePtr &root() const { return root_; }

  // The connectivity of the document as it stands; null if there is no
  // tree (an ASCII document). Rebuilt here if anything has changed.
  //
  // Debug builds compare a cached model with a fresh one now and then
  // (at most once a second) and log a difference once: a cache served
  // stale means some path changed records without invalidating it.
  const sch::core::NetModel *netModel();

  void invalidateNetModel() { model_.reset(); }

protected:
  void checkStale();

  NodePtr                      root_;
  std::unique_ptr<sch::core::NetModel> model_;

  QElapsedTimer staleTimer;
  QString       lastStaleReport;
};

} // namespace sch::render
