//-----------------------------------------------------------------------------
// This file is part of the the QParser2 project.  You can find the complete
// project here:  https://github.com/robdunn4/QSpice/
//-----------------------------------------------------------------------------
#include "QItemTypes.h"

using enum QPI;

// clang-format off
EnumMapper<QPI> QItemTypes::enumMap({
     {SCH, "schematic"},
     {SYM, "symbol"},
     {COMP, "component"},
     {TYPE, "type:"},
     {DESC, "description:"},
     {SHORTED, "shorted pins:"},
     {LIB, "library file:"},
     {LINE, "line"},
     {TEXT, "text"},
     {PIN, "pin"},
     {RECT, "rect"},
     {NET, "net"},
     {WIRE, "wire"},
     {ZIGZAG, "zigzag"},
     {ELLIPSE, "ellipse"},
     {ARC3P, "arc3p"},
     {JUNCTION, "junction"},
     {COIL, "coil"},
     {TRIANGLE, "triangle"},
     {TAP, "tap"}
 });
// clang-format on

std::string QItemTypes::getStr(QPI enumID) { return enumMap.enumToStr(enumID); }

QPI QItemTypes::getEnum(std::string str) { return enumMap.strToEnum(str); }
