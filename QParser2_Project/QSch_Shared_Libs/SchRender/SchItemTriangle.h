#pragma once
#include "ItemTriangle.h"
#include "SchItemBase.h"

namespace sch::render {

// A filled/outlined triangle -- three points, line and fill colours.
//
// This class did not exist until triangles became placeable. Stock
// symbols use them (the NPN's arrowhead is one), and QParser2 parsed
// them all along, but SchTreeBuilder had no case for QPI::TRIANGLE --
// so every triangle in every symbol was silently absent from the scene.
class SchItemTriangle : public SchItemBase {
public:
  explicit SchItemTriangle(ItemTrianglePtr item,
                           QGraphicsItem *parent = nullptr);

  Rect  getRect() const noexcept override;
  Point getPosition() const noexcept override;
  void  moveTo(Point p) noexcept override;

  QList<Point> controlPoints() const override;

  // Picking: the triangle as drawn -- see SchItemBase::hitLevel().
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

  int  fillColorRaw() const override { return item_->fillColor.getValue(); }
  void setFillColorRaw(int raw) override {
    item_->fillColor.setValue(raw);
    update();
  }

protected:
  void paint(QPainter *painter, const QStyleOptionGraphicsItem *option,
             QWidget *widget) override;

private:
  ItemTrianglePtr item_;
};

} // namespace sch::render
