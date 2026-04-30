//-----------------------------------------------------------------------------
// This file is part of the the QParser2 project.  You can find the complete
// project here:  https://github.com/robdunn4/QSpice/
//-----------------------------------------------------------------------------
#include "ArgUtils2.h"
#include <stdexcept>
#include <vector>

// ============================================================================
// ArgRotAlign implementation
// ============================================================================

unsigned char ArgRotAlign::getAlignV() const { return value & 0b0011; }

unsigned char ArgRotAlign::getAlignH() const { return value & 0b1100; }

unsigned char ArgRotAlign::getAlign() const { return value & 0b1111; }

int ArgRotAlign::getRotCode() const { return (value & 0xF0) >> 4; }

int ArgRotAlign::getRotDeg() const { return getRotCode() * 45; }

void ArgRotAlign::setAlignV(unsigned char vert) {
  value = (value & ~0b0011) | vert;
}

void ArgRotAlign::setAlignH(unsigned char horiz) {
  value = (value & ~0b1100) | horiz;
}

void ArgRotAlign::setAlign(unsigned char align) {
  value = (value & ~0b1111) | align;
}

void ArgRotAlign::setRotCode(int code) {
  if (code < 0 || code > 15) {
    throw std::invalid_argument("Rotation code must be 0-15");
  }
  value = (value & 0x0F) | (code << 4);
}

void ArgRotAlign::setRotDeg(int deg) {
  if (deg % 45 != 0) {
    throw std::invalid_argument("Rotation must be multiple of 45 degrees");
  }
  int code = (deg / 45) % 16; // Normalize to 0-15
  setRotCode(code);
}

ArgRotAlign ArgRotAlign::create(unsigned char vert, unsigned char horiz,
                                int rotationCode) {
  if (rotationCode < 0 || rotationCode > 15) {
    throw std::invalid_argument("Rotation code must be 0-15");
  }
  int val = vert | horiz | (rotationCode << 4);
  return ArgRotAlign(val);
}

// ============================================================================
// ArgLibString implementation
// ============================================================================

// TODO: revisit -- should this fail if not '|' lib entry?
std::vector<std::string> ArgLibString::splitSubcktStrs() const {
  std::vector<std::string> result;
  std::string              current;

  auto iter = value.cbegin();
  if (*iter == '|') iter++; // this isn't really right

  for (; iter != value.cend(); ++iter) {
    if (*iter == '\\' && std::next(iter) != value.cend() &&
        *std::next(iter) == 'n') {
      result.push_back(current);
      current.clear();
      ++iter; // skip 'n'
    } else {
      current += *iter;
    }
  }

  result.push_back(current);
  return result;
}

void ArgLibString::mergeSubcktStrs(const std::vector<std::string> &strList) {
  value = "|";
  for (size_t i = 0; i < strList.size(); ++i) {
    if (i > 0) value += "\\n";
    value += strList[i];
  }
}
