//-----------------------------------------------------------------------------
// This file is part of the the QParser2 project.  You can find the complete
// project here:  https://github.com/robdunn4/QSpice/
//-----------------------------------------------------------------------------

#include "PinItems.h"
#include <cctype>
#include <fstream>
#include <sstream>
#include <stdexcept>

//----------------------------------------
// PinItem
//----------------------------------------
PinItem::PinItem(const std::string &name, PinItemType type,
                 const std::string &netName)
    : name(name), type(type), netName(netName) {}

std::shared_ptr<PinItem> PinItem::create(const std::string &name,
                                         PinItemType        type,
                                         const std::string &netName) {
  return std::make_shared<PinItem>(name, type, netName);
}

PinItemType PinItem::getType() const { return type; }

const std::string &PinItem::getName() const { return name; }

const std::string &PinItem::getNetName() const { return netName; }

void PinItem::setType(PinItemType type) { this->type = type; }

void PinItem::setName(const std::string &name) { this->name = name; }

void PinItem::setNetName(const std::string &netName) {}

bool PinItem::operator==(const PinItem &other) const {
  return type == other.type && name == other.name;
}

bool PinItem::operator!=(const PinItem &other) const {
  return !(*this == other);
}

//----------------------------------------
// PinItemList
//----------------------------------------
int PinItemList::add(const PinItemPtr &item) {
  if (!item) return -1; // null pointer
  items.push_back(item);
  return 0; // success
}

const PinItemPtr &PinItemList::operator[](std::size_t index) const {
  return items.at(index);
}

PinItemPtr &PinItemList::operator[](std::size_t index) {
  return items.at(index);
}

std::size_t PinItemList::size() const { return items.size(); }

bool PinItemList::empty() const { return items.empty(); }

PinItemList::pinItems::iterator PinItemList::begin() { return items.begin(); }

PinItemList::pinItems::iterator PinItemList::end() { return items.end(); }

PinItemList::pinItems::const_iterator PinItemList::begin() const {
  return items.begin();
}

PinItemList::pinItems::const_iterator PinItemList::end() const {
  return items.end();
}

PinItemList::pinItems::const_iterator PinItemList::cbegin() const {
  return items.cbegin();
}

PinItemList::pinItems::const_iterator PinItemList::cend() const {
  return items.cend();
}

//----------------------------------------
// PinItemList parsing
//----------------------------------------

void PinItemList::parseStream(std::istream      &stream,
                              const std::string &sourceName) {
  std::string line;
  int         lineNum = 0;

  while (std::getline(stream, line)) {
    ++lineNum;

    // Strip trailing carriage return (for Windows-style line endings).
    if (!line.empty() && line.back() == '\r') line.pop_back();

    // Skip empty lines and comment lines.
    if (line.empty() || line[0] == '*') continue;

    std::istringstream iss(line);

    // First token: pin type indicator (single character expected).
    std::string typeToken;
    if (!(iss >> typeToken)) continue; // blank after trim — shouldn't happen

    if (typeToken.size() != 1) {
      throw std::runtime_error(sourceName + ":" + std::to_string(lineNum) +
                               ": pin type must be a single character (I/O/G),"
                               " got \"" +
                               typeToken + "\"");
    }

    PinItemType pinType;
    switch (std::toupper(static_cast<unsigned char>(typeToken[0]))) {
    case 'I':
    case 'i':
      pinType = PinItemType::Input;
      break;
    case 'O':
    case 'o':
      pinType = PinItemType::Output;
      break;
    case 'B': // TODO:  Implement bi-directional GPIO pin type
    case 'b':
      throw std::runtime_error(
          sourceName + ":" + std::to_string(lineNum) +
          ": Bi-directionalGPIO pin type not yet implemented");
      pinType = PinItemType::Gpio;
      break;
    default:
      throw std::runtime_error(sourceName + ":" + std::to_string(lineNum) +
                               ": unknown pin type '" + typeToken[0] + "'");
    }

    // Second token: pin name.
    std::string pinName;
    if (!(iss >> pinName)) {
      throw std::runtime_error(sourceName + ":" + std::to_string(lineNum) +
                               ": missing pin name after type indicator");
    }

    // Third toke: net name (optional)
    std::string netName_;
    if (!(iss >> netName_) || netName_[0] == '*') netName_ = "";

    int result = add(PinItem::create(pinName, pinType, netName_));
    if (result != 0)
      throw std::runtime_error(sourceName + ":" + std::to_string(lineNum) +
                               ": failed to add pin \"" + pinName +
                               "\" (error " + std::to_string(result) + ")");
  }

  if (stream.bad())
    throw std::runtime_error(sourceName + ": I/O error while reading");
}

void PinItemList::parseFile(const std::string &path) {
  std::ifstream file(path);
  if (!file.is_open())
    throw std::runtime_error("Cannot open pin list file: \"" + path + "\"");
  parseStream(file, path);
}
