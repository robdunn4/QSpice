#include "QArgUtils2.h"
#include <stdexcept>

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