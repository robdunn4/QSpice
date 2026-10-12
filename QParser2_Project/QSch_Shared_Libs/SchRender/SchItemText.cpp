#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable : 4996) // Qt headers reference APIs deprecated for Qt 7
#endif

#include "SchItemText.h"

#include <QtGlobal> // qWarning

#include <QFont>
#include <QFontMetricsF>
#include <QPainter>
#include <QPen>

#ifdef _MSC_VER
#pragma warning(pop)
#endif

#include <cmath>
#include <utility>

#include "RenderStyle.h"
#include "SchColorDecode.h"
#include "SchRotDecode.h"
#include "SchStringDecode.h"
#include "SchTextLayout.h"

namespace sch::render {

SchItemText::SchItemText(ItemTextPtr item, QGraphicsItem *parent)
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

  setZValue(kZText);
}

Rect SchItemText::getRect() const noexcept {
  // Text that isn't drawn occupies nothing. Previously paint() skipped
  // hidden text while getRect() still reported its full extent --
  // harmless only because SchItemComp::getRect() excludes text children
  // from a component's box, so the oversized bounds fed into nothing.
  if (!isDisplayed()) return Rect(item_->pt1.getValue(),
                                  item_->pt1.getValue());

  // The DISPLAY form: overbar markers are not drawn and take no space
  // (unless Raw mode shows them -- see overbarDisplayText()),
  // so an element carrying them must not report bounds as though they
  // were characters. This is what makes selecting and hit-testing match
  // what is on screen.
  const QString text =
      overbarDisplayText(recordText());
  const QFontMetricsF fm(makeFont(item_->fontSize.getValue()));
  const int           alignH = effectiveAlignH();
  const int           alignV = effectiveAlignV();

  // CONFIRMED bug: the single-line path below measures with
  // QFontMetricsF::boundingRect(QString), which per Qt's documentation
  // treats '\n' as an ordinary character rather than a line break. A
  // two-line comment therefore reported a ONE-line box -- so the second
  // line fell outside the item's bounds entirely and couldn't be
  // clicked, rubber-band selected, or dragged by.
  //
  // Multi-line goes through the shared block layout, which is also what
  // the inline editor uses, so bounds and edit box agree. Single-line
  // deliberately keeps its existing path untouched: that one is
  // calibrated against real QSpice screenshots (loose rect for
  // placement, tightened for the reported box), and there's no reason
  // to disturb every existing label to fix a multi-line case.
  QRectF textRect;
  if (text.contains(QLatin1Char('\n'))) {
    textRect = layoutTextBlock(fm, text, alignH, alignV).box;
  } else {
    const QRectF local = alignedTextRect(fm, text, alignH, alignV);
    textRect           = tightenTextRect(fm, text, local);
  }

  // CONFIRMED bug: this used to hand textRect straight to
  // toGenerousRect(), which assumes the rect is in the Y-down text
  // frame and merely flips it. But paint() also ROTATES the painter to
  // keep the label upright against its parent's rotation -- so for a
  // rotated component the glyphs landed in a box at right angles to the
  // one reported here. The item claimed a wide, short rect and painted
  // a tall, narrow one, and Qt only ever invalidates what an item
  // claims: dragging R1 smeared its refdes across the canvas.
  //
  // Mapping through textXform() -- the same transform paint() applies
  // -- gives the rect the glyphs actually occupy, already in this
  // item's own local (Y-up) frame, so the flip toGenerousRect() would
  // do is neither needed nor wanted here.
  const QRectF painted = textXform().mapRect(textRect);

  constexpr double kMargin = 20.0;
  const QRectF     padded =
      painted.adjusted(-kMargin, -kMargin, kMargin, kMargin);

  const Point anchor = item_->pt1.getValue();
  return Rect(Point(anchor.x + static_cast<int>(std::floor(padded.left())),
                    anchor.y + static_cast<int>(std::floor(padded.top()))),
              Point(anchor.x + static_cast<int>(std::ceil(padded.right())),
                    anchor.y + static_cast<int>(std::ceil(padded.bottom()))));
}

QString SchItemText::recordText() const {
  return sch::core::lineBreaksFromRecord(sch::core::decodeQSpiceString(item_->text.getValue()));
}

Point SchItemText::getPosition() const noexcept {
  return item_->pt1.getValue();
}

void SchItemText::moveTo(Point p) noexcept {
  prepareGeometryChange();
  item_->pt1.setValue(p);
}

SchItemBase::TextKind SchItemText::textKind() const {
  // SPICE directives, comments, and symbol attribute text all edit the
  // same way: multi-line capable, full anchor grid.
  return TextKind::Directive;
}

