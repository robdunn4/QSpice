#ifndef APPEXCEPTION_H
#define APPEXCEPTION_H

#include <stdexcept>
#include <string>

class FileError : public std::runtime_error {
public:
  FileError(const std::string &what_arg) : std::runtime_error(what_arg){};
  FileError(const char *what_arg) : std::runtime_error(what_arg){};
};

class ParseError : public std::runtime_error {
public:
  ParseError(const std::string &what_arg) : std::runtime_error(what_arg){};
  ParseError(const char *what_arg) : std::runtime_error(what_arg){};

  // protected:
  //   const char *lineText_arg;
};

#endif // APPEXCEPTION_H
