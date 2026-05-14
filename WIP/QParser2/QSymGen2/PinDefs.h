//-----------------------------------------------------------------------------
// This file is part of the the QParser2 project.  You can find the complete
// project here:  https://github.com/robdunn4/QSpice/
//-----------------------------------------------------------------------------
#pragma once
#include "NullStream.h"
#include "StrList.h"
#include <iostream>

// individual Pin Definitions
class PinDef {
public:
  // default constructor produces an invalid state; must set values after
  // construction
  PinDef() {};
  // this constructor may produce an invalid state -- check with isValid() after
  // construction if in doubt
  PinDef(char type, std::string name, std::string altText = "")
      : type(type), name(name), altText(altText) {}

  // class state is invalid until values set
  bool isValid() const { return isValidType(type) && name.length(); }

  // set class values; returns false if invalid state
  bool setValues(char type, std::string name, std::string altText = "") {
    this->type    = type;
    this->name    = name;
    this->altText = altText;

    return isValid();
  };

  // check if instance has valid type
  bool isValidType() const;

  // get instance type index
  int getTypeNdx() const;

  // get type name string
  std::string_view getTypeName() const;

  // static methods
  static bool isValidType(char type);

  // returns -1 if the pin type isn't valid; otherwise, returns an index into
  // typeNames[]
  static int getTypeNdx(char type);

  // returns a string name for the pin type if valid; if invalid, returns ""
  static std::string_view getTypeName(char type);

  // returns string from typeNames[] or ""
  static std::string_view getTypeName(int ndx);

public:
  char        type{'\x0'}; // not valid
  std::string name{""};    // empty, not valid
  std::string altText{""}; // empty, valid

protected:
  static constexpr char             typeChars[]{"IOBX"};
  static constexpr std::string_view typeNames[]{"Input", "Output", "BiDir",
                                                "Skip"};
};

// parser
class PinDefList : public std::vector<PinDef> {
public:
  PinDefList() {};

  int parseLines(const StrList strList, std::ostream &errStrm = nullStream);

public:
  // std::string manufacturer{"[Manufacturer]"};
  std::string partNbr{"[PartNbr]"};
  // std::string altPartNbrs{""}; // now expecting this info in description
  // record
  std::string description{"Symbol generated with QSymGen2"};
  int         biDirPinCnt = 0;
  int         inPinCnt    = 0;
  int         outPinCnt   = 0;
  int         skipPinCnt  = 0;
  int         totPinCnt   = 0;

protected:
  std::string parseRemainder(std::istringstream &ss);
};
