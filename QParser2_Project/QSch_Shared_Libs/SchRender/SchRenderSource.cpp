#include "SchRenderSource.h"

#include <QGraphicsItem>
#include <QGraphicsScene>
#include <QList>
#include <QPainter>

#include "SelHiLiteItem.h"

namespace sch::render {

QRectF SchRenderSource::printableSceneRect() const {
  QRectF          extent;
  QGraphicsScene *scene = renderScene();
  if (scene == nullptr) return extent;

  for (const QGraphicsItem *item : scene->items()) {
    if (!item->isVisible() || isPrintExcluded(item)) continue;
    extent |= item->sceneBoundingRect();
  }

  // Only with something to show: the underlay alone is not a drawing.
  if (!extent.isEmpty()) extent |= underlayExtent();
  return extent;
}

bool SchRenderSource::hasPrintableIn(const QRectF &sceneRect) const {
  QGraphicsScene *scene = renderScene();
  if (scene == nullptr) return false;

  const QList<QGraphicsItem *> found =
      scene->items(sceneRect, Qt::IntersectsItemBoundingRect);
  for (const QGraphicsItem *item : found)
    if (item->isVisible() && !isPrintExcluded(item)) return true;
  return false;
}

void SchRenderSource::renderSceneForPrint(QPainter     *painter,
                                          const QRectF &target,
                                          const QRectF &source,
                                          bool          withBackground) const {
  QGraphicsScene *scene = renderScene();
  if (scene == nullptr || painter == nullptr) return;

  // Everything that is not the document goes out of sight for the
  // duration, and back afterwards exactly as it was.
  QList<QGraphicsItem *> hidden;
  for (QGraphicsItem *item : scene->items()) {
    if (!item->isVisible() || !isPrintExcluded(item)) continue;
    item->setVisible(false);
    hidden.append(item);
  }

  painter->save();
  painter->setClipRect(target, Qt::IntersectClip);

  // The source's background; the scene has none of its own, and the
  // grid belongs to the editor's view, so it is not drawn.
  if (withBackground) painter->fillRect(target, renderBackground());

  // Scene Y runs UPWARD on screen; QGraphicsScene::render() maps the
  // source's top (smallest Y) to the target's top. Flipped about the
  // target's horizontal center line, the page matches the screen.
  painter->translate(0.0, target.top() + target.bottom());
  painter->scale(1.0, -1.0);

  SelHiLiteStyle::suppressed = true;

  // The underlay, in scene units: the same mapping render() uses below.
  painter->save();
  painter->translate(target.left(), target.top());
  painter->scale(target.width() / source.width(),
                 target.height() / source.height());
  painter->translate(-source.left(), -source.top());
  renderUnderlay(painter, source);
  painter->restore();

  scene->render(painter, target, source, Qt::IgnoreAspectRatio);
  SelHiLiteStyle::suppressed = false;

  painter->restore();

  for (QGraphicsItem *item : hidden) item->setVisible(true);
}

} // namespace sch::render
