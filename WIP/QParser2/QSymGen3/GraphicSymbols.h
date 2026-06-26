//-----------------------------------------------------------------------------
// This file is part of the the QSymGen3 command-line tool contained in the
// QParser2 project.  You can find the complete project here:
// https://github.com/robdunn4/QSpice/
//-----------------------------------------------------------------------------
#pragma once
#include "ItemTree.h"
#include <ArgUtils2.h>

namespace GraphicsSymbols {
enum Type { CIRCLE, TRIANGLE_RIGHT, TRIANGLE_LEFT, TRIANGLE_UP, TRIANGLE_DOWN };

// note: size of 100 is approximately equivalent to font size of 1.0f
void addSymbol(const NodePtr &node, Type symType, Point pos, int width,
               ArgColor lineColor, ArgColor fillColor);

} // namespace GraphicsSymbols
