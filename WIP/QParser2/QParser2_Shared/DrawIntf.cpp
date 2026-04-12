//-----------------------------------------------------------------------------
// This file is part of the the QParser2 project.  You can find the complete
// project here:  https://github.com/robdunn4/QSpice/
//-----------------------------------------------------------------------------
#include "DrawIntf.h"

Point DrawIntf::getCenter() const noexcept { return getRect().getCenter(); }

bool DrawIntf::contains(Point p) const noexcept {
  return getRect().contains(p);
}

bool DrawIntf::intersects(Rect r) const noexcept {
  return getRect().intersects(r);
}

void DrawIntf::moveBy(Point delta) noexcept {
  moveTo(getPosition() + delta);
}

bool DrawIntf::hitTest(Point p) const noexcept {
  return isVisible() && contains(p);
}

bool DrawIntf::isVisible() const noexcept { return visible_; }

void DrawIntf::setVisible(bool vis) noexcept { visible_ = vis; }