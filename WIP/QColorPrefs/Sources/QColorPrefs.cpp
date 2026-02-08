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
const char *VersionID = "v1.2   [" __TIMESTAMP__ "]";

const char *WAVSEC = "waveform";
const char *SCHSEC = "schematic";

// Global verbose flag
bool g_verbose = false;

// Base registry path for all values
const char *REGISTRY_BASE_PATH =
    "Software\\Marcus Aurelius Software LLC\\QSPICE\\[Preferences]";

// Structure to hold registry value configuration
struct RegistryValue {
  std::string section;
  std::string valueName;
  std::string comment; // Optional comment for documentation
};

// Registry values to be saved/restored
std::vector<RegistryValue> g_registryValues = {

    // Schematic-related values
    {SCHSEC, "CADHighlightColor", "Highlight Color"},
    {SCHSEC, "CADBackgroundColor", "Background"},
    {SCHSEC, "CADbomberSight", "Schematic Grid Color"},
    {SCHSEC, "CADgrapicAnnotation", "Graphic Annotation Color"},
    {SCHSEC, "CADsolidAnnotation", "Solid Annotation Fill Color"},
    {SCHSEC, "CADgrapicText", "Symbol Graphic Text Color"},
    {SCHSEC, "CADnormalText", "Component Text Color"},
    {SCHSEC, "CADnormalSchText", "Schematic Text Color"},
    {SCHSEC, "CADcommentText", "Comment Text Color"},
    {SCHSEC, "CADnetLabelText", "Net Label Color"},
    {SCHSEC, "CADwireColor", "Wire Color"},
    {SCHSEC, "CADhighlightWireColor", "Highlight Wire Color"},
    {SCHSEC, "CADpinDotColor", "Pin Dot Color"},
    {SCHSEC, "CADspritedAnchor", "Spirited Anchor Color"},
    {SCHSEC, "CADpinColor", "Pin Color Color"},
    {SCHSEC, "CADportBkColor", "Port Background Color"},
    {SCHSEC, "CADhierarchalBlock", "Hierarchical Box Color"},
    {SCHSEC, "CADhierarchalFill", "Hierarchical Fill Color"},
    {SCHSEC, "CADunconnectedPin", "Unconnected Pin Color"},
    {SCHSEC, "CADbusTapError", "Incorrect Bus Tap Color"},

    // Waveform-related values
    {WAVSEC, "NumberDataTraceColors", "Number of Trace Colors"},
    {WAVSEC, "CursorBackFore", "Attached Cursor Text Color"},
    {WAVSEC, "CursorBackGnd", "Attached Cursor Background"},
    {WAVSEC, "CursorLineColor", "Cursor Color"},
    {WAVSEC, "FrameColor", "Ticks & Axis Color"},
    {WAVSEC, "BackgroundColor", "Background Color"},
    {WAVSEC, "WaveFormViewerFontPoints", "not a color but relevant"},
    {WAVSEC, "WaveformPlotLineWidth", "not a color but relevant"},
    {WAVSEC, "DataColor1", "Waveform Trace #1 Color"},
    {WAVSEC, "DataColor2", "Waveform Trace #2 Color"},
    {WAVSEC, "DataColor3", "Waveform Trace #3 Color"},
    {WAVSEC, "DataColor4", "Waveform Trace #4 Color"},
    {WAVSEC, "DataColor5", "Waveform Trace #5 Color"},
    {WAVSEC, "DataColor6", "Waveform Trace #6 Color"},
    {WAVSEC, "DataColor7", "Waveform Trace #7 Color"},
    {WAVSEC, "DataColor8", "Waveform Trace #8 Color"},
    {WAVSEC, "DataColor9", "Waveform Trace #9 Color"},
    {WAVSEC, "DataColor10", "Waveform Trace #10 Color"},
    {WAVSEC, "DataColor11", "Waveform Trace #11 Color"},
    {WAVSEC, "DataColor12", "Waveform Trace #12 Color"},
    {WAVSEC, "DataColor13", "Waveform Trace #13 Color"},
    {WAVSEC, "DataColor14", "Waveform Trace #14 Color"},
    {WAVSEC, "DataColor15", "Waveform Trace #15 Color"},
    {WAVSEC, "DataColor16", "Waveform Trace #16 Color"},
    {WAVSEC, "DataColor17", "Waveform Trace #17 Color"},
    {WAVSEC, "DataColor18", "Waveform Trace #18 Color"},
    {WAVSEC, "DataColor19", "Waveform Trace #19 Color"},
    {WAVSEC, "DataColor20", "Waveform Trace #20 Color"},
    {WAVSEC, "DataColor21", "Waveform Trace #21 Color"},
    {WAVSEC, "DataColor22", "Waveform Trace #22 Color"},
    {WAVSEC, "DataColor23", "Waveform Trace #23 Color"},
    {WAVSEC, "DataColor24", "Waveform Trace #24 Color"}

};

