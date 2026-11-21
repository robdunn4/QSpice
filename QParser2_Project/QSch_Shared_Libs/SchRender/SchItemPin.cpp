#include "SchItemPin.h"
#include "SchItemNet.h"
#include "SchItemRect.h"
#include "SelHiLiteItem.h" // SelHiLiteStyle::suppressed

#include <QGraphicsScene>
#include <QPainter>
#include <QPen>

#include <algorithm>
#include <cmath>

#include <ItemSym.h>
#include <ItemType.h>
#include <SchPointKey.h>
#include <SchStringDecode.h>

#include "RenderStyle.h"
#include "SchColorDecode.h"
#include "SchItemWire.h"
#include "SchPen.h"
#include "SchScene.h"
#include "SchTextLayout.h"

#include <utility>

namespace sch::render {

namespace {
constexpr double kMarkerRadius = 4.0; // scene units
constexpr double kMarkerMinPx  = 1.0; // ...but never under 2 px across

// Half-size of the unattached-pin box: in scene units, so it scales with
// zoom as QSpice's does, with a DEVICE-pixel floor so it never vanishes
// (Robert, 2026-11-18). Smaller than an edit handle up close: it is an
// annotation, not something to grab.
constexpr double kUnconnectedBoxUnits = 12.0;
constexpr double kUnconnectedBoxPx    = 3.0;

// The highlight square, in device pixels -- larger than the unconnected
// mark so the two are not mistaken for each other.
constexpr double kHighlightBoxPx = 5.0;

// ...and its scene-unit size, which takes over once zoomed in far
// enough that the device floor would look too small against the symbol.
constexpr double kHighlightBoxUnits = 6.0;

// A SYMBOL document's pin: an open BOX, as QSpice's symbol editor draws
// one -- half its size in scene units (Robert's VisTest capture), with a
// device-pixel floor so it stays a box zoomed out.
constexpr double kSymBoxUnits = 20.0;
constexpr double kSymBoxPx    = 4.0;

// A DLL port's mark, in scene units, measured from the pin inward: the
// gap before it, its length along the pin and its half-width across.
constexpr double kPortMarkGap  = 6.0;
constexpr double kPortMarkLen  = 28.0;
constexpr double kPortMarkHalf = 10.0;

// How far the mark can reach from the pin, for the bounds.
constexpr double kPortMarkReach = kPortMarkGap + kPortMarkLen + 4.0;

// The block's box, in its own frame: its largest rectangle -- as the
// editor's blockpin::blockBox() picks it. Null if it has none.
QRectF blockBoxOf(const QGraphicsItem *block) {
  QRectF best;
  if (block == nullptr) return best;

  double bestArea = 0.0;
  for (QGraphicsItem *child : block->childItems()) {
    const SchItemRect *rect = dynamic_cast<const SchItemRect *>(child);
    if (rect == nullptr) continue;

    const QRectF box = child->mapRectToParent(rect->elementRect()).normalized();
    const double area = box.width() * box.height();
    if (area <= bestArea) continue;

    bestArea = area;
    best     = box;
  }
  return best;
}

// A symbol document's box: its largest top-level rectangle -- a symbol
// edited on its own has no component to own its pins.
QRectF symbolBoxOf(const QGraphicsScene *scene) {
  QRectF best;
  if (scene == nullptr) return best;

  double bestArea = 0.0;
  for (QGraphicsItem *item : scene->items()) {
    if (item->parentItem() != nullptr) continue;

    const SchItemRect *rect = dynamic_cast<const SchItemRect *>(item);
    if (rect == nullptr) continue;

    const QRectF box  = item->mapRectToScene(rect->elementRect()).normalized();
    const double area = box.width() * box.height();
    if (area <= bestArea) continue;

    bestArea = area;
    best     = box;
  }
  return best;
}

// The unit vector from `p` INTO `box`, across its nearest edge. A pin
// with no box points along +X.
QPointF inwardAt(const QRectF &box, const QPointF &p) {
  if (box.isNull()) return QPointF(1.0, 0.0);

  // Nearest edge first: left, right, low Y, high Y.
  const double dist[4] = {
      std::abs(p.x() - box.left()), std::abs(p.x() - box.right()),
      std::abs(p.y() - box.top()), std::abs(p.y() - box.bottom())};
  const QPointF dirs[4] = {QPointF(1.0, 0.0), QPointF(-1.0, 0.0),
                           QPointF(0.0, 1.0), QPointF(0.0, -1.0)};

  int nearest = 0;
  for (int i = 1; i < 4; ++i)
    if (dist[i] < dist[nearest]) nearest = i;
  return dirs[nearest];
}
} // namespace

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

