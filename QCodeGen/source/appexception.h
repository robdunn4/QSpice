#ifndef APPEXCEPTION_H
#define APPEXCEPTION_H

#include "strings.h"
#include <stdexcept>

class FileError : public std::runtime_error {
public:
  FileError(const String &what_arg) : std::runtime_error(what_arg){};
  FileError(const char *what_arg) : std::runtime_error(what_arg){};
};

class ParseError : public std::runtime_error {
public:
  ParseError(const String &what_arg) : std::runtime_error(what_arg){};
  ParseError(const char *what_arg) : std::runtime_error(what_arg){};
};

#endif // APPEXCEPTION_H
