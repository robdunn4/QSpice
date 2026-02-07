/*
 * QColorPrefs.cpp -- A command-line utility to save/restore QSpice color
 *                    settings.
 *
 * The complete source code, documentation, and MSVS project files for the
 * current official version of this project is available at:
 *
 *   https://github.com/robdunn4/QSpice/
 *
 */
#include <algorithm>
#include <fstream>
#include <iostream>
#include <map>
#include <string>
#include <vector>
#include <windows.h>

const char *ProgName = "QColorPrefs.exe";
const char *VersionID = "v1.1   [" __TIMESTAMP__ "]";

const char *WAVSEC = "waveform";
const char *SCHSEC = "schematic";

// Base registry path for all values
const char *REGISTRY_BASE_PATH =
    "Software\\Marcus Aurelius Software LLC\\QSPICE\\[Preferences]";

// Structure to hold registry value configuration
struct RegistryValue {
  std::string section;
  std::string valueName;
};

// Registry values to be saved/restored
std::vector<RegistryValue> g_registryValues = {

    // Schematic-related values
    {SCHSEC, "CADHighlightColor"},     // Highlight Color
    {SCHSEC, "CADBackgroundColor"},    // Background
    {SCHSEC, "CADbomberSight"},        // Schematic Grid Color?
    {SCHSEC, "CADgrapicAnnotation"},   // Graphic Annotation Color
    {SCHSEC, "CADsolidAnnotation"},    // Solid Annotation Fill Color
    {SCHSEC, "CADgrapicText"},         // Symbol Graphic Text Color
    {SCHSEC, "CADnormalText"},         // Component Text Color
    {SCHSEC, "CADnormalSchText"},      // Schematic Text Color
    {SCHSEC, "CADcommentText"},        // Comment Text Color
    {SCHSEC, "CADnetLabelText"},       // Net Label Color
    {SCHSEC, "CADwireColor"},          // Wire Color
    {SCHSEC, "CADhighlightWireColor"}, // Highlight Wire Color
    {SCHSEC, "CADpinDotColor"},        // Pin Dot Color
    {SCHSEC, "CADspritedAnchor"},      // Spirited Anchor Color
    {SCHSEC, "CADpinColor"},           // Pin Color Color [sic]
    {SCHSEC, "CADportBkColor"},        // Port Background Color
    {SCHSEC, "CADhierarchalBlock"},    // Hierarchical Box Color
    {SCHSEC, "CADhierarchalFill"},     // Hierarchical Fill Color
    {SCHSEC, "CADunconnectedPin"},     // Unconnected Pin Color
    {SCHSEC, "CADbusTapError"},        // Incorrect Bus Tap Color

    // Waveform-related values
    {WAVSEC, "NumberDataTraceColors"},
    {WAVSEC, "CursorBackFore"},           // Attached Cursor Text Color
    {WAVSEC, "CursorBackGnd"},            // Attached Cursor Background
    {WAVSEC, "CursorLineColor"},          // Cursor Color
    {WAVSEC, "FrameColor"},               // Ticks & Axis Color
    {WAVSEC, "BackgroundColor"},          // Background Color
    {WAVSEC, "WaveFormViewerFontPoints"}, // not a color but relevant
    {WAVSEC, "WaveformPlotLineWidth"},    // not a color but relevant
    {WAVSEC, "DataColor1"},
    {WAVSEC, "DataColor2"},
    {WAVSEC, "DataColor3"},
    {WAVSEC, "DataColor4"},
    {WAVSEC, "DataColor5"},
    {WAVSEC, "DataColor6"},
    {WAVSEC, "DataColor7"},
    {WAVSEC, "DataColor8"},
    {WAVSEC, "DataColor9"},
    {WAVSEC, "DataColor10"},
    {WAVSEC, "DataColor11"},
    {WAVSEC, "DataColor12"},
    {WAVSEC, "DataColor13"},
    {WAVSEC, "DataColor14"},
    {WAVSEC, "DataColor15"},
    {WAVSEC, "DataColor16"},
    {WAVSEC, "DataColor17"},
    {WAVSEC, "DataColor18"},
    {WAVSEC, "DataColor19"},
    {WAVSEC, "DataColor20"},
    {WAVSEC, "DataColor21"},
    {WAVSEC, "DataColor22"},
    {WAVSEC, "DataColor23"},
    {WAVSEC, "DataColor24"}

};

