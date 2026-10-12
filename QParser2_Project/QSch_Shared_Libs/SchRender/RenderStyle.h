#pragma once

#include <QColor>
#include <QtGlobal>

namespace sch::render {

// The colours and line metrics the schematic is DRAWN with -- every value
// the item classes, SchPen and SchColorDecode used to read straight from
// QSpice's registry settings (sch::link::prefs).
//
// Plain values: this knows nothing about where they came from. The
// editor fills it from QSpice's [Preferences] after they are read
// (qspiceRenderStyle(), QSpiceStyle.h); another program -- the planned
// print/export tool -- may fill it however it likes. Until someone does,
// it holds a stock QSpice installation's values, so drawing is never
// left without a palette.
//
// ONE current style for the process: every window draws with the same
// palette. A per-scene style (a print palette, say) can come later
// without touching the items again.
//
// Colour names follow sch::link::prefs, which documents the registry key
// behind each. Header-only data; no moc.
struct RenderStyle {
  // --- Colours. Stock values are QSpice's defaults, already turned from
  // the registry's 0xBBGGRR into RGB.
  QColor highlight          = QColor(100, 100, 50);
  QColor background         = QColor(255, 255, 233);
  QColor grid               = QColor(0, 0, 0);
  QColor graphicAnnotation  = QColor(0, 0, 255);
  QColor solidAnnotationFill = QColor(255, 255, 0);
  QColor graphicText        = QColor(0, 0, 255);
  QColor componentText      = QColor(0, 0, 0);
  QColor schematicText      = QColor(0, 0, 0);
  QColor commentText        = QColor(0, 0, 255);
  QColor netLabelText       = QColor(255, 0, 0);
  QColor wire               = QColor(0, 0, 255);
  QColor highlightWire      = QColor(225, 225, 100);
  QColor pinDot             = QColor(0, 0, 0);
  QColor spritedAnchor      = QColor(255, 255, 0);
  QColor pin                = QColor(0, 0, 0);
  QColor portBk             = QColor(255, 255, 255);
  QColor hierarchicalBlock  = QColor(0, 0, 255);
  QColor hierarchicalFill   = QColor(200, 200, 200);
  QColor unconnectedPin     = QColor(255, 255, 255);
  QColor busTapError        = QColor(255, 0, 0);

  // --- Metrics.
  double junctionSize       = 1.0; // JunctionSize multiplier
  double cadLineWidth       = 1.5; // line width that "0" in a record means
  double busWidthMultiplier = 2.0;
  double defaultTextSize    = 1.0;

  // The style everything draws with.
  static const RenderStyle &current();

  // Replaces it. Items already on screen repaint with the new values on
  // their next paint; nothing is repainted here.
  static void setCurrent(const RenderStyle &style);

  // A colour as QSpice stores it, 0xBBGGRR -- NOT 0xRRGGBB; see
  // SchLink's QSpicePrefs.h for how that order was confirmed.
  static QColor fromBgr(quint32 bgr);
};

} // namespace sch::render
