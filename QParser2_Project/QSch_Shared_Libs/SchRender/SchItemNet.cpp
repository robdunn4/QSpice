#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable : 4996) // Qt headers reference APIs deprecated for Qt 7
#endif

#include "SchItemNet.h"
#include "SchItemWire.h"

#include <QFont>
#include <QFontMetricsF>
#include <QPainter>
#include <QPolygonF>
#include <QTransform>
#include <QPen>

#ifdef _MSC_VER
#pragma warning(pop)
#endif

#include <utility>

#include "RenderStyle.h"
#include "SchStringDecode.h"
#include "SchPen.h"
#include "SchTextLayout.h"

namespace sch::render {

namespace {

// ItemNet has no color field at all -- these are fixed conventions
// confirmed by visual comparison to real QSpice output, not decoded
// from the file.
// QSpice's own net-label colour (CADnetLabelText), which defaults to
// red -- the value this used to hard-code. Read from prefs so a
// customised QSpice palette is honoured.
QColor netNameColor() { return RenderStyle::current().netLabelText; }

const QColor kNetDescColor(0, 150, 0);

// --- Ground-family symbols -------------------------------------------
//
// A net whose name is GND, EARTH or CHASSIS draws a symbol instead of
// its name. MEASURED from QSpice's own rendering of a file containing
// all three in two orientations, against scale rects in the same file:
// each symbol occupies 160 units across by 80 deep, anchored at the
// middle of its top edge, with a full-width bar on that edge.
//
// Defined here pointing DOWN (the symbol extends to -y from the
// anchor); orientation is applied by groundXform() below.
enum class GroundKind { None, Gnd, Earth, Chassis };

constexpr double kGroundHalfWidth = 80.0;
constexpr double kGroundDepth     = 80.0;

GroundKind groundKindFor(const QString &name) {
  if (name.compare(QStringLiteral("GND"), Qt::CaseInsensitive) == 0)
    return GroundKind::Gnd;
  if (name.compare(QStringLiteral("EARTH"), Qt::CaseInsensitive) == 0)
    return GroundKind::Earth;
  if (name.compare(QStringLiteral("CHASSIS"), Qt::CaseInsensitive) == 0)
    return GroundKind::Chassis;
  return GroundKind::None;
}

// Orientation comes from the net's OWN rotateAlign -- the same field
// that positions a net NAME, used the same way: the alignment names the
// edge the anchor sits on, and the symbol extends away from it.
//
// CONFIRMED from the reference file: alignment 13 (H=CENTER, V=NORTH)
// draws downward from an anchor on the top edge; alignment 11 (H=EAST,
// V=CENTER) draws leftward from an anchor on the right edge. The two
// remaining combinations follow by symmetry and have NOT been seen in a
// file.
//
// Returned as a transform applied to the canonical downward geometry.
QTransform groundXform(int alignH, int alignV) {
  QTransform xform;

  // Rotations are expressed directly as the mapping of the canonical
  // "down" axis, rather than through painter rotation, because this
  // same transform is needed by getRect() where there is no painter.
  if (alignV == sch::core::kAlignVNorth) return xform;              // extends down
  if (alignV == sch::core::kAlignVSouth) return xform.rotate(180);  // extends up
  if (alignH == sch::core::kAlignHEast) return xform.rotate(-90);   // extends left
  if (alignH == sch::core::kAlignHWest) return xform.rotate(90);    // extends right

  return xform; // nothing recognised -- treat as the common downward case
}

// The symbol's line work in canonical (downward) coordinates, relative
// to the anchor. Returned as polylines so one routine serves painting
// and bounds alike.
QList<QPolygonF> groundShape(GroundKind kind) {
  const double w = kGroundHalfWidth;

  switch (kind) {
  case GroundKind::Gnd: {
    // Full-width top edge, apex at the bottom centre.
    QPolygonF tri;
    tri << QPointF(-w, 0) << QPointF(w, 0) << QPointF(0, -kGroundDepth)
        << QPointF(-w, 0);
    return {tri};
  }

  case GroundKind::Earth: {
    // Three bars, halving in width as they descend.
    QPolygonF bar1, bar2, bar3;
    bar1 << QPointF(-w, 0) << QPointF(w, 0);
    bar2 << QPointF(-w / 2, -30) << QPointF(w / 2, -30);
    bar3 << QPointF(-w / 4, -60) << QPointF(w / 4, -60);
    return {bar1, bar2, bar3};
  }

  case GroundKind::Chassis: {
    // Top bar plus three hatch strokes running down and to the left,
    // each dropping 50 units while moving 30 to the left.
    QPolygonF bar, s1, s2, s3;
    bar << QPointF(-w, 0) << QPointF(w, 0);
    s1 << QPointF(-50, 0) << QPointF(-80, -50);
    s2 << QPointF(0, 0) << QPointF(-30, -50);
    s3 << QPointF(50, 0) << QPointF(20, -50);
    return {bar, s1, s2, s3};
  }

  case GroundKind::None:
    break;
  }

  return {};
}

// The shape in the item's own coordinates: canonical geometry, oriented,
// then moved to the anchor.
QList<QPolygonF> groundPolys(GroundKind kind, int alignH, int alignV,
                             const QPointF &anchor) {
  const QTransform xform = groundXform(alignH, alignV);

  QList<QPolygonF> polys;
  for (const QPolygonF &poly : groundShape(kind))
    polys.append(xform.map(poly).translated(anchor));
  return polys;
}

// name/desc positioned as if a newline separated them: same horizontal
// alignment rule (alignH) applied independently to each line's own
// width, stacked vertically so the two-line block as a whole stays on
// the correct side of the anchor (matching what a single line would do
// for that alignV, rather than name and desc straddling the anchor on
// opposite sides of it). Both rects are in the local, counter-flipped
// (Qt-native) frame paint() draws in.
struct NetTextLayout {
  QRectF nameRect; // isNull() if no name is drawn (ground symbol, or empty)
  QRectF descRect; // isNull() if desc is empty
};

// Side padding for a label anchored on its LEFT or RIGHT edge: a fraction
// of the font HEIGHT, measured against QSpice's own port rendering (its
// box is aspect 1.95 where the bare text rect is 1.38, about 57 units a
// side at font size 1).
double labelSidePad(const QFontMetricsF &fm) { return 0.30 * fm.height(); }

// How far the text moves AWAY from an edge anchor -- every label, port or
// not. Without it plain text started right on the anchor and crowded
// the wire it hangs from; with it, a port's box (the text rect widened
// by the same pad) lands with its anchored edge exactly on the anchor.
// So a port is just a plain label with a box drawn around it, and the
// two place their text identically. A centred label needs no shift.
//
// UNMEASURED for plain labels: the amount is the port's, which is what
// QSpice's framed text implies, but bare text has not been compared.
double labelTextShift(const QFontMetricsF &fm, int alignH) {
  if (alignH == sch::core::kAlignHWest) return labelSidePad(fm);
  if (alignH == sch::core::kAlignHEast) return -labelSidePad(fm);

  return 0.0;
}

NetTextLayout computeLayout(const QFontMetricsF &fm, const QString &name,
                            const QString &desc, int alignH, int alignV,
                            bool drawName) {
  NetTextLayout layout;
  // name's position is computed exactly as if desc didn't exist at all
  // -- unaffected by whether a description is present.
  if (drawName) layout.nameRect = alignedTextRect(fm, name, alignH, alignV);
  if (!desc.isEmpty())
    layout.descRect = alignedTextRect(fm, desc, alignH, alignV);

  if (drawName && !desc.isEmpty()) {
    // "As if a carriage return was placed between them": desc always
    // follows as the next line down from name's own (unmoved) top, one
    // line-height below, using the SAME horizontal alignment rule
    // (alignH) applied independently to desc's own width. This can put
    // desc past the anchor point for some alignV values (e.g. SOUTH,
    // where name's own bottom sits at the anchor) -- confirmed that's
    // genuinely how QSpice renders it, not something to correct for.
    layout.descRect.moveTop(layout.nameRect.top() + fm.height());
  }

  // Both lines move together, so the description stays under the name.
  const double shift = labelTextShift(fm, alignH);
  if (!layout.nameRect.isNull()) layout.nameRect.translate(shift, 0.0);
  if (!layout.descRect.isNull()) layout.descRect.translate(shift, 0.0);

  return layout;
}

} // namespace

SchItemNet::SchItemNet(ItemNetPtr item, QGraphicsItem *parent)
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