SchTextEditInfo SchItemText::textEditInfo() const {
  SchTextEditInfo info;
  // The RAW form, markers included: the editor shows them so they can
  // be seen, edited and removed like any other character. QSpice hides
  // them and renders the bars live instead; showing them keeps the
  // editor's text and its metrics the same string, which avoids a
  // second layout path through the whole editor.
  info.text   = recordText();
  info.anchor = item_->pt1.getValue();
  info.color =
      SchColorDecode::fromArgColor(item_->textColor.getValue(), defaultTextColor());
  info.fontPixelSize = fontPixelSize(item_->fontSize.getValue());
  info.alignH        = item_->rotateAlign.getAlignH();
  info.alignV        = item_->rotateAlign.getAlignV();
  return info;
}

void SchItemText::applyTextEdit(const SchTextEditInfo &info) {
  prepareGeometryChange();
  // Stored value includes surrounding quotes (confirmed: ArgString
  // keeps the raw tokenized form, quotes included) -- quote() adds
  // them back. New text is always encoded as UTF-8 rather than
  // attempting to preserve whatever encoding the original happened to
  // use; sch::core::decodeQSpiceString() already reads UTF-8 correctly, so this
  // is safe going forward even though some existing files predate this
  // and use Latin-1.
  //
  // Line feeds go back as QSpice's "\n" -- see sch::core::lineBreaksToRecord().
  item_->text.setValue(
      ArgString(sch::core::encodeQSpiceString(sch::core::lineBreaksToRecord(info.text))).quote().getValue());

  // EDITING MUST NOT HIDE A VISIBLE ELEMENT.
  //
  // An alignment of 0/0 marks a symbol's text invisible. But the user
  // could only have opened an editor on something they could SEE and
  // click, and changing where the text hangs from is not a request to
  // hide it -- so an edit that would clear both nibbles keeps the
  // alignment the element already had.
  //
  // This is an invariant about EDITING, not about the format: a file
  // may legitimately contain hidden attribute text, and it still loads,
  // draws as hidden, and shows its Invis. tick. What must be impossible
  // is an edit turning visible into hidden behind the user's back.
  const bool wasHidden = sch::core::alignMarksHidden(item_->rotateAlign.getAlignH(),
                                          item_->rotateAlign.getAlignV());
  const bool wouldHide = sch::core::alignMarksHidden(info.alignH, info.alignV);

  if (wouldHide && !wasHidden) {
    qWarning("SchItemText: ignoring an edit that would have hidden a visible "
             "text element (alignment 0/0); keeping its alignment");
  } else {
    // setAlignH()/setAlignV() each mask in only their own bits, leaving
    // ArgRotAlign's rotation nibble untouched -- which matters, since
    // nothing in text editing should disturb the element's rotation.
    item_->rotateAlign.setAlignH(static_cast<unsigned char>(info.alignH));
    item_->rotateAlign.setAlignV(static_cast<unsigned char>(info.alignV));
  }
  item_->pt1.setValue(info.anchor);
}

double SchItemText::fontSize() const { return item_->fontSize.getValue(); }

void SchItemText::setFontSize(double size) {
  prepareGeometryChange();

  // The record stores a float; the cast is explicit so the narrowing is
  // deliberate rather than a warning.
  item_->fontSize.setValue(static_cast<float>(size));
}

bool SchItemText::isComment() const { return item_->textFlags.isComment(); }

void SchItemText::setComment(bool comment) {
  prepareGeometryChange();

  const int flags = item_->textFlags.getValue();
  item_->textFlags.setValue(comment ? (flags | 0x1) : (flags & ~0x1));
}

int SchItemText::orientCode() const {
  // The WHOLE rotateAlign byte: rotation in the high nibble, alignment
  // in the low one. Mirroring changes both, so they travel together.
  return (item_->rotateAlign.getRotCode() << 4) |
         item_->rotateAlign.getAlign();
}

void SchItemText::setOrientCode(int code) {
  prepareGeometryChange();
  item_->rotateAlign.setRotCode((code >> 4) & 0xF);
  item_->rotateAlign.setAlign(static_cast<unsigned char>(code & 0xF));
}

int SchItemText::orientAfterTurn(bool ccw) const {
  // Rotation changes the ROTATION ONLY -- the anchor stays where it is,
  // in both senses: the same corner of the text and the same
  // coordinate.
  //
  // MEASURED from a QSpice file containing one directive in each
  // orientation: Normal, +90, -90 and UpDown all carry alignment W/N
  // and differ only in the rotation nibble (0, 2, 6, 4).
  const int rot   = (orientCode() >> 4) & 0xF;
  const int align = orientCode() & 0xF;

  return (((rot + (ccw ? 2 : 6)) & 0x7) << 4) | align;
}

