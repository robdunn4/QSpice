#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable : 4996) // Qt headers reference APIs deprecated for Qt 7
#endif

#include "DocView.h"

#include <QDragEnterEvent>
#include <QDragMoveEvent>
#include <QDropEvent>
#include <QFileInfo>
#include <QGraphicsItem>
#include <QMimeData>
#include <QMouseEvent>
#include <QUrl>

#ifdef _MSC_VER
#pragma warning(pop)
#endif

#include "SchItemComp.h"

namespace {

// How far from a component's line work a click still hits it, in SCREEN
// pixels.
constexpr double kPickTolerancePx = 4.0;

} // namespace

DocView::DocView(QWidget *parent) : SchViewBase(parent) { setAcceptDrops(true); }

QStringList DocView::droppableFiles(const QMimeData *mime) {
  if (mime == nullptr || !mime->hasUrls()) return QStringList();

  QStringList paths;
  for (const QUrl &url : mime->urls()) {
    if (!url.isLocalFile()) return QStringList();

    const QFileInfo info(url.toLocalFile());
    if (!info.isFile()) return QStringList(); // a folder, or gone

    const QString suffix = info.suffix();
    if (suffix.compare(QLatin1String("qsch"), Qt::CaseInsensitive) != 0 &&
        suffix.compare(QLatin1String("qsym"), Qt::CaseInsensitive) != 0)
      return QStringList();

    paths.append(info.filePath());
  }
  return paths;
}

// Overridden at all because QGraphicsView passes drags to its scene,
// which refuses them; these take them for the window instead.
void DocView::dragEnterEvent(QDragEnterEvent *event) {
  if (droppableFiles(event->mimeData()).isEmpty()) {
    event->ignore();
    return;
  }
  event->acceptProposedAction();
}

void DocView::dragMoveEvent(QDragMoveEvent *event) { event->acceptProposedAction(); }

void DocView::dropEvent(QDropEvent *event) {
  const QStringList paths = droppableFiles(event->mimeData());
  if (paths.isEmpty()) {
    event->ignore();
    return;
  }

  event->acceptProposedAction();
  emit filesDropped(paths);
}

void DocView::mouseDoubleClickEvent(QMouseEvent *event) {
  if (event->button() != Qt::LeftButton) {
    SchViewBase::mouseDoubleClickEvent(event);
    return;
  }

  const NodePtr compNode = componentAt(event->position().toPoint());
  if (compNode == nullptr) {
    SchViewBase::mouseDoubleClickEvent(event);
    return;
  }

  event->accept();
  emit componentDoubleClicked(compNode);
}

NodePtr DocView::componentAt(const QPoint &viewPos) const {
  if (scene() == nullptr) return nullptr;

  const double scale = transform().m11();
  const double tolerance = (scale > 0.0) ? kPickTolerancePx / scale : kPickTolerancePx;

  const QPointF scenePos = mapToScene(viewPos);
  const QRectF probe(scenePos.x() - tolerance, scenePos.y() - tolerance, 2.0 * tolerance,
                     2.0 * tolerance);

  // Topmost first. A hit on a pin, label or line inside a symbol counts
  // for the component that holds it.
  for (QGraphicsItem *item : scene()->items(probe, Qt::IntersectsItemBoundingRect,
                                            Qt::DescendingOrder)) {
    const sch::render::SchItemComp *comp =
        dynamic_cast<const sch::render::SchItemComp *>(item->topLevelItem());
    if (comp == nullptr) continue;

    const sch::render::SchItemBase::HitLevel level =
        comp->hitLevel(comp->mapFromScene(scenePos), tolerance);
    if (level == sch::render::SchItemBase::HitLevel::None) continue;

    return comp->node();
  }

  return nullptr;
}
