//-----------------------------------------------------------------------------
// This file is part of the the QSymGen3 command-line tool contained in the
// QParser2 project.  You can find the complete project here:
// https://github.com/robdunn4/QSpice/
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

    if (tokenChar == 'R') {
      std::string valStr;
      if (!(ss >> valStr)) {
        errStrm << std::format(
            " *** Warning:  R record missing value @ line {}; using default "
            "ROUT={}.\n",
            lineNbr, rout);
      } else {
        try {
          rout = std::stoi(valStr);
        } catch (const std::exception &) {
          errStrm << std::format(
              " *** Warning:  R record invalid value \"{}\" @ line {}; using "
              "default ROUT={}.\n",
              valStr, lineNbr, rout);
        }
      }
      continue;
    }

    if (!PinDef::isValidType(tokenChar)) {
      errStrm << format(" *** Invalid token @ line {}: {}\n", lineNbr, line);
      errState = true;
      continue;
    }

    // X supports an optional integer repeat count: "X" alone means 1 blank
    // position (unchanged); "X i" expands to i consecutive X records. This
    // is pure sugar -- each expanded record is a completely ordinary X
    // entry, indistinguishable from i separate "X" lines, so nothing
    // downstream needs to know this shorthand exists.
    if (tokenChar == 'X') {
      int         count = 1;
      std::string countStr;
      if (ss >> countStr && countStr[0] != '*') {
        try {
          int parsed = std::stoi(countStr);
          if (parsed >= 1) {
            count = parsed;
          } else {
            errStrm << std::format(
                " *** Warning:  X record invalid count \"{}\" @ line {}; "
                "using default count=1.\n",
                countStr, lineNbr);
          }
        } catch (const std::exception &) {
          errStrm << std::format(
              " *** Warning:  X record invalid count \"{}\" @ line {}; "
              "using default count=1.\n",
              countStr, lineNbr);
        }
      }

      for (int i = 0; i < count; ++i) {
        PinDef pinDef('X', "---");
        skipPinCnt++;
        push_back(pinDef);
      }
      continue;
    }

    char        pinType = tokenChar;
    std::string pinName;

    if (!(ss >> token)) {
      errStrm << format(" *** Pin token missing name @ line {}: {}\n",
                        lineNbr, line);
      errState = true;
      continue;
    }
    pinName = token;

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
    case 'V':
      vddPinCnt++;
      vddPinName = pinName; // last V pin wins; warned below if >1
      break;
    case 'G':
      gndPinCnt++;
      gndPinName = pinName; // last G pin wins; warned below if >1
      break;
    case 'K':
      clockPinCnt++;
      clockPinName = pinName; // last K pin wins; warned below if >1
      break;
    }
    push_back(pinDef);
  }

  // warn if VDD, GND, or Clock pin counts are not exactly 1
  bool vgcErr = false;

  if (vddPinCnt != 1) {
    if (vddPinCnt == 0) errStrm << " *** Warning:  No VDD pin ('V') found.\n";
    else
      errStrm << std::format(
          " *** Warning:  {} VDD pins ('V') found; exactly 1 expected.\n",
          vddPinCnt);
    vgcErr = true;
  }

  if (gndPinCnt != 1) {
    if (gndPinCnt == 0) errStrm << " *** Warning:  No GND pin ('G') found.\n";
    else
      errStrm << std::format(
          " *** Warning:  {} GND pins ('G') found; exactly 1 expected.\n",
          gndPinCnt);
    vgcErr = true;
  }

  if (clockPinCnt != 1) {
    if (clockPinCnt == 0)
      errStrm << " *** Warning:  No SimClock pin ('K') found.\n";
    else
      errStrm << std::format(
          " *** Warning:  {} SimClock pins ('K') found; exactly 1 expected.\n",
          clockPinCnt);
    vgcErr = true;
  }

  if (vgcErr) {
    std::cerr << "Continue anyway (not recommended)? (y/N): ";
    std::string answer;
    std::getline(std::cin, answer);
    if (answer.empty() ||
        std::tolower(static_cast<unsigned char>(answer[0])) != 'y') {
      errState = true;
      return 1;
    }
  }

  // as a last check, warn if no 'P' record found...
  if (!partList.size()) {
    errStrm << std::format(" *** Warning:  Part list record ('P') not found.  "
                           "Using default part value=\"{}\".\n",
                           partNbr);
    partList.push_back(partNbr);
  }

  // finalize total pin count
  totPinCnt =
      inPinCnt + outPinCnt + biDirPinCnt + vddPinCnt + gndPinCnt + clockPinCnt;

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
