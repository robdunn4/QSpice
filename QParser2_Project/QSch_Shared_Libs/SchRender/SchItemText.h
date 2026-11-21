#pragma once
#include <ItemText.h>

#include "SchItemBase.h"
#include <QColor>
#include <QTransform>

namespace sch::render {

// STUB: renders the string at pt1 with basic alignment; rotation is not
// yet applied (attribute-text rotation vs. parent rotate/mirror state
// was already flagged in project notes as not tracking cleanly -- needs
// dedicated investigation before wiring rotation in here).
class SchItemText : public SchItemBase {
public:
  explicit SchItemText(ItemTextPtr item, QGraphicsItem *parent = nullptr);

  Rect  getRect() const noexcept override;
  Point getPosition() const noexcept override;
  void  moveTo(Point p) noexcept override;

  // Its pin -- see SchItemBase::onDisabledPin().
  int assocPinIndex() const override { return item_->pinNdx.getValue(); }

  DrawKind drawKind() const override;

  TextKind textKind() const override;
  // Whether this text is drawn at all: the record's hidden flag, plus
  // the zero-alignment rule for symbol attribute text. PUBLIC because
  // the pick pipeline needs it -- hidden text is not drawn and so must
  // not be clickable. A SYMBOL document's own hidden text IS drawn --
  // faded (isFaded()) -- and so is displayed.
  bool isDisplayed() const;

  // A symbol's text that is HIDDEN -- its hidden bit, the only thing that
  // hides one -- wherever the symbol is: placed, or edited on its own.
  bool isHiddenInSymbol() const;

  // Drawn faded: hidden, in a symbol document (SchScene::symbolDocument()).
  bool isFaded() const;

  // Mark Text Anchor Points (SchScene::textAnchorsMarked()): whether this
  // text shows its anchor mark -- only while it is displayed -- and the
  // anchor, in this item's own coordinates. On screen the view draws the
  // mark, in output paint() does -- the same filled dot either way, of
  // anchorMarkRadius().
  bool    anchorMarked() const;
  QPointF anchorPoint() const;

  // The mark's radius in schematic units, for a painter `scale` device
  // pixels to the unit: it scales with zoom, as QSpice's markers do, but
  // never shrinks below a few pixels across -- the unattached-pin box's
  // size (Robert, 2026-11-18).
  static constexpr double kAnchorMarkUnits = 12.0; // 24 units across
  static constexpr double kAnchorMarkMinPx = 3.0;  // 6 pixels across
  static double           anchorMarkRadius(double scale);

  // Draws the mark at `center` on `painter`, as set up now: an open
  // circle in RenderStyle's anchor-mark colors, a 1-pixel line.
  static void paintAnchorMark(QPainter *painter, const QPointF &center);

  // Drawn in HIGHLIGHT color (red, as a highlighted pin's marker) while
  // its row is selected in the Symbol Properties pane, so the pane and
  // the drawing point at each other.
  bool isHighlighted() const { return highlighted; }

  void setHighlighted(bool on) {
    if (highlighted == on) return;

    highlighted = on;
    update();
  }

  // A value typed for this attribute, as it would be stored: by the rules
  // of the Programmable Attribute the record holds (an Integer's is cut
  // to its leading whole number and clamped -- see
  // sch::progattr::ProgAttr::acceptValue()); unchanged for anything else.
  // `adjusted` is set when that changed what was typed, spaces aside.
  QString acceptValue(const QString &typed, bool *adjusted = nullptr) const;

  // Whether the text can be edited where it is drawn. Not a Multiple
  // Choice shown as one (sch::progattr): its value is picked from the
  // list in Symbol Properties, and a double-click on it does nothing, as
  // in QSpice. With Raw Attributes on it is ordinary text again.
  bool editsInline() const;

  // The comment flag -- bit 0 of the record's textFlags. It is the only
  // thing separating a SPICE directive from a comment, and it decides
  // the color as well.
  // The record's font SIZE MULTIPLIER -- not a point size. QSpice
  // stores a scale factor that makeFont() turns into a real font.
  double fontSize() const override;
  void   setFontSize(double size) override;

  bool isComment() const;
  void setComment(bool comment);

  int  orientCode() const override;
  void setOrientCode(int code) override;
  int  orientAfterTurn(bool ccw) const override;
  int  orientAfterMirror() const override;

  SchTextEditInfo textEditInfo() const override;
  void            applyTextEdit(const SchTextEditInfo &info) override;

  int  textColorRaw() const override { return item_->textColor.getValue(); }
  void setTextColorRaw(int raw) override {
    item_->textColor.setValue(raw);
    update();
  }

protected:
  bool highlighted = false; // see setHighlighted()

  void paint(QPainter *painter, const QStyleOptionGraphicsItem *option,
             QWidget *widget) override;

  // Horizontal alignment as it must be APPLIED, which is not always
  // what the record stores: a mirrored parent swaps EAST and WEST.
  // Never write this value back to the model -- the file keeps the
  // unmirrored value, and applyTextEdit() must too.
  // Which QSpice default applies when the record says "use the type
  // default" (ArgColor high byte 0x01).
  //
  // QSpice distinguishes three text roles, and they are genuinely
  // different registry entries: text inside a symbol, a component's
  // attribute labels, and free text placed on the schematic. This item
  // class serves all three, so the role is decided by where the text
  // sits -- a child of a component is component text, a top-level one
  // is schematic text.

  QColor defaultTextColor() const override;

  // The record's text as the GUI works with it: decoded, and QSpice's
  // "\n" turned into real line feeds (sch::core::lineBreaksFromRecord()).
  // Overbar markers are still in it.
  QString recordText() const;

  // The text as it is SHOWN, overbar markers still in it: recordText(),
  // except that a placed component's Programmable Attribute shows as
  // "NAME=VALUE" while View > Raw Attributes is off -- see
  // sch::progattr. A symbol's own text (a top-level item, in a symbol
  // document) always shows as written, as in QSpice's symbol editor.
  QString shownText() const;

  // How many leading characters of shownText() the inline editor must
  // leave alone: a shown Programmable Attribute's "NAME=". 0 otherwise.
  int shownLockedPrefix() const;

  int effectiveAlignH() const;
  int effectiveAlignV() const;

  // Degrees the painter is rotated by so the label stays upright
  // despite the parent's rotation. Shared by paint() and getRect() --
  // see textXform().
  double counterRotateDeg() const;

  // Maps text-layout coordinates (the Y-down frame alignedTextRect()
  // and layoutTextBlock() return rects in) into this item's own local
  // frame.
  //
  // MUST be the single source of truth for both paint() and getRect().
  // paint() applies these operations to the painter; getRect() maps the
  // text rect through the identical transform to find where the glyphs
  // actually land. When the two got out of step -- getRect() reporting
  // an unrotated box while paint() drew a rotated one -- a rotated
  // component's label smeared on every drag, because the area the item
  // CLAIMED to occupy wasn't the area it painted.
  QTransform textXform() const;

private:
  ItemTextPtr item_;
};

} // namespace sch::render
