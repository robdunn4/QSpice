//-----------------------------------------------------------------------------
// This file is part of the the QSymGen3 command-line tool contained in the
// QParser2 project.  You can find the complete project here:
// https://github.com/robdunn4/QSpice/
//-----------------------------------------------------------------------------
#pragma once
#include <ostream>
#include <streambuf>

class NullBuf : public std::streambuf {
protected:
  int overflow(int c) override { return c; } // discard everything
};

class NullStream : public std::ostream {
public:
  NullStream() : std::ostream(&nullBuf) {}

private:
  NullBuf nullBuf;
};

// C++17 inline variable — one instance, no ODR issues
inline NullStream nullStream;