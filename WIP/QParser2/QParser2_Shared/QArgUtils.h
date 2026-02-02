//-----------------------------------------------------------------------------
// This file is part of the the QParser2 project.  You can find the complete
// project here:  https://github.com/robdunn4/QSpice/
//-----------------------------------------------------------------------------
/*
 * Classes for simple argument types/values.  The idea is that we start with
 * basic types that can later be derived to more descriptive types.  For
 * example, we might have an argument that initially we recognize as a float
 * (ArgFloat). Later in the reverse-engineering we might discover that it's
 * actually font size. At that point, we can derive an ArgFontSize class that
 * specializes for font-size-specific functionality....
 */
#pragma once
#include <string>

// ============================================================================
// Point
// ============================================================================
struct Point {
  int x;
  int y;

  Point();
  Point(int x_, int y_);

  // Add another point to this point and return result
  Point add(const Point &other) const;

  // Operator overload for addition
  Point operator+(const Point &other) const;

  // Add another point to this point (modifies this point)
  Point &operator+=(const Point &other);
};

// ============================================================================
// ArgInt
// ============================================================================
class ArgInt {
protected:
  int value;

public:
  ArgInt();
  ArgInt(int val);
  ArgInt(const std::string &str);
  ArgInt(const ArgInt &other);

  // Getter
  int getValue() const;

  // Setter (raw value -- use derived class methods when possible)
  void setValue(int value) { this->value = value; }

  // Conversion operator
  operator int() const;

  // Static parse method
  static int parse(const std::string &str);

  // toString methods
  std::string toString() const;
  static std::string toString(int value);
};

// ============================================================================
// ArgFloat
// ============================================================================
class ArgFloat {
protected:
  float value;

public:
  // Constructors
  ArgFloat();
  ArgFloat(float val);
  ArgFloat(const std::string &str);
  ArgFloat(const ArgFloat &other);

  // Getter
  float getValue() const;

  // Setter (raw value -- use derived class methods when possible)
  void setValue(float value) { this->value = value; }

  // Conversion operator
  operator float() const;

  // Static parse method
  static float parse(const std::string &str);

  // toString methods
  std::string toString(int precision = 0) const;
  static std::string toString(float value, int precision = 0);
};

// ============================================================================
// ArgHex
// ============================================================================
class ArgHex {
protected:
  int value;

public:
  // Constructors
  ArgHex();
  ArgHex(int val);
  ArgHex(const std::string &str);
  ArgHex(const ArgHex &other);

  // Getter
  int getValue() const;

  // Setter (raw value -- use derived class methods when possible)
  void setValue(int value) { this->value = value; }

  // Conversion operator
  operator int() const;

  // Static parse method
  static int parse(const std::string &str);

  // toString methods
  std::string toString(int width = 0) const;
  static std::string toString(int value, int width = 0);
};

// ============================================================================
// ArgBool
// ============================================================================
class ArgBool {
protected:
  bool value;

public:
  // Constructors
  ArgBool();
  ArgBool(bool val);
  ArgBool(const std::string &str);
  ArgBool(const ArgBool &other);

  // Getter
  bool getValue() const;

  // Setter (raw value -- use derived class methods when possible)
  void setValue(bool value) { this->value = value; }

  // Conversion operator
  operator bool() const;

  // Static parse method
  static bool parse(const std::string &str);

  // toString methods
  std::string toString() const;
  static std::string toString(bool value);
};

// ============================================================================
// ArgString
//
// Note:  Text and comment strings have special encoding.  Enabling/disabling
//        overbars in the GUI menu doesn't appear to have a special flag;
//        instead QSpice appears to add "overbar codes" in front of each
//        character that should have an overbar.  (This can be done manually
//        character by character in the GUI IIRC.)
// ============================================================================
class ArgString {
protected:
  std::string value;

public:
  // Constructors
  ArgString();
  ArgString(const std::string &str);
  ArgString(const ArgString &other);

  // Getter
  std::string getValue() const;

  // Setter (raw value -- use derived class methods when possible)
  void setValue(std::string value) { this->value = value; }

  // Conversion operator
  operator std::string() const;

  // Static parse method
  static std::string parse(const std::string &str);

  // toString methods
  std::string toString() const;
  static std::string toString(const std::string &value);
};

// ============================================================================
// ArgPoint
// ============================================================================
class ArgPoint {
protected:
  Point value;

public:
  // Constructors
  ArgPoint();
  ArgPoint(const Point &p);
  ArgPoint(int x, int y);
  ArgPoint(const std::string &str);
  ArgPoint(const ArgPoint &other);

  // Getter
  Point getValue() const;

  // Setter (raw value -- use derived class methods when possible)
  void setValue(Point value) { this->value = value; }

  // Conversion operator
  operator Point() const;

  // Static parse method
  static Point parse(const std::string &str);

  // toString methods
  std::string toString() const;
  static std::string toString(const Point &point);
};