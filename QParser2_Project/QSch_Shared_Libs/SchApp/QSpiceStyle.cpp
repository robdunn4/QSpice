#include "QSpiceStyle.h"

#include "QSpicePrefs.h"

// SchRender names used in this file.
using sch::render::RenderStyle;

namespace sch::app {

RenderStyle qspiceRenderStyle() {
  namespace prefs = sch::link::prefs;

  RenderStyle style;
  style.highlight           = RenderStyle::fromBgr(prefs::highlightColor());
  style.background          = RenderStyle::fromBgr(prefs::backgroundColor());
  style.grid                = RenderStyle::fromBgr(prefs::gridColor());
  style.graphicAnnotation   = RenderStyle::fromBgr(prefs::graphicAnnotation());
  style.solidAnnotationFill = RenderStyle::fromBgr(prefs::solidAnnotationFill());
  style.graphicText         = RenderStyle::fromBgr(prefs::graphicText());
  style.componentText       = RenderStyle::fromBgr(prefs::componentText());
  style.schematicText       = RenderStyle::fromBgr(prefs::schematicText());
  style.commentText         = RenderStyle::fromBgr(prefs::commentText());
  style.netLabelText        = RenderStyle::fromBgr(prefs::netLabelText());
  style.wire                = RenderStyle::fromBgr(prefs::wireColor());
  style.highlightWire       = RenderStyle::fromBgr(prefs::highlightWireColor());
  style.pinDot              = RenderStyle::fromBgr(prefs::pinDotColor());
  style.spritedAnchor       = RenderStyle::fromBgr(prefs::spritedAnchor());
  style.pin                 = RenderStyle::fromBgr(prefs::pinColor());
  style.portBk              = RenderStyle::fromBgr(prefs::portBkColor());
  style.hierarchicalBlock   = RenderStyle::fromBgr(prefs::hierarchicalBlock());
  style.hierarchicalFill    = RenderStyle::fromBgr(prefs::hierarchicalFill());
  style.unconnectedPin      = RenderStyle::fromBgr(prefs::unconnectedPin());
  style.busTapError         = RenderStyle::fromBgr(prefs::busTapError());
  style.junctionSize        = prefs::junctionSize();
  style.cadLineWidth        = prefs::cadLineWidth();
  style.busWidthMultiplier  = prefs::busWidthMultiplier();
  style.defaultTextSize     = prefs::defaultTextSize();
  return style;
}

} // namespace sch::app