void ShowUsage() {
  std::cout << "Usage: " << ProgName << " <option> <filepath>\n\n";
  std::cout << "Options:\n";
  std::cout << "  -s         Save QSpice registry color settings to file\n";
  std::cout
      << "  -rall      Restore all QSpice registry color settings from file\n";
  std::cout << "  -rwav      Restore only waveform color settings from file\n";
  std::cout
      << "  -rsch      Restore only schematic color settings from file\n\n";
  std::cout << "Example:\n";
  std::cout << "  " << ProgName << " -s mycolors\n";
  std::cout << "  " << ProgName << " -rall mycolors.qcolorpref\n";
  std::cout << "  " << ProgName << " -rwav mycolors.qcolorpref\n";
  std::cout << "  " << ProgName << " -rsch mycolors.qcolorpref\n\n";
  std::cout << "Note: If no extension is provided, .qcolorpref will be added "
               "automatically.\n";
}

std::string NormalizeFilePath(const std::string &filepath) {
  std::string normalized = filepath;

  // Check if file has an extension
  size_t lastDot = normalized.find_last_of('.');
  size_t lastSlash = normalized.find_last_of("\\/");

  // If no dot found, or dot is before last slash (part of directory name)
  if (lastDot == std::string::npos ||
      (lastSlash != std::string::npos && lastDot < lastSlash)) {
    normalized += ".qcolorpref";
  }

  return normalized;
}

std::string ToLower(const std::string &str) {
  std::string lower = str;
  std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);
  return lower;
}

bool ReadRegistryValue(HKEY hive, const std::string &subkey,
                       const std::string &valueName, std::string &value) {
  HKEY hKey;
  LONG result = RegOpenKeyExA(hive, subkey.c_str(), 0, KEY_READ, &hKey);

  if (result != ERROR_SUCCESS) {
    std::cerr << "Error: Could not open registry key: " << subkey << std::endl;
    return false;
  }

  // First, get the size of the string
  DWORD dataSize = 0;
  DWORD type;

  result =
      RegQueryValueExA(hKey, valueName.c_str(), NULL, &type, NULL, &dataSize);

  if (result != ERROR_SUCCESS) {
    RegCloseKey(hKey);
    std::cerr << "Error: Could not read registry value: " << valueName << " in "
              << subkey << std::endl;
    return false;
  }

  if (type != REG_SZ) {
    RegCloseKey(hKey);
    std::cerr << "Error: Registry value is not a string (REG_SZ): " << valueName
              << std::endl;
    return false;
  }

  // Allocate buffer and read the string
  std::vector<char> buffer(dataSize);
  result = RegQueryValueExA(hKey, valueName.c_str(), NULL, &type,
                            reinterpret_cast<LPBYTE>(buffer.data()), &dataSize);

  RegCloseKey(hKey);

  if (result != ERROR_SUCCESS) {
    std::cerr << "Error: Could not read registry value: " << valueName << " in "
              << subkey << std::endl;
    return false;
  }

  value = std::string(buffer.data());
  return true;
}

