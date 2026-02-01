/*
 * StrUtils.h -- Utility methods for string/number conversion.
 */
#pragma once
#include <string>
#include <vector>

namespace StrUtils {

int strToInt(const std::string &str);
long strToLong(const std::string &str);
float strToFloat(const std::string &str);
double strToDouble(const std::string &str);

std::string intToStr(int value);
std::string longToStr(long value);
std::string floatToStr(float value, int precision = 6);
std::string doubleToStr(double value, int precision = 6);

// Trim whitespace helpers
std::string trim(const std::string &str);
std::string trimLeft(const std::string &str);
std::string trimRight(const std::string &str);

// parse into elements
typedef std::vector<std::string> StrList;
StrList tokenize(const std::string &input);

std::string forceQuotes(std::string str);

} // namespace StrUtils