  // BELOW the wires, unlike other text.
  //
  // A port's box is filled, and QSpice draws the wires over it -- the
  // wire runs to the anchor and disappears under the box rather than
  // stopping short of it. Ordinary labels are transparent, so their
  // being below changes nothing.
  setZValue(kZFill);
}

namespace {

// A port's BOX, in the text frame -- the one definition, so painting and
// bounds cannot disagree about where it is.
//
// The text rect widened by labelSidePad() on each side. The text has
// already been moved off an edge anchor by that same amount (see
// computeLayout()), so the box's anchored edge sits exactly on the
// anchor.
QRectF portBoxRect(const QFontMetricsF &fm, const QRectF &nameRect) {
  const double pad = labelSidePad(fm);
  return nameRect.adjusted(-pad, 0.0, pad, 0.0);
}

} // namespace

QString SchItemNet::netDesc() const {
  return sch::core::decodeQSpiceString(item_->netDesc.getValue());
}

void SchItemNet::setNetDesc(const QString &text) {
  prepareGeometryChange();

  item_->netDesc.setValue(
      text.isEmpty()
          ? std::string()
          : ArgString(sch::core::encodeQSpiceString(text)).quote().getValue());

  update();
}

bool SchItemNet::isGroundName(const QString &name) {
  return groundKindFor(name) != GroundKind::None;
}