bool WriteRegistryValue(HKEY hive, const std::string &subkey,
                        const std::string &valueName,
                        const std::string &value) {
  HKEY hKey;
  LONG result = RegOpenKeyExA(hive, subkey.c_str(), 0, KEY_WRITE, &hKey);

  if (result != ERROR_SUCCESS) {
    std::cerr << "Error: Could not open registry key for writing: " << subkey
              << std::endl;
    return false;
  }

  result = RegSetValueExA(hKey, valueName.c_str(), 0, REG_SZ,
                          reinterpret_cast<const BYTE *>(value.c_str()),
                          static_cast<DWORD>(value.length() + 1));

  RegCloseKey(hKey);

  if (result != ERROR_SUCCESS) {
    std::cerr << "Error: Could not write registry value: " << valueName
              << " in " << subkey << std::endl;
    return false;
  }

  return true;
}

bool SavePreferences(const std::string &filepath) {
  if (g_registryValues.empty()) {
    std::cerr << "Error: No registry values configured to save.\n";
    return false;
  }

  // Check if file exists and prompt for confirmation
  std::ifstream testFile(filepath);
  if (testFile.good()) {
    testFile.close();
    std::cout << "File '" << filepath << "' already exists.\n";
    std::cout << "Overwrite? (y/n): ";
    std::string response;
    std::getline(std::cin, response);

    if (response.empty() || (ToLower(response)[0] != 'y')) {
      std::cout << "Operation cancelled.\n";
      return false;
    }
  }

  std::ofstream file(filepath);
  if (!file.is_open()) {
    std::cerr << "Error: Could not create file: " << filepath << std::endl;
    return false;
  }

  file << "# " << ProgName << " saved preferences\n";
  file << "# Generated automatically - do not edit unless you know what you're "
          "doing\n";
  file << "# Registry path: HKEY_CURRENT_USER\\" << REGISTRY_BASE_PATH
       << "\n\n";

  // Organize values by section
  std::map<std::string, std::vector<std::pair<std::string, std::string>>>
      sections;

  for (const auto &regValue : g_registryValues) {
    std::string value;
    if (ReadRegistryValue(HKEY_CURRENT_USER, REGISTRY_BASE_PATH,
                          regValue.valueName, value)) {
      sections[regValue.section].push_back(
          std::make_pair(regValue.valueName, value));
    } else {
      std::cerr << "Warning: Skipping " << regValue.valueName
                << " (could not read)\n";
    }
  }

  // Write sections to file
  int totalSaved = 0;
  for (const auto &section : sections) {
    file << "[" << section.first << "]\n";
    for (const auto &kvp : section.second) {
      file << kvp.first << "=" << kvp.second << "\n";
      totalSaved++;
    }
    file << "\n";
  }

  file.close();

  std::cout << "Successfully saved " << totalSaved
            << " preference(s) to: " << filepath << std::endl;
  return true;
}

