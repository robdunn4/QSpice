//-----------------------------------------------------------------------------
// This file is part of the the QParser2 project.  You can find the complete
// project here:  https://github.com/robdunn4/QSpice/
//-----------------------------------------------------------------------------
#include "DrawIntf.h"

Point DrawIntf::getCenter() const noexcept { return getBounds().center(); }

bool DrawIntf::contains(Point p) const noexcept {
  return getBounds().contains(p);
}

bool DrawIntf::intersects(Rect r) const noexcept {
  return getBounds().intersects(r);
}

void DrawIntf::move(Point delta) noexcept {
  setPosition(getPosition() + delta);
}

bool DrawIntf::hitTest(Point p) const noexcept {
  return isVisible() && contains(p);
}

bool DrawIntf::isVisible() const noexcept { return visible_; }

void DrawIntf::setVisible(bool vis) noexcept { visible_ = vis; }