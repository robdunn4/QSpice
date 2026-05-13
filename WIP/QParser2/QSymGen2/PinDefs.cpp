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

    //  // --- Parse line fields ---
    std::istringstream ss(line);
    std::string        token;

    if (!(ss >> token)) continue; // blank line
    char tokenChar = toupper(token[0]);

    if (tokenChar == '*') continue; // comment
    if (token.length() > 1) {
      errStrm << format(" *** Malformed token @ line {}: {}\n", lineNbr, line);
      continue;
    }

    // handle manufacturer/part number/etc. here
    if (tokenChar == 'M') {
      manufacturer = line.substr(2);
      continue;
    }
    if (tokenChar == 'P') {
      ss >> token;
      if (!token.length()) {
        errStrm << format(" *** Malformed token @ line {}: {}\n", lineNbr,
                          line);
        continue;
      }
      partNbr = token;
      // TODO: Revisit
      altPartNbrs = parseRemainder(ss);
      continue;
    }
    if (tokenChar == 'D') {
      description = line.substr(2);
      continue;
    }

    if (!PinDef::isValidType(tokenChar)) {
      errStrm << format(" *** Invalid token @ line {}: {}\n", lineNbr, line);
      continue;
    }
    char pinType = tokenChar;

    if (!(ss >> token)) {
      errStrm << format(" *** Pin token missing name @ line {}: {}\n", lineNbr,
                        line);
      continue;
    }
    std::string pinName = token;

    std::string pinAltText;
    while (ss >> token) {
      if (token[0] == '*') break;
      pinAltText += token;
    }

    PinDef pinDef(pinType, pinName, pinAltText);
    if (!pinDef.isValid()) {
      errStrm << std::format(
          " *** Unexpected error after parsing pin definition @ line {}\n",
          lineNbr);
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

  totPinCnt = inPinCnt + outPinCnt + biDirPinCnt + skipPinCnt;
  return 0;
}

// check if instance has valid type
bool PinDef::isValidType() const {
  return isValidType(type); // call static version
}

// get instance type index
int PinDef::getTypeNdx() const {
  return getTypeNdx(type); // call static version
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
// typeNames[]
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