bool RestorePreferences(const std::string &filepath,
                        const std::string &sectionFilter = "") {
  std::ifstream file(filepath);
  if (!file.is_open()) {
    std::cerr << "Error: Could not open file: " << filepath << std::endl;
    return false;
  }

  // Prompt for confirmation before modifying registry
  std::cout << "This will write values from '" << filepath
            << "' to the Windows registry.\n";
  if (!sectionFilter.empty()) {
    std::cout << "Only restoring [" << sectionFilter << "] section.\n";
  }
  std::cout << "If QSpice is open, please close it before continuing.\n";

  std::cout << "Continue? (y/n): ";
  std::string response;
  std::getline(std::cin, response);

  if (response.empty() || (ToLower(response)[0] != 'y')) {
    file.close();
    std::cout << "Operation cancelled.\n";
    return false;
  }

  std::string currentSection;
  std::map<std::string, std::map<std::string, std::string>> preferences;
  std::string line;
  int lineNumber = 0;

  // Parse the preferences file
  while (std::getline(file, line)) {
    lineNumber++;

    // Trim whitespace
    line.erase(0, line.find_first_not_of(" \t\r\n"));
    line.erase(line.find_last_not_of(" \t\r\n") + 1);

    // Skip empty lines and comments
    if (line.empty() || line[0] == '#') {
      continue;
    }

    // Check for section header
    if (line[0] == '[') {
      size_t endBracket = line.find(']');
      if (endBracket == std::string::npos) {
        std::cerr << "Warning: Malformed section header at line " << lineNumber
                  << std::endl;
        continue;
      }
      currentSection = line.substr(1, endBracket - 1);
      continue;
    }

    // Parse key=value (only if we have a current section)
    if (currentSection.empty()) {
      std::cerr << "Warning: Key-value pair outside of section at line "
                << lineNumber << std::endl;
      continue;
    }

    size_t equalsPos = line.find('=');
    if (equalsPos == std::string::npos) {
      std::cerr << "Warning: Malformed key-value pair at line " << lineNumber
                << std::endl;
      continue;
    }

    std::string key = line.substr(0, equalsPos);
    std::string value = line.substr(equalsPos + 1);

    // Trim key and value
    key.erase(0, key.find_first_not_of(" \t"));
    key.erase(key.find_last_not_of(" \t") + 1);
    value.erase(0, value.find_first_not_of(" \t"));
    value.erase(value.find_last_not_of(" \t") + 1);

    preferences[currentSection][key] = value;
  }

  file.close();

  if (preferences.empty()) {
    std::cerr << "Error: No valid preferences found in file.\n";
    return false;
  }

  // Write values to registry
  int successCount = 0;
  int failCount = 0;
  int notFoundCount = 0;
  int skippedCount = 0;

  for (const auto &regValue : g_registryValues) {
    // Skip if section filter is specified and doesn't match
    if (!sectionFilter.empty() && regValue.section != sectionFilter) {
      skippedCount++;
      continue;
    }

    // Look for the value in the appropriate section
    auto sectionIt = preferences.find(regValue.section);
    if (sectionIt == preferences.end()) {
      std::cerr << "Warning: Section '" << regValue.section
                << "' not found in file\n";
      notFoundCount++;
      continue;
    }

    auto valueIt = sectionIt->second.find(regValue.valueName);
    if (valueIt == sectionIt->second.end()) {
      std::cerr << "Warning: Value '" << regValue.valueName
                << "' not found in section '" << regValue.section << "'\n";
      notFoundCount++;
      continue;
    }

    if (WriteRegistryValue(HKEY_CURRENT_USER, REGISTRY_BASE_PATH,
                           regValue.valueName, valueIt->second)) {
      successCount++;
    } else {
      failCount++;
    }
  }

  std::cout << "Restore complete: " << successCount << " succeeded";
  if (skippedCount > 0) {
    std::cout << ", " << skippedCount << " skipped (different section)";
  }
  if (notFoundCount > 0) {
    std::cout << ", " << notFoundCount << " not found in file";
  }
  if (failCount > 0) {
    std::cout << ", " << failCount << " failed";
  }
  std::cout << std::endl;

  return failCount == 0;
}

int main(int argc, char *argv[]) {
  // show version ID
  std::cout << ProgName << " " << VersionID << "\n\n";

  // Check command line arguments
  if (argc != 3) {
    std::cerr << "Error: Invalid number of arguments.\n\n";
    ShowUsage();
    return 1;
  }

  std::string option = ToLower(argv[1]);
  std::string filepath = NormalizeFilePath(argv[2]);

  bool isSave = (option == "-s");
  bool isRestoreAll = (option == "-rall");
  bool isRestoreWav = (option == "-rwav");
  bool isRestoreSch = (option == "-rsch");

  if (!isSave && !isRestoreAll && !isRestoreWav && !isRestoreSch) {
    std::cerr << "Error: Invalid option '" << argv[1] << "'.\n\n";
    ShowUsage();
    return 1;
  }

  bool success;
  if (isSave) {
    success = SavePreferences(filepath);
  } else if (isRestoreAll) {
    success = RestorePreferences(filepath);
  } else if (isRestoreWav) {
    success = RestorePreferences(filepath, WAVSEC);
  } else { // isRestoreSch
    success = RestorePreferences(filepath, SCHSEC);
  }

  return success ? 0 : 1;
}
