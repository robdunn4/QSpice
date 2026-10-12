#pragma once
#include <QFont>
#include <QFontMetricsF>
#include <QPair>
#include <QPainter>
#include <QList>
#include <QRectF>
#include <QSizeF>
#include <QString>
#include <QFontInfo>
#include <QDebug>

#include "SchAlign.h"
#include <QStringList>
#include <algorithm>
#include <cmath>

#include "ArgUtils.h"
#include "SchRect.h"

namespace sch::render {

// SIZE calibration, measured against QSPICE's own rendering.
//
// QSpice's cap height is simply PROPORTIONAL to the record's fontSize,
// with no constant term: fitted across ten sizes from 0.65 to 5.0 in
// QSpice's own output, a least-squares line gives an intercept of -2.3
// units, which is nothing.
//
// Two different measures of "cap height" are involved, and mixing them
// is worth 4%:
//
//   INK height -- what you measure in a screenshot: the tallest dark
//     pixels of the string. It includes antialiasing fringes and the
//     OVERSHOOT of round glyphs (S, 0) above the cap line. QSpice
//     measures 121.1 x fontSize this way.
//
//   METRIC height -- QFontMetricsF::capHeight(), the font's declared cap
//     line. About 4.4% SMALLER than the ink height for this string in
//     Tahoma, measured by rendering it both ways.
//
// The probe below uses the metric, so the target must be in metric terms
// too: 121.1 / 1.044 = 116.0. Setting the metric to 121.1 -- which an
// earlier attempt did -- renders text ~4% taller than QSpice.
//
// An earlier calibration had a large intercept and an unexplained
// "non-linearity at small sizes". Both were artefacts of measuring OUR
// OWN output rather than QSpice's, so the fit described the intercept
// already in our own formula.
constexpr double kCapHeightPerFontSize = 116.0;

// QSpice draws schematic text in TAHOMA, so we ask for it -- and then
// MEASURE what we actually got.
//
// The font size needed for a given cap height depends on the font's own
// cap ratio: Tahoma's is 0.727 em, Segoe UI's is 0.700. Hard-coding a
// conversion therefore silently encodes an assumption about which font
// is in use -- and when the family request did not take effect,
// schematic text came out 7% short with a ratio that drifted with size,
// which is exactly what a wrong cap ratio looks like.
//
// So the conversion is derived once, at first use, from the font Qt
// really resolved. If Tahoma is missing and something else is
// substituted, the text is still the right HEIGHT; only the letterforms
// differ.
//
// The probe also reports the resolved family, so a substitution shows up
// in the log rather than as mysteriously wrong metrics.
//
inline QFont schematicFontFamily() {
  QFont font;

  // A fallback list rather than a bare name: Tahoma ships with Windows
  // but is not guaranteed elsewhere, and QFont substitutes silently.
  font.setFamilies({QStringLiteral("Tahoma"), QStringLiteral("DejaVu Sans"),
                    QStringLiteral("Verdana")});
  return font;
}

// The schematic font at a PIXEL size in schematic units.
//
// PIXEL size, not point size, so text is the same size relative to the
// drawing on every device. A point size is converted at the DEVICE's
// resolution: right on a 96-dpi screen, about six times too big on a
// 600-dpi printer, whose world transform then scales it again. A pixel
// size is in the painter's own (scene) units and is transformed with
// everything else.
//
// QFont takes whole pixels; at schematic scale (about 160 units per
// fontSize step) rounding moves the cap height by well under 1%.
inline QFont fontAtPixelSize(double pixelSize) {
  QFont font = schematicFontFamily();
  font.setPixelSize(std::max(1, static_cast<int>(std::lround(pixelSize))));
  return font;
}

// The pixel size, in schematic units, for a record's fontSize -- so the
// font's cap height is kCapHeightPerFontSize x fontSize.
inline double fontPixelSize(double fontSizeValue) {
  // Pixels per unit of fontSize, worked out once from the resolved
  // font's own cap height. Cap height is linear in pixel size, and a
  // pixel size has no device in it, so one probe serves screen and
  // printer alike.
  static const double pixelsPerFontSize = []() {
    constexpr int kProbePixels = 1000;

    QFont probe = schematicFontFamily();
    probe.setPixelSize(kProbePixels);

    const QFontMetricsF metrics(probe);
    const double        cap = metrics.capHeight();

    // qWarning rather than qInfo: qInfo did not reach the log, and a
    // substituted font is worth flagging anyway -- the text will be the
    // right height, but the letterforms will not be QSpice's.
    const QString resolved = QFontInfo(probe).family();

    qWarning().noquote()
        << "Schematic text font: requested Tahoma, resolved to" << resolved
        << "-- cap height" << cap << "px at" << kProbePixels << "px";

    // A font with no usable cap metric would give a nonsense scale;
    // Tahoma's cap ratio (0.727 em) is the sensible thing to assume.
    if (cap <= 0.0) return kCapHeightPerFontSize / 0.727;

    return kCapHeightPerFontSize * kProbePixels / cap;
  }();

  return std::max(1.0, fontSizeValue * pixelsPerFontSize);
}

inline QFont makeFont(double fontSizeValue) {
  return fontAtPixelSize(fontPixelSize(fontSizeValue));
}

// QSpice's rotateAlign encodes which EDGE (or center) of the text sits at
// the anchor point (pt1) -- e.g. SOUTH means the anchor is at the
// BOTTOM of the text, so the text hangs upward from it. This is NOT the
// same thing as Qt::Alignment applied within some fixed-size box: a
// fixed box only works if that box is sized to the text's own natural
// extent, otherwise (as discovered comparing against real QSpice
// output) a non-centered alignment places the text however far the
// box's edge happens to be from the anchor, which for an arbitrarily
// large placeholder box can be hundreds of scene units away from where
// it should actually appear.
//
// Returns a rect already positioned in the LOCAL, anchor-relative
// coordinate frame -- translate to the anchor, then draw this rect with
// Qt::AlignLeft | Qt::AlignTop (the rect's own position/size already
// encodes the real alignment; Qt's own alignment flags aren't needed
// beyond that).
//
// alignH/alignV are the raw values from ArgRotAlign::getAlignH()/
// getAlignV(): WEST=0b0100, EAST=0b1000, CENTER_H=0b1100 (or anything
// else); NORTH=0b01, SOUTH=0b10, CENTER_V=0b11 (or anything else).
//
// The vertical mapping assumes the caller has already counter-scaled
// the painter by (1,-1) to undo SchView's view-level Y-flip (as
// SchItemText/SchItemNet both do) -- within that locally-corrected
// frame, increasing Y is downward (Qt's native sense), so NORTH (visibly
// up) is the smaller-Y edge and SOUTH (visibly down) is the larger-Y
// edge.
// Places a box of the given size against the anchor per alignH/alignV,
// returning it in the same local, anchor-relative frame described above.
// Split out of alignedTextRect() so the inline text editor
// (SchEditText) can place its edit BOX -- whose size comes from its own
// multi-line layout, not from a single call to
// QFontMetricsF::boundingRect() -- by exactly the same rule the text
// itself is placed by. Getting these two out of step would show up
// immediately as the edit box not landing on the text it replaces.
// The raw alignment nibbles as ArgRotAlign::getAlignH()/getAlignV()
// return them. Named here (rather than left as bare literals at each
// use) now that the anchor-grid code has to both READ and WRITE them --
// a mistyped bit pattern in the write direction silently relocates the
// element in the saved file, which is a much worse failure than a
// mis-drawn box.
//
// Anything outside these values means "centered": getAlignH() masks to
// 0b1100, so 0b0000 is reachable too and falls through to the same
// centered treatment CENTER_H gets.
// An alignment nibble of ZERO -- no horizontal or vertical bits set at
// all -- appears to mark text that is not displayed on a schematic.
//
// STATUS: OBSERVED, MECHANISM UNCONFIRMED. Do not treat this as a
// decoded rule.
//
// What is actually known:
//   - Nibble 0 is not drawn. The AnalogLatch's Vdd and Vss pins and its
//     "LATCH" text all carry 0, and QSpice draws none of them.
//   - Nibble 15 IS drawn. That symbol's "Y1" label carries 0b1111 --
//     explicitly centred, both bits set in each field -- and QSpice
//     draws it.
//   - Reported behaviour: the SYMBOL EDITOR offers an alignment point
//     (corners, edge midpoints, centre), and choosing CENTRE warns that
//     the text will be hidden. So "centred" and "hidden" are linked
//     somewhere in QSpice's UI.
//
// What is NOT known: what that centre button actually writes. If it
// writes 0, this rule is right for the right reason. If it writes
// 0b1111 and QSpice hides that, then this rule is right only by
// accident -- "Y1" would have to be displayed for some other reason
// (being the reference designator?) -- and a genuinely centred comment
// would render here when QSpice hides it.
//
// The rule below fits every element in the one symbol available, and
// there is no counter-example, which is the whole of the evidence.
//
// PINS differ (2026-10-04, GmEnable symbol): a pin whose nibble is 15 is
// NOT drawn -- its label hides when centred. SchItemPin::labelVisible()
// adds that rule for pins only; text keeps this one.
//
// DECISIVE TEST, for when the symbol editor is built: create a text
// element, click the centre alignment point, accept the warning, and
// read the nibble QSpice wrote. That single value settles it.
// --- Overbars ---
//
// A marker character applies an overbar to the character that FOLLOWS
// it. The marker itself is never drawn and takes no horizontal space,
// so "<marker>t<marker>e<marker>x<marker>t" renders as "text" with a
// bar across it.
//
// Everything therefore works on two versions of the same string:
//   DISPLAY -- markers removed. Drives layout, painting, bounds and
//              hit-testing, so a marker never widens a text element.
//   RAW     -- markers present. What the record stores and what the
//              inline editor edits, so the markers can be seen and
//              removed like any other character. QSpice hides them
//              while editing and applies the bars live; showing them
//              instead keeps the editor's text and its metrics the
//              same thing, which is worth more than the fidelity.
constexpr char16_t kOverbarMarker = u'\u00AC';

// View > Raw Attributes: a GLOBAL display mode, shared by every open
// schematic and symbol. When on, overbar markers are drawn as the
// characters they are and no bars are drawn; when off, the markers are
// hidden and the bars drawn. A plain static, like SelHiLiteStyle, because
// the item classes that paint text have no route to the preferences.
// MainWin sets it and asks every view to refresh.
struct SchRawText {
  static inline bool enabled = false;
};

inline QString stripOverbarMarkers(const QString &raw);

// The text as it should APPEAR, and therefore be measured: markers
// stripped normally, kept in Raw mode. Every site that lays out or
// draws text with possible overbars goes through this, so bounds and
// painting cannot disagree.
inline QString overbarDisplayText(const QString &raw) {
  return SchRawText::enabled ? raw : stripOverbarMarkers(raw);
}

inline QString stripOverbarMarkers(const QString &raw) {
  QString out;
  out.reserve(raw.size());

  for (int i = 0; i < raw.size(); ++i) {
    if (raw[i] == QChar(kOverbarMarker)) continue;
    out.append(raw[i]);
  }

  return out;
}

// Runs of consecutive overbarred characters, as [start, end) indices
// into the DISPLAY string -- consecutive so that a marked run draws one
// continuous bar rather than a bar per glyph.
inline QList<QPair<int, int>> overbarRuns(const QString &raw) {
  QList<QPair<int, int>> runs;

  int  displayIndex = 0;
  int  runStart     = -1;
  bool marked       = false;

  for (int i = 0; i < raw.size(); ++i) {
    if (raw[i] == QChar(kOverbarMarker)) {
      marked = true;
      continue;
    }

    if (marked) {
      if (runStart < 0) runStart = displayIndex;
    } else if (runStart >= 0) {
      runs.append(qMakePair(runStart, displayIndex));
      runStart = -1;
    }

    marked = false;
    ++displayIndex;
  }

  if (runStart >= 0) runs.append(qMakePair(runStart, displayIndex));

  return runs;
}

// Draws the bars for one already-painted line.
//
// Drawn as lines rather than through any text mechanism, deliberately.
// The Unicode options are worse: U+0305 COMBINING OVERLINE renders per
// glyph, so adjacent characters get separate dashes instead of one
// continuous bar, and it needs the font to carry the mark at all;
// U+203E OVERLINE is a spacing character and occupies its own cell, so
// it cannot sit above anything. Drawing also gives control of weight
// and height, which those do not.
//
// Sized from CAP HEIGHT, not ascent: ascent includes room for
// accents, so a bar placed against it floats well above the letters.
// Cap height is the top of the capitals, which is where the bar should
// sit.
inline void drawOverbars(QPainter *painter, const QFontMetricsF &fm,
                         const QString &rawLine, const QString &displayLine,
                         double left, double baseline) {
  // Raw mode shows the markers instead of the bars.
  if (SchRawText::enabled) return;

  const QList<QPair<int, int>> runs = overbarRuns(rawLine);
  if (runs.isEmpty()) return;

  const double capHeight = (fm.capHeight() > 0.0) ? fm.capHeight()
                                                  : fm.ascent() * 0.7;

  const double thickness = qMax(capHeight * 0.10, 1.0);
  const double y         = baseline - capHeight - thickness * 1.5;

  // Explicitly in the TEXT's colour and weight. The painter's pen is
  // the text pen, but its WIDTH is whatever drew the glyphs -- usually
  // hairline -- which gives a bar far too faint against the letters.
  const QPen saved = painter->pen();

  QPen barPen(saved.color());
  barPen.setWidthF(thickness);
  barPen.setCapStyle(Qt::FlatCap);
  painter->setPen(barPen);

  for (const QPair<int, int> &run : runs) {
    const double x0 =
        left + fm.horizontalAdvance(displayLine.left(run.first));
    const double x1 =
        left + fm.horizontalAdvance(displayLine.left(run.second));

    painter->drawLine(QPointF(x0, y), QPointF(x1, y));
  }

  painter->setPen(saved);
}

// Invert OverBar: toggles the bar on each character whose position lies
// in [start, end) of the RAW string -- markers present, as the inline
// editor shows them.
//
// Per character, not all-or-nothing: a barred character loses its
// marker, an unbarred one gains one, so inverting twice restores the
// original. A marker belongs to the character AFTER it, so a character
// is selected by its own position, and its marker is toggled even when
// it lies just outside the range; a marker selected without its
// character is left alone. Line breaks never take a marker.
//
// The whole text is [0, raw.size()). Returns the new string and where
// the selected characters now lie, so the editor can keep them
// selected.
struct OverbarEdit {
  QString text;
  int     selStart = 0;
  int     selEnd   = 0;
};

inline OverbarEdit invertOverbars(const QString &raw, int start, int end) {
  const QChar marker(kOverbarMarker);

  OverbarEdit out;
  out.selStart = -1;

  int i = 0;
  while (i < raw.size()) {
    // A lone marker at the very end has no character to bar; kept as it
    // is, and never selected.
    if (raw[i] == marker && i + 1 >= raw.size()) {
      out.text.append(marker);
      ++i;
      continue;
    }

    const bool barred  = (raw[i] == marker);
    const int  charPos = barred ? i + 1 : i;
    const QChar ch     = raw[charPos];

    const bool selected = (charPos >= start && charPos < end);
    const bool isBreak  = (ch == QLatin1Char('\n') || ch == QLatin1Char('\r'));

    bool barNow = barred;
    if (selected) barNow = isBreak ? false : !barred;

    if (selected && out.selStart < 0)
      out.selStart = static_cast<int>(out.text.size());

    if (barNow) out.text.append(marker);
    out.text.append(ch);

    if (selected) out.selEnd = static_cast<int>(out.text.size());

    i = charPos + 1;
  }

  // Nothing selected: the text is unchanged and so is the range.
  if (out.selStart < 0) {
    out.selStart = start;
    out.selEnd   = end;
  }

  return out;
}

// The alignment codes (kAlign*) are in SchAlign.h.

inline QRectF alignedRectForSize(const QSizeF &size, int alignH, int alignV) {
  double dx = -size.width() / 2.0;    // CENTER_H (default/fallback)
  if (alignH == sch::core::kAlignHWest) dx = 0.0; // anchor at the text's left edge
  else if (alignH == sch::core::kAlignHEast)
    dx = -size.width(); // anchor at the text's right edge

  double dy = -size.height() / 2.0;      // CENTER_V (default/fallback)
  if (alignV == sch::core::kAlignVNorth) dy = 0.0;  // anchor at the text's top edge
  else if (alignV == sch::core::kAlignVSouth)
    dy = -size.height(); // anchor at the text's bottom edge

  return QRectF(dx, dy, size.width(), size.height());
}

inline QRectF alignedTextRect(const QFontMetricsF &fm, const QString &text,
                              int alignH, int alignV) {
  return alignedRectForSize(fm.boundingRect(text).size(), alignH, alignV);
}

// Ink extent of the font above and below the baseline, measured from a
// reference string carrying both an ascender and a descender.
//
// NOT fm.ascent()/fm.descent(). Our calibrated point sizes are chosen to
// match QSpice's rendered CAP HEIGHT (see makeFont() above), which makes
// the font's own ascent much larger than the text you actually see --
// the same effect already documented for boundingRect(), where a
// representative string measured ~201 units loose against ~130 units of
// real ink. Laying lines out on font ascent left a large empty band
// above the first line and pushed the last line's descenders out
// through the bottom of the box.
//
// Measured from a fixed reference string rather than from the text
// itself, so the result depends only on the font: content-dependent
// metrics would make every line shift vertically as characters are
// typed, and would leave a line of "abc" sitting at a different height
// than the same line once a "g" is added to it.
struct TextInkMetrics {
  double ascent  = 0.0; // baseline up to the top of an ascender
  double descent = 0.0; // baseline down to the bottom of a descender
};

// Reference string for the measurement below. Deliberately NOT just
// "Ag": a capital A only reaches CAP height, while the dot on an "i"
// and the ascenders on "b d f h k l" all sit above it in most fonts --
// measuring against "Ag" alone produced a box whose top edge cut
// through the dot of an "i" (confirmed on screen). This covers tall
// ascenders, dotted lowercase, caps, and the usual descenders.
//
// Still not exhaustive: accented capitals (Á, Ä, Å) are taller again,
// and scripts outside Latin taller still. The box padding below
// absorbs that -- getting this reference exactly right for every
// possible glyph isn't achievable, so it only has to be close.
inline const QString &inkReferenceText() {
  static const QString reference = QStringLiteral("AbdfhkliQgjpqy");
  return reference;
}

inline TextInkMetrics textInkMetrics(const QFontMetricsF &fm) {
  // tightBoundingRect() is baseline-relative: negative above, positive
  // below.
  const QRectF ink = fm.tightBoundingRect(inkReferenceText());
  if (ink.height() <= 0.0) return {fm.ascent(), fm.descent()};
  return {-ink.top(), ink.bottom()};
}

// Breathing room between the text and the edit box drawn around it.
// Proportional to the font's own ink height, so it stays visually
// constant across the very wide range of font sizes a schematic uses
// rather than vanishing on large text and swamping small text.
constexpr double kTextBoxPaddingFactor = 0.12;

inline double textBoxPadding(const QFontMetricsF &fm) {
  const TextInkMetrics ink = textInkMetrics(fm);
  return (ink.ascent + ink.descent) * kTextBoxPaddingFactor;
}

// Vertical distance between successive baselines in a multi-line
// element. A factor of 1.0 would put a descender from one line exactly
// against the ascender of the next; this leaves a small gap on top of
// that and nothing more.
//
// NOTE: an ESTIMATE. Unlike the font-size calibration above, this has
// not been fitted against real QSpice output -- no multi-line reference
// file has been measured yet. A screenshot of a real multi-line SPICE
// directive would settle it the same way the 5-point font-size
// calibration was settled.
constexpr double kLineSpacingFactor = 1.05;

inline double textLineHeight(const QFontMetricsF &fm) {
  const TextInkMetrics ink = textInkMetrics(fm);
  return (ink.ascent + ink.descent) * kLineSpacingFactor;
}

// A whole (possibly multi-line) text block, laid out in the local,
// anchor-relative, Y-down frame described above.
//
// Shared between the inline editor (SchEditText) and the items that
// render text, so the two cannot disagree about where lines sit -- the
// edit box has to overlay the text it replaces, and the text has to
// land back where the box showed it on commit. Keeping two copies of
// this arithmetic is how that guarantee gets lost.
//
// Draw each line at its BASELINE (baselines[i], x at lineRects[i].left()
// -- lines are justified to the anchor's side), not
// by handing lineRects[i] to drawText() with Qt::AlignTop -- that flag
// positions text by the font's own ascent, which is exactly the metric
// these rects are deliberately not built from. lineRects are the ink
// bands, for caret height, selection highlighting, and bounds.
struct TextBlockLayout {
  QStringList   lines;
  QList<QRectF> lineRects;
  QList<double> baselines;
  QRectF        box;
  double        lineHeight = 0.0;
};

inline TextBlockLayout layoutTextBlock(const QFontMetricsF &fm,
                                       const QString &text, int alignH,
                                       int alignV, double minWidth = 0.0) {
  TextBlockLayout layout;
  layout.lines = text.split(QLatin1Char('\n'));
  if (layout.lines.isEmpty()) layout.lines.append(QString());
  layout.lineHeight = textLineHeight(fm);

  const bool           multiLine = layout.lines.size() > 1;
  const TextInkMetrics ink       = textInkMetrics(fm);

  // horizontalAdvance(), not boundingRect(): the ink extent omits
  // trailing spaces, which puts a caret positioned by advance outside a
  // box measured by ink (confirmed -- see SchEditText).
  QList<double> widths;
  double        blockWidth = minWidth;
  for (const QString &line : layout.lines) {
    const double w = fm.horizontalAdvance(line);
    widths.append(w);
    blockWidth = std::max(blockWidth, w);
  }

  // A single line keeps the text's own measured height and the font-
  // ascent baseline, which is what the existing calibrated single-line
  // rendering already does -- this must stay byte-for-byte equivalent
  // to the old behavior for the overwhelmingly common one-line case.
  // Only stacked lines use the ink metrics.
  double rowHeight = multiLine ? (ink.ascent + ink.descent)
                               : fm.boundingRect(text).height();
  if (rowHeight <= 0.0) rowHeight = ink.ascent + ink.descent;

  const double firstBaseline = multiLine ? ink.ascent : fm.ascent();
  const double blockHeight =
      multiLine ? rowHeight + layout.lineHeight * (layout.lines.size() - 1)
                : rowHeight;

  layout.box =
      alignedRectForSize(QSizeF(blockWidth, blockHeight), alignH, alignV);

  // Each line is justified to the anchor's side: lines of a block
  // anchored on its left edge start together, on its right edge end
  // together, and on a centre line share a centre. A single line fills
  // the box, so this changes nothing there.
  for (int i = 0; i < layout.lines.size(); ++i) {
    const double top = layout.box.top() + i * layout.lineHeight;

    double left = layout.box.left();
    if (alignH == sch::core::kAlignHEast) left = layout.box.right() - widths[i];
    else if (alignH != sch::core::kAlignHWest)
      left = layout.box.center().x() - widths[i] / 2.0;

    layout.lineRects.append(QRectF(left, top, widths[i], rowHeight));
    layout.baselines.append(layout.box.top() + firstBaseline +
                            i * layout.lineHeight);
  }

  return layout;
}

// Trims an already-positioned aligned rect (from alignedTextRect(),
// using the loose/font-metrics QFontMetricsF::boundingRect()) down to
// the text's actual inked extent, for use as getRect()'s return value
// -- NOT for the actual drawing position, which stays on the loose rect
// unchanged (see makeFont()/alignedTextRect() -- that positioning is
// calibrated against real QSpice screenshots and shouldn't be touched
// here).
//
// CONFIRMED by direct measurement: QFontMetricsF::boundingRect()
// reserves vertical space for the font's full ascent+descent
// regardless of whether the specific string actually has descenders --
// "R1"/"1K"/"Vin" all measured a CONSTANT 201-unit loose height at a
// representative calibrated size, while tightBoundingRect() (the
// actual inked extent) measured only 127-132 units for those same
// strings -- a large amount of untouched vertical margin,
// disproportionately affecting height since width is already fairly
// tight in most fonts. This is exactly the reported "large margins
// vertically, correct horizontally" pattern.
//
// Rather than recompute alignment independently from
// tightBoundingRect() (risking a mismatch with where the loose-rect-
// based drawing actually puts the text -- e.g. SOUTH alignment's
// "bottom" differs between loose and tight metrics, since loose
// reserves descender space a no-descender string never uses), this
// takes the DIFFERENCE between loose and tight natural measurements on
// each side and trims the already-correctly-positioned aligned rect by
// those amounts. This keeps the box anchored exactly where the loose
// rect was (matching the real drawn text), just trimmed to size.
inline QRectF tightenTextRect(const QFontMetricsF &fm, const QString &text,
                              const QRectF &aligned) {
  const QRectF looseNatural = fm.boundingRect(text);
  const QRectF tightNatural = fm.tightBoundingRect(text);

  const double leftTrim   = tightNatural.left() - looseNatural.left();
  const double topTrim    = tightNatural.top() - looseNatural.top();
  const double rightTrim  = looseNatural.right() - tightNatural.right();
  const double bottomTrim = looseNatural.bottom() - tightNatural.bottom();

  return aligned.adjusted(leftTrim, topTrim, -rightTrim, -bottomTrim);
}

// Converts a "local" rect (as returned by alignedTextRect(), in the
// counter-flipped/Qt-native frame paint() draws text in) into our own
// Rect type, for use as getRect()'s return value.
//
// CONFIRMED bug (visual comparison against real QSpice output): the
// original conversion used static_cast<int>() directly on the raw
// float edges, which TRUNCATES toward zero -- this shrinks the box on
// BOTH edges (a negative left edge truncates to a less-negative, i.e.
// smaller-magnitude, value; a positive right edge truncates down too),
// so boundingRect() came out measurably smaller than the actual painted
// text. QGraphicsItem::boundingRect() must fully contain everything
// actually painted, or Qt's dirty-region-based incremental repainting
// can clip whatever falls just outside the reported box -- this didn't
// show up in a one-shot full-scene render (the original test method),
// only in a live, interactively-scrolled QGraphicsView, which is where
// it was actually caught (visible truncation on "C1"/"1u" labels).
//
// Fixed by rounding OUTWARD (floor the low edge, ceil the high edge)
// instead of truncating, plus a small flat safety margin on top of that
// to absorb any residual font-metric imprecision and give Qt's dirty-
// region-based repainting some slack.
//
// NOTE: a proportional margin (30% of the text's own size) was tried
// here first, reasoning that a small flat constant might not scale with
// very large calibrated font sizes. That turned out to be a real bug in
// its own right, found while testing drag-select: a long string like
// "Net description" at our calibrated ~130pt font produces a
// boundingRect() wide enough, once padded by 30%, to extend hundreds of
// scene units past the actual text -- far enough to overlap completely
// unrelated components elsewhere in the schematic. Confirmed directly:
// clicking on a resistor 700 units away from a "Vout"/"Net description"
// label was hit-testing the label instead, because its bloated
// boundingRect() covered the resistor's location too. Since then,
// Qt::TextDontClip was found to be the actual fix for text clipping
// (paint() now uses it directly, independent of getRect()'s accuracy),
// so this margin no longer needs to be aggressive -- it's just here for
// hit-testing/repaint slack, where a small flat value is both
// sufficient and safe regardless of how long or how large a given
// string is.
// The conversion alone, rounded outward, with NO margin -- for drawn
// geometry that sits in the text frame but is exact rather than glyph
// extent (a port's box), where padding would only push the selection
// outline away from what is drawn.
inline Rect toItemRect(Point anchor, const QRectF &local) {
  // local.top()/bottom() are in the counter-flipped (Qt-native, Y-down)
  // frame -- negate to get back to our Y-up Rect convention (Rect's own
  // normalize() sorts out which ends up as top vs. bottom).
  return Rect(Point(anchor.x + static_cast<int>(std::floor(local.left())),
                    anchor.y - static_cast<int>(std::floor(local.top()))),
              Point(anchor.x + static_cast<int>(std::ceil(local.right())),
                    anchor.y - static_cast<int>(std::ceil(local.bottom()))));
}

inline Rect toGenerousRect(Point anchor, const QRectF &local) {
  constexpr double kMargin = 20.0;
  return toItemRect(anchor,
                    local.adjusted(-kMargin, -kMargin, kMargin, kMargin));
}

} // namespace sch::render
