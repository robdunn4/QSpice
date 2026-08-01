//-----------------------------------------------------------------------------
// This file is part of the the QSymGen3 command-line tool contained in the
// QParser2 project.  You can find the complete project here:
// https://github.com/robdunn4/QSpice/
//-----------------------------------------------------------------------------
#pragma once
#include <format>
#include <string_view>

namespace VersionInfo {
	constexpr std::string_view progName = "QSymGen3";
	constexpr std::string_view compileDate = __DATE__ " " __TIME__;
	constexpr std::string_view version = "0.7.13";

#if defined(_DEBUG)
	constexpr std::string_view buildType = "DEBUG";
#else
	constexpr std::string_view buildType = "RELEASE";
#endif

	inline const std::string versionInfo = std::format(
		"{} v{} [{} Build, {}]", progName, version, buildType, compileDate);

} // namespace VersionInfo