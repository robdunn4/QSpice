#pragma once

#include "RenderStyle.h"

// QSpice's palette and metrics, as read by sch::link::prefs, turned into
// the RenderStyle everything draws with.
//
// Here, in SchApp, because it joins two libraries that are kept apart:
// SchLink knows nothing about drawing (it hands out raw 0xBBGGRR
// integers), and SchRender knows nothing about the registry. A program
// that wants QSpice's colours calls this after prefs::load() (and, for
// one that runs without QSpice, prefs::fillMissing()):
//
//   RenderStyle::setCurrent(sch::app::qspiceRenderStyle());
namespace sch::app {

sch::render::RenderStyle qspiceRenderStyle();

} // namespace sch::app
