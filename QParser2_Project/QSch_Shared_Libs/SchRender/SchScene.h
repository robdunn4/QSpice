#pragma once

#include <QColor>
#include <QElapsedTimer>
#include <QGraphicsScene>
#include <QString>

#include <memory>

#include <ItemTree.h>
#include <SchNetModel.h>

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

  // Hide Reference Designators: each component's designator -- its
  // first attribute text, R1, Q3 -- is not drawn, and so cannot be
  // clicked, printed or exported. Display only: the document is
  // unchanged. Text the scene gains later follows it (see
  // SchItemText::isDisplayed()).
  void setRefDesHidden(bool hidden);
  bool refDesHidden() const { return refDesHidden_; }

  // Mark Text Anchor Points: each displayed text element -- top-level and
  // in symbols, not net or pin labels -- shows a small black circle where
  // it is anchored. Display only, but drawn in EVERY output too while on:
  // printed and exported as shown (Robert, 2026-11-16).
  void setTextAnchorsMarked(bool marked);
  bool textAnchorsMarked() const { return textAnchorsMarked_; }

  // A SYMBOL document's scene: the symbol's HIDDEN parts -- texts with the
  // hidden bit, internal pins' boxes -- are drawn FADED rather than left
  // out, and can be picked and edited like anything else, as QSpice's
  // symbol editor does (Robert, 2026-11-17; VisTest, 2026-11-18). A
  // hidden pin label is not drawn even so. On screen and in output
  // alike: what is printed is what is seen.
  void setSymbolDocument(bool symbol) { symbolDocument_ = symbol; }
  bool symbolDocument() const { return symbolDocument_; }

  // A color as a hidden part is drawn: `color` faded toward the canvas.
  static QColor faded(const QColor &color);

protected:
  void checkStale();

  NodePtr                              root_;
  bool                                 refDesHidden_ = false;
  bool                                 textAnchorsMarked_ = false;
  bool                                 symbolDocument_    = false;
  std::unique_ptr<sch::core::NetModel> model_;

  QElapsedTimer staleTimer;
  QString       lastStaleReport;
};

}   // namespace sch::render
