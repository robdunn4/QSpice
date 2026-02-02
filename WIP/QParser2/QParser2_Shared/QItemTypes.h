//-----------------------------------------------------------------------------
// This file is part of the the QParser2 project.  You can find the complete
// project here:  https://github.com/robdunn4/QSpice/
//-----------------------------------------------------------------------------
/*
 * QItemTypes.h -- Utility class for mapping text to enum for the item types.
 */
#pragma once
#include "EnumStrMap.h"

enum class QPI
{
  UNKNOWN,  // invalid
  SCH,      // "schematic"
  SYM,      // "symbol"
  COMP,     // "component"
  TYPE,     // "type:"
  DESC,     // "description:"
  SHORTED,  // "shorted pins:"
  LIB,      // "library file:"
  LINE,     // "line"
  TEXT,     // "text"
  PIN,      // "pin"
  RECT,     // "rect"
  NET,      // "net"
  WIRE,     // "wire"
  ZIGZAG,   // "zigzag"
  ELLIPSE,  // "ellipse"
  ARC3P,    // "arc3p"
  JUNCTION, // "junction"
  COIL,     // "coil"
  TRIANGLE, // "triangle"
  TAP       // "tap"
};

class QItemTypes {
public:
  static EnumMapper<QPI> enumMap;

  static std::string getStr(QPI enumID);
  static QPI getEnum(std::string str);
};