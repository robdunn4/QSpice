#include "SchScene.h"

#include <QDebug>
#include <QGraphicsItem>

#include "SchConnDump.h"

namespace sch::render {

namespace {

// How often, at most, a Debug build checks the cache against a fresh
// model.
constexpr qint64 kStaleCheckMs = 1000;

} // namespace

SchScene::SchScene(QObject *parent) : QGraphicsScene(parent) {}

SchScene *SchScene::of(const QGraphicsItem *item) {
  if (item == nullptr) return nullptr;
  return dynamic_cast<SchScene *>(item->scene());
}

void SchScene::setRoot(const NodePtr &root) {
  root_ = root;
  model_.reset();
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
  const QString fresh  = sch::core::conndump::dumpModel(sch::core::NetModel(root_), QString());
  const QString diff   = sch::core::conndump::firstDifference(cached, fresh);
  if (diff.isEmpty() || diff == lastStaleReport) return;

  lastStaleReport = diff;
  qWarning().noquote() << QStringLiteral("SchScene: net model served STALE (a path changed "
                                         "records without invalidating it), %1 (cached | fresh)")
                              .arg(diff);
}

} // namespace sch::render
