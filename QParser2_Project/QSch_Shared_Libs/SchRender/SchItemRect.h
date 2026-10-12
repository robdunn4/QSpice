#pragma once
#include <QImage>

#include "ItemRect.h"
#include "SchBoxFrame.h"
#include "SchItemBase.h"

namespace sch::render {

// A «rect» record -- and, when its fill is an IMAGE mode (0x05-0x0B) and
// it carries P11, a bitmap: the image file's bytes as one unbroken run
// of hex digits (a PNG, as QSpice writes it). The bitmap is stretched to
// fill the rectangle, so resizing the rect resizes the image.

class SchItemRect : public SchItemBase {
public:
  explicit SchItemRect(ItemRectPtr item, QGraphicsItem *parent = nullptr);

  Rect  getRect() const noexcept override;
  Point getPosition() const noexcept override;
  void  moveTo(Point p) noexcept override;

  QList<Point> controlPoints() const override;
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

  // The fill mode says "image" and there is image data to draw.
  bool hasImage() const;

  // Picking: the rectangle as drawn, and whether its inside counts --
  // see SchItemBase::hitLevel().
  QPainterPath outlinePath() const override;
  bool         interiorHollow() const override;

  // A new record holding `image` as a PNG, `unitsPerPixel` schematic
  // units per image pixel, its bottom-left corner at the origin.
  static ItemRectPtr makeImageRecord(const QImage &image, double unitsPerPixel);

  // The frame this shape draws in -- see SchBoxFrame. Read-only: an odd
  // code tilts the shape 45 degrees, and nothing in the UI writes one.
  SchBoxFrame::Frame boxFrame() const {
    const Point p1 = item_->pt1.getValue();
    const Point p2 = item_->pt2.getValue();

    return SchBoxFrame::frameFor(p1.x, p1.y, p2.x, p2.y,
                                 item_->rotate.getValue());
  }

protected:
  void paint(QPainter *painter, const QStyleOptionGraphicsItem *option,
             QWidget *widget) override;

private:
  // The record's bitmap, decoded on first use. The data never changes
  // under an item -- a new image means a new record -- so it is kept.
  const QImage &image() const;

  void drawImage(QPainter *painter, const QRectF &rect) const;

  ItemRectPtr item_;

  mutable QImage imageCache;
  mutable bool   imageDecoded = false;
};

} // namespace sch::render
