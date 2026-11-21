#include "SchScene.h"

#include <QDebug>
#include <QGraphicsItem>

#include <SchConnDump.h>

#include "RenderStyle.h"
#include "SchItemBase.h"
#include "SchItemText.h"

namespace sch::render {

namespace {

// How often, at most, a Debug build checks the cache against a fresh
// model.
constexpr qint64 kStaleCheckMs = 1000;

}   // namespace

SchScene::SchScene(QObject *parent) : QGraphicsScene(parent) {}

SchScene *SchScene::of(const QGraphicsItem *item) {
  if (item == nullptr) return nullptr;
  return dynamic_cast<SchScene *>(item->scene());
}

void SchScene::setRefDesHidden(bool hidden) {
  if (hidden == refDesHidden_) return;
  refDesHidden_ = hidden;

  // Attribute text -- text inside a component -- changes size: hidden,
  // it has none. Only a component's first text is affected, but asking
  // each is no cheaper than telling them all.
  for (QGraphicsItem *item : items()) {
    SchItemText *text = dynamic_cast<SchItemText *>(item);
    if (text != nullptr && text->parentItem() != nullptr)
      text->displayChanged();
  }
}

void SchScene::setTextAnchorsMarked(bool marked) {
  if (marked == textAnchorsMarked_) return;
  textAnchorsMarked_ = marked;

  // The bounds do not change: an output mark lies within a text's own
  // margin, and the screen mark is the view's to draw. A repaint is all.
  update();
}

QColor SchScene::faded(const QColor &color) {
  // Two thirds of the way to the background: still its own hue, plainly
  // not the real thing.
  constexpr double kToward    = 0.65;
  const QColor     background = RenderStyle::current().background;
  return QColor(
      static_cast<int>(color.red() +
                       (background.red() - color.red()) * kToward),
      static_cast<int>(color.green() +
                       (background.green() - color.green()) * kToward),
      static_cast<int>(color.blue() +
                       (background.blue() - color.blue()) * kToward));
}

void SchScene::setRoot(const NodePtr &root) {
  root_ = root;
  model_.reset();

  // The drawing order depends on the document kind -- a symbol
  // document's top level uses the symbol order -- and the items may
  // have been added before the root was known.
  for (QGraphicsItem *item : items())
    if (SchItemBase *schItem = dynamic_cast<SchItemBase *>(item))
      if (schItem->parentItem() == nullptr) schItem->refreshDrawZ();
}

const sch::core::NetModel *SchScene::netModel() {
  if (root_ == nullptr) return nullptr;

  if (model_ == nullptr) {
    model_ = std::make_unique<sch::core::NetModel>(root_);
    return model_.get();
  }

#ifndef NDEBUG
  checkStale();
#endif
  return model_.get();
}

void SchScene::checkStale() {
  if (staleTimer.isValid() && staleTimer.elapsed() < kStaleCheckMs) return;
  staleTimer.start();

  const QString cached = sch::core::conndump::dumpModel(*model_, QString());
  const QString fresh =
      sch::core::conndump::dumpModel(sch::core::NetModel(root_), QString());
  const QString diff = sch::core::conndump::firstDifference(cached, fresh);
  if (diff.isEmpty() || diff == lastStaleReport) return;

  lastStaleReport = diff;
  qWarning().noquote() << QStringLiteral(
      "SchScene: net model served STALE (a path changed "
      "records without invalidating it), %1 (cached | fresh)")
                              .arg(diff);
}

}   // namespace sch::render
