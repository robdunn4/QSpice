//-----------------------------------------------------------------------------
// This file is part of the the QParser2 project.  You can find the complete
// project here:  https://github.com/robdunn4/QSpice/
//-----------------------------------------------------------------------------
#pragma once

#include <istream>
#include <memory>
#include <string>
#include <vector>

enum class PinItemType
{
  Input,
  Output,
  Gpio
};

class PinItem {
public:
  PinItem() = default;
  PinItem(const std::string &name, PinItemType type,
          const std::string &netName);

  PinItem(const PinItem &)                = default;
  PinItem(PinItem &&) noexcept            = default;
  PinItem &operator=(const PinItem &)     = default;
  PinItem &operator=(PinItem &&) noexcept = default;
  ~PinItem()                              = default;

  static std::shared_ptr<PinItem>
  create(const std::string &name, PinItemType type, const std::string &netName);

  PinItemType        getType() const;
  const std::string &getName() const;
  const std::string &getNetName() const;

  void setType(PinItemType type);
  void setName(const std::string &name);
  void setNetName(const std::string &netName);

  bool operator==(const PinItem &other) const;
  bool operator!=(const PinItem &other) const;

protected:
  // Members ordered to match constructor parameter order (name, type).
  std::string name;
  PinItemType type{PinItemType::Input};
  std::string netName;
};

using PinItemPtr = std::shared_ptr<PinItem>;

class PinItemList {
public:
  using pinItems = std::vector<PinItemPtr>;

  virtual ~PinItemList() = default;

  int add(const PinItemPtr &item);

  const PinItemPtr &operator[](std::size_t index) const;
  PinItemPtr       &operator[](std::size_t index);

  std::size_t size() const;
  bool        empty() const;

  // Parse pin definitions from a text stream.
  // Throws std::runtime_error on any parse error.
  void parseStream(std::istream      &stream,
                   const std::string &sourceName = "<stream>");

  // Parse pin definitions from a file at the given path.
  // Throws std::runtime_error if the file cannot be opened or on any parse
  // error.
  void parseFile(const std::string &path);

  pinItems::iterator       begin();
  pinItems::iterator       end();
  pinItems::const_iterator begin() const;
  pinItems::const_iterator end() const;
  pinItems::const_iterator cbegin() const;
  pinItems::const_iterator cend() const;

protected:
  pinItems items;
};
