#pragma once
#include "ItemLine.h"
#include "SchItemBase.h"

namespace sch::render {

class SchItemLine : public SchItemBase {
public:
  explicit SchItemLine(ItemLinePtr item, QGraphicsItem *parent = nullptr);

  Rect  getRect() const noexcept override;
  Point getPosition() const noexcept override;
  void  moveTo(Point p) noexcept override;

  QList<Point> controlPoints() const override;

  // Picking: the segment as drawn -- see SchItemBase::hitLevel(). Its
  // shape() is the bounding box, which for a diagonal line is mostly
  // empty space.
  QPainterPath outlinePath() const override;
  void         setControlPoint(int index, Point p) override;

  int  lineWidth() const override { return item_->lineWidth.getValue(); }
  void setLineWidth(int w) override {
    prepareGeometryChange();
    item_->lineWidth.setValue(w);
    update();
  }

  int  lineType() const override { return item_->lineType.getValue(); }
  void setLineType(int t) override {
    item_->lineType.setValue(t);
    update();
  }

  int  lineColorRaw() const override { return item_->lineColor.getValue(); }
  void setLineColorRaw(int raw) override {
    item_->lineColor.setValue(raw);
    update();
  }

protected:
  void paint(QPainter *painter, const QStyleOptionGraphicsItem *option,
             QWidget *widget) override;

private:
  ItemLinePtr item_;
};

} // namespace sch::render
