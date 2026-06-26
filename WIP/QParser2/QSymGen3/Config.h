//-----------------------------------------------------------------------------
// This file is part of the the QSymGen3 command-line tool contained in the
// QParser2 project.  You can find the complete project here:
// https://github.com/robdunn4/QSpice/
//-----------------------------------------------------------------------------
#pragma once
#include <string_view>

namespace Config {

// input file extension
constexpr std::string_view inputExtension = ".qpins";

// QSpice registry keys for installation directory.
// Full path: HKCU\Software\Marcus Aurelius Software LLC\InstallDir
// The default value under the InstallDir subkey holds the install path.
constexpr std::string_view qspiceRegKey    =
    "Software\\Marcus Aurelius Software LLC";
constexpr std::string_view qspiceRegSubKey = "InstallDir";

// QUX executable name (expected in QSpice install directory)
constexpr std::string_view quxExeName = "QUX.exe";

// DLL source template filename (expected in same directory as QSymGen3.exe)
constexpr std::string_view dllTemplateName = "QMdbSim2_Dll_Template.cpp";

// substitution tokens used in the DLL template
constexpr std::string_view tokenEvalFuncName    = "__EVAL_FUNC_NAME__";
constexpr std::string_view tokenUDataSnippet    = "__UDATA_CODE_SNIPPET__";
constexpr std::string_view tokenQSymGen3Snippet = "__QSymGen3_Code_Snippet__";

} // namespace Config
