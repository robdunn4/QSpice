#include "RenderStyle.h"

namespace sch::render {

namespace {

RenderStyle currentStyle; // stock until someone sets it

} // namespace

const RenderStyle &RenderStyle::current() { return currentStyle; }

void RenderStyle::setCurrent(const RenderStyle &style) { currentStyle = style; }

QColor RenderStyle::fromBgr(quint32 bgr) {
  const int b = static_cast<int>((bgr >> 16) & 0xFF);
  const int g = static_cast<int>((bgr >> 8) & 0xFF);
  const int r = static_cast<int>(bgr & 0xFF);
  return QColor(r, g, b);
}

} // namespace sch::render
