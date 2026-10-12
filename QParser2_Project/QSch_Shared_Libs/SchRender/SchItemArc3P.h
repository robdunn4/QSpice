#pragma once
#include "ItemArc3P.h"
#include "SchItemBase.h"

namespace sch::render {

// Renders QSpice's three-point «arc3p». The four-point «arc» is a
// different record -- ItemArc4P -- with its own semantics.
class SchItemArc3P : public SchItemBase {
public:
  explicit SchItemArc3P(ItemArc3pPtr item, QGraphicsItem *parent = nullptr);

  Rect  getRect() const noexcept override;
  Point getPosition() const noexcept override;
  void  moveTo(Point p) noexcept override;

  QList<Point> controlPoints() const override;

  // The arc as drawn, sampled -- painted, and used for picking (see
  // SchItemBase::hitLevel()). Empty when the three points coincide.
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
  ItemArc3pPtr item_;
};

} // namespace sch::render
