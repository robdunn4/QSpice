//-----------------------------------------------------------------------------
// This file is part of the the QParser2 project.  You can find the complete
// project here:  https://github.com/robdunn4/QSpice/
//-----------------------------------------------------------------------------
#include "StrUtils.h"
#include <iostream>
#include <sstream>

namespace StrUtils {

int strToInt(const std::string &str) {
  try {
    std::string trimmed = trim(str);
    if (trimmed.empty()) {
      throw std::invalid_argument("Empty string");
    }
    return std::stoi(trimmed);
  } catch (const std::exception &e) {
    throw std::runtime_error("Cannot convert '" + str +
                             "' to int: " + e.what());
  }
}

long strToLong(const std::string &str) {
  try {
    std::string trimmed = trim(str);
    if (trimmed.empty()) {
      throw std::invalid_argument("Empty string");
    }
    return std::stol(trimmed);
  } catch (const std::exception &e) {
    throw std::runtime_error("Cannot convert '" + str +
                             "' to long: " + e.what());
  }
}

float strToFloat(const std::string &str) {
  try {
    std::string trimmed = trim(str);
    if (trimmed.empty()) {
      throw std::invalid_argument("Empty string");
    }
    return std::stof(trimmed);
  } catch (const std::exception &e) {
    throw std::runtime_error("Cannot convert '" + str +
                             "' to float: " + e.what());
  }
}

double strToDouble(const std::string &str) {
  try {
    std::string trimmed = trim(str);
    if (trimmed.empty()) {
      throw std::invalid_argument("Empty string");
    }
    return std::stod(trimmed);
  } catch (const std::exception &e) {
    throw std::runtime_error("Cannot convert '" + str +
                             "' to double: " + e.what());
  }
}

std::string intToStr(int value) { return std::to_string(value); }

std::string longToStr(long value) { return std::to_string(value); }

std::string floatToStr(float value, int precision) {
  std::ostringstream oss;
  oss.precision(precision);
  oss << std::fixed << value;
  return oss.str();
}

std::string doubleToStr(double value, int precision) {
  std::ostringstream oss;
  oss.precision(precision);
  oss << std::fixed << value;
  return oss.str();
}

std::string trim(const std::string &str) { return trimLeft(trimRight(str)); }

std::string trimLeft(const std::string &str) {
  size_t start = str.find_first_not_of(" \t\n\r\f\v");
  return (start == std::string::npos) ? "" : str.substr(start);
}

std::string trimRight(const std::string &str) {
  size_t end = str.find_last_not_of(" \t\n\r\f\v");
  return (end == std::string::npos) ? "" : str.substr(0, end + 1);
}

// tokenize() -- parses a string into individual strings.
//
// Note:  This process is fragile.  It is possible for a GUI user to insert
// double-quotes into some fields which may break QSpice.  This code probably
// works no better...
//
StrList tokenize(const std::string &input) {
  std::istringstream iss(input);
  std::string curToken;
  StrList tokens;

  while (iss.good()) {
    iss >> curToken;
    if (!curToken.length()) continue;
    if (curToken[0] == '"') {
      std::string remainder;
      getline(iss, remainder);
      curToken += remainder;
    }
    tokens.push_back(curToken);
  }

  return tokens;
}

std::string forceQuotes(std::string str) {
  if (!str.length()) return "\"\"";
  if (*str.begin() != '\"' || *(str.end() - 1) != '\"')
    return "\"" + str + "\"";
  return str;
}

} // namespace StrUtils