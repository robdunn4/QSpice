#include "Version.h"

#include <format>

namespace Version {
std::string versionStr() {
  return std::format("{}.{}.{}{}", kVersionMajor, kVersionMinor, kVersionPatch,
                     kIsDebugBuild ? " (DEBUG)" : "");
}

std::string winTitleStr() {
  return std::format("{} v{}", kProgramName, versionStr());
}
} // namespace Version
