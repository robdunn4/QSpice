#pragma once
#include <QColor>
#include <QSet>
#include <QtGlobal>

#include "RenderStyle.h"

namespace sch::render {

// Decodes the raw ArgColor integer: 0xXXBBGGRR.
//
// The full mode table is documented in QParser2_Shared's ArgUtils2.h,
// which is authoritative -- this is a transcription, not a discovery:
//
//   0x00       custom RGB, from the low three bytes
//   0x01       the type's default colour, or no fill
//   0x02       background colour from the registry
//   0x03       foreground colour from the registry
//   0x04       solid fill colour from the registry
//   0x05-0x0B  background IMAGE, with a Windows raster op (SRCCOPY,
//              SRCPAINT, SRCAND, SRCINVERT, SRCERASE, NOTSRCERASE,
//              MERGEPAINT respectively)
//
// Modes 0x01 through 0x04 still carry populated RGB bytes, copied from
// the registry when the colour was chosen. Those are deliberately
// ignored here: the registry value wins, so changing a QSpice
// preference restyles existing files rather than leaving them on
// whatever the colour was when they were saved.
//
// FOREGROUND AND BACKGROUND are GDI vocabulary, not QSpice preference
// names, which is why neither has its own registry entry. In GDI they
// are the pair a monochrome bitmap or hatch brush is drawn with -- set
// bits in the foreground, clear bits in the background -- and their
// position in the table, immediately before the image modes, suggests
// they exist to serve those.
//
// Mapped here to the nearest schematic equivalents:
//   foreground -> CADgrapicAnnotation, i.e. "whatever symbols are drawn
//     in". Behaviourally this is what the V source's +/- bars and the
//     NPN's triangle do: filled solid in the same blue as the line
//     work.
//   background -> CADBackgroundColor, i.e. the canvas colour. Not so
//     much a colour choice as a knockout -- an opaque shape that hides
//     what is behind it while looking empty.
//
// BOTH MAPPINGS ARE INFERRED. No reference file has been seen using
// 0x02 at all, and 0x03 has only been seen where the element's line
// colour happens to equal CADgrapicAnnotation, so the two readings of
// it cannot be told apart from the evidence available. A shape with an
// explicit RGB line colour and a 0x03 fill would separate them: this
// mapping fills it with the palette blue, the alternative fills it with
// that shape's own line colour.

namespace SchColorDecode {

inline int colorMode(int raw) {
  return static_cast<int>(static_cast<unsigned char>((raw >> 24) & 0xFF));
}

inline QColor rgbFromArgColor(int raw) {
  const unsigned char b = static_cast<unsigned char>((raw >> 16) & 0xFF);
  const unsigned char g = static_cast<unsigned char>((raw >> 8) & 0xFF);
  const unsigned char r = static_cast<unsigned char>(raw & 0xFF);
  return QColor(r, g, b);
}

// Reports each unrecognised mode once, so a file that uses one says so
// without filling the log. These are the values needed to finish the
// palette mapping.
inline void noteUnknownMode(int raw, const char *usage) {
  static QSet<int> reported;
  const int        mode = colorMode(raw);
  if (reported.contains(mode)) return;
  reported.insert(mode);
  qWarning("ArgColor: unrecognised colour mode 0x%02x (raw 0x%08x, used as "
           "%s) -- falling back; this mode still needs mapping to a QSpice "
           "palette entry",
           mode, static_cast<unsigned int>(raw), usage);
}

// Line, outline and text colours.
//
// Modes confirmed against QSpice's own rendering:
//   0x04  BusStuff_QSpice.qsch's rect is line 0x4000000, fill 0x1000000,
//         and QSpice draws its outline in the element's DEFAULT line
//         colour (CADgrapicAnnotation blue), NOT CADsolidAnnotation
//         yellow. So "solid fill colour" is a fill-only meaning; as a
//         line colour 0x04 is the same as 0x01. The X1 hierarchical
//         block's rect (line 0x4000000, blue border) agrees.
inline QColor fromArgColor(int raw, const QColor &defaultColor) {
  const int mode = colorMode(raw);
  if (mode == 0x00) return rgbFromArgColor(raw);
  if (mode == 0x01 || mode == 0x04) return defaultColor;

  noteUnknownMode(raw, "a line/text colour");
  return defaultColor;
}

// Fill colours.
//
// Modes confirmed against QSpice's own rendering:
//   0x01  V1's ellipse is 0x1000000 and draws hollow.
//   0x03  V1's +/- bars and the NPN's triangle are 0x3000000 and draw
//         solid in the symbol's line colour.
//   0x04  the AnalogLatch's body is 0x4000000 and draws yellow, which
//         is CADsolidAnnotation exactly.
//
// lineColor is this element's resolved outline colour. No longer used
// for 0x03 -- see the foreground note above -- but kept as the
// parameter because it is the right fallback if that mapping turns out
// to be wrong.
inline QColor fromArgFill(int raw, const QColor &lineColor) {
  Q_UNUSED(lineColor);

  switch (colorMode(raw)) {
  case 0x00:
    return rgbFromArgColor(raw);
  case 0x01:
    return Qt::transparent;
  case 0x02:
    return RenderStyle::current().background;
  case 0x03:
    return RenderStyle::current().graphicAnnotation;
  case 0x04:
    return RenderStyle::current().solidAnnotationFill;
  default:
    break;
  }

  // 0x05-0x0B are image fills. A rect with image data draws the bitmap
  // itself (SchItemRect::drawImage()) and never asks for a colour; any
  // other shape has no image to draw, so it is left unfilled rather than
  // given a colour that would be wrong in a different way.
  const int mode = colorMode(raw);
  if (mode >= 0x05 && mode <= 0x0B) return Qt::transparent;

  noteUnknownMode(raw, "a fill colour");
  return Qt::transparent;
}

} // namespace SchColorDecode

} // namespace sch::render
