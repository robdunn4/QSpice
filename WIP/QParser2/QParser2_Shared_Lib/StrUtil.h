//-----------------------------------------------------------------------------
// This file is part of the the QParser2 project.  You can find the complete
// project here:  https://github.com/robdunn4/QSpice/
//-----------------------------------------------------------------------------
/*
 * StrUtil.h -- Utility methods for string/number conversion.
 */
#pragma once
#include "StrList.h"
#include <istream>
#include <ostream>

namespace StrUtil {

int    strToInt(const std::string &str);
long   strToLong(const std::string &str);
float  strToFloat(const std::string &str);
double strToDouble(const std::string &str);

std::string intToStr(int value);
std::string longToStr(long value);
std::string floatToStr(float value, int precision = 6);
std::string doubleToStr(double value, int precision = 6);

// Trim whitespace helpers
std::string trim(const std::string &str);
std::string trimLeft(const std::string &str);
std::string trimRight(const std::string &str);

// parse a string into space-delimited elements with possible quotes
StrList tokenize(const std::string &input);

std::string forceQuotes(std::string str);

// Split a string with embedded \n or \r\n into a StrList
StrList fromString(const std::string &input);

// Read lines from an istream into a StrList
StrList fromStream(std::istream &input);

// Join a StrList into a single string with a given delimiter (default: \n)
std::string toString(const StrList &lines, const std::string &delimiter = "\n");

// Write a StrList to an ostream
void toStream(const StrList &lines, std::ostream &output,
              const std::string &delimiter = "\n");

} // namespace StrUtil
