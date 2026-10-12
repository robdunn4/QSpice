#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable : 4996) // Qt headers reference APIs deprecated for Qt 7
#endif

#include "SchItemTriangle.h"

#include "SchPen.h"

#include <QPainter>
#include <QPen>
#include <QPolygonF>

#ifdef _MSC_VER
#pragma warning(pop)
#endif

#include <utility>

#include "RenderStyle.h"
#include "SchColorDecode.h"

namespace sch::render {

SchItemTriangle::SchItemTriangle(ItemTrianglePtr item, QGraphicsItem *parent)
    : SchItemBase(parent), item_(std::move(item)) {
  const bool topLevel = (parent == nullptr);
  setFlag(QGraphicsItem::ItemIsSelectable, topLevel);
  setFlag(QGraphicsItem::ItemIsMovable, topLevel);

  setZValue(kZFill);
}

Rect SchItemTriangle::getRect() const noexcept {
  const Point a = item_->pt1.getValue();
  const Point b = item_->pt2.getValue();
  const Point c = item_->pt3.getValue();

  return Rect(Point(qMin(a.x, qMin(b.x, c.x)), qMax(a.y, qMax(b.y, c.y))),
              Point(qMax(a.x, qMax(b.x, c.x)), qMin(a.y, qMin(b.y, c.y))));
}

Point SchItemTriangle::getPosition() const noexcept {
  return item_->pt1.getValue();
}

void SchItemTriangle::moveTo(Point p) noexcept {
  prepareGeometryChange();

  // All three corners move together: pt1 is the handle, the others keep
  // their offsets from it.
  const Point a = item_->pt1.getValue();
  const Point d(p.x - a.x, p.y - a.y);

  const Point b = item_->pt2.getValue();
  const Point c = item_->pt3.getValue();

  item_->pt1.setValue(p);
  item_->pt2.setValue(Point(b.x + d.x, b.y + d.y));
  item_->pt3.setValue(Point(c.x + d.x, c.y + d.y));
}

QList<Point> SchItemTriangle::controlPoints() const {
  return {item_->pt1.getValue(), item_->pt2.getValue(), item_->pt3.getValue()};
}

void SchItemTriangle::setControlPoint(int index, Point p) {
  prepareGeometryChange();

  if (index == 0) item_->pt1.setValue(p);
  else if (index == 1) item_->pt2.setValue(p);
  else if (index == 2) item_->pt3.setValue(p);
}

void SchItemTriangle::paint(QPainter *painter, const QStyleOptionGraphicsItem *,
                            QWidget *) {
  const QColor lineColor = SchColorDecode::fromArgColor(
      item_->lineColor.getValue(), RenderStyle::current().graphicAnnotation);

  QPen pen = SchPen::forElement(painter, tint(lineColor),
                                item_->lineWidth.getValue(),
                                item_->lineType.getValue());
  painter->setPen(pen);

  const QColor fill =
      SchColorDecode::fromArgFill(item_->fillColor.getValue(), lineColor);

  // Unstuffed: OUTLINE only -- see SchItemRect::paint().
  if (isUnstuffed()) painter->setBrush(Qt::NoBrush);
  else painter->setBrush(fill);

  QPolygonF tri;
  tri << toQ(item_->pt1.getValue()) << toQ(item_->pt2.getValue())
      << toQ(item_->pt3.getValue());

  painter->drawPolygon(tri);
}

QPainterPath SchItemTriangle::outlinePath() const {
  QPolygonF tri;
  tri << toQ(item_->pt1.getValue()) << toQ(item_->pt2.getValue())
      << toQ(item_->pt3.getValue());

  QPainterPath path;
  path.addPolygon(tri);
  path.closeSubpath();
  return path;
}

} // namespace sch::render
