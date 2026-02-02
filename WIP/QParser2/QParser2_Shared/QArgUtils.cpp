//-----------------------------------------------------------------------------
// This file is part of the the QParser2 project.  You can find the complete
// project here:  https://github.com/robdunn4/QSpice/
//-----------------------------------------------------------------------------
#include "QArgUtils.h"
#include <iomanip>
#include <regex>
#include <sstream>
#include <stdexcept>

// ============================================================================
// Point implementation
// ============================================================================

Point::Point() : x(0), y(0) {}

Point::Point(int x_, int y_) : x(x_), y(y_) {}

Point Point::add(const Point &other) const {
  return Point(x + other.x, y + other.y);
}

Point Point::operator+(const Point &other) const {
  return Point(x + other.x, y + other.y);
}

Point &Point::operator+=(const Point &other) {
  x += other.x;
  y += other.y;
  return *this;
}

// ============================================================================
// ArgInt implementation
// ============================================================================

ArgInt::ArgInt() : value(0) {}

ArgInt::ArgInt(int val) : value(val) {}

ArgInt::ArgInt(const std::string &str) : value(parse(str)) {}

ArgInt::ArgInt(const ArgInt &other) : value(other.value) {}

int ArgInt::getValue() const { return value; }

ArgInt::operator int() const { return value; }

int ArgInt::parse(const std::string &str) {
  try {
    size_t pos;
    int value = std::stoi(str, &pos);
    // Check if entire string was consumed
    if (pos != str.length()) {
      throw std::invalid_argument("Invalid integer format");
    }
    return value;
  } catch ([[maybe_unused]] const std::exception &e) {
    throw std::invalid_argument("Failed to parse integer: " + str);
  }
}

std::string ArgInt::toString() const { return std::to_string(value); }

std::string ArgInt::toString(int value) { return std::to_string(value); }

// ============================================================================
// ArgFloat implementation
// ============================================================================

ArgFloat::ArgFloat() : value(0.0f) {}

ArgFloat::ArgFloat(float val) : value(val) {}

ArgFloat::ArgFloat(const std::string &str) : value(parse(str)) {}

ArgFloat::ArgFloat(const ArgFloat &other) : value(other.value) {}

float ArgFloat::getValue() const { return value; }

ArgFloat::operator float() const { return value; }

float ArgFloat::parse(const std::string &str) {
  try {
    size_t pos;
    float value = std::stof(str, &pos);
    if (pos != str.length()) {
      throw std::invalid_argument("Invalid float format");
    }
    return value;
  } catch ([[maybe_unused]] const std::exception &e) {
    throw std::invalid_argument("Failed to parse float: " + str);
  }
}

std::string ArgFloat::toString(int precision) const {
  std::ostringstream oss;
  // oss << std::fixed << std::setprecision(precision) << value;
  oss << value;
  return oss.str();
}

std::string ArgFloat::toString(float value, int precision) {
  std::ostringstream oss;
  // oss << std::fixed << std::setprecision(precision) << value;
  oss << value;
  return oss.str();
}

// ============================================================================
// ArgHex implementation
// Note:  Apparently hex values should be lower case.
// ============================================================================

ArgHex::ArgHex() : value(0) {}

ArgHex::ArgHex(int val) : value(val) {}

ArgHex::ArgHex(const std::string &str) : value(parse(str)) {}

ArgHex::ArgHex(const ArgHex &other) : value(other.value) {}

int ArgHex::getValue() const { return value; }

ArgHex::operator int() const { return value; }

int ArgHex::parse(const std::string &str) {
  try {
    // Ensure string starts with "0x" or "0X"
    if (str.length() < 3 ||
        (str.substr(0, 2) != "0x" && str.substr(0, 2) != "0X")) {
      throw std::invalid_argument("Hex string must start with 0x");
    }

    size_t pos;
    int value = std::stoi(str, &pos, 16);
    if (pos != str.length()) {
      throw std::invalid_argument("Invalid hex format");
    }
    return value;
  } catch ([[maybe_unused]] const std::exception &e) {
    throw std::invalid_argument("Failed to parse hex: " + str);
  }
}

std::string ArgHex::toString(int width) const {
  std::ostringstream oss;
  oss << "0x" << std::setfill('0') << std::setw(width) << std::hex
      << std::nouppercase << value;
  return oss.str();
}

std::string ArgHex::toString(int value, int width) {
  std::ostringstream oss;
  oss << "0x" << std::setfill('0') << std::setw(width) << std::hex
      << std::nouppercase << value;
  return oss.str();
}

// ============================================================================
// ArgBool implementation
// ============================================================================

ArgBool::ArgBool() : value(false) {}

ArgBool::ArgBool(bool val) : value(val) {}

ArgBool::ArgBool(const std::string &str) : value(parse(str)) {}

ArgBool::ArgBool(const ArgBool &other) : value(other.value) {}

bool ArgBool::getValue() const { return value; }

ArgBool::operator bool() const { return value; }

bool ArgBool::parse(const std::string &str) {
  if (str == "true") return true;
  if (str == "false") return false;
  throw std::invalid_argument("Failed to parse bool: " + str);
}

std::string ArgBool::toString() const { return value ? "true" : "false"; }

std::string ArgBool::toString(bool value) { return value ? "true" : "false"; }

// ============================================================================
// ArgString implementation
// ============================================================================

ArgString::ArgString() : value("") {}

ArgString::ArgString(const std::string &str) : value(str) {}

ArgString::ArgString(const ArgString &other) : value(other.value) {}

std::string ArgString::getValue() const { return value; }

ArgString::operator std::string() const { return value; }

std::string ArgString::parse(const std::string &str) { return str; }

std::string ArgString::toString() const { return value; }

std::string ArgString::toString(const std::string &value) { return value; }

// ============================================================================
// ArgPoint implementation
// ============================================================================

ArgPoint::ArgPoint() : value() {}

ArgPoint::ArgPoint(const Point &p) : value(p) {}

ArgPoint::ArgPoint(int x, int y) : value(x, y) {}

ArgPoint::ArgPoint(const std::string &str) : value(parse(str)) {}

ArgPoint::ArgPoint(const ArgPoint &other) : value(other.value) {}

Point ArgPoint::getValue() const { return value; }

ArgPoint::operator Point() const { return value; }

Point ArgPoint::parse(const std::string &str) {
  // Pattern matches: '(', integer (with optional sign), ',',
  // integer (with optional sign), ')' - NO whitespace allowed
  static const std::regex pattern(R"(\((-?\d+),(-?\d+)\))");
  std::smatch matches;

  if (!std::regex_match(str, matches, pattern)) {
    throw std::invalid_argument("Invalid point format: " + str);
  }

  try {
    int x = std::stoi(matches[1].str());
    int y = std::stoi(matches[2].str());
    return Point(x, y);
  } catch ([[maybe_unused]] const std::exception &e) {
    throw std::invalid_argument("Failed to parse point values: " + str);
  }
}

std::string ArgPoint::toString() const {
  std::ostringstream oss;
  oss << "(" << value.x << "," << value.y << ")";
  return oss.str();
}

std::string ArgPoint::toString(const Point &point) {
  std::ostringstream oss;
  oss << "(" << point.x << "," << point.y << ")";
  return oss.str();
}