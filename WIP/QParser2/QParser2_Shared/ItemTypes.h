//-----------------------------------------------------------------------------
// This file is part of the the QParser2 project.  You can find the complete
// project here:  https://github.com/robdunn4/QSpice/
//-----------------------------------------------------------------------------
/*
 * ItemTypes.h -- Utility class for mapping text to enum for the item types.
 */
#pragma once
#include "EnumStrMap.h"

//-----------------------------------------------------------------------------
// Note:  Item* order appears to be important.  The below QPI order is, I
// think, correct for sorting.  However, the order *could* be different for
// elements inside a symbol vs at the schematic level.  TBD
//
// Further, all elements of a given Item* type must be grouped together.  That
// is, all ItemLine, QRect, QText, etc., items occur consecutively.  As far as
// I know at the moment, order within Item* type doesn't  matter except for the
// below....
//
// Item* records that contain ArgPinNdx and ArgLookupNdx values that are not -1
// are indexes into the ItemPin and ItemText groups respectively.  If ItemPin
// or ItemText records are added, deleted, or reordered, these indexes must be
// updated to reflect the correct index into the respective group.
//-----------------------------------------------------------------------------

enum class QPI
{
  UNKNOWN,  // invalid (or any)
  SCH,      // "schematic"
  COMP,     // "component"
  SYM,      // "symbol"
  TYPE,     // "type:"
  DESC,     // "description:"
  LIB,      // "library file:"
  SHORTED,  // "shorted pins:"
  NET,      // "net"
  JUNCTION, // "junction"
  WIRE,     // "wire"
  TAP,      // "tap"
  LINE,     // "line"
  RECT,     // "rect"
  ELLIPSE,  // "ellipse"
  ARC3P,    // "arc3p"
  TRIANGLE, // "triangle"
  COIL,     // "coil"
  ZIGZAG,   // "zigzag"
  TEXT,     // "text"
  PIN,      // "pin"
};

class ItemTypes {
public:
  static EnumMapper<QPI> enumMap;

  static std::string getStr(QPI enumID);
  static QPI         getEnum(std::string str);
};
