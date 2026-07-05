//-----------------------------------------------------------------------------
// This file is part of the the QParser2 project.  You can find the complete
// project here:  https://github.com/robdunn4/QSpice/
//-----------------------------------------------------------------------------
#include "PinDefs.h"
#include "StrList.h"
#include <format>
#include <sstream>

int PinDefList::parseLines(const StrList strList, std::ostream &errStrm) {
  int lineNbr = 0;

  for (const auto &line : strList) {
    ++lineNbr;

    // Skip blank lines and comments
    if (line.empty() || line.front() == '*') continue;

    // parse line
    std::istringstream ss(line);
    std::string        token;

    if (!(ss >> token)) continue; // blank line
    char tokenChar = toupper(token[0]);

    if (tokenChar == '*') continue; // comment

    if (token.length() > 1) {
      errStrm << format(" *** Malformed token @ line {}: {}\n", lineNbr, line);
      errState = true;
      continue;
    }

    if (tokenChar == 'D') {
      description = line.substr(2);
      continue;
    }

    if (tokenChar == 'P') {
      std::string devName;
      while (ss >> devName) {
        if (devName[0] == '*') break;
        partList.push_back(devName);
      }
      if (!partList.size()) {
        errStrm << format(" *** Invalid part list token @ line {}: {}\n",
                          lineNbr, line);
        errState = true;
      }
      continue;
    }

    if (!PinDef::isValidType(tokenChar)) {
      errStrm << format(" *** Invalid token @ line {}: {}\n", lineNbr, line);
      errState = true;
      continue;
    }

    char        pinType = tokenChar;
    std::string pinName = "---"; // default for 'X' type (skip)

    if (!(ss >> token)) {
      if (pinType != 'X') {
        errStrm << format(" *** Pin token missing name @ line {}: {}\n",
                          lineNbr, line);
        errState = true;
        continue;
      }
    } else pinName = token;

    std::string pinAltText;
    while (ss >> token) {
      if (token[0] == '*') break;
      if (pinAltText.length()) pinAltText += " ";
      pinAltText += token;
    }

    PinDef pinDef(pinType, pinName, pinAltText);
    if (!pinDef.isValid()) {
      errStrm << std::format(
          " *** Unexpected error after parsing pin definition @ line {}\n",
          lineNbr);
      errState = true;
      continue;
    }

    switch (pinType) {
    case 'I':
      inPinCnt++;
      break;
    case 'O':
      outPinCnt++;
      break;
    case 'B':
      biDirPinCnt++;
      break;
    case 'X':
      skipPinCnt++;
      break;
    }
    push_back(pinDef);
  }

  // as a last check, warn if no 'P' record found...
  if (!partList.size()) {
    errStrm << std::format(" *** Warning:  Part list record ('P') not found.  "
                           "Using default part value=\"{}\".\n",
                           partNbr);
    partList.push_back(partNbr);
  }

  // finalize total pin count
  totPinCnt = inPinCnt + outPinCnt + biDirPinCnt + skipPinCnt;

  return errState == true;
}

// check if instance has valid type
bool PinDef::isValidType() const {
  return isValidType(type); // call static version
}

// get type name string
std::string_view PinDef::getTypeName() const {
  return getTypeName(type); // call static version
}

// static methods
bool PinDef::isValidType(char type) {
  // sizeof(typeChars) includes trailing null byte -- exclude
  for (int i = 0; i < sizeof(typeChars) - 1; i++)
    if (type == typeChars[i]) return true;
  return false;
}

// returns -1 if the pin type isn't valid; otherwise, returns an index into
// typeNames[] -- do we need this?
int PinDef::getTypeNdx(char type) {
  // sizeof(typeChars) includes trailing null byte -- exclude
  for (int i = 0; i < sizeof(typeChars) - 1; i++)
    if (type == typeChars[i]) return i;
  return -1;
}

// returns a string name for the pin type if valid; if invalid, returns ""
std::string_view PinDef::getTypeName(char type) {
  int ndx = getTypeNdx(type);
  if (ndx < 0) return "";
  return typeNames[ndx];
}

// returns string from typeNames[] or ""
std::string_view PinDef::getTypeName(int ndx) {
  if (ndx < 0 || ndx >= sizeof(typeNames)) return "";
  return typeNames[ndx];
}

// gets the remaining text up until '*' -- redundant spaces removed
std::string PinDefList::parseRemainder(std::istringstream &ss) {
  std::string remainder;
  std::getline(ss, remainder);

  // Strip leading whitespace
  auto pos  = remainder.find_first_not_of(" \t");
  remainder = (pos != std::string::npos) ? remainder.substr(pos) : "";

  // Truncate at asterisk and strip trailing whitespace
  auto star = remainder.find(" *");
  if (star != std::string::npos) remainder = remainder.substr(0, star);

  auto end = remainder.find_last_not_of(" \t");
  return (end != std::string::npos) ? remainder.substr(0, end + 1) : "";
}