  refreshDrawZ();
}

Rect SchItemPin::getRect() const noexcept {
  const Point p = item_->pt1.getValue();
  // kMarkerRadius is a double (it is a drawing dimension) while Point
  // holds ints -- rounded explicitly rather than left to an implicit
  // narrowing conversion, which is what the C4244 warnings here were.
  const int r = static_cast<int>(std::lround(kMarkerRadius));

  // A DLL port's mark reaches further, inside the box; a symbol
  // document's pin box further still.
  int reach = onDllBlock() ? static_cast<int>(std::ceil(kPortMarkReach)) : r;
  if (inSymbolDocument())
    reach = std::max(reach, static_cast<int>(std::ceil(kSymBoxUnits)));
  const Rect marker(Point(p.x - reach, p.y + reach),
                    Point(p.x + reach, p.y - reach));

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

SchTextEditInfo SchItemPin::textEditInfo() const {
  SchTextEditInfo info;
  // The anchor is the LABEL's: the pin's point plus the label offset.
  const Point at     = item_->pt1.getValue();
  const Point offset = item_->pt2.getValue();

  info.text          = pinLabel();
  info.anchor        = Point(at.x + offset.x, at.y + offset.y);
  info.color         = SchColorDecode::fromArgColor(item_->textColor.getValue(),
                                                    defaultTextColor());
  info.fontPixelSize = fontPixelSize(item_->fontSize.getValue());
  info.alignH        = item_->rotateAlign.getAlignH();
  info.alignV        = item_->rotateAlign.getAlignV();
  return info;
}

void SchItemPin::applyTextEdit(const SchTextEditInfo &info) {
  // Before the label changes: its width is part of getRect().
  prepareGeometryChange();

  item_->pinLabel.setValue(
      ArgString(sch::core::encodeQSpiceString(info.text)).quote().getValue());

  // The ALIGNMENT -- nothing else. As in QSpice, a new anchor leaves the
  // label's anchor point (its offset from the pin) where it is, and the
  // text hangs from it differently. Centered both ways HIDES a pin's
  // label (labelVisible()); the editor asked the user before allowing it.
  item_->rotateAlign.setAlignH(static_cast<unsigned char>(info.alignH));
  item_->rotateAlign.setAlignV(static_cast<unsigned char>(info.alignV));

  update();
}

int SchItemPin::orientCode() const {
  return (item_->rotateAlign.getRotCode() << 4) | item_->rotateAlign.getAlign();
}

void SchItemPin::setOrientCode(int code) {
  prepareGeometryChange();
  item_->rotateAlign.setRotCode((code >> 4) & 0xF);
  item_->rotateAlign.setAlign(static_cast<unsigned char>(code & 0xF));
  update();
}

double SchItemPin::fontSize() const { return item_->fontSize.getValue(); }

void SchItemPin::setFontSize(double size) {
  prepareGeometryChange();

  // The record stores a float; the cast makes the narrowing deliberate.
  item_->fontSize.setValue(static_cast<float>(size));
  update();
}

Point SchItemPin::labelOffset() const { return item_->pt2.getValue(); }

void SchItemPin::setLabelOffset(const Point &offset) {
  prepareGeometryChange();
  item_->pt2.setValue(offset);
  update();
}

QString SchItemPin::pinLabel() const {
  return sch::core::decodeQSpiceString(item_->pinLabel.getValue());
}

bool SchItemPin::labelVisible() const {
  if (sch::core::decodeQSpiceString(item_->pinLabel.getValue()).isEmpty())
    return false;

  // Independent rules, from the AnalogLatch symbol, where QSpice draws
  // EN, IN, CLK and OUT but not Vdd, Vss or the inverted Q, and the
  // GmEnable symbol (1b). They differ in how well established they are
  // -- see each.

  // 1. A zero alignment nibble. CONFIRMED (2026-11-18, VisTest): QSpice
  //    draws no label for a pin with alignment 0, in a schematic or in
  //    its symbol editor, though its Invis. column stays clear. It fits
  //    Vdd and Vss too (nibble 0, not drawn) against every shown pin (7
  //    or 11). PINS ONLY: a TEXT with alignment 0 is drawn, centered.
  //
  //    This replaced an earlier guess keyed on the pt2 label offset
  //    being (0,0), which fitted the same two pins for no principled
  //    reason at all.
  const int alignH = item_->rotateAlign.getAlignH();
  const int alignV = item_->rotateAlign.getAlignV();
  if (sch::core::alignMarksHidden(alignH, alignV)) return false;

  // 1b. Centered both ways (nibble 0b1111) -- CONFIRMED by VisTest too.
  //     A pin label is placed by an
  //     edge or corner, never by its center -- QSpice's symbol editor
  //     treats the center point as "hidden". Robert's GmEnable symbol:
  //     OUT, IN- and IN+ carry 15 and are not shown, EN carries 7 (west)
  //     and is. PINS ONLY: centered TEXT is drawn (AnalogLatch's "Y1"),
  //     so sch::core::alignMarksHidden() itself is left alone.
  if (alignH == sch::core::kAlignHCenter && alignV == sch::core::kAlignVCenter)
    return false;

  // 2. An attached net name (P9). CONFIRMED -- not from the file, but
  //    from the behavior of QSpice itself: giving a pin a net name
  //    hides its label. The inverted Q is the file-level example, being
  //    the only hidden pin whose offset and alignment otherwise look
  //    like a visible one.
  if (isInternal()) return false;

  return true;
}

QString SchItemPin::connectedNetName() const {
  const QString own =
      sch::core::decodeQSpiceString(item_->netName.getValue()).trimmed();
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
  return labelAlignH(item_->rotateAlign.getAlignH(),
                     item_->rotateAlign.getRotDeg());
}

int SchItemPin::effectiveAlignV() const {
  return labelAlignV(item_->rotateAlign.getAlignV(),
                     item_->rotateAlign.getRotDeg());
}

bool SchItemPin::showsUnconnectedMark() const {
  // An internal pin is not drawn, so it cannot be flagged either.
  if (isInternal()) return false;

  // A pin of a symbol edited on its own: nothing is ever wired to it
  // there, so every one would be flagged.
  if (parentItem() == nullptr) return false;

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

bool SchItemPin::inSymbolDocument() const {
  if (parentItem() != nullptr) return false;

  const SchScene *sc = SchScene::of(this);
  return sc != nullptr && sc->symbolDocument();
}

QRectF SchItemPin::bodyRect() const {
  if (isInternal() && !inSymbolDocument()) return QRectF();

  const Point  p = item_->pt1.getValue();
  const double r = inSymbolDocument() ? kSymBoxUnits : kMarkerRadius;
  return QRectF(p.x - r, p.y - r, r * 2.0, r * 2.0);
}

QRectF SchItemPin::labelRect() const {
  if (!labelVisible()) return QRectF();

  // The DISPLAY form, as paint() draws it -- measuring the raw label
  // counted overbar markers as characters and made the bounds too wide.
  const QString label = overbarDisplayText(
      sch::core::decodeQSpiceString(item_->pinLabel.getValue()));

  const QFontMetricsF fm(makeFont(item_->fontSize.getValue()));
  // pt2 is the offset from the pin point to its label (per the record
  // documentation), so the label anchors there, not at the pin itself.
  const QPointF anchor =
      toQ(item_->pt1.getValue()) + toQ(item_->pt2.getValue());
  const QRectF local =
      alignedTextRect(fm, label, effectiveAlignH(), effectiveAlignV());

  // Through the same transform paint() draws with, so the reported box
  // is the one the glyphs occupy -- rotated, unmirrored and turned as
  // they are. (It used to apply only the Y-flip, which was right for an
  // upright label alone.)
  const QRectF painted =
      labelXform(static_cast<double>(item_->rotateAlign.getRotDeg()))
          .mapRect(local);
  return painted.translated(anchor);
}

void SchItemPin::paint(QPainter *painter, const QStyleOptionGraphicsItem *,
                       QWidget *) {
  // Internal pins draw nothing at all -- but in a symbol document, where
  // their box is drawn faded (their label never is, as in QSpice).
  if (isInternal() && !inSymbolDocument()) return;

  const QPointF center = toQ(item_->pt1.getValue());

  painter->setPen(Qt::NoPen);

  if (inSymbolDocument()) {
    // QSpice's symbol editor's pin: an open box round the point, filled
    // red while its row is selected in the Symbol Properties pane.
    const QTransform xform = painter->transform();
    const double     scale = std::hypot(xform.m11(), xform.m12());
    const double     half =
        qMax((scale > 0.0) ? (kSymBoxPx / scale) : 0.0, kSymBoxUnits);
    const QRectF box(center.x() - half, center.y() - half, half * 2, half * 2);

    const QColor color =
        isInternal() ? SchScene::faded(tint(RenderStyle::current().pinDot))
                     : tint(RenderStyle::current().pinDot);
    QPen pen(color);
    pen.setCosmetic(true);
    SchPen::enforcePrintMin(pen, painter);

    if (highlighted && !SelHiLiteStyle::suppressed) {
      painter->setPen(Qt::NoPen);
      painter->setBrush(QColor(Qt::red));
      painter->drawRect(box);
    } else {
      painter->setPen(pen);
      painter->setBrush(Qt::NoBrush);
      painter->drawRect(box);
      painter->setPen(Qt::NoPen);
      painter->setBrush(color);
      painter->drawEllipse(center, kMarkerRadius / 2.0, kMarkerRadius / 2.0);
    }
  } else if (highlighted && !SelHiLiteStyle::suppressed) {
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
        QRectF(center.x() - half, center.y() - half, half * 2, half * 2));
  } else {
    const double scale = SchPen::painterScale(painter);
    const double radius =
        std::max(kMarkerRadius, (scale > 0.0) ? kMarkerMinPx / scale : 0.0);
    painter->setBrush(tint(RenderStyle::current().pinDot));
    painter->drawEllipse(center, radius, radius);
  }

  // An unattached pin gets a small open box, so a pin nobody wired to
  // is visible at a glance rather than only at netlist time.
  //
  // NOT over a highlight: it is a small filled box in its own color, and
  // painting it on top left the red square with a hole in the middle.
  // Only the mark is skipped -- the label below is drawn either way; an
  // early return here once hid it while the pin was highlighted.
  const bool showHighlight = highlighted && !SelHiLiteStyle::suppressed;
  if (!showHighlight && showsUnconnectedMark()) {
    // Sized in scene units, never below a few DEVICE pixels -- drawn
    // here rather than as its own item, so it needs the painter's scale
    // to convert. hypot of the first row, not m11, which is scale*cos of
    // any rotation.
    const QTransform xform = painter->transform();
    const double     scale = std::hypot(xform.m11(), xform.m12());
    const double     half  = std::max(
        kUnconnectedBoxUnits, (scale > 0.0) ? kUnconnectedBoxPx / scale : 0.0);

    const QPointF c = toQ(item_->pt1.getValue());

    QPen pen(Qt::black);
    pen.setCosmetic(true);
    SchPen::enforcePrintMin(pen, painter);
    painter->setPen(pen);
    painter->setBrush(tint(RenderStyle::current().unconnectedPin));
    painter->drawRect(QRectF(c.x() - half, c.y() - half, half * 2, half * 2));
  }

  if (onDllBlock()) paintPortMark(painter);

  // The pin LABEL ("EN", "CLK", "OUT" ...), where this pin has a visible
  // one -- see labelVisible(). The MARKER above is drawn either way:
  // QSpice still shows the connection point for Vdd, Vss and the
  // inverted Q, it just doesn't name them.
  if (!labelVisible()) return;

  const QString rawLabel =
      sch::core::decodeQSpiceString(item_->pinLabel.getValue());
  const QString label = overbarDisplayText(rawLabel);

  const QFont font = makeFont(item_->fontSize.getValue());

  painter->save();
  painter->translate(toQ(item_->pt1.getValue()) + toQ(item_->pt2.getValue()));

  // The SAME transform SchItemText draws with -- a pin's label is symbol
  // text like any other: its own rotation from rotateAlign, the parent's
  // rotation, scale and mirror canceled, the Y-flip undone, and the
  // readability half turn. See SchItemBase::labelXform().
  painter->setTransform(
      labelXform(static_cast<double>(item_->rotateAlign.getRotDeg())),
      /*combine=*/true);

  painter->setFont(font);
  painter->setPen(tint(SchColorDecode::fromArgColor(item_->textColor.getValue(),
                                                    defaultTextColor())));

  const QFontMetricsF fm(font);
  const QRectF        rect =
      alignedTextRect(fm, label, effectiveAlignH(), effectiveAlignV());
  painter->drawText(rect, Qt::AlignLeft | Qt::AlignTop | Qt::TextDontClip,
                    label);

  drawOverbars(painter, fm, rawLabel, label, rect.left(),
               rect.top() + fm.ascent());

  painter->restore();
}

QColor SchItemPin::defaultTextColor() const {
  return RenderStyle::current().pin;
}

bool SchItemPin::onDllBlock() const {
  // A component's pin: its symbol. A symbol document's own pin (no
  // parent item): the symbol record it hangs from.
  NodePtr symNode;
  if (const SchItemBase *block =
          dynamic_cast<const SchItemBase *>(parentItem())) {
    if (block->node() == nullptr) return false;
    symNode = block->node()->findFirstOf(QPI::SYM);
  } else if (parentItem() == nullptr && node() != nullptr) {
    symNode = node()->parent();
    if (symNode != nullptr && symNode->getEnumID() != QPI::SYM)
      symNode = nullptr;
  }
  if (symNode == nullptr) return false;

  const NodePtr typeNode = symNode->findFirstOf(QPI::TYPE);
  if (typeNode == nullptr) return false;

  const ItemTypePtr type = std::static_pointer_cast<ItemType>(typeNode->item());
  if (type == nullptr) return false;

  const QString text =
      sch::core::decodeQSpiceString(type->text.getValue()).trimmed();
  return text.compare(QStringLiteral("Ø(.DLL)"), Qt::CaseInsensitive) == 0;
}

bool SchItemPin::portTypeUnset() const {
  if (!onDllBlock()) return false;

  const int direction = portDirection();
  if (direction == 0) return true;
  if (direction == ArgPinInfo::TYPE_DLLGND) return false;
  return portDataType() == 0;
}

QString SchItemPin::directionName(int direction) {
  switch (direction) {
  case ArgPinInfo::TYPE_INPORT:
    return QObject::tr("Input");
  case ArgPinInfo::TYPE_OUTPORT:
    return QObject::tr("OutPut");
  case ArgPinInfo::TYPE_DLLGND:
    return QObject::tr("DLL's GND");
  default:
    return QString();
  }
}

QString SchItemPin::dataTypeName(int dataType) {
  // QSpice's labels -- "float" is its 64-bit double, "shortfloat" C's
  // float.
  static const char *const kNames[] = {nullptr,
                                       "boolean",
                                       "char",
                                       "unsigned char",
                                       "short",
                                       "unsigned short",
                                       "integer",
                                       "unsigned integer",
                                       "shortfloat(32 bit)",
                                       "float(64 bit double)",
                                       "integer 64",
                                       "unsigned integer 64",
                                       "Bit Vector"};

  if (dataType <= 0 || dataType >= static_cast<int>(std::size(kNames)))
    return QString();
  return QObject::tr(kNames[dataType]);
}

void SchItemPin::paintPortMark(QPainter *painter) {
  const int  direction = portDirection();
  const int  dataType  = portDataType();
  const bool unset     = portTypeUnset();

  // The tooltip, kept here because this is where the port is drawn:
  // its type can change from the menu, and its block's type from the
  // Symbol Properties pane, neither of which this pin is told about.
  QString tipText;
  if (unset) tipText = QObject::tr("Port type not set");
  else if (direction == ArgPinInfo::TYPE_DLLGND)
    tipText = directionName(direction);
  else
    tipText = QStringLiteral("%1: %2").arg(directionName(direction),
                                           dataTypeName(dataType));
  if (toolTip() != tipText) setToolTip(tipText);

  // On screen only, as every editing mark.
  if (SelHiLiteStyle::suppressed) return;

  // OUTSIDE the box, on the lead: inside, the label starts right at the
  // pin (QSpice gives its ports no indent) and the mark would sit under
  // its first letters.
  const QPointF p   = toQ(item_->pt1.getValue());
  const QRectF  frame = (parentItem() != nullptr)
                            ? blockBoxOf(parentItem())
                            : mapRectFromScene(symbolBoxOf(scene()));
  const QPointF out   = -inwardAt(frame, p);
  const QPointF across(-out.y(), out.x());

  const QPointF nearEnd = p + out * kPortMarkGap;
  const QPointF farEnd  = p + out * (kPortMarkGap + kPortMarkLen);

  painter->save();
  painter->setRenderHint(QPainter::Antialiasing, true);

  // UNSET: a red "!", upright whatever the block's turn or mirror.
  if (unset) {
    painter->translate((nearEnd + farEnd) / 2.0);
    painter->setTransform(labelXform(0.0), /*combine=*/true);

    QFont font = makeFont(0.5);
    font.setBold(true);
    painter->setFont(font);
    painter->setPen(QColor(Qt::red));

    const QFontMetricsF fm(font);
    const QRectF        box = fm.boundingRect(QStringLiteral("!"));
    painter->drawText(QPointF(-box.width() / 2.0, fm.ascent() / 2.0 - 1.0),
                      QStringLiteral("!"));
    painter->restore();
    return;
  }

  const QColor color = tint(RenderStyle::current().pin);

  // DLL's GND: three bars, shortening away from the box.
  if (direction == ArgPinInfo::TYPE_DLLGND) {
    QPen pen(color);
    pen.setCosmetic(true);
    pen.setWidthF(1.5);
    painter->setPen(pen);

    const double step = kPortMarkLen / 3.0;
    for (int bar = 0; bar < 3; ++bar) {
      const QPointF mid  = nearEnd + out * (step * bar);
      const double  half = kPortMarkHalf * (1.0 - bar * 0.3);
      painter->drawLine(mid - across * half, mid + across * half);
    }
    painter->restore();
    return;
  }

  // Input points INTO the box, OutPut out of it.
  const bool    input = (direction == ArgPinInfo::TYPE_INPORT);
  const QPointF tip   = input ? nearEnd : farEnd;
  const QPointF base  = input ? farEnd : nearEnd;

  QPolygonF head;
  head << tip << base + across * kPortMarkHalf << base - across * kPortMarkHalf;

  painter->setPen(Qt::NoPen);
  painter->setBrush(color);
  painter->drawPolygon(head);
  painter->restore();
}

} // namespace sch::render
