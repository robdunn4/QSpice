/*
 * QArgUtils2.h -- Specializations of generic argument types.
 *
 * TODO:  Consider breaking up/grouping types into "graphical" vs other types.
 */

#pragma once
#include "QArgUtils.h"

// ============================================================================
// ArgImage
// ============================================================================

class ArgImage : public ArgString {
public:
  using ArgString::ArgString; // Inherit constructors

  // Explicit conversion required
  explicit ArgImage(const ArgString &other) : ArgString(other) {}
};

// ============================================================================
// ArgFontSize
// ============================================================================

class ArgFontSize : public ArgFloat {
public:
  using ArgFloat::ArgFloat; // Inherit constructors

  // Explicit conversion required
  explicit ArgFontSize(const ArgFloat &other) : ArgFloat(other) {}
};

// ============================================================================
// ArgColor
//
// Here's how I *think* colors work:
//   0xXXBBGGRR where BB, GG, and RR are blue, green, and red values.
//
//   If XX = 0x00, then the specified RGB color is used.
//   If XX = 0x01, then the default color specified for the type of item is
//                 fetched from the registry.
//   If XX = 0x05, then use a background image (see QItemRect).
//   If XX = 0x??, it corresponds to other specific registry entries (e.g.,
//                 schematic background color)
//
// ============================================================================

class ArgColor : public ArgHex {
public:
  using ArgHex::ArgHex; // Inherit constructors

  // Explicit conversion required
  explicit ArgColor(const ArgHex &other) : ArgHex(other) {}
};

// ============================================================================
// ArgStuff
//
// 0 = stuff, 1 = don't stuff (or disabled block), 2 = short with jumper
// ============================================================================

class ArgStuff : public ArgInt {
public:
  using ArgInt::ArgInt; // Inherit constructors

  // Explicit conversion required
  explicit ArgStuff(const ArgInt &other) : ArgInt(other) {}
};

// ============================================================================
// ArgLineType
//
// Various integral values...
// ============================================================================

class ArgLineType : public ArgInt {
public:
  using ArgInt::ArgInt; // Inherit constructors

  // Explicit conversion required
  explicit ArgLineType(const ArgInt &other) : ArgInt(other) {}
};

// ============================================================================
// ArgLineWidth
//
// Various integral values...
// ============================================================================

class ArgLineWidth : public ArgInt {
public:
  using ArgInt::ArgInt; // Inherit constructors

  // Explicit conversion required
  explicit ArgLineWidth(const ArgInt &other) : ArgInt(other) {}
};

// ============================================================================
// ArgRot
//
// Rotation code (0-15) rotates text by 45-degree increments counter-clockwise.
// (0 = horizontal/no rotation.)
// ============================================================================

class ArgRot : public ArgInt {
public:
  using ArgInt::ArgInt; // Inherit constructors

  // Explicit conversion required
  explicit ArgRot(const ArgInt &other) : ArgInt(other) {}
};

// ============================================================================
// ArgRotAlign
//
// Note: It appears that QSpice may create text or comments with a
//       rotation/alignment value of 0x00.  In that case, it appears to be
//       equivalent to 0x0F, i.e., no rotation and center vertical/horizontally.
//       Need to research further/verify.
//
//       Rotation code (0-15) rotates text by 45-degree increments
//       counter-clockwise.  (0 = horizontal/no rotation.)
//
//       Sometimes, the aligment bits are zero.  This seems to occur when the
//       text is part of a symbol (which can be hidden in the symbol browser)
//       and appears to be equivalent to center/center alignment.  However, I'm
//       not entirely sure....
// ============================================================================

class ArgRotAlign : public ArgInt {
public:
  // Inherit constructors
  using ArgInt::ArgInt;

  // Vertical alignment values (bits 0-1)
  static constexpr unsigned char NORTH = 0b01;
  static constexpr unsigned char SOUTH = 0b10;
  static constexpr unsigned char CENTER_V = 0b11;

  // Horizontal alignment values (bits 2-3, shifted)
  static constexpr unsigned char WEST = 0b0100;
  static constexpr unsigned char EAST = 0b1000;
  static constexpr unsigned char CENTER_H = 0b1100;

  // Extract vertical alignment (bits 0-1)
  unsigned char getAlignV() const;

  // Extract horizontal alignment (bits 2-3)
  unsigned char getAlignH() const;

  // Extract combined alignment (bits 0-3)
  unsigned char getAlign() const;

  // Get rotation code (bits 4-7, value 0-15)
  int getRotCode() const;

  // Get rotation in degrees (rotation code * 45)
  int getRotDeg() const;

  // Set vertical alignment (preserves horizontal and rotation)
  void setAlignV(unsigned char vert);

  // Set horizontal alignment (preserves vertical and rotation)
  void setAlignH(unsigned char horiz);

  // Set combined alignment (preserves rotation)
  void setAlign(unsigned char align);

  // Set rotation by code (0-15, preserves alignment)
  void setRotCode(int code);

  // Set rotation by degrees (must be multiple of 45, preserves alignment)
  void setRotDeg(int deg);

  // Create from components
  static ArgRotAlign create(unsigned char vert, unsigned char horiz,
                            int rotationCode = 0);
};

// ============================================================================
// ArgPinInfo
//
// Note: It appears that QSpice may creates a with port type and data type value
//       of 0x00.  That appears to indicate that type and direction are
//       undefined.
//
//       Port and data types are encoded in the LSB.  Bit vector ranges are
//       encoded in the next higher two bytes.
//
// For now, I'm deliberately not allowing getting/setting of port type
// separately from data type information.  The pin information should be set all
// at once....
// ============================================================================

class ArgPinInfo : public ArgInt {
public:
  // Inherit constructors
  using ArgInt::ArgInt;

  // port types (low nibble of LSB) -- zero is invalid?
  static constexpr unsigned char TYPE_INPORT = 0x01;
  static constexpr unsigned char TYPE_OUTPORT = 0x02;
  static constexpr unsigned char TYPE_DLLGND = 0x03;

  // data types (high nibble of LSB) -- zero is invalid?
  static constexpr unsigned char DATA_BOOL = 0x01;
  static constexpr unsigned char DATA_CHAR = 0x02;
  static constexpr unsigned char DATA_UCHAR = 0x03;
  static constexpr unsigned char DATA_SHORT = 0x04;
  static constexpr unsigned char DATA_USHORT = 0x05;
  static constexpr unsigned char DATA_INT = 0x06;
  static constexpr unsigned char DATA_UINT = 0x07;
  static constexpr unsigned char DATA_SFLOAT = 0x08;
  static constexpr unsigned char DATA_FLOAT = 0x09;
  static constexpr unsigned char DATA_INT64 = 0x0A;
  static constexpr unsigned char DATA_UINT64 = 0x0B;
  static constexpr unsigned char DATA_BITVECTOR = 0x0C;

  // return false if port or data type is zero (invalid)
  bool getInfo(unsigned char &portType, unsigned char &dataType,
               unsigned char &bvec1, unsigned char &bvec2) const {
    portType = value & 0x0F;
    dataType = (value >> 4) & 0x0F;
    bvec1 = (value >> 8) & 0x00FF;
    bvec2 = (value >> 16) & 0x00FF;
    return dataType && portType;
  }

  void setInfo(unsigned char portType, unsigned char dataType,
               unsigned char bvec1 = 0, unsigned bvec2 = 0) {

    // if (!portType || !dataType) throw...;
    value = bvec2 << 16;
    value |= bvec1 << 8;
    value |= (dataType & 0x0F) << 4;
    value |= portType & 0x0F;
  }
};