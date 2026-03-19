//-----------------------------------------------------------------------------
// This file is part of the the QParser2 project.  You can find the complete
// project here:  https://github.com/robdunn4/QSpice/
//-----------------------------------------------------------------------------
/*
 * EnumStrMap.h -- Template for enum/string bi-directional lookups.
 */
#pragma once
#include <initializer_list>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <utility>

template <typename T>
concept EnumType = std::is_enum_v<T>;

template <EnumType EnumID, typename EnumStr = std::string> class EnumMapper {
private:
  std::unordered_map<EnumStr, EnumID> EnumStrToID;
  std::unordered_map<EnumID, EnumStr> EnumIDToStr;

public:
  EnumMapper() = default;

  EnumMapper(std::initializer_list<std::pair<EnumID, EnumStr>> mappings) {
    addMappings(mappings);
  }

  void addMapping(EnumID enumID, const EnumStr &enumStr) {
    EnumStrToID[enumStr] = enumID;
    EnumIDToStr[enumID]  = enumStr;
  }

  void addMappings(std::initializer_list<std::pair<EnumID, EnumStr>> mappings) {
    for (const auto &[enumID, enumStr] : mappings) {
      addMapping(enumID, enumStr);
    }
  }

  EnumID strToEnum(const EnumStr &enumStr) const {
    auto it = EnumStrToID.find(enumStr);
    if (it == EnumStrToID.end()) {
      throw std::invalid_argument("Unknown EnumStr: " + enumStr);
    }
    return it->second;
  }

  EnumStr enumToStr(EnumID enumID) const {
    auto it = EnumIDToStr.find(enumID);
    if (it == EnumIDToStr.end()) {
      throw std::invalid_argument("Unknown EnumID enum value: " +
                                  std::to_string(static_cast<int>(enumID)));
    }
    return it->second;
  }
};
