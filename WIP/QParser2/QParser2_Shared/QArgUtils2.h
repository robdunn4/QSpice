//-----------------------------------------------------------------------------
// This file is part of the the QParser2 project.  You can find the complete
// project here:  https://github.com/robdunn4/QSpice/
//-----------------------------------------------------------------------------
/*
 * QArgUtils2.h -- Specializations of generic argument types.
 */
#pragma once
#include "QArgUtils.h"
#include <iostream>

// ============================================================================
// TODO:  Revisit This!  Extract these notes (once confirmed) to an overview
// document....
//
// The order of QItem* elements within a QItemSym matters.  It's complicated....
//
// Within a QItemSym all QItem* elements of a given type occur as a group
// without interceding elements of other types.  That is, all QItemText records
// are grouped, all QItemPin records are grouped, all QItemRect records are
// grouped, etc.  (I assume that there is a specific order of groups by QItem*
// type but haven't yet reverse-engineered the required order.)
//
// Within a group, the first element of that type is at index 0.  The index
// position is used in ArgLookupNdx and ArgPinNdx fields to refer to the
// related item.  If records are inserted/deleted/reordered outside of the GUI,
// take care to adjust any Arg*Ndx records.
//
// As a contrived (and possibly incorrect) example, graphical elements such as a
// QItemRect or QItemText may be associated with a QItemPin record in the
// ArgPinNdx argument.  The index value of ArgPinNdx points to the index within
// the group of QItemPin records.
// ============================================================================

// ============================================================================
// ArgUnkown -- intended for debugging currently unknown parameters; writes
// message to stderr if value != -1
// ============================================================================

class ArgUnknown : public ArgInt {
public:
  // using ArgInt::ArgInt; // Inherit constructors
  ArgUnknown() : ArgInt(0) {}
  ArgUnknown(int val) : ArgInt(val) { checkValue(); }
  ArgUnknown(const std::string &str) : ArgInt(str) { checkValue(); }

  // Explicit conversion required
  explicit ArgUnknown(const ArgInt &other) : ArgInt(other) { checkValue(); }

  void checkValue() {
    if (value != -1) {
      // for now, show message on cerr?  set breakpoint here...
      std::cerr << "A parameter of type ArgUknown isn't -1.  Please send files "
                   "to author for investigation.\n";
    }
  }
};

// ============================================================================
// ArgLookupNdx -- used with programmable symbols? (details TBD)
//
// I assume this is used with the LOOKUP programmable attribute.
// A first thing to note is that the QItemSym symbol name is the name of the
// symbol file (without the extension).  This, I think, becomes the first
// level subfolder under [QSpice Folder]\Repository\SYMBOLNAME\...
//
// Of course, once you drag/drop a symbol into a schematic, the entire symbol
// is copied into the schematic so the symbol name normally doesn't matter.
// But, as above, the symbol name will matter if you're using this LOOKUP
// thing.  Seems to me that, if you had something that worked and changed
// the name of the symbol file, it would break the link to the proper folder
// name....
//
// =====
//
// Additional Info:  The ArgLookupNdx value is an index into the QItemText
// record group.  That QItemText record is expected to contain a programmable
// attribute ("LOOKUP") that ultimately loaded the record from a symbol file
// found in [QSpice]\Repository\SYMNAME.
//
// TODO:  Verify above and explain more clearly.
//
// ============================================================================

// class ArgLookupNdx : public ArgInt {
// public:
//   using ArgInt::ArgInt; // Inherit constructors
//
//   // Explicit conversion required
//   explicit ArgLookupNdx(const ArgInt &other) : ArgInt(other) {}
// };

// for now, deriving from ArgUnknown to generate debugging messages if not -1...
class ArgLookupNdx : public ArgUnknown {
public:
  using ArgUnknown::ArgUnknown; // Inherit constructors

  // Explicit conversion required
  explicit ArgLookupNdx(const ArgUnknown &other) : ArgUnknown(other) {}
};

// ============================================================================
// ArgPinNdx -- used with programmable symbols?
//
// Some built-in symbols use this parameter (Ã, ¥, €, and £?) so can't just
// throw errors when other than -1 is encountered...
//
// -----
//
// OK, new information.  Graphical elements (rectangles, triangles, etc.) can
// set this parameter to a pin index.  This is the zero-based index of the
// QItemPin record within the enclosing symbol.  If this index is not -1 and the
// QItemPin net name is "¥" (0xA5) then the graphic element is rendered
// invisible and the pin is disabled.
//
// The Symbol Editor has a right-click menu item for graphical elements to set
// the pin association, "Associated Pin".  However, it seems to be
// unimplemented.
//
// Mentioned to Mike on 2026.02.08.  Fixed on 2026.02.08.  (He's fast!)
// ============================================================================

class ArgPinNdx : public ArgInt {
public:
  using ArgInt::ArgInt; // Inherit constructors

  // Explicit conversion required
  explicit ArgPinNdx(const ArgInt &other) : ArgInt(other) {}
};

// ============================================================================
// ArgTextFlags
// ============================================================================

class ArgTextFlags : public ArgInt {
public:
  using ArgInt::ArgInt; // Inherit constructors

  // Explicit conversion required
  explicit ArgTextFlags(const ArgInt &other) : ArgInt(other) {}

  static constexpr int COMMENT_BIT = 0x01;
  static constexpr int VISIBLE_BIT = 0x02;

  // TODO:  Consider making the commentVisible stuff a class of its own...

  // the visible/hidden bit affects only symbol attribute visiblity, i.e.,
  // top-level text items are always visible as far as I can determine
  bool isComment() const { return value & COMMENT_BIT; }
  // bool isDirective() const { return !isComment(); }
  bool isHidden() const { return value & VISIBLE_BIT; }
  // bool isVisible() const { return !isHidden(); }

  // set/clear flags without altering other bits (just in case they are
  // used for something as yet unknown)
  void setComment() { value |= COMMENT_BIT; }
  void clearComment() { value &= ~COMMENT_BIT; }
  void setHidden() { value |= VISIBLE_BIT; }
  void clearHidden() { value &= ~VISIBLE_BIT; }
};

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
// New Info:  2026.02.10...
//
// XX also encodes BLT mode for images (not sure if it might more)
//
//   If XX = 0x05, BLT = SRCCOPY
//   If XX = 0x06, BLT = SRCPAINT
//   If XX = 0x07, BLT = SRCAND
//   If XX = 0x08, BLT = SRCINVERT
//   If XX = 0x09, BLT = SRCERASE
//   If XX = 0x0A, BLT = NOTSRCERASE
//   If XX = 0x0B, BLT = MERGEPAINT
//
// What are the meanings for 0x02 - 0x04?  (Above suggests values that I don't
// remember today... or it was mere conjecture when I first wrote it?)
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