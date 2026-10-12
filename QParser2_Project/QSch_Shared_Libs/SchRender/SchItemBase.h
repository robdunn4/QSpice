#pragma once
#include <QColor>
#include <QGraphicsItem>
#include <QPainterPath>
#include <QList>
#include <QString>
#include <QTransform>

#include "ArgUtils.h"
#include "ItemTree.h"
#include "SchRect.h"

namespace sch::render {


// Snapshot of a text-like element's editable state, handed to the
// inline editor when live-edit begins (see
// SchItemBase::textEditInfo()). Font size is already the calibrated
// PIXEL size in schematic units -- fontPixelSize() of the record's raw
// fontSize field -- not the raw value, so the editor doesn't need to
// know about that calibration at all.
struct SchTextEditInfo {
  QString text;
  Point   anchor;
  QColor  color         = Qt::black;
  double  fontPixelSize = 0.0;
  int     alignH        = 0;
  int     alignV        = 0;
};

// True if two snapshots describe the same EDITABLE state. Deliberately
// ignores color and fontPixelSize: those are read-only inputs to the
// editor (it renders with them, it never writes them back), so a
// difference there could only come from the element itself changing
// underneath the edit, not from anything the user did in the box.
//
// Hand-written rather than a defaulted operator== because Point (from
// QParser2_Shared) has no equality operator of its own and can't be
// given one from here.
inline bool sameTextEditState(const SchTextEditInfo &a,
                              const SchTextEditInfo &b) {
  return a.text == b.text && a.anchor.x == b.anchor.x &&
         a.anchor.y == b.anchor.y && a.alignH == b.alignH &&
         a.alignV == b.alignV;
}

// Common base for all QSpiceGraphics wrapper items.
//
// getRect()/getPosition()/moveTo() are OUR OWN interface (not inherited
// from anything in QParser2_Shared -- that library's DrawIntf class was
// a temporary first attempt and has since been removed from the library
// entirely, so this project has no dependency on it at all). Each
// concrete wrapper class implements these three by reading/writing the
// fields of the typed ItemXxx pointer it holds. Point (from ArgUtils.h)
// is still QParser2_Shared's own type; Rect used to live there too
// (DrawUtils.h) but was removed along with DrawIntf, so it's now defined
// in our own SchRect.h instead.
class SchItemBase : public QGraphicsItem {
public:
  using QGraphicsItem::QGraphicsItem;

  // Something global that this item's DRAWN SIZE depends on has changed
  // -- View > Raw Attributes, which shows or hides overbar markers and
  // so changes text widths. prepareGeometryChange() is protected, so a
  // caller outside the item cannot do this itself.
  void displayChanged() {
    prepareGeometryChange();
    update();
  }

  virtual Rect  getRect() const noexcept     = 0;
  virtual Point getPosition() const noexcept = 0;
  virtual void  moveTo(Point p) noexcept     = 0;

  // --- Live-edit support ---
  //
  // Live-edit (entered by double-clicking a top-level item) shows small
  // draggable handle boxes at each of a graphical element's control
  // points, letting the user reshape it directly. Coordinates are in
  // this item's own local coordinate system -- for the top-level items
  // live-edit currently applies to, that's the same as scene
  // coordinates, since none of them have a parent transform.
  //
  // Default: no control points (e.g. SchItemComp, which will get its
  // own, different live-edit treatment later -- see the project notes
  // this was deferred against).
  virtual QList<Point> controlPoints() const { return {}; }

  // Sets the control point at the given index (matching
  // controlPoints()'s own ordering) to a new value. Must call
  // prepareGeometryChange() itself, the same as moveTo() does, since
  // this changes the item's geometry.
  //
  // ORDERING, and it matters: prepareGeometryChange() goes FIRST, before
  // the values getRect() reads are actually changed. Qt removes the item
  // from the scene's BSP index using the boundingRect() it reports at
  // that moment -- so calling it afterwards searches the NEW rect's
  // leaves and leaves the item still referenced under its OLD one. The
  // stale entry is harmless while the item lives (a redundant hit) and
  // fatal once it's deleted: the next paint walks the tree and
  // dereferences freed memory. Every mutator in this hierarchy had the
  // call last until an access violation in QGraphicsSceneBspTree::items
  // during paint traced back to exactly this.
  virtual void setControlPoint(int index, Point p) {
    Q_UNUSED(index);
    Q_UNUSED(p);
  }