bool SchItemNet::isGroundSymbol() const {
  return groundKindFor(sch::core::decodeQSpiceString(item_->netName.getValue())) !=
         GroundKind::None;
}

int SchItemNet::groundAlign() const {
  return item_->rotateAlign.getAlign();
}

void SchItemNet::setGroundAlign(int align) {
  prepareGeometryChange();
  item_->rotateAlign.setAlign(static_cast<unsigned char>(align));
}

Rect SchItemNet::getRect() const noexcept {
  const QString name = sch::core::decodeQSpiceString(item_->netName.getValue());
  const QString desc = sch::core::decodeQSpiceString(item_->netDesc.getValue());
  const GroundKind kind = groundKindFor(name);
  const Point      p    = item_->pt1.getValue();

  if (kind != GroundKind::None) {
    // Bounds come from the SAME polygons paint() draws, so an oriented
    // symbol can't report the downward-facing box while drawing a
    // sideways one.
    //
    // Accumulated point by point rather than with QRectF::united().
    // united() returns THE OTHER rectangle when one is empty, and a
    // horizontal polyline's bounding rect has zero height and so counts
    // as empty -- so EARTH's three bars and CHASSIS's top bar were
    // discarded one after another and the symbol reported barely any
    // bounds at all. GND escaped it only because a triangle's box has
    // area.
    bool   first = true;
    double minX = 0.0, minY = 0.0, maxX = 0.0, maxY = 0.0;

    for (const QPolygonF &poly :
         groundPolys(kind, item_->rotateAlign.getAlignH(),
                     item_->rotateAlign.getAlignV(), QPointF(0, 0))) {
      for (const QPointF &pt : poly) {
        if (first) {
          minX = maxX = pt.x();
          minY = maxY = pt.y();
          first = false;
          continue;
        }
        minX = qMin(minX, pt.x());
        maxX = qMax(maxX, pt.x());
        minY = qMin(minY, pt.y());
        maxY = qMax(maxY, pt.y());
      }
    }

    // Built DIRECTLY, without toGenerousRect().
    //
    // That helper is for text: it pads by 20 units to give glyph
    // hit-testing some slack, and it converts from the Y-down text
    // frame. Neither applies here -- these polygons are exact geometry
    // already in the item's Y-up frame, so the padding only inflated
    // the symbol's selection box beyond what is drawn (an exclusive
    // band had to enclose 20 units of empty space on every side before
    // it would select), and the frame conversion was a round trip
    // through a mirror for nothing.
    //
    // Repaint coverage is unaffected: boundingRect() adds
    // SchItemBase::kPaintSlack on top of whatever this returns.
    return Rect(Point(p.x + static_cast<int>(std::floor(minX)),
                      p.y + static_cast<int>(std::ceil(maxY))),
                Point(p.x + static_cast<int>(std::ceil(maxX)),
                      p.y + static_cast<int>(std::floor(minY))));
  }

  // Measured as DISPLAYED: overbar markers hidden unless Raw mode.
  const QString shownName = overbarDisplayText(name);
  const QString shownDesc = overbarDisplayText(desc);

  const QFontMetricsF fm(makeFont(item_->fontSize.getValue()));
  const NetTextLayout layout =
      computeLayout(fm, shownName, shownDesc, item_->rotateAlign.getAlignH(),
                    item_->rotateAlign.getAlignV(), !shownName.isEmpty());

  // A PORT is its BOX: the drawn border is what the user sees as the
  // label, so selection, hit-testing and repaint all follow it. The
  // tightened text rect below sits inside the box, which left the
  // selection outline drawn through the port's border rather than
  // around it.
  //
  // No margin: the box is exact geometry, not glyph extent.
  if (isPort() && !layout.nameRect.isNull())
    return toItemRect(p, portBoxRect(fm, layout.nameRect));

  // The NAME only. A description is documentation hung below the label,
  // not part of it: including it made the selection box reach well past
  // what the user thinks of as the net.
  QRectF united;
  if (!layout.nameRect.isNull())
    united = tightenTextRect(fm, shownName, layout.nameRect);

  return toGenerousRect(p, united);
}

