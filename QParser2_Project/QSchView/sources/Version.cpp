#include "Version.h"

#include <format>

namespace Version {
std::string numberStr() {
  return std::format("{}.{}.{}", kVersionMajor, kVersionMinor, kVersionPatch);
}

std::string versionStr() {
  return numberStr() + (kIsDebugBuild ? " (DEBUG)" : "");
}

std::string aboutVersionStr() {
  return numberStr() + (kIsDebugBuild ? " (DEBUG)" : " (RELEASE)");
}

std::string winTitleStr() {
  return std::format("{} v{}", kProgramName, versionStr());
}
}   // namespace Version
