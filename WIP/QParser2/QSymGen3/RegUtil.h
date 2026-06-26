//-----------------------------------------------------------------------------
// This file is part of the the QSymGen3 command-line tool contained in the
// QParser2 project.  You can find the complete project here:
// https://github.com/robdunn4/QSpice/
//-----------------------------------------------------------------------------
#pragma once
#include <string>

namespace RegUtil {

// Reads the QSpice installation directory from:
//   HKEY_CURRENT_USER\Software\Marcus Aurelius Software LLC\InstallDir
// Returns the path string on success, or an empty string on any failure.
std::string getQSpiceInstallDir();

} // namespace RegUtil