QRectF SchItemNet::boundingRect() const {
  // A port's box needs nothing extra here: getRect() returns the box,
  // and the base bounds come from getRect().
  const QRectF bounds = SchItemBase::boundingRect();

  const QString desc = sch::core::decodeQSpiceString(item_->netDesc.getValue());
  if (desc.isEmpty() || descHidden_ || isGroundSymbol()) return bounds;

  // The description's own rect, in the item's frame. paint() draws it
  // through a Y-flip, so the rect computed in the text frame is
  // mirrored back here.
  const QString name =
      overbarDisplayText(sch::core::decodeQSpiceString(item_->netName.getValue()));
  const QFontMetricsF fm(makeFont(item_->fontSize.getValue()));

  const NetTextLayout layout = computeLayout(
      fm, name, overbarDisplayText(desc), item_->rotateAlign.getAlignH(),
      item_->rotateAlign.getAlignV(), !name.isEmpty());

  if (layout.descRect.isNull()) return bounds;

  // toGenerousRect(), the same conversion getRect() uses for the name.
  //
  // It takes the ANCHOR, which is what the hand-rolled flip here got
  // wrong: these items hold ABSOLUTE scene coordinates rather than using
  // setPos -- paint() translates by pt1 itself -- so a rect built in the
  // local text frame and never offset lands near the scene origin
  // instead of at the label, leaving the description outside the
  // reported bounds after all.
  //
  // It also handles the Y-flip and the text padding, so there is one
  // conversion rather than a copy that can disagree with it.
  const QRectF descBounds =
      toQ(toGenerousRect(item_->pt1.getValue(), layout.descRect));

  return bounds.united(descBounds.adjusted(-kPaintSlack, -kPaintSlack,
                                           kPaintSlack, kPaintSlack));
}

Point SchItemNet::getPosition() const noexcept { return item_->pt1.getValue(); }

void SchItemNet::moveTo(Point p) noexcept {
  prepareGeometryChange();
  item_->pt1.setValue(p);
}

QList<QPointF> SchItemNet::connectionPoints() const {
  // A net label connects at its anchor point, not anywhere in its text.
  return {toQ(item_->pt1.getValue())};
}

QString SchItemNet::netName() const {
  return sch::core::decodeQSpiceString(item_->netName.getValue()).trimmed();
}

double SchItemNet::fontSize() const { return item_->fontSize.getValue(); }

void SchItemNet::setFontSize(double size) {
  prepareGeometryChange();
  item_->fontSize.setValue(static_cast<float>(size));
}

SchItemBase::TextKind SchItemNet::textKind() const {
  // Node labels get the single-anchor-point treatment (per spec) and
  // are single-line only.
  return TextKind::NetName;
}

SchTextEditInfo SchItemNet::textEditInfo() const {
  SchTextEditInfo info;
  info.text   = sch::core::decodeQSpiceString(item_->netName.getValue());
  info.anchor = item_->pt1.getValue();
  // ItemNet has no color field -- the editor shows the same fixed
  // convention colour paint() draws the name in.
  info.color         = netNameColor();
  info.fontPixelSize = fontPixelSize(item_->fontSize.getValue());
  info.alignH        = item_->rotateAlign.getAlignH();
  info.alignV        = item_->rotateAlign.getAlignV();
  return info;
}

void SchItemNet::applyTextEdit(const SchTextEditInfo &info) {
  prepareGeometryChange();

  item_->netName.setValue(
      ArgString(sch::core::encodeQSpiceString(info.text)).quote().getValue());

  // Net names only ever show the single active anchor point, so
  // alignment can't actually change through editing -- applied anyway
  // rather than special-cased, so this stays correct if node labels
  // ever do get the full grid.
  item_->rotateAlign.setAlignH(static_cast<unsigned char>(info.alignH));
  item_->rotateAlign.setAlignV(static_cast<unsigned char>(info.alignV));
  item_->pt1.setValue(info.anchor);
}

