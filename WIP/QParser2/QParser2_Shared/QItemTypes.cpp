//-----------------------------------------------------------------------------
// This file is part of the the QParser2 project.  You can find the complete
// project here:  https://github.com/robdunn4/QSpice/
//-----------------------------------------------------------------------------
#include "QItemTypes.h"

using enum QPI;

// clang-format off
EnumMapper<QPI> QItemTypes::enumMap({
     {SCH,      "schematic"},
     {COMP,     "component"},
     {SYM,      "symbol"},
     {TYPE,     "type:"},
     {DESC,     "description:"},
     {LIB,      "library file:"},
     {SHORTED,  "shorted pins:"},
     {NET,      "net"},
     {JUNCTION, "junction"},
     {WIRE,     "wire"},
     {TAP,      "tap"},
     {LINE,     "line"},
     {RECT,     "rect"},
     {ELLIPSE,  "ellipse"},
     {ARC3P,    "arc3p"},
     {TRIANGLE, "triangle"},
     {COIL,     "coil"},
     {ZIGZAG,   "zigzag"},
     {TEXT,     "text"},
     {PIN,      "pin"},
 });
// clang-format on

std::string QItemTypes::getStr(QPI enumID) { return enumMap.enumToStr(enumID); }

QPI QItemTypes::getEnum(std::string str) { return enumMap.strToEnum(str); }
