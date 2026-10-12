#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable : 4996) // Qt headers reference APIs deprecated for Qt 7
#endif

#include "SchItemPin.h"
#include "SelHiLiteItem.h" // SelHiLiteStyle::suppressed
#include "SchItemNet.h"

#include <QGraphicsScene>
#include <QPainter>
#include <QPen>

#ifdef _MSC_VER
#pragma warning(pop)
#endif

#include <cmath>

#include "RenderStyle.h"
#include "SchColorDecode.h"
#include "SchStringDecode.h"
#include "SchPointKey.h"
#include "SchPen.h"
#include "SchItemWire.h"
#include "SchScene.h"
#include "SchTextLayout.h"

#include <utility>

namespace sch::render {

namespace {
constexpr double kMarkerRadius = 4.0; // scene units

// Half-size of the unattached-pin box, in DEVICE pixels. Smaller than
// an edit handle: it is an annotation, not something to grab.
constexpr double kUnconnectedBoxPx = 3.0;

// The highlight square, in device pixels -- larger than the unconnected
// mark so the two are not mistaken for each other.
constexpr double kHighlightBoxPx = 5.0;

// ...and its scene-unit size, which takes over once zoomed in far
// enough that the device floor would look too small against the symbol.
constexpr double kHighlightBoxUnits = 6.0;
}

SchItemPin::SchItemPin(ItemPinPtr item, QGraphicsItem *parent)
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

  setZValue(kZPin);
}

Rect SchItemPin::getRect() const noexcept {
  const Point p = item_->pt1.getValue();
  // kMarkerRadius is a double (it is a drawing dimension) while Point
  // holds ints -- rounded explicitly rather than left to an implicit
  // narrowing conversion, which is what the C4244 warnings here were.
  const int r = static_cast<int>(std::lround(kMarkerRadius));

  const Rect marker(Point(p.x - r, p.y + r), Point(p.x + r, p.y - r));

  // Merged with the label's extent: the marker alone under-reports what
  // this element paints, and an element that paints outside its bounds
  // smears when dragged.
  const QRectF label = labelRect();
  if (label.isNull()) return marker;

  return marker.merge(Rect(static_cast<int>(std::floor(label.left())),
                           static_cast<int>(std::ceil(label.bottom())),
                           static_cast<int>(std::ceil(label.right())),
                           static_cast<int>(std::floor(label.top()))));
}

Point SchItemPin::getPosition() const noexcept { return item_->pt1.getValue(); }

void SchItemPin::moveTo(Point p) noexcept {
  prepareGeometryChange();
  item_->pt1.setValue(p);
}

QList<QPointF> SchItemPin::connectionPoints() const {
  return {toQ(item_->pt1.getValue())};
}

QList<Point> SchItemPin::controlPoints() const {
  return {item_->pt1.getValue()};
}

void SchItemPin::setControlPoint(int index, Point p) {
  prepareGeometryChange();
  if (index == 0) item_->pt1.setValue(p);
}

bool SchItemPin::labelVisible() const {
  if (sch::core::decodeQSpiceString(item_->pinLabel.getValue()).isEmpty()) return false;

  // Independent rules, from the AnalogLatch symbol, where QSpice draws
  // EN, IN, CLK and OUT but not Vdd, Vss or the inverted Q, and the
  // GmEnable symbol (1b). They differ in how well established they are
  // -- see each.

  // 1. A zero alignment nibble. OBSERVED, MECHANISM UNCONFIRMED: it
  //    fits Vdd and Vss (nibble 0, not drawn) against every shown pin
  //    (7 or 11), but what QSpice actually writes when a symbol's text
  //    is set to "hidden" has not been read back. The full argument,
  //    and the test that settles it, are in sch::core::alignMarksHidden()'s
  //    comment in SchTextLayout.h.
  //
  //    This replaced an earlier guess keyed on the pt2 label offset
  //    being (0,0), which fitted the same two pins for no principled
  //    reason at all.
  const int alignH = item_->rotateAlign.getAlignH();
  const int alignV = item_->rotateAlign.getAlignV();
  if (sch::core::alignMarksHidden(alignH, alignV)) return false;

  // 1b. Centred both ways (nibble 0b1111). A pin label is placed by an
  //     edge or corner, never by its centre -- QSpice's symbol editor
  //     treats the centre point as "hidden". Robert's GmEnable symbol:
  //     OUT, IN- and IN+ carry 15 and are not shown, EN carries 7 (west)
  //     and is. PINS ONLY: centred TEXT is drawn (AnalogLatch's "Y1"),
  //     so sch::core::alignMarksHidden() itself is left alone.
  if (alignH == sch::core::kAlignHCenter && alignV == sch::core::kAlignVCenter) return false;

  // 2. An attached net name (P9). CONFIRMED -- not from the file, but
  //    from the behaviour of QSpice itself: giving a pin a net name
  //    hides its label. The inverted Q is the file-level example, being
  //    the only hidden pin whose offset and alignment otherwise look
  //    like a visible one.
  if (isInternal()) return false;

  return true;
}