void SchItemNet::paint(QPainter *painter, const QStyleOptionGraphicsItem *,
                       QWidget *) {
  const QString name = sch::core::decodeQSpiceString(item_->netName.getValue());
  const QString desc = sch::core::decodeQSpiceString(item_->netDesc.getValue());

  // Nets named GND, EARTH or CHASSIS draw a symbol instead of their
  // name. Geometry and orientation are described where the shapes are
  // defined, near the top of this file.
  //
  // Drawn directly in toQ() space (no local counter-flip) since line
  // work doesn't have the "renders upside down" problem text glyphs do
  // -- same convention as Rect/Line/Wire.
  const GroundKind kind = groundKindFor(name);
  if (kind != GroundKind::None) {
    // The ground SYMBOL is wire-coloured, not net-label-coloured: it is
    // drawn line work, the same blue as the wires it connects to, and
    // only the net NAME text uses the net-label colour. Briefly setting
    // it to netLabelText() turned every ground symbol red.
    // The WIRE's pen, so on paper and in SVG it matches the wires.
    QPen pen = SchItemWire::wirePen(painter);
    SchPen::toGeometric(pen, painter);
    painter->setPen(pen);
    painter->setBrush(Qt::NoBrush);

    for (const QPolygonF &poly :
         groundPolys(kind, item_->rotateAlign.getAlignH(),
                     item_->rotateAlign.getAlignV(),
                     toQ(item_->pt1.getValue())))
      painter->drawPolyline(poly);
  }

  const QFont font = makeFont(item_->fontSize.getValue());
  painter->setFont(font);
  const QFontMetricsF fm(font);

  painter->save();
  painter->translate(toQ(item_->pt1.getValue()));
  painter->scale(1.0, -1.0); // counteract SchView's view-level Y-flip

  const int  alignH   = item_->rotateAlign.getAlignH();
  const int  alignV   = item_->rotateAlign.getAlignV();
  const bool drawName = (kind == GroundKind::None) && !name.isEmpty();

  // name/desc are RAW (overbar markers present); these are what is
  // drawn and laid out -- see overbarDisplayText().
  const QString shownName = overbarDisplayText(name);
  const QString shownDesc = overbarDisplayText(desc);

  const NetTextLayout layout =
      computeLayout(fm, shownName, shownDesc, alignH, alignV, drawName);

  // Qt::TextDontClip is essential here -- see the identical rationale
  // in SchItemText.cpp (confirmed directly: without this flag,
  // QPainter::drawText() clips to the given rect regardless of the
  // actual text extent).
  if (drawName) {
    // A PORT is boxed: white behind the name with a border in the net
    // colour, which is how QSpice shows that the name reaches beyond
    // this sheet. An ordinary net label is text on the bare background.
    //
    // WHITE rather than the background colour: the box is meant to
    // stand out from the sheet, so following a dark background would
    // defeat it.
    if (isPort()) {
      // The text's metric rect, WIDENED.
      //
      // Measured against QSpice's own rendering of the same file, with
      // both normalised by box height so the image scales cancel: the
      // heights agree (top and bottom gaps 0.21/0.17 against our
      // 0.25/0.18), while QSpice's box is half again as wide -- aspect
      // 1.95 against 1.38, about 57 units a side at font size 1.
      //
      // Expressed as a fraction of the font's HEIGHT so it tracks the
      // font size, there being no fixed unit it could plausibly be.
      //
      // UNVERIFIED in one respect: every name in the test file is three
      // characters, so a per-character padding would measure identically
      // to this constant one. Names of differing lengths would separate
      // the two.
      const QRectF box = portBoxRect(fm, layout.nameRect);

      QPen border(netNameColor());
      border.setCosmetic(true); // one pixel at any zoom, like a wire
      SchPen::enforcePrintMin(border, painter);

      painter->setPen(border);
      painter->setBrush(QColor(Qt::white));
      painter->drawRect(box);
      painter->setBrush(Qt::NoBrush);
    }

    painter->setPen(netNameColor());
    painter->drawText(layout.nameRect,
                      Qt::AlignLeft | Qt::AlignTop | Qt::TextDontClip,
                      shownName);

    // Overbars are part of the NAME: Q and barred Q are different nets.
    // drawText(rect, AlignTop) puts the baseline one ascent below the
    // top, which is where the bars are measured from.
    drawOverbars(painter, fm, name, shownName, layout.nameRect.left(),
                 layout.nameRect.top() + fm.ascent());
  }
  // A GROUND never shows one. QSpice keeps the field on a ground symbol
  // and offers the menu entry, but never draws the result -- so a
  // description typed there would be invisible, which is worse than not
  // offering it. We do neither.
  if (!desc.isEmpty() && kind == GroundKind::None && !descHidden_) {
    painter->setPen(kNetDescColor);
    painter->drawText(layout.descRect,
                      Qt::AlignLeft | Qt::AlignTop | Qt::TextDontClip,
                      shownDesc);
    drawOverbars(painter, fm, desc, shownDesc, layout.descRect.left(),
                 layout.descRect.top() + fm.ascent());
  }

  painter->restore();
}

} // namespace sch::render