void ShowUsage() {
  std::cout << "Usage: " << ProgName << " [options] <filepath>\n\n";
  std::cout << "Options:\n";
  std::cout << "  -sall      Save all QSpice registry color settings to file\n";
  std::cout << "  -ssch      Save only schematic color settings to file\n";
  std::cout << "  -swav      Save only waveform color settings to file\n";
  std::cout
      << "  -rall      Restore all QSpice registry color settings from file\n";
  std::cout << "  -rwav      Restore only waveform color settings from file\n";
  std::cout << "  -rsch      Restore only schematic color settings from file\n";
  std::cout << "  -v         Verbose mode (show detailed warnings)\n\n";
  std::cout << "Example:\n";
  std::cout << "  " << ProgName << " -sall mycolors\n";
  std::cout << "  " << ProgName << " -ssch schematic_colors\n";
  std::cout << "  " << ProgName << " -swav waveform_colors\n";
  std::cout << "  " << ProgName << " -rall mycolors.qcolorpref\n";
  std::cout << "  " << ProgName << " -v -rwav mycolors.qcolorpref\n";
  std::cout << "  " << ProgName << " -rsch mycolors.qcolorpref\n\n";
  std::cout << "Note: If no extension is provided, .qcolorpref will be added "
               "automatically.\n";
  std::cout << "      The -v option can be combined with any other option for "
               "verbose output.\n";
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

bool SavePreferences(const std::string &filepath,
                     const std::string &sectionFilter = "") {
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
  file << "# Registry path: HKEY_CURRENT_USER\\" << REGISTRY_BASE_PATH << "\n";
  if (!sectionFilter.empty()) {
    file << "# Section filter: [" << sectionFilter << "] only\n";
  }
  file << "\n";

  // Organize values by section with comments
  std::map<std::string,
           std::vector<std::tuple<std::string, std::string, std::string>>>
      sections;

  int skippedCount = 0;
  for (const auto &regValue : g_registryValues) {
    // Skip if section filter is specified and doesn't match
    if (!sectionFilter.empty() && regValue.section != sectionFilter) {
      skippedCount++;
      continue;
    }

    std::string value;
    if (ReadRegistryValue(HKEY_CURRENT_USER, REGISTRY_BASE_PATH,
                          regValue.valueName, value)) {
      sections[regValue.section].push_back(
          std::make_tuple(regValue.valueName, value, regValue.comment));
    } else {
      if (g_verbose) {
        std::cerr << "Warning: Skipping " << regValue.valueName
                  << " (could not read)\n";
      }
    }
  }

  // Write sections to file
  int totalSaved = 0;
  for (const auto &section : sections) {
    file << "[" << section.first << "]\n";

    // Find the maximum key=value length for alignment
    size_t maxKeyValueLen = 0;
    for (const auto &item : section.second) {
      size_t keyValueLen = std::get<0>(item).length() + 1 +
                           std::get<1>(item).length(); // key + '=' + value
      if (keyValueLen > maxKeyValueLen) {
        maxKeyValueLen = keyValueLen;
      }
    }

    // Write each key=value pair with aligned comments
    for (const auto &item : section.second) {
      std::string key = std::get<0>(item);
      std::string value = std::get<1>(item);
      std::string comment = std::get<2>(item);

      file << key << "=" << value;

      // Add inline comment if available, with padding for alignment
      if (!comment.empty()) {
        size_t currentLen = key.length() + 1 + value.length();
        size_t padding =
            maxKeyValueLen - currentLen + 1; // +1 for at least one space
        file << std::string(padding, ' ') << "# " << comment;
      }
      file << "\n";
      totalSaved++;
    }
    file << "\n";
  }

  file.close();

  std::cout << "Successfully saved " << totalSaved
            << " preference(s) to: " << filepath;
  if (skippedCount > 0 && g_verbose) {
    std::cout << " (" << skippedCount << " skipped due to section filter)";
  }
  std::cout << std::endl;
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

    // Strip inline comment (anything after #) from value
    size_t commentPos = value.find('#');
    if (commentPos != std::string::npos) {
      value = value.substr(0, commentPos);
    }

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
      if (g_verbose) {
        std::cerr << "Warning: Key '" << regValue.valueName << "' - Section '"
                  << regValue.section << "' not found in file\n";
      }
      notFoundCount++;
      continue;
    }

    auto valueIt = sectionIt->second.find(regValue.valueName);
    if (valueIt == sectionIt->second.end()) {
      if (g_verbose) {
        std::cerr << "Warning: Key '" << regValue.valueName
                  << "' not found in section '" << regValue.section << "'\n";
      }
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
  if (argc < 3) {
    std::cerr << "Error: Invalid number of arguments.\n\n";
    ShowUsage();
    return 1;
  }

  // Parse options and filepath
  std::string option;
  std::string filepath;

  // Check for -v flag in any position
  for (int i = 1; i < argc; i++) {
    std::string arg = ToLower(argv[i]);
    if (arg == "-v") {
      g_verbose = true;
    } else if (option.empty() && arg[0] == '-') {
      option = arg;
    } else if (filepath.empty()) {
      filepath = argv[i];
    }
  }

  if (option.empty() || filepath.empty()) {
    std::cerr << "Error: Missing required option or filepath.\n\n";
    ShowUsage();
    return 1;
  }

  filepath = NormalizeFilePath(filepath);

  bool isSaveAll = (option == "-sall");
  bool isSaveSch = (option == "-ssch");
  bool isSaveWav = (option == "-swav");
  bool isRestoreAll = (option == "-rall");
  bool isRestoreWav = (option == "-rwav");
  bool isRestoreSch = (option == "-rsch");

  if (!isSaveAll && !isSaveSch && !isSaveWav && !isRestoreAll &&
      !isRestoreWav && !isRestoreSch) {
    std::cerr << "Error: Invalid option '" << option << "'.\n\n";
    ShowUsage();
    return 1;
  }

  if (g_verbose) {
    std::cout << "Verbose mode enabled.\n";
  }

  bool success;
  if (isSaveAll) {
    success = SavePreferences(filepath);
  } else if (isSaveSch) {
    success = SavePreferences(filepath, SCHSEC);
  } else if (isSaveWav) {
    success = SavePreferences(filepath, WAVSEC);
  } else if (isRestoreAll) {
    success = RestorePreferences(filepath);
  } else if (isRestoreWav) {
    success = RestorePreferences(filepath, WAVSEC);
  } else { // isRestoreSch
    success = RestorePreferences(filepath, SCHSEC);
  }

  return success ? 0 : 1;
}