  // --- Inline text-edit support ---
  //
  // For text-like elements (SchItemText/SchItemNet), live-edit shows an
  // inline text editor (SchEditText) instead of control-point handles.
  //
  // textKind() is both the capability test and the dispatch: None means
  // this element isn't text-editable at all; anything else says what
  // KIND of text it is, and the editor picks its editing context from
  // that (QSchEdit's SchEditTextCtx.h -- decoration style, multi-line
  // legality, the right-click menu). Default: not text-editable.
  enum class TextKind {
    None,      // not text
    Directive, // SPICE directives, comments, symbol attribute text
    NetName,   // net labels
  };
  virtual TextKind textKind() const { return TextKind::None; }

  // Everything the editor needs to stand in for this element visually:
  // the current text, its anchor point, and the font/color/alignment it
  // renders with. Returned as one struct rather than as four separate
  // virtuals because they're always wanted together, always come from
  // the same underlying record, and the set will grow (the anchor grid
  // needs to write alignment back).
  virtual SchTextEditInfo textEditInfo() const { return SchTextEditInfo(); }

  // Applies an edited snapshot back to the model: text, anchor point,
  // and alignment (the three things editing can change -- color and
  // font size are read-only here). Called by EditTextSessionCmd, not during
  // editing: the editor keeps its own state and the model is untouched
  // until the edit commits.
  //
  // Anchor and alignment travel together on purpose. Clicking a
  // different anchor point changes BOTH -- the alignment value and a
  // compensating pt1 -- and applying one without the other would jump
  // the element by up to its own width or height.
  virtual void applyTextEdit(const SchTextEditInfo &info) { Q_UNUSED(info); }

  // The element's own geometry, exactly -- no slack. This is what the
  // selection outline is drawn around and what hit-testing uses.
  QRectF elementRect() const { return toQ(getRect()); }

  // The element's CORE geometry, for deciding what a selection band
  // encloses -- as opposed to everything it paints.
  //
  // The two differ where an element draws text that is part of its
  // appearance but not part of its shape: a pin's label is the case
  // that matters. elementRect() must cover the label, or it smears when
  // dragged; a component's selection box must NOT, or a band has to
  // enclose labels that may sit well outside the symbol's outline
  // before it will select it.
  //
  // Defaults to elementRect(); only elements with that split override.
  virtual QRectF bodyRect() const { return elementRect(); }

  // elementRect() plus a slack allowance (see kPaintSlack), united with
  // childrenBoundingRect(). Strictly a REPAINT region -- it is not the
  // element's geometry and must not be used as such; elementRect() is.
  QRectF boundingRect() const override;

  // Pinned to elementRect(), NOT to the padded boundingRect() above.
  // QGraphicsItem's default shape() follows boundingRect(), which would
  // otherwise make every element selectable from the slack distance
  // away -- the padding is for repainting, and must not leak into
  // hit-testing or rubber-band selection.
  QPainterPath shape() const override;

  // --- Picking: how firmly a point hits this element ---
  //
  // Ordered weakest to strongest; a click compares them across every
  // element under the cursor (SchViewInteract::hitsAt()).
  //
  //   Inside  within a HOLLOW shape's outline, not on it. Not a click
  //           hit at all -- it only lets a drag grab a shape that is
  //           ALREADY selected.
  //   Weak    within a FILLED shape's interior.
  //   Body    a component's body (or one of its labels).
  //   Firm    on the thing itself: an outline or stroke, a wire, a pin,
  //           a junction, text.
  enum class HitLevel { None = 0, Inside = 1, Weak = 2, Body = 3, Firm = 4 };

  // How `localPos` (this element's own coordinates) hits it, allowing
  // `tolerance` (same units) for a stroke -- a few SCREEN pixels, which
  // the caller converts.
  //
  // Default: an element with an outlinePath() is hit on its stroke
  // (pen half-width plus tolerance), and -- when it has a fill field --
  // inside it, Weak or Inside by interiorHollow(). Anything else is hit
  // Firm anywhere within shape(), widened by the tolerance.
  virtual HitLevel hitLevel(const QPointF &localPos, double tolerance) const;

