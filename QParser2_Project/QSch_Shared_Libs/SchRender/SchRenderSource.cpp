#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable : 4996) // Qt headers reference APIs deprecated for Qt 7
#endif

#include "SchRenderSource.h"

#include <QGraphicsItem>
#include <QGraphicsScene>
#include <QList>
#include <QPainter>

#ifdef _MSC_VER
#pragma warning(pop)
#endif

#include "SelHiLiteItem.h"

namespace sch::render {

QRectF SchRenderSource::printableSceneRect() const {
  QRectF extent;
  QGraphicsScene *scene = renderScene();
  if (scene == nullptr) return extent;

  for (const QGraphicsItem *item : scene->items()) {
    if (!item->isVisible() || isPrintExcluded(item)) continue;
    extent |= item->sceneBoundingRect();
  }
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

void SchRenderSource::renderSceneForPrint(QPainter *painter, const QRectF &target,
                                          const QRectF &source, bool withBackground) const {
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
  // target's horizontal centre line, the page matches the screen.
  painter->translate(0.0, target.top() + target.bottom());
  painter->scale(1.0, -1.0);

  SelHiLiteStyle::suppressed = true;
  scene->render(painter, target, source, Qt::IgnoreAspectRatio);
  SelHiLiteStyle::suppressed = false;

  painter->restore();

  for (QGraphicsItem *item : hidden) item->setVisible(true);
}

} // namespace sch::render
