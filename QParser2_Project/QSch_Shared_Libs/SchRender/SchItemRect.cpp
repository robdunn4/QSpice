#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable : 4996) // Qt headers reference APIs deprecated for Qt 7
#endif

#include "SchItemRect.h"

#include "SchPen.h"
#include "SchBoxFrame.h"

#include <QBuffer>
#include <QByteArray>
#include <QDebug>
#include <QPaintEngine>
#include <QPainter>
#include <QPen>

#ifdef _MSC_VER
#pragma warning(pop)
#endif

#include <algorithm>
#include <cmath>
#include <utility>

#include "RenderStyle.h"
#include "SchColorDecode.h"

namespace sch::render {

SchItemRect::SchItemRect(ItemRectPtr item, QGraphicsItem *parent)
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

  setZValue(kZFill);
}

Rect SchItemRect::getRect() const noexcept {
  // The bounds of the shape AS DRAWN, which an odd rotation code tilts
  // -- so not simply the two corner points. Selection and repaint both
  // read this, and a shape drawn outside its own bounds smears when
  // dragged.
  const SchBoxFrame::Frame frame = boxFrame();

  if (!frame.tilted())
    return Rect(item_->pt1.getValue(), item_->pt2.getValue());

  const QRectF bounds = frame.sceneBounds();

  return Rect(Point(static_cast<int>(std::lround(bounds.left())),
                    static_cast<int>(std::lround(bounds.top()))),
              Point(static_cast<int>(std::lround(bounds.right())),
                    static_cast<int>(std::lround(bounds.bottom()))));
}

Point SchItemRect::getPosition() const noexcept {
  return item_->pt1.getValue();
}

void SchItemRect::moveTo(Point p) noexcept {
  prepareGeometryChange();
  const Point delta = p - getPosition();
  item_->pt1.setValue(item_->pt1.getValue() + delta);
  item_->pt2.setValue(item_->pt2.getValue() + delta);
}

QList<Point> SchItemRect::controlPoints() const {
  return {item_->pt1.getValue(), item_->pt2.getValue()};
}

void SchItemRect::setControlPoint(int index, Point p) {
  prepareGeometryChange();
  if (index == 0) item_->pt1.setValue(p);
  else if (index == 1) item_->pt2.setValue(p);
}

void SchItemRect::paint(QPainter *painter, const QStyleOptionGraphicsItem *,
                        QWidget *) {
  // A hierarchical block is coloured entirely from QSpice's
  // preferences, NOT from the record. CONFIRMED: the X1 block's rect
  // carries 0x4000000 for both line and fill -- the same fill value the
  // AnalogLatch body uses for yellow -- yet QSpice draws it grey with a
  // blue border. What distinguishes them is P9 (isHierarchicalBlock),
  // which ItemRect.h flags and which is 1 there and 0 on the latch.
  const bool hierarchical = item_->isHierarchicalBlock.getValue() != 0;

  const QColor lineColor =
      hierarchical ? RenderStyle::current().hierarchicalBlock
                   : SchColorDecode::fromArgColor(
                         item_->lineColor.getValue(),
                         RenderStyle::current().graphicAnnotation);

  QPen pen = SchPen::forElement(painter, tint(lineColor),
                                item_->lineWidth.getValue(),
                                item_->lineType.getValue());
  painter->setPen(pen);

  // An unstuffed component is drawn as an OUTLINE: its fills go
  // transparent rather than grey. A solid grey body would hide the
  // symbol's own shape, which is the thing the user still has to
  // recognise -- the greying says "not fitted", not "not there".
  //
  // An IMAGE fill draws the bitmap instead, and the outline over it.
  const bool withImage = !hierarchical && hasImage();

  if (isUnstuffed() || withImage) {
    painter->setBrush(Qt::NoBrush);
  } else {
    const QColor fill = hierarchical ? RenderStyle::current().hierarchicalFill
                                     : SchColorDecode::fromArgFill(
                                           item_->fillColor.getValue(), lineColor);
    painter->setBrush(fill);
  }

  // elementRect(), NOT boundingRect(). This is the element's own
  // geometry; boundingRect() is the REPAINT region and carries
  // SchItemBase::kPaintSlack plus the children's extent.
  //
  // CONFIRMED regression: drawing boundingRect() here inflated every
  // rect and ellipse by a slack width on each side -- invisible on
  // large shapes, glaring on the thin 4-unit bars inside the V symbol,
  // which came out roughly ten times too thick. It also painted right
  // at the edge of the reported bounds, so the cosmetic pen spilled
  // past them and the drag smearing came straight back.
  // Drawn in the shape's OWN frame, which an odd rotation code tilts
  // 45 degrees -- see SchBoxFrame. An even code gives the identity
  // transform and the ordinary upright shape.
  const SchBoxFrame::Frame frame = boxFrame();

  painter->save();
  painter->translate(frame.centre);
  painter->rotate(frame.degrees());

  if (withImage) drawImage(painter, frame.localRect());
  painter->drawRect(frame.localRect());

  painter->restore();
}

bool SchItemRect::hasImage() const {
  const int mode = SchColorDecode::colorMode(item_->fillColor.getValue());
  if (mode < 0x05 || mode > 0x0B) return false;
  return !item_->imageData.getValue().empty();
}

const QImage &SchItemRect::image() const {
  if (imageDecoded) return imageCache;
  imageDecoded = true;

  // Hex digits -> the image file's own bytes; the format is whatever
  // they say (PNG from QSpice).
  const QByteArray bytes =
      QByteArray::fromHex(QByteArray::fromStdString(item_->imageData.getValue()));
  imageCache = QImage::fromData(bytes);

  if (imageCache.isNull())
    qWarning() << "Rect image: could not decode" << bytes.size()
               << "bytes of image data";

  return imageCache;
}

void SchItemRect::drawImage(QPainter *painter, const QRectF &rect) const {
  const QImage &img = image();
  if (img.isNull() || rect.isEmpty()) return;

  painter->save();

  // Image rows run DOWN; the view shows scene Y running UP. Flipped
  // about the rect's horizontal centre line, the picture is upright.
  painter->translate(0.0, rect.top() + rect.bottom());
  painter->scale(1.0, -1.0);

  // The mode's Windows raster op, where the paint engine has raster
  // ops (the screen does; printers and PDF do not -- they get a plain
  // copy). 0x05 SRCCOPY is an ordinary draw, transparency honoured.
  if (painter->paintEngine() != nullptr &&
      painter->paintEngine()->hasFeature(QPaintEngine::RasterOpModes)) {
    switch (SchColorDecode::colorMode(item_->fillColor.getValue())) {
    case 0x06: // SRCPAINT: OR
      painter->setCompositionMode(QPainter::RasterOp_SourceOrDestination);
      break;
    case 0x07: // SRCAND: AND
      painter->setCompositionMode(QPainter::RasterOp_SourceAndDestination);
      break;
    case 0x08: // SRCINVERT: XOR
      painter->setCompositionMode(QPainter::RasterOp_SourceXorDestination);
      break;
    case 0x09: // SRCERASE: source AND NOT destination
      painter->setCompositionMode(QPainter::RasterOp_SourceAndNotDestination);
      break;
    case 0x0A: // NOTSRCERASE: NOT (source OR destination)
      painter->setCompositionMode(QPainter::RasterOp_NotSourceAndNotDestination);
      break;
    case 0x0B: // MERGEPAINT: NOT source OR destination
      painter->setCompositionMode(QPainter::RasterOp_NotSourceOrDestination);
      break;
    default:
      break;
    }
  }

  painter->setRenderHint(QPainter::SmoothPixmapTransform);
  painter->drawImage(rect, img);

  painter->restore();
}

ItemRectPtr SchItemRect::makeImageRecord(const QImage &image,
                                         double unitsPerPixel) {
  ItemRectPtr rec = ItemRect::makePtr();

  // Stored as PNG whatever it was loaded from -- what QSpice writes, and
  // lossless.
  QByteArray png;
  QBuffer    buffer(&png);
  buffer.open(QIODevice::WriteOnly);
  image.save(&buffer, "PNG");

  rec->imageData = ArgImage(png.toHex().toStdString());

  const int w = std::max(1, static_cast<int>(std::lround(image.width() * unitsPerPixel)));
  const int h = std::max(1, static_cast<int>(std::lround(image.height() * unitsPerPixel)));
  rec->pt1.setValue(Point(0, 0));
  rec->pt2.setValue(Point(w, h));

  // As QSpice's own image rect: image fill SRCCOPY, a dotted outline in
  // the element's default colour.
  rec->lineColor.setValue(0x01000000);
  rec->lineType.setValue(2);
  rec->fillColor.setValue(0x05000000);
  return rec;
}

QPainterPath SchItemRect::outlinePath() const {
  const SchBoxFrame::Frame frame = boxFrame();

  QPainterPath local;
  local.addRect(frame.localRect());
  return frame.toItem(local);
}

bool SchItemRect::interiorHollow() const {
  // A hierarchical block is filled from the preferences, not the record
  // -- see paint().
  if (item_->isHierarchicalBlock.getValue() == 0) return SchItemBase::interiorHollow();
  if (isUnstuffed()) return true;

  const QColor fill = RenderStyle::current().hierarchicalFill;
  return fill.alpha() == 0 || fill.rgb() == RenderStyle::current().background.rgb();
}

} // namespace sch::render
