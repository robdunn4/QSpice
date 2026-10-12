#pragma once

#include <QColor>
#include <QGraphicsItem>
#include <QPainter>
#include <QPainterPath>
#include <QPen>
#include <QRectF>
#include <QStyleOptionGraphicsItem>
#include <QTransform>

#include <algorithm>
#include <cmath>

namespace sch::render {

// Styling for the selection highlight, shared by every wrapped item.
//
// A plain struct of inline statics rather than a Prefs lookup because
// SelHiLiteItem is a header-only template instantiated inside the item
// classes, which have no access to the Prefs object (they're built by
// SchTreeBuilder, which isn't given one). MainWin pushes the value here
// once at startup, after prefs.load().
struct SelHiLiteStyle {
  // Overwritten from Prefs::selHiLiteColor() at startup; this default
  // only applies if that never happens.
  static inline QColor color = QColor(0, 170, 0);

  // Gap between the element's own bounding box and the highlight
  // outline, in DEVICE pixels, so it looks the same at any zoom.
  static inline double marginPixels = 3.0;

  // Ceiling on that gap once converted to scene units. Zoomed far
  // enough out, marginPixels converts to an enormous scene distance --
  // and the outline has to stay inside the item's reserved
  // boundingRect (see below) or the scene leaves smears behind when the
  // item moves. Below roughly 0.08 zoom the gap therefore stops
  // growing and starts shrinking on screen, which is invisible in
  // practice at that scale.
  static inline double maxSceneMargin = 40.0;

  // True while a view renders for PRINTING: no highlight of any kind --
  // selection outlines, the highlighted pin -- is drawn. Set and cleared
  // by SchRenderSource::renderSceneForPrint(); painting is single-threaded.
  static inline bool suppressed = false;
};

// Mix-in that draws a rectangular outline around a selected item,
// offset a few pixels outside its bounding box.
//
// Replaces two earlier treatments: Qt's default dashed outline (easy to
// miss at typical zoom levels) and then a semi-transparent grey mask
// over the item's own shape, which was certainly visible but obscured
// the very thing it was highlighting.
//
// Usage: wrap any QGraphicsItem-derived shape class that implements the
// usual paint()/shape() pair, e.g. SelHiLiteItem<SchItemRect> or
// SelHiLiteItem<SchItemComp>. Wired into every top-level (independently
// selectable) SchItem* wrapper via SchTreeBuilder, so every element
// gets identical selection styling from this one place -- and NOT only
// on SchItemBase subclasses: SchClpbrd instantiates it on a plain
// QGraphicsRectItem for pasted content, so nothing here may assume the
// wrapped type is one of ours.
//
// The outline is a BOX around the bounding rect, not a tracing of the
// element's own outline, so a diagonal wire highlights as a box around
// its full extent rather than as a thin line along it. That's now a
// deliberate choice rather than the side effect it used to be: a box
// offset outside the element reads clearly for every element type,
// including thin and line-like ones that a tight outline would hug
// invisibly.
template <typename BaseItem> class SelHiLiteItem : public BaseItem {
public:
  using BaseItem::BaseItem;

  // Reserves room for the outline, which sits OUTSIDE the wrapped
  // item's own bounding box. Painting beyond boundingRect() leaves
  // artifacts behind when the scene repaints, so the reserved area has
  // to cover the widest gap the outline can ever be drawn at --
  // SelHiLiteStyle::maxSceneMargin, which is exactly why that ceiling
  // exists.
  QRectF boundingRect() const override {
    // On top of the base item's own repaint slack, not instead of it --
    // that slack covers cosmetic pen width, this covers the outline's
    // offset.
    const double m = SelHiLiteStyle::maxSceneMargin;
    return BaseItem::boundingRect().adjusted(-m, -m, m, m);
  }

  // Hit-testing stays with the wrapped item's own shape, which
  // SchItemBase pins to the bare element geometry. Neither the repaint
  // slack nor the highlight margin may leak into it, or elements become
  // selectable from a margin's distance away.
  QPainterPath shape() const override { return BaseItem::shape(); }

protected:
  // The element's own geometry, with no repaint slack -- the outline
  // has to sit a fixed gap outside what you can actually SEE, not
  // outside the padded rect used for invalidation.
  //
  // SchItemBase-derived items expose that as elementRect(), since their
  // boundingRect() carries SchItemBase::kPaintSlack. But this template
  // is ALSO instantiated on plain Qt classes that have no such method
  // (SchClpbrd pastes SelHiLiteItem<QGraphicsRectItem>), so the
  // preferred accessor is detected rather than assumed; for those,
  // boundingRect() already IS the bare geometry.
  QRectF baseRect() const {
    if constexpr (requires(const BaseItem &item) { item.elementRect(); })
      return BaseItem::elementRect();
    else return BaseItem::boundingRect();
  }

  void paint(QPainter *painter, const QStyleOptionGraphicsItem *option,
             QWidget *widget) override {
    // Suppress Qt's default dashed selection outline by stripping
    // State_Selected before delegating to the base class's paint().
    QStyleOptionGraphicsItem plainOption(*option);
    plainOption.state &= ~QStyle::State_Selected;

    BaseItem::paint(painter, &plainOption, widget);

    if (!this->isSelected() || SelHiLiteStyle::suppressed) return;

    painter->save();

    // Device scale as the LENGTH of the transformed unit X vector, not
    // m11().
    //
    // CONFIRMED bug: m11() is the scale only for an unrotated,
    // unmirrored item. In general it is scale*cos(rotation), and it
    // goes negative under a mirror -- so of the eight rotate codes only
    // two (0 and mirror+180, where m11 = +scale) ever got the intended
    // 3-pixel gap. The 180-degree and mirror cases fell through the
    // "deviceScale > 0" guard to the ceiling, and the 90/270 cases had
    // m11 ~ 0, making the computed gap enormous and likewise clamped.
    // The margin looked plausible in every case, which is why it went
    // unnoticed: the ceiling is a sane-looking number.
    //
    // hypot(m11, m12) is the length of the image of (1,0), so it gives
    // the real on-screen scale regardless of rotation or mirroring, and
    // is always positive.
    const QTransform xform = painter->transform();
    const double     deviceScale = std::hypot(xform.m11(), xform.m12());
    const double     margin =
        (deviceScale > 0.0)
            ? std::min(SelHiLiteStyle::marginPixels / deviceScale,
                       SelHiLiteStyle::maxSceneMargin)
            : SelHiLiteStyle::maxSceneMargin;

    QPen pen(SelHiLiteStyle::color, 2);
    pen.setCosmetic(true); // constant 2px outline at any zoom
    painter->setPen(pen);
    painter->setBrush(Qt::NoBrush);
    painter->drawRect(baseRect().adjusted(-margin, -margin, margin, margin));

    painter->restore();
  }
};

} // namespace sch::render