  // The line work as drawn, in local coordinates; empty for elements
  // picked by shape() instead. Closed for filled shapes.
  virtual QPainterPath outlinePath() const { return QPainterPath(); }

  // A shape whose interior LOOKS empty: no fill, a fill in the canvas
  // background colour (a knockout), or an unstuffed component's outline
  // drawing. Image fills are content, never hollow.
  virtual bool interiorHollow() const;

  // Paint order within a container (a symbol's elements, or the scene's
  // top-level items). Qt paints equal-Z siblings in insertion order,
  // which follows the QPI sort order of the tree -- and that put FILLED
  // shapes after the things they should sit behind, so a filled symbol
  // body painted over its own pins and labels. An opaque body made
  // everything else in the symbol invisible.
  //
  // Ordered by what should be readable on top of what, not by element
  // type: areas first, then line work, then connection markers, then
  // text.
  enum ZOrder {
    kZFill    = 0, // rect, ellipse -- may be opaque, so underneath
    kZGraphic = 1, // lines, zigzag, coil, arc, wires
    kZPin     = 2, // pin markers
    kZJunction = 3,
    kZText    = 4 // labels last -- always readable
  };

  // --- Document tree back-reference ---
  //
  // The TreeNode this wrapper was built from, set by SchTreeBuilder.
  //
  // Needed because editing can now ADD and REMOVE elements (splitting a
  // wire, deriving a junction), and those have to change the document
  // tree, not only the scene -- the tree is what Save writes. Without a
  // link back, removing an element would mean searching the whole tree
  // for the node whose item() happens to match.
  //
  // May be null: items created during an edit are attached to the tree
  // by the command that creates them, and anything not built from a
  // tree (future UI-only overlays) has no node at all.
  void           setNode(const NodePtr &node) { node_ = node; }
  const NodePtr &node() const { return node_; }

  // --- Electrical connectivity ---
  //
  // Whether this element can participate in electrical connections at
  // all: wires, pins, net labels, junctions and components. Graphical
  // elements (rect, line, ellipse, zigzag, coil, text) are not
  // electrical even though a line looks like a wire.
  //
  // Drives two things: the snap granularity for a drag (electrical
  // geometry must land on 100-unit boundaries or QSpice will not treat
  // items as connected), and which elements participate in junction
  // derivation.
  virtual bool isElectrical() const { return false; }

  // The points at which this element connects, in the element's OWN
  // local coordinates -- a wire's two endpoints, a pin's or net label's
  // single point, a component's pins gathered from its children.
  //
  // Local rather than scene coordinates so the caller decides when to
  // map: a component's children are positioned under its transform, and
  // mapping once at the top is both cheaper and harder to get wrong
  // than having every element reach for its own scene transform.
  // sceneConnectionPoints() does the mapping.
  virtual QList<QPointF> connectionPoints() const { return {}; }

  // connectionPoints(), mapped to scene coordinates.
  QList<QPointF> sceneConnectionPoints() const {
    QList<QPointF> points;
    for (const QPointF &p : connectionPoints()) points.append(mapToScene(p));
    return points;
  }

  // Whether this element's parent component is mirrored. SchItemComp
  // expresses a mirror as a negative-X scale in its own transform(),
  // separately from setRotation().
  //
  // Elements need this because a mirror is NOT applied uniformly by
  // QSpice: see SchItemZigzag::paint() and SchItemText::textXform().
  bool parentMirrored() const;

  // Uniform scale the parent's transform applies, or 1.0 with no
  // parent.
  //
  // A component rotated to an odd code carries a sqrt(2) scale -- see
  // sch::core::rotdecode::rotMatrix(). Symbol GEOMETRY should take that scale
  // (QSpice draws a diagonal symbol 41% larger), but TEXT should not:
  // measured against QSpice's own output, a label on a 45-degree symbol
  // is the same glyph size as one on an upright symbol, merely rotated.
  // Anything drawing text inside a component divides it out.
  double parentScale() const;

