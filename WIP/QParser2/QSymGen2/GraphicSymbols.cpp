//-----------------------------------------------------------------------------
// This file is part of the the QParser2 project.  You can find the complete
// project here:  https://github.com/robdunn4/QSpice/
//-----------------------------------------------------------------------------
#include "GraphicSymbols.h"
#include <ItemBase.h>
#include <ItemEllipse.h>
#include <ItemTriangle.h>

// add a graphic symbol to the node; code is unnecessarily redundant but
// clear and easy to add additional symbols
void GraphicsSymbols::addSymbol(const NodePtr &node, Type symType, Point pos,
                                int width, ArgColor lineColor,
                                ArgColor fillColor) {
  ItemBasePtr itemPtr;
  Point       p1, p2, p3;

  int top, left, bot, right;
  int halfWidth = width / 2;
  top           = pos.y + halfWidth;
  bot           = pos.y - halfWidth;
  left          = pos.x - halfWidth;
  right         = pos.x + halfWidth;
  int vCenter   = pos.y;
  int hCenter   = pos.x;

  switch (symType) {
  case CIRCLE:
    itemPtr = ItemEllipse::makePtr(ArgPoint(left, top), ArgPoint(right, bot),
                                   ArgRot(), ArgLineWidth(), ArgLineType(),
                                   ArgColor(lineColor), ArgColor(fillColor),
                                   ArgLookupNdx(), ArgPinNdx());
    break;

  case TRIANGLE_RIGHT:
    p1      = Point(left, top);
    p2      = Point(left, bot);
    p3      = Point(right, vCenter);
    itemPtr = ItemTriangle::makePtr(p1, p2, p3, ArgLineWidth(), ArgLineType(),
                                    lineColor, fillColor, ArgLookupNdx(),
                                    ArgPinNdx());
    break;

  case TRIANGLE_LEFT:
    p1      = Point(right, top);
    p2      = Point(right, bot);
    p3      = Point(left, vCenter);
    itemPtr = ItemTriangle::makePtr(p1, p2, p3, ArgLineWidth(), ArgLineType(),
                                    lineColor, fillColor, ArgLookupNdx(),
                                    ArgPinNdx());
    break;

  case TRIANGLE_UP:
    p1      = Point(hCenter, top);
    p2      = Point(left, bot);
    p3      = Point(right, bot);
    itemPtr = ItemTriangle::makePtr(p1, p2, p3, ArgLineWidth(), ArgLineType(),
                                    lineColor, fillColor, ArgLookupNdx(),
                                    ArgPinNdx());
    break;

  case TRIANGLE_DOWN:
    p1      = Point(hCenter, bot);
    p2      = Point(left, top);
    p3      = Point(right, top);
    itemPtr = ItemTriangle::makePtr(p1, p2, p3, ArgLineWidth(), ArgLineType(),
                                    lineColor, fillColor, ArgLookupNdx(),
                                    ArgPinNdx());
    break;
  }

  node->addLast(itemPtr);
}