QString SchItemPin::connectedNetName() const {
  const QString own = sch::core::decodeQSpiceString(item_->netName.getValue()).trimmed();
  if (!own.isEmpty()) return own;

  if (scene() == nullptr) return QString();

  // Whatever shares the pin's point. Every wire in a net carries the
  // same name once nameNets() has run, so the first one found is the
  // right answer -- no need to walk the net.
  const QPointF at = mapToScene(toQ(item_->pt1.getValue()));

  // The document's connectivity. A scene that is not a document's (a
  // symbol preview) has no nets to name.
  SchScene *sc = SchScene::of(this);
  if (sc == nullptr) return QString();

  const sch::core::NetModel *model = sc->netModel();
  if (model == nullptr) return QString();

  return model->netNameAt(sch::core::keyFor(at));
}

bool SchItemPin::isInternal() const {
  return !sch::core::decodeQSpiceString(item_->netName.getValue()).isEmpty();
}

// As SchItemText's: see SchItemBase::labelAlignH().
int SchItemPin::effectiveAlignH() const {
  return labelAlignH(item_->rotateAlign.getAlignH(), item_->rotateAlign.getRotDeg());
}

int SchItemPin::effectiveAlignV() const {
  return labelAlignV(item_->rotateAlign.getAlignV(), item_->rotateAlign.getRotDeg());
}

bool SchItemPin::showsUnconnectedMark() const {
  // An internal pin is not drawn, so it cannot be flagged either.
  if (isInternal()) return false;

  // EVERY pin is a candidate.
  //
  // This briefly skipped pins whose alignment nibble is zero, on the
  // reading that such a pin is "hidden". That rule is correct for a
  // pin's LABEL -- no alignment means no label placement, so no text is
  // drawn -- but it says nothing about whether the pin is meant to be
  // wired. A resistor's pins carry alignment 0 and pt2 (0,0), exactly
  // like Vdd and Vss, so the marker vanished from the passive
  // components it is most useful on and survived only on symbols whose
  // pins happen to have placed labels.
  //
  // ItemPin has no field that distinguishes an implicit pin from an
  // ordinary one -- ArgPinInfo is port and data typing only -- so
  // nothing is excluded here.

  // Attachment is judged GEOMETRICALLY: is a wire endpoint on this pin?
  //
  // Not by the pin's netName. QSpice does maintain that field as wires
  // attach and detach, but it belongs to the SPICE netlist, which this
  // editor does not deal with yet -- our own wire net names go stale on
  // every topology change, so a marker driven from them would report on
  // bookkeeping we are not yet keeping rather than on what is actually
  // wired.
  //
  // Coincidence is exact, the same rule connectivity uses everywhere.
  const QList<QPointF> own = sceneConnectionPoints();
  if (own.isEmpty()) return true;

  // Anything electrical counts, not only wires: a pin touching another
  // pin, or a net label's anchor, is connected too -- see sch::core::NetModel.
  //
  // Only in a document. A scene that is not a document's (a symbol
  // preview) shows no marks: nothing there could be connected.
  SchScene *sc = SchScene::of(this);
  if (sc == nullptr) return false;

  const sch::core::NetModel *model = sc->netModel();
  if (model == nullptr) return false;

  return model->pinMarked(node(), sch::core::keyFor(own.first()));
}

QRectF SchItemPin::bodyRect() const {
  if (isInternal()) return QRectF();

  const Point  p = item_->pt1.getValue();
  const double r = kMarkerRadius;
  return QRectF(p.x - r, p.y - r, r * 2.0, r * 2.0);
}

QRectF SchItemPin::labelRect() const {
  if (!labelVisible()) return QRectF();

  // The DISPLAY form, as paint() draws it -- measuring the raw label
  // counted overbar markers as characters and made the bounds too wide.
  const QString label =
      overbarDisplayText(sch::core::decodeQSpiceString(item_->pinLabel.getValue()));

  const QFontMetricsF fm(makeFont(item_->fontSize.getValue()));
  // pt2 is the offset from the pin point to its label (per the record
  // documentation), so the label anchors there, not at the pin itself.
  const QPointF anchor = toQ(item_->pt1.getValue()) + toQ(item_->pt2.getValue());
  const QRectF  local  = alignedTextRect(fm, label, effectiveAlignH(), effectiveAlignV());

  // Through the same transform paint() draws with, so the reported box
  // is the one the glyphs occupy -- rotated, unmirrored and turned as
  // they are. (It used to apply only the Y-flip, which was right for an
  // upright label alone.)
  const QRectF painted =
      labelXform(static_cast<double>(item_->rotateAlign.getRotDeg())).mapRect(local);
  return painted.translated(anchor);
}

void SchItemPin::paint(QPainter *painter, const QStyleOptionGraphicsItem *,
                       QWidget *) {
  // Internal pins draw nothing at all.
  if (isInternal()) return;

  const QPointF centre = toQ(item_->pt1.getValue());

  painter->setPen(Qt::NoPen);

  if (highlighted && !SelHiLiteStyle::suppressed) {
    // A SQUARE, as QSpice marks pins -- filled red while this pin's row
    // is selected in the Symbol Properties pane.
    //
    // Sized as the LARGER of a device-pixel floor and a scene-unit
    // size, so it behaves sensibly at both ends: zoomed out it stays a
    // legible few pixels, and zoomed in it grows with the component
    // rather than staying a speck on a symbol that now fills the view.
    const QTransform xform = painter->transform();
    const double     scale = std::hypot(xform.m11(), xform.m12());

    const double floorHalf = (scale > 0.0) ? (kHighlightBoxPx / scale) : 0.0;
    const double half      = qMax(floorHalf, kHighlightBoxUnits);

    painter->setBrush(QColor(Qt::red));
    painter->drawRect(
        QRectF(centre.x() - half, centre.y() - half, half * 2, half * 2));

    // The unconnected mark is NOT drawn over a highlight: it is a small
    // filled box in its own colour, and painting it on top left the red
    // square with a hole in the middle.
    return;
  } else {
    painter->setBrush(tint(RenderStyle::current().pinDot));
    painter->drawEllipse(centre, kMarkerRadius, kMarkerRadius);
  }

  // An unattached pin gets a small open box, so a pin nobody wired to
  // is visible at a glance rather than only at netlist time.
  if (showsUnconnectedMark()) {
    // Sized in DEVICE pixels like the edit handles, so it stays legible
    // at any zoom -- but drawn here rather than as its own item, so it
    // needs the painter's scale to convert. hypot of the first row, not
    // m11, which is scale*cos of any rotation.
    const QTransform xform = painter->transform();
    const double     scale = std::hypot(xform.m11(), xform.m12());
    const double     half =
        (scale > 0.0) ? (kUnconnectedBoxPx / scale) : kMarkerRadius;

    const QPointF c = toQ(item_->pt1.getValue());

    QPen pen(Qt::black);
    pen.setCosmetic(true);
    SchPen::enforcePrintMin(pen, painter);
    painter->setPen(pen);
    painter->setBrush(tint(RenderStyle::current().unconnectedPin));
    painter->drawRect(QRectF(c.x() - half, c.y() - half, half * 2, half * 2));
  }

  // The pin LABEL ("EN", "CLK", "OUT" ...), where this pin has a visible
  // one -- see labelVisible(). The MARKER above is drawn either way:
  // QSpice still shows the connection point for Vdd, Vss and the
  // inverted Q, it just doesn't name them.
  if (!labelVisible()) return;

  const QString rawLabel = sch::core::decodeQSpiceString(item_->pinLabel.getValue());
  const QString label    = overbarDisplayText(rawLabel);

  const QFont font = makeFont(item_->fontSize.getValue());

  painter->save();
  painter->translate(toQ(item_->pt1.getValue()) + toQ(item_->pt2.getValue()));

  // The SAME transform SchItemText draws with -- a pin's label is symbol
  // text like any other: its own rotation from rotateAlign, the parent's
  // rotation, scale and mirror cancelled, the Y-flip undone, and the
  // readability half turn. See SchItemBase::labelXform().
  painter->setTransform(labelXform(static_cast<double>(item_->rotateAlign.getRotDeg())),
                        /*combine=*/true);

  painter->setFont(font);
  painter->setPen(tint(SchColorDecode::fromArgColor(
      item_->textColor.getValue(), RenderStyle::current().pin)));

  const QFontMetricsF fm(font);
  const QRectF        rect = alignedTextRect(fm, label, effectiveAlignH(), effectiveAlignV());
  painter->drawText(rect, Qt::AlignLeft | Qt::AlignTop | Qt::TextDontClip,
                    label);

  drawOverbars(painter, fm, rawLabel, label, rect.left(),
               rect.top() + fm.ascent());

  painter->restore();
}

} // namespace sch::render