  // --- Symbol labels: attribute text and pin names ---
  //
  // A label inside a component is drawn upright against the component's
  // rotation, mirror and diagonal scale, turned by its OWN record
  // rotation (`ownRotDeg`) -- and then, if that would leave it upside
  // down on screen, turned a further half turn so it reads normally.
  //
  // MEASURED against QSpice (PolarityTest.qsch): mirrored resistors at
  // codes 14, 13 and 11 came out at 180, -135 and +135 degrees, where
  // QSpice shows 0, +45 and -45 -- each exactly half a turn away. No
  // linear rule in the component and label rotations fits all sixteen
  // orientations; "never upside down" fits every one. Applied to every
  // label, mirrored or not: one that is already readable is unaffected.
  //
  // UNCONFIRMED: labels that end up exactly vertical are left as they
  // are, whichever way they read.

  // Maps the Y-down text-layout frame into this element's own frame.
  // The single source of truth for drawing a label and for its bounds.
  QTransform labelXform(double ownRotDeg) const;

  // Whether labelXform() added the readability half turn.
  bool labelTurned(double ownRotDeg) const;

  // The alignment a label is LAID OUT with -- the record's own nibble,
  // west and east swapped under a mirror to make up for the mirror
  // cancellation. The readability turn swaps NOTHING: QSpice turns the
  // box about its anchor, so it moves to the anchor's other side. The
  // record itself is never changed; editing works on the stored value.
  int labelAlignH(int alignH, double ownRotDeg) const;
  int labelAlignV(int alignV, double ownRotDeg) const;

  // --- Rotation, for elements that store one ---
  //
  // Returns -1 when the element has no rotation of its own (a wire is
  // rotated by moving its endpoints, not by a field). Codes follow
  // ArgRot: bits 0-2 the eighth-turn, bit 3 the mirror.
  // --- Orientation ---
  //
  // An OPAQUE, element-specific code. A component stores ArgRot (three
  // rotation bits plus a mirror bit); a text element stores the whole
  // ArgRotAlign byte, because mirroring text changes its ALIGNMENT as
  // well as its rotation and the two must move together. Callers never
  // interpret the value -- they ask the element what it becomes after a
  // turn or a mirror.
  //
  // -1 means the element has no orientation the user can change: a wire
  // turns by having its endpoints moved, and a net label's alignment is
  // derived from what it connects to rather than chosen.
  // --- Font size ---
  //
  // The record's size MULTIPLIER, not a point size. Text elements and
  // net labels both carry one; everything else returns -1 and is not
  // offered the choice.
  // --- Edit guides ---
  //
  // Construction geometry drawn while the element is in live edit:
  // dashed lines and dots showing what its handles MEAN, where that is
  // not obvious from the element itself. In ITEM coordinates.
  //
  // Empty by default -- most elements need none, since their handles sit
  // on the thing they control. The four-point arc is the case that does:
  // two handles bound an ellipse the arc may barely touch, and two more
  // are rays whose distance from the centre is ignored.
  struct EditGuide {
    QPainterPath   dashed;
    QList<QPointF> dots;
  };

  virtual EditGuide editGuide() const { return EditGuide(); }

  // --- Display options that painting needs ---
  //
  // Held as a static because painting has no route to the per-view
  // Prefs object: items are constructed by the tree builder, which
  // knows nothing about the view. SchView pushes the value in when it
  // is created or the preference changes.
  //
  // One flag hardly justifies a mechanism; when there are several, this
  // wants to become a small struct rather than a growing list of
  // statics.
  static void setShowResistorOrient(bool show) { showResistorOrient_ = show; }
  static bool showResistorOrient() { return showResistorOrient_; }

  // --- Unstuffed components are drawn GREY ---
  //
  // A component marked "do not stuff" or "stuff with jumper" is drawn
  // in one flat colour, every part of it: lines, fills, text and pin
  // markers alike. So each element asks whether it belongs to such a
  // component and passes its own colour through tint().
  //
  // Done per element rather than by tinting the whole component,
  // because the JUMPER must stay wire-coloured while everything around
  // it goes grey -- a whole-subtree effect would grey that too.
  static void setDoNotStuffColor(const QColor &c) { doNotStuffColor_ = c; }
  static QColor doNotStuffColor() { return doNotStuffColor_; }

  // Jumper width, in device pixels -- see Prefs::jumperWidthPx().
  static void   setJumperWidthPx(double px) { jumperWidthPx_ = px; }
  static double jumperWidthPx() { return jumperWidthPx_; }

  // True when this element is, or sits inside, an unstuffed component.
  bool isUnstuffed() const;

