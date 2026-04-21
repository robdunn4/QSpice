//-----------------------------------------------------------------------------
// This file is part of the the QParser2 project.  You can find the complete
// project here:  https://github.com/robdunn4/QSpice/
//-----------------------------------------------------------------------------
#include "SymData.h"

inline SymPin::SymPin(const PinItemPtr pinItem) {
  ArgString   pinLabel(pinItem->getName());
  ArgString   netName(pinItem->getNetName());
  Point       pt(0, 0);
  ArgRotAlign rotateAlign;
  ArgColor    color(0x000000); // default
  ArgFontSize fontSize(1.0);   // default
  ArgPinInfo  pinInfo;

  switch (pinItem->getType()) {
  case PinItemType::Input:
    pinInfo.setInfo(ArgPinInfo::TYPE_INPORT, ArgPinInfo::DATA_FLOAT);
    break;
  case PinItemType::Output:
    pinInfo.setInfo(ArgPinInfo::TYPE_OUTPORT, ArgPinInfo::DATA_FLOAT);
    break;
  case PinItemType::Gpio:
    // TODO
    assert(false && "GPIO pin type not yet implemented");
    break;
  default:
    assert(false && "Unknown PinItemType");
    break;
  }

  itemPin = ItemPin::makePtr(pt, pt, fontSize, rotateAlign, pinInfo, color,
                             ArgLookupNdx(-1), pinLabel, netName);
}

SymData::SymData(const PinItemList &pinList) {
  for (const auto &pinItem : pinList) {
    emplace_back(pinItem);
  }
}

int SymData::calcRows() const {
  return (int)((size() + 1) / 2); // two columns, so divide by 2 and round up
}

// fix up coordinates & alignment of pins based on column/row and pin spacing
void SymData::fixCoords() {
  int   rows    = calcRows();
  int   height  = rows * rowSpacing;
  int   xOffset = width / 2;
  Point topLeft_(-xOffset, height / 2);
  Point botRight_(xOffset, -height / 2);

  topLeft  = topLeft_ + Point(0, rowSpacing / 2);
  botRight = botRight_ + Point(0, rowSpacing / 2);

  // adjust pin coordinates based on column/row and pin spacing
  iterator iter    = begin();
  iterator endIter = end();
  for (int col_ = 0; col_ < 2 && iter < endIter; ++col_) {
    int         x        = col_ ? xOffset : -xOffset; // left or right column
    int         y        = height / 2;                // start at top
    ArgRotAlign rotAlign = col_ ? ArgRotAlign::EAST | ArgRotAlign::CENTER_V
                                : ArgRotAlign::WEST | ArgRotAlign::CENTER_V;
    for (int row_ = 0; row_ < rows && iter < endIter; ++row_, iter++) {
      // set alignmenet
      iter->itemPin->pt1 = Point(x, y);
      iter->itemPin->pt2 = Point(col_ ? -20 : 20, 0);
      iter->itemPin->rotateAlign.setAlign(rotAlign);
      y -= rowSpacing; // move down for next pin
    }
  }
}