int SchItemText::orientAfterMirror() const {
  const int rot   = (orientCode() >> 4) & 0xF;
  const int align = orientCode() & 0xF;

  // Mirroring NEGATES the rotation and swaps one alignment axis.
  //
  // MEASURED from the same file: mirroring +90 (rot 2) gives rot 6, and
  // mirroring -90 (rot 6) gives rot 2, while 0 and 4 are unchanged --
  // which is exactly negation. Text is never drawn backwards; the
  // glyphs stay readable and only the direction reverses.
  const int newRot = (8 - (rot & 0x7)) & 0x7;

  // ...and the anchor swaps on whichever axis currently maps to SCREEN
  // X, since that is the axis being mirrored about. Horizontal text (0,
  // 4) swaps WEST/EAST; vertical text (2, 6) swaps NORTH/SOUTH.
  //
  // Confirmed both ways: Normal and UpDown go W->E with N untouched,
  // while +90 and -90 go N->S with W untouched.
  int alignH = align & 0b1100;
  int alignV = align & 0b0011;

  const bool textIsVertical = (rot == 2 || rot == 6);

  if (textIsVertical) {
    if (alignV == sch::core::kAlignVNorth) alignV = sch::core::kAlignVSouth;
    else if (alignV == sch::core::kAlignVSouth) alignV = sch::core::kAlignVNorth;
  } else {
    if (alignH == sch::core::kAlignHWest) alignH = sch::core::kAlignHEast;
    else if (alignH == sch::core::kAlignHEast) alignH = sch::core::kAlignHWest;
  }

  // 45-degree rotations are UNMEASURED -- no reference file has one
  // mirrored. Treated as horizontal, which is a guess.
  return (newRot << 4) | alignH | alignV;
}

bool SchItemText::isDisplayed() const {
  // The record's own hidden flag (VISIBLE_BIT), which applies to symbol
  // attribute text.
  if (item_->textFlags.isHidden()) return false;

  // ...and, within a symbol, a zero alignment nibble, which appears to
  // mark text that is not drawn on a schematic. OBSERVED, MECHANISM
  // UNCONFIRMED -- see sch::core::alignMarksHidden() in SchTextLayout.h for what
  // is actually known and the test that would settle it.
  //
  // Restricted to symbol children because that is the only place the
  // rule has any evidence behind it; a top-level text element with no
  // alignment bits still has to go somewhere, and alignedRectForSize()
  // centres it.
  if (parentItem() != nullptr &&
      sch::core::alignMarksHidden(item_->rotateAlign.getAlignH(),
                       item_->rotateAlign.getAlignV()))
    return false;

  return true;
}

QColor SchItemText::defaultTextColor() const {
  // QSpice keeps three separate text colours, and this one class serves
  // all three roles, so the role has to be worked out here.
  //
  // The comment flag is bit 0 of the record's own textFlags (clear =
  // text, set = comment), which ArgTextFlags already decodes. Comments
  // and SPICE directives were previously lumped in with schematic text
  // and all came out black; CADcommentText is blue in a stock
  // installation, which is what QSpice actually shows.
  if (item_->textFlags.isComment()) return RenderStyle::current().commentText;

  // Otherwise the distinction is placement: a child of a component is
  // that component's attribute text, a top-level one is free text on
  // the schematic.
  //
  // NOT distinguished yet: CADgrapicText, for text inside a SYMBOL as
  // opposed to a placed component's labels. Telling those apart needs
  // to know the document is a symbol being edited rather than a
  // schematic, which this item cannot see from here.
  return (parentItem() != nullptr) ? RenderStyle::current().componentText
                                   : RenderStyle::current().schematicText;
}

// The alignments the label is laid out with: the record's, adjusted for
// the parent's mirror and for the readability turn -- see
// SchItemBase::labelAlignH(). Top-level text has neither, and gets its
// record's values unchanged.
int SchItemText::effectiveAlignH() const {
  return labelAlignH(item_->rotateAlign.getAlignH(), counterRotateDeg());
}

int SchItemText::effectiveAlignV() const {
  return labelAlignV(item_->rotateAlign.getAlignV(), counterRotateDeg());
}

// The label's own rotation, straight from its record.
//
// ArgRotAlign carries a rotation code in bits 4-7 (getRotDeg() = code *
// 45), and it is the label's rotation WITHIN the symbol -- so the
// on-screen angle is the parent's rotation plus this, and the local
// rotation this function returns is simply this value. The parent does
// not enter into it at all.
//
// This replaces a heuristic that ignored the record and guessed the
// tilt from the PARENT's rotation code: odd codes got +45 if the code
// was above 4 and -45 otherwise. That reproduced the only two cases
// then available (codes 7 and 1) because in those files the label's own
// rotation happened to correlate with the component's, and its own
// comment flagged codes 3 and 5 as extrapolated and untested.
//
// A file exercising all four diagonal codes showed the extrapolation
// was wrong, and why no parent-based rule could work:
//
//   comp code 7 (315), label rot 2 ( 90) -> on screen  45   [heuristic agreed]
//   comp code 1 ( 45), label rot 6 (270) -> on screen -45   [heuristic agreed]
//   comp code 5 (225), label rot 2 ( 90) -> on screen -45   [heuristic said +45]
//   comp code 3 (135), label rot 6 (270) -> on screen  45   [heuristic said -45]
//
// Codes 5 and 7 share a label rotation but need opposite tilts, so no
// function of the parent's code alone can produce both. The record says
// it outright.
//
// It also explains the earlier observation that labels stay upright for
// 90-degree multiples: nothing enforces that, it is just what those
// files' label rotations encoded.
double SchItemText::counterRotateDeg() const {
  return static_cast<double>(item_->rotateAlign.getRotDeg());
}

// Rotation by the record, the parent's scale and mirror cancelled, the
// view's Y-flip undone, and the readability half turn when needed -- all
// in SchItemBase::labelXform(), which pin labels share.
QTransform SchItemText::textXform() const { return labelXform(counterRotateDeg()); }

void SchItemText::paint(QPainter *painter, const QStyleOptionGraphicsItem *,
                        QWidget *) {
  if (!isDisplayed()) return;

  // RAW carries the overbar markers; DISPLAY is what is actually drawn
  // and measured. See kOverbarMarker in SchTextLayout.h.
  const QString raw  = recordText();
  const QString text = overbarDisplayText(raw);
  if (text.isEmpty()) return;

  QPen pen(tint(SchColorDecode::fromArgColor(item_->textColor.getValue(),
                                             defaultTextColor())));
  painter->setPen(pen);

  const QFont font = makeFont(item_->fontSize.getValue());
  painter->setFont(font);

  painter->save();
  painter->translate(toQ(item_->pt1.getValue()));
  // The rotation, mirror-cancellation and Y-flip all live in
  // textXform(), so getRect() can map through exactly the same thing.
  painter->setTransform(textXform(), /*combine=*/true);

  const QFontMetricsF fm(font);
  const int           alignH = effectiveAlignH();
  const int           alignV = effectiveAlignV();

  // Multi-line text is drawn line by line at the shared layout's own
  // line rects rather than handed to drawText() as one string: Qt would
  // space the lines by QFontMetricsF::lineSpacing(), which at our
  // calibrated point sizes is far looser than the text looks and
  // wouldn't match the edit box the user just typed in. See
  // textLineHeight().
  if (text.contains(QLatin1Char('\n'))) {
    const TextBlockLayout block = layoutTextBlock(fm, text, alignH, alignV);

    // The raw lines run in step with the display lines: stripping never
    // removes a newline, so line N of one corresponds to line N of the
    // other.
    const QStringList rawLines = raw.split(QLatin1Char('\n'));

    for (int i = 0; i < block.lines.size(); ++i) {
      painter->drawText(QPointF(block.lineRects[i].left(), block.baselines[i]),
                        block.lines[i]);

      if (i < rawLines.size())
        drawOverbars(painter, fm, rawLines[i], block.lines[i],
                     block.lineRects[i].left(), block.baselines[i]);
    }

    painter->restore();
    return;
  }

  const QRectF rect = alignedTextRect(fm, text, alignH, alignV);
  // Qt::TextDontClip is essential here: without it, QPainter::drawText()
  // clips rendering to the given rect (confirmed directly -- a
  // deliberately-undersized rect without this flag cut text off mid-
  // glyph; with it, the same rect renders the full text unclipped,
  // extending beyond the rect as needed). The rect only controls
  // alignment/positioning; it should never be able to truncate the
  // actual rendered text, regardless of any mismatch between our
  // computed rect and the font's real rendering extent.
  painter->drawText(rect, Qt::AlignLeft | Qt::AlignTop | Qt::TextDontClip,
                    text);

  // drawText(rect, AlignTop) puts the baseline one ascent below the
  // rect's top, which is where the bars are measured from.
  drawOverbars(painter, fm, raw, text, rect.left(), rect.top() + fm.ascent());

  painter->restore();
}

} // namespace sch::render