  // The colour to actually paint with.
  QColor tint(const QColor &color) const {
    return isUnstuffed() ? doNotStuffColor_ : color;
  }

  // --- Line width and style ---
  //
  // Graphic elements only -- line, rect, ellipse, coil, zigzag, both
  // arcs, triangle. Everything else reports -1, which is how the menu
  // knows not to offer the entries: wires, pins and text have no such
  // fields in their records.
  //
  // Width 0 means "default", resolved from CADlineWidth at paint time;
  // see SchPen.h for the measured meaning of both values.
  // --- Stuffing (components only) ---
  //
  // ArgStuff: 0 normal, 1 do not stuff, 2 stuff with a jumper. -1 for
  // anything that is not a component.
  virtual int  stuffCode() const { return -1; }
  virtual void setStuffCode(int) {}

  virtual int  lineWidth() const { return -1; }
  virtual void setLineWidth(int) {}
  virtual int  lineType() const { return -1; }
  virtual void setLineType(int) {}

  // --- Colours, as RAW ArgColor values ---
  //
  // Raw rather than QColor because the MODE matters: "default",
  // "background" and "foreground" are references resolved from the
  // registry at paint time, not colours. -1 means the element has no
  // such field, which is how the menus know what to offer.
  virtual int  lineColorRaw() const { return -1; }
  virtual void setLineColorRaw(int) {}
  virtual int  fillColorRaw() const { return -1; }
  virtual void setFillColorRaw(int) {}
  virtual int  textColorRaw() const { return -1; }
  virtual void setTextColorRaw(int) {}

  virtual double fontSize() const { return -1.0; }
  virtual void   setFontSize(double) {}

  virtual int  orientCode() const { return -1; }
  virtual void setOrientCode(int) {}

  // What orientCode() should become after a quarter turn, or after a
  // mirror about the vertical axis. Element-specific because the rules
  // genuinely differ -- see the implementations.
  virtual int orientAfterTurn(bool ccw) const { return orientCode(); }
  virtual int orientAfterMirror() const { return orientCode(); }

  // Repaint slack, in scene units, added around every element's
  // geometry.
  //
  // Elements are drawn with COSMETIC pens, whose width is in device
  // pixels and therefore extends an unpredictable number of scene units
  // past the geometry -- more the further you zoom out. Antialiasing
  // adds another fraction of a pixel. Reporting the bare geometry meant
  // those edge pixels fell outside the reported bounding rect, so the
  // scene never invalidated them and dragging left a trail behind.
  //
  // NOT scale-aware, and can't be: boundingRect() has no access to the
  // view's zoom, and making it zoom-dependent would mean a
  // prepareGeometryChange() on every zoom step. This is a fixed
  // allowance sized for normal working zoom levels; the element is only
  // a few pixels tall at the zoom levels where it would be too small to
  // cover a cosmetic pen anyway.
  static constexpr double kPaintSlack = 16.0;

  // Smallest dimension shape() will report, so an axis-aligned element
  // never produces a zero-area path. See shape().
  static constexpr double kMinShapeSize = 1.0;

protected:
  // labelXform() without the readability turn.
  QTransform labelBaseXform(double ownRotDeg) const;

  NodePtr node_;

  static bool showResistorOrient_;
  static QColor doNotStuffColor_;
  static double jumperWidthPx_;

  // Converts a QParser2 Point/Rect (Y-up, inclusive-integer coordinates)
  // directly to Qt types with NO axis flip. The Y-flip is applied once,
  // globally, as part of SchView's own transform -- so every wrapper
  // item can use QParser2 coordinates as its local item coordinates
  // completely unchanged. Confirmed by rendering Simple.qsch: content
  // at higher model-Y (e.g. the Vout net at y=300) correctly appears
  // above content at lower Y (components at y=-200).
  static QPointF toQ(Point p) { return QPointF(p.x, p.y); }
  static QRectF  toQ(Rect r) {
    return QRectF(QPointF(r.left, r.top), QPointF(r.right, r.bottom))
        .normalized();
  }

  // Default "not implemented yet" appearance: a dashed bounding-box
  // outline, so element types without a real paint() override yet are
  // still visible (not invisible) during incremental rollout.
  void paint(QPainter *painter, const QStyleOptionGraphicsItem *option,
             QWidget *widget) override;
};

} // namespace sch::render
