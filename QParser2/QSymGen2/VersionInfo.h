//-----------------------------------------------------------------------------
// This file is part of the the QParser2 project.  You can find the complete
// project here:  https://github.com/robdunn4/QSpice/
//-----------------------------------------------------------------------------
#pragma once
#include <format>
#include <string_view>

namespace VersionInfo {
constexpr std::string_view progName    = "QSymGen2";
constexpr std::string_view compileDate = __DATE__ " " __TIME__;
constexpr std::string_view version     = "0.3.0";

#if defined(__DEBUG__)
constexpr std::string_view buildType = "Debug";
#else
constexpr std::string_view buildType = "Release";
#endif

inline const std::string versionInfo = std::format(
    "{} v{} [{} Build, {}]", progName, version, buildType, compileDate);

} // namespace VersionInfo