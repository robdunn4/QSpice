#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable : 4996) // Qt headers reference APIs deprecated for Qt 7
#endif

#include "SchItemLine.h"

#include "SchPen.h"

#include <QPainter>
#include <QPen>

#ifdef _MSC_VER
#pragma warning(pop)
#endif

#include <utility>

#include "RenderStyle.h"
#include "SchColorDecode.h"

namespace sch::render {

SchItemLine::SchItemLine(ItemLinePtr item, QGraphicsItem *parent)
    : SchItemBase(parent), item_(std::move(item)) {
  // Only truly top-level items are independently selectable/movable --
  // children of a SchItemComp (the leaf elements making up a symbol's
  // geometry) act as part of their parent component, not as independent
  // objects. Matches standard EDA convention: clicking or rubber-band-
  // selecting any part of a placed component selects/moves the whole
  // component, not one internal element.
  const bool topLevel = (parent == nullptr);
  setFlag(QGraphicsItem::ItemIsSelectable, topLevel);
  setFlag(QGraphicsItem::ItemIsMovable, topLevel);

  setZValue(kZGraphic);
}

Rect SchItemLine::getRect() const noexcept {
  return Rect(item_->pt1.getValue(), item_->pt2.getValue());
}

Point SchItemLine::getPosition() const noexcept {
  return item_->pt1.getValue();
}

void SchItemLine::moveTo(Point p) noexcept {
  prepareGeometryChange();
  const Point delta = p - getPosition();
  item_->pt1.setValue(item_->pt1.getValue() + delta);
  item_->pt2.setValue(item_->pt2.getValue() + delta);
}

QList<Point> SchItemLine::controlPoints() const {
  return {item_->pt1.getValue(), item_->pt2.getValue()};
}

void SchItemLine::setControlPoint(int index, Point p) {
  prepareGeometryChange();
  if (index == 0) item_->pt1.setValue(p);
  else if (index == 1) item_->pt2.setValue(p);
}

void SchItemLine::paint(QPainter *painter, const QStyleOptionGraphicsItem *,
                        QWidget *) {
  QPen pen = SchPen::forElement(
      painter,
      tint(SchColorDecode::fromArgColor(item_->lineColor.getValue(),
                                        RenderStyle::current().graphicAnnotation)),
      item_->lineWidth.getValue(), item_->lineType.getValue());
  painter->setPen(pen);
  painter->drawLine(toQ(item_->pt1.getValue()), toQ(item_->pt2.getValue()));
}

QPainterPath SchItemLine::outlinePath() const {
  QPainterPath path(toQ(item_->pt1.getValue()));
  path.lineTo(toQ(item_->pt2.getValue()));
  return path;
}

} // namespace sch::render
