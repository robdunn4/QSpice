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
#include <ctime>

const char *ProgName = "QColorPrefs.exe";
const char *VersionID = "v1.3   [" __TIMESTAMP__ "]";

const char *WAVSEC = "waveform";
const char *SCHSEC = "schematic";

// Global verbose flag
bool g_verbose = false;

// Global test mode flag
bool g_testMode = false;

// Get current date and time as formatted string
std::string GetCurrentDateTime() {
  std::time_t now = std::time(nullptr);
  std::tm timeInfo;
  
  // Use localtime_s (secure version) instead of deprecated localtime
  if (localtime_s(&timeInfo, &now) != 0) {
    return "Unknown Date";
  }
  
  char buffer[100];
  // Format: February 12, 2026 at 2:30 PM
  std::strftime(buffer, sizeof(buffer), "%B %d, %Y at %I:%M %p", &timeInfo);
  return std::string(buffer);
}

// Extract version string without compile timestamp
std::string GetVersionString() {
  // VersionID is like "v1.3   [Feb 12 2026 14:30:45]"
  // Extract just "v1.3"
  std::string version = VersionID;
  size_t pos = version.find("   [");
  if (pos != std::string::npos) {
    return version.substr(0, pos);
  }
  return version;
}

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
    {SCHSEC, "CADspritedAnchor", "Sprited Anchor Color"},
    {SCHSEC, "CADpinColor", "Pin Color Color [sic]"},
    {SCHSEC, "CADportBkColor", "Port Background Color"},
    {SCHSEC, "CADhierarchalBlock", "Hierarchical Box Color"},
    {SCHSEC, "CADhierarchalFill", "Hierarchical Fill Color"},
    {SCHSEC, "CADunconnectedPin", "Unconnected Pin Color"},
    {SCHSEC, "CADbusTapError", "Incorrect Bus Tap Color"},
    {SCHSEC, "JunctionSize", "Junction Size"},
    {SCHSEC, "CADlineWidth", "CAD Line Width"},
    {SCHSEC, "BUSwidthMultiplier", "Bus Width Multiplier"},
    {SCHSEC, "DefaultTextSize", "Default Text Size"},

    // Waveform-related values
    {WAVSEC, "NumberDataTraceColors", "Number of Trace Colors"},
    {WAVSEC, "CursorBackFore", "Attached Cursor Text Color"},
    {WAVSEC, "CursorBackGnd", "Attached Cursor Background"},
    {WAVSEC, "CursorLineColor", "Cursor Color"},
    {WAVSEC, "FrameColor", "Ticks & Axis Color"},
    {WAVSEC, "BackgroundColor", "Background Color"},
    {WAVSEC, "WaveFormViewerFontPoints", "Font Point Size"},
    {WAVSEC, "WaveformPlotLineWidth", "Plot Line Thickness"},
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
  std::cout << "  -uall      Update all existing entries in file with current registry values\n";
  std::cout << "  -usch      Update only schematic entries in file with current registry values\n";
  std::cout << "  -uwav      Update only waveform entries in file with current registry values\n";
  std::cout
      << "  -rall      Restore all QSpice registry color settings from file\n";
  std::cout << "  -rwav      Restore only waveform color settings from file\n";
  std::cout << "  -rsch      Restore only schematic color settings from file\n";
  std::cout << "  -v         Verbose mode: show per-value before/after changes\n";
  std::cout << "  -t         Test mode: show what would happen without making changes\n\n";
  std::cout << "Example:\n";
  std::cout << "  " << ProgName << " -sall mycolors\n";
  std::cout << "  " << ProgName << " -ssch schematic_colors\n";
  std::cout << "  " << ProgName << " -swav waveform_colors\n";
  std::cout << "  " << ProgName << " -uall mycolors.qcolorpref\n";
  std::cout << "  " << ProgName << " -uwav mycolors.qcolorpref\n";
  std::cout << "  " << ProgName << " -t -rall mycolors.qcolorpref\n";
  std::cout << "  " << ProgName << " -t -v -sall mycolors.qcolorpref\n";
  std::cout << "  " << ProgName << " -v -rwav mycolors.qcolorpref\n";
  std::cout << "  " << ProgName << " -rsch mycolors.qcolorpref\n\n";
  std::cout << "Note: If no extension is provided, .qcolorpref will be added "
               "automatically.\n";
  std::cout << "      The -v option can be combined with any other option for "
               "verbose output.\n";
  std::cout << "      The -t option shows actions without making changes "
               "(dry-run mode).\n";
  std::cout << "      Update operations only modify existing keys in the file, "
               "preserving structure.\n";
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

// Open a registry key for reading or writing. Caller must call CloseRegistryKey
// when done.
bool OpenRegistryKey(HKEY hive, const std::string &subkey, REGSAM access,
                     HKEY &hKey) {
  LONG result = RegOpenKeyExA(hive, subkey.c_str(), 0, access, &hKey);
  if (result != ERROR_SUCCESS) {
    std::cerr << "Error: Could not open registry key: " << subkey << std::endl;
    return false;
  }
  return true;
}

void CloseRegistryKey(HKEY hKey) { RegCloseKey(hKey); }

bool ReadRegistryValue(HKEY hKey, const std::string &valueName,
                       std::string &value) {
  // First, get the size of the string
  DWORD dataSize = 0;
  DWORD type;

  LONG result =
      RegQueryValueExA(hKey, valueName.c_str(), NULL, &type, NULL, &dataSize);

  if (result != ERROR_SUCCESS) {
    std::cerr << "Error: Could not read registry value: " << valueName
              << std::endl;
    return false;
  }

  if (type != REG_SZ) {
    std::cerr << "Error: Registry value is not a string (REG_SZ): " << valueName
              << std::endl;
    return false;
  }

  // Allocate buffer and read the string
  std::vector<char> buffer(dataSize);
  result = RegQueryValueExA(hKey, valueName.c_str(), NULL, &type,
                            reinterpret_cast<LPBYTE>(buffer.data()), &dataSize);

  if (result != ERROR_SUCCESS) {
    std::cerr << "Error: Could not read registry value: " << valueName
              << std::endl;
    return false;
  }

  value = std::string(buffer.data());
  return true;
}

bool WriteRegistryValue(HKEY hKey, const std::string &valueName,
                        const std::string &value) {
  LONG result = RegSetValueExA(hKey, valueName.c_str(), 0, REG_SZ,
                               reinterpret_cast<const BYTE *>(value.c_str()),
                               static_cast<DWORD>(value.length() + 1));

  if (result != ERROR_SUCCESS) {
    std::cerr << "Error: Could not write registry value: " << valueName
              << std::endl;
    return false;
  }

  return true;
}

// Parse a .qcolorpref file into a section->key->value map. Returns an empty
// map if the file cannot be opened or contains no valid entries.
std::map<std::string, std::map<std::string, std::string>>
ParsePreferencesFile(const std::string &filepath) {
  std::map<std::string, std::map<std::string, std::string>> prefs;
  std::ifstream f(filepath);
  if (!f.is_open())
    return prefs;

  std::string currentSection;
  std::string line;
  while (std::getline(f, line)) {
    line.erase(0, line.find_first_not_of(" \t\r\n"));
    if (line.empty() || line[0] == '#')
      continue;
    if (line[0] == '[') {
      size_t end = line.find(']');
      if (end != std::string::npos)
        currentSection = line.substr(1, end - 1);
      continue;
    }
    if (currentSection.empty())
      continue;
    size_t eq = line.find('=');
    if (eq == std::string::npos)
      continue;
    std::string key = line.substr(0, eq);
    std::string value = line.substr(eq + 1);
    // Strip inline comment
    size_t cp = value.find('#');
    if (cp != std::string::npos)
      value = value.substr(0, cp);
    // Trim
    key.erase(0, key.find_first_not_of(" \t"));
    key.erase(key.find_last_not_of(" \t") + 1);
    value.erase(0, value.find_first_not_of(" \t"));
    value.erase(value.find_last_not_of(" \t") + 1);
    prefs[currentSection][key] = value;
  }
  return prefs;
}

// Print one verbose change line with column-aligned values.
// labelWidth: pre-computed max width of "  key (comment):" across all entries
//             in the current operation, so value columns line up.
// beforeValueWidth: pre-computed max width of before values for alignment
// afterValueWidth: pre-computed max width of after values for alignment
// hasBefore:  false for new-file saves (no prior value exists).
// beforeValue/afterValue: the two sides of the change.
// When hasBefore is true and the values are equal, prints "==" instead of "->".
void PrintVerboseLine(size_t labelWidth, size_t beforeValueWidth,
                      size_t afterValueWidth, const std::string &key,
                      const std::string &comment, bool hasBefore,
                      const std::string &beforeValue,
                      const std::string &afterValue) {
  // Build the label: "  key (comment):"
  std::string label = "  " + key + " (" + comment + "):";
  // Pad to labelWidth so all value columns align
  if (label.size() < labelWidth)
    label += std::string(labelWidth - label.size(), ' ');
  std::cout << label << " ";
  
  if (hasBefore) {
    // Right-align the before value
    if (beforeValue.size() < beforeValueWidth)
      std::cout << std::string(beforeValueWidth - beforeValue.size(), ' ');
    std::cout << beforeValue << " ";
    
    // Arrow or equals
    const std::string &arrow = (beforeValue == afterValue) ? "==" : "->";
    std::cout << arrow << " ";
    
    // Left-align the after value
    std::cout << afterValue;
    if (afterValue.size() < afterValueWidth)
      std::cout << std::string(afterValueWidth - afterValue.size(), ' ');
  } else {
    // No before value - align appropriately
    std::cout << std::string(beforeValueWidth, ' ') << " -> " << afterValue;
    if (afterValue.size() < afterValueWidth)
      std::cout << std::string(afterValueWidth - afterValue.size(), ' ');
  }
  std::cout << "\n";
}

bool SavePreferences(const std::string &filepath,
                     const std::string &sectionFilter = "") {
  if (g_registryValues.empty()) {
    std::cerr << "Error: No registry values configured to save.\n";
    return false;
  }

  // Determine whether the file already exists and, if verbose, parse it now
  // so we can show before/after values.
  bool fileExisted = false;
  std::map<std::string, std::map<std::string, std::string>> existingPrefs;

  {
    std::ifstream testFile(filepath);
    if (testFile.good()) {
      fileExisted = true;
      testFile.close();
      if (g_verbose || g_testMode)
        existingPrefs = ParsePreferencesFile(filepath);

      if (!g_testMode) {
        std::cout << "File '" << filepath << "' already exists.\n";
        std::cout << "Overwrite? (y/n): ";
        std::string response;
        std::getline(std::cin, response);
        if (response.empty() || (ToLower(response)[0] != 'y')) {
          std::cout << "Operation cancelled.\n";
          return false;
        }
      }
    }
  }

  // In test mode, we'll just build the data structure but not write the file
  std::ofstream file;
  if (!g_testMode) {
    file.open(filepath);
    if (!file.is_open()) {
      std::cerr << "Error: Could not create file: " << filepath << std::endl;
      return false;
    }

    file << "# This file was created by QColorPrefs " << GetVersionString() << " on " 
         << GetCurrentDateTime() << "\n";
    file << "# Generated automatically - do not edit unless you know what you're "
            "doing\n";
    file << "# Registry path: HKEY_CURRENT_USER\\" << REGISTRY_BASE_PATH << "\n";
    if (!sectionFilter.empty()) {
      file << "# Section filter: [" << sectionFilter << "] only\n";
    }
    file << "\n";
  } else {
    if (fileExisted) {
      std::cout << "TEST MODE: Would overwrite existing file '" << filepath << "'\n";
    } else {
      std::cout << "TEST MODE: Would create new file '" << filepath << "'\n";
    }
    std::cout << "\n";
  }

  // Organize values by section with comments
  std::map<std::string,
           std::vector<std::tuple<std::string, std::string, std::string>>>
      sections;

  HKEY hKey;
  if (!OpenRegistryKey(HKEY_CURRENT_USER, REGISTRY_BASE_PATH, KEY_READ,
                       hKey)) {
    file.close();
    return false;
  }

  int skippedCount = 0;
  for (const auto &regValue : g_registryValues) {
    // Skip if section filter is specified and doesn't match
    if (!sectionFilter.empty() && regValue.section != sectionFilter) {
      skippedCount++;
      continue;
    }

    std::string value;
    if (ReadRegistryValue(hKey, regValue.valueName, value)) {
      sections[regValue.section].push_back(
          std::make_tuple(regValue.valueName, value, regValue.comment));
    } else {
      if (g_verbose) {
        std::cerr << "Warning: Skipping " << regValue.valueName
                  << " (could not read)\n";
      }
    }
  }

  CloseRegistryKey(hKey);

  // Write sections to file
  int totalSaved = 0;
  int changedCount = 0;
  int unchangedCount = 0;

  for (const auto &section : sections) {
    if (!g_testMode) {
      file << "[" << section.first << "]\n";
    }

    // Pre-compute widths for this section only (for verbose/test output)
    size_t labelWidth = 0;
    size_t beforeValueWidth = 0;
    size_t afterValueWidth = 0;
    
    if (g_verbose || g_testMode) {
      for (const auto &item : section.second) {
        const std::string &key = std::get<0>(item);
        const std::string &value = std::get<1>(item);
        const std::string &comment = std::get<2>(item);
        
        // Compute label width: "  key (comment):" length
        size_t w = 2 + key.size() + 2 + comment.size() + 2;
        if (w > labelWidth)
          labelWidth = w;
        
        // Compute after value width
        if (value.size() > afterValueWidth)
          afterValueWidth = value.size();
        
        // Compute before value width (if file existed)
        if (fileExisted) {
          auto secIt = existingPrefs.find(section.first);
          if (secIt != existingPrefs.end()) {
            auto valIt = secIt->second.find(key);
            if (valIt != secIt->second.end()) {
              if (valIt->second.size() > beforeValueWidth)
                beforeValueWidth = valIt->second.size();
            }
          }
        }
      }
    }

    // Find the maximum key=value length for alignment of inline comments
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

      if (!g_testMode) {
        file << key << "=" << value;

        // Add inline comment if available, with padding for alignment
        if (!comment.empty()) {
          size_t currentLen = key.length() + 1 + value.length();
          size_t padding =
              maxKeyValueLen - currentLen + 1; // +1 for at least one space
          file << std::string(padding, ' ') << "# " << comment;
        }
        file << "\n";
      }
      totalSaved++;

      // Verbose or test mode: show before/after for this entry
      if (g_verbose || g_testMode) {
        std::string beforeValue;
        bool hasBefore = false;
        if (fileExisted) {
          hasBefore = true;
          auto secIt = existingPrefs.find(section.first);
          if (secIt != existingPrefs.end()) {
            auto valIt = secIt->second.find(key);
            if (valIt != secIt->second.end())
              beforeValue = valIt->second;
          }
        }
        
        // Track if value changed
        if (hasBefore && beforeValue == value) {
          unchangedCount++;
        } else {
          changedCount++;
        }
        
        PrintVerboseLine(labelWidth, beforeValueWidth, afterValueWidth, key,
                         comment, hasBefore, beforeValue, value);
      }
    }
    if (!g_testMode) {
      file << "\n";
    }
    
    // Blank line between sections in verbose/test mode
    if ((g_verbose || g_testMode) && !section.second.empty()) {
      std::cout << std::endl;
    }
  }

  if (!g_testMode) {
    file.close();
    std::cout << "Successfully saved " << totalSaved
              << " preference(s) to: " << filepath;
  } else {
    if (fileExisted) {
      std::cout << "TEST MODE: Would change " << changedCount
                << " value(s) in: " << filepath;
      if (unchangedCount > 0) {
        std::cout << " (" << unchangedCount << " unchanged)";
      }
    } else {
      std::cout << "TEST MODE: Would create new file with " << totalSaved
                << " value(s): " << filepath;
    }
  }
  if (skippedCount > 0 && (g_verbose || g_testMode)) {
    std::cout << ", " << skippedCount << " skipped due to section filter";
  }
  std::cout << std::endl;
  return true;
}

bool UpdatePreferences(const std::string &filepath,
                       const std::string &sectionFilter = "") {
  // Check if file exists
  std::ifstream testFile(filepath);
  if (!testFile.good()) {
    std::cerr << "Error: File '" << filepath << "' does not exist.\n";
    std::cerr << "Update operations require an existing file to modify.\n";
    std::cerr << "Use -sall, -ssch, or -swav to create a new file.\n";
    return false;
  }
  testFile.close();

  // Parse the existing file
  std::map<std::string, std::map<std::string, std::string>> existingPrefs =
      ParsePreferencesFile(filepath);

  if (existingPrefs.empty()) {
    std::cerr << "Error: Could not parse existing file or file is empty.\n";
    return false;
  }

  // Read current registry values
  HKEY hKey;
  if (!OpenRegistryKey(HKEY_CURRENT_USER, REGISTRY_BASE_PATH, KEY_READ,
                       hKey)) {
    return false;
  }

  // Build updated sections - only include keys that exist in the file
  std::map<std::string,
           std::vector<std::tuple<std::string, std::string, std::string>>>
      sections;
  
  int updatedCount = 0;
  int unchangedCount = 0;
  int skippedCount = 0;
  int notInFileCount = 0;

  for (const auto &regValue : g_registryValues) {
    // Skip if section filter is specified and doesn't match
    if (!sectionFilter.empty() && regValue.section != sectionFilter) {
      skippedCount++;
      continue;
    }

    // Check if this key exists in the file
    auto sectionIt = existingPrefs.find(regValue.section);
    if (sectionIt == existingPrefs.end()) {
      notInFileCount++;
      continue;
    }

    auto valueIt = sectionIt->second.find(regValue.valueName);
    if (valueIt == sectionIt->second.end()) {
      notInFileCount++;
      continue;
    }

    // Key exists in file - read current registry value
    std::string currentValue;
    if (ReadRegistryValue(hKey, regValue.valueName, currentValue)) {
      sections[regValue.section].push_back(
          std::make_tuple(regValue.valueName, currentValue, regValue.comment));
      
      // Track if value changed
      if (currentValue != valueIt->second) {
        updatedCount++;
      } else {
        unchangedCount++;
      }
    } else {
      if (g_verbose) {
        std::cerr << "Warning: Could not read registry value: "
                  << regValue.valueName << "\n";
      }
    }
  }

  CloseRegistryKey(hKey);

  if (sections.empty()) {
    std::cerr << "Error: No matching keys found in file to update.\n";
    return false;
  }

  // Write updated file (or show what would be written in test mode)
  std::ofstream file;
  if (!g_testMode) {
    file.open(filepath);
    if (!file.is_open()) {
      std::cerr << "Error: Could not open file for writing: " << filepath
                << std::endl;
      return false;
    }

    file << "# This file was modified by QColorPrefs " << GetVersionString() << " on " 
         << GetCurrentDateTime() << "\n";
    file << "# Generated automatically - do not edit unless you know what you're "
            "doing\n";
    file << "# Registry path: HKEY_CURRENT_USER\\" << REGISTRY_BASE_PATH << "\n";
    if (!sectionFilter.empty()) {
      file << "# Section filter: [" << sectionFilter << "] only\n";
    }
    file << "\n";
  } else {
    std::cout << "TEST MODE: Would update file '" << filepath << "'\n";
    std::cout << "\n";
  }

  // Pre-compute widths per section for verbose output
  for (const auto &section : sections) {
    if (!g_testMode) {
      file << "[" << section.first << "]\n";
    }

    size_t labelWidth = 0;
    size_t beforeValueWidth = 0;
    size_t afterValueWidth = 0;

    if (g_verbose || g_testMode) {
      for (const auto &item : section.second) {
        const std::string &key = std::get<0>(item);
        const std::string &value = std::get<1>(item);
        const std::string &comment = std::get<2>(item);

        // Compute label width
        size_t w = 2 + key.size() + 2 + comment.size() + 2;
        if (w > labelWidth)
          labelWidth = w;

        // After value width (current registry value)
        if (value.size() > afterValueWidth)
          afterValueWidth = value.size();

        // Before value width (old file value)
        auto secIt = existingPrefs.find(section.first);
        if (secIt != existingPrefs.end()) {
          auto valIt = secIt->second.find(key);
          if (valIt != secIt->second.end()) {
            if (valIt->second.size() > beforeValueWidth)
              beforeValueWidth = valIt->second.size();
          }
        }
      }
    }

    // Find max key=value length for comment alignment
    size_t maxKeyValueLen = 0;
    for (const auto &item : section.second) {
      size_t keyValueLen = std::get<0>(item).length() + 1 +
                           std::get<1>(item).length();
      if (keyValueLen > maxKeyValueLen) {
        maxKeyValueLen = keyValueLen;
      }
    }

    // Write each key=value pair
    for (const auto &item : section.second) {
      std::string key = std::get<0>(item);
      std::string value = std::get<1>(item);
      std::string comment = std::get<2>(item);

      if (!g_testMode) {
        file << key << "=" << value;

        // Add inline comment if available
        if (!comment.empty()) {
          size_t currentLen = key.length() + 1 + value.length();
          size_t padding = maxKeyValueLen - currentLen + 1;
          file << std::string(padding, ' ') << "# " << comment;
        }
        file << "\n";
      }

      // Verbose or test mode output
      if (g_verbose || g_testMode) {
        std::string beforeValue;
        bool hasBefore = false;
        auto secIt = existingPrefs.find(section.first);
        if (secIt != existingPrefs.end()) {
          auto valIt = secIt->second.find(key);
          if (valIt != secIt->second.end()) {
            beforeValue = valIt->second;
            hasBefore = true;
          }
        }
        PrintVerboseLine(labelWidth, beforeValueWidth, afterValueWidth, key,
                         comment, hasBefore, beforeValue, value);
      }
    }
    if (!g_testMode) {
      file << "\n";
    }
    
    // Blank line after section in verbose/test mode
    if ((g_verbose || g_testMode) && !section.second.empty()) {
      std::cout << std::endl;
    }
  }

  if (!g_testMode) {
    file.close();
    std::cout << "Update complete: " << updatedCount << " changed, "
              << unchangedCount << " unchanged";
  } else {
    std::cout << "TEST MODE: Would change " << updatedCount << " value(s)";
    if (unchangedCount > 0 || notInFileCount > 0) {
      std::cout << " (";
      if (unchangedCount > 0) {
        std::cout << unchangedCount << " unchanged";
        if (notInFileCount > 0) {
          std::cout << ", ";
        }
      }
      if (notInFileCount > 0) {
        std::cout << notInFileCount << " not in original file";
      }
      std::cout << ")";
    }
  }
  if (skippedCount > 0 && (g_verbose || g_testMode)) {
    std::cout << ", " << skippedCount << " skipped (section filter)";
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

  // Prompt for confirmation before modifying registry (skip in test mode)
  if (!g_testMode) {
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
  } else {
    std::cout << "TEST MODE: Would write values from '" << filepath
              << "' to Windows registry\n";
    if (!sectionFilter.empty()) {
      std::cout << "TEST MODE: Would restore only [" << sectionFilter << "] section\n";
    }
    std::cout << "\n";
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

  // Write values to registry.
  // Open with KEY_READ|KEY_WRITE so we can read the current value before
  // writing when verbose mode is active.
  HKEY hKey;
  if (!OpenRegistryKey(HKEY_CURRENT_USER, REGISTRY_BASE_PATH,
                       KEY_READ | KEY_WRITE, hKey)) {
    return false;
  }

  int successCount = 0;
  int failCount = 0;
  int notFoundCount = 0;
  int skippedCount = 0;
  int changedCount = 0;
  int unchangedCount = 0;

  // Group registry values by section for per-section alignment
  std::map<std::string, std::vector<const RegistryValue*>> valuesBySection;
  for (const auto &rv : g_registryValues) {
    if (!sectionFilter.empty() && rv.section != sectionFilter) {
      skippedCount++;
      continue;
    }
    valuesBySection[rv.section].push_back(&rv);
  }

  // Process each section separately for proper alignment
  for (const auto &sectionPair : valuesBySection) {
    const std::string &currentSection = sectionPair.first;
    const std::vector<const RegistryValue*> &sectionValues = sectionPair.second;

    // Pre-compute widths for this section only (for verbose/test output)
    size_t labelWidth = 0;
    size_t beforeValueWidth = 0;
    size_t afterValueWidth = 0;
    
    if (g_verbose || g_testMode) {
      for (const auto *rv : sectionValues) {
        // Compute label width: "  key (comment):" length
        size_t w = 2 + rv->valueName.size() + 2 + rv->comment.size() + 2;
        if (w > labelWidth)
          labelWidth = w;
        
        // Look for the value in the preferences to get after value width
        auto sectionIt = preferences.find(rv->section);
        if (sectionIt != preferences.end()) {
          auto valueIt = sectionIt->second.find(rv->valueName);
          if (valueIt != sectionIt->second.end()) {
            if (valueIt->second.size() > afterValueWidth)
              afterValueWidth = valueIt->second.size();
          }
        }
        
        // Get current registry value for before value width
        std::string currentValue;
        if (ReadRegistryValue(hKey, rv->valueName, currentValue)) {
          if (currentValue.size() > beforeValueWidth)
            beforeValueWidth = currentValue.size();
        }
      }
    }

    // Process all values in this section
    for (const auto *regValue : sectionValues) {
      // Look for the value in the appropriate section
      auto sectionIt = preferences.find(regValue->section);
      if (sectionIt == preferences.end()) {
        if (g_verbose) {
          std::cerr << "Warning: Key '" << regValue->valueName << "' - Section '"
                    << regValue->section << "' not found in file\n";
        }
        notFoundCount++;
        continue;
      }

      auto valueIt = sectionIt->second.find(regValue->valueName);
      if (valueIt == sectionIt->second.end()) {
        if (g_verbose) {
          std::cerr << "Warning: Key '" << regValue->valueName
                    << "' not found in section '" << regValue->section << "'\n";
        }
        notFoundCount++;
        continue;
      }

      // In verbose or test mode, read the current registry value before writing so we
      // can display the before/after change.
      std::string beforeValue;
      bool hasBefore = false;
      if (g_verbose || g_testMode) {
        hasBefore = ReadRegistryValue(hKey, regValue->valueName, beforeValue);
        // Failure to read is non-fatal here; hasBefore stays false.
      }

      if (!g_testMode) {
        if (WriteRegistryValue(hKey, regValue->valueName, valueIt->second)) {
          successCount++;
          
          // Track if value actually changed
          if (hasBefore && beforeValue == valueIt->second) {
            unchangedCount++;
          } else {
            changedCount++;
          }
          
          if (g_verbose) {
            PrintVerboseLine(labelWidth, beforeValueWidth, afterValueWidth,
                             regValue->valueName, regValue->comment, hasBefore,
                             beforeValue, valueIt->second);
          }
        } else {
          failCount++;
        }
      } else {
        // Test mode - just show what would happen
        successCount++;
        
        // Track if value would change
        if (hasBefore && beforeValue == valueIt->second) {
          unchangedCount++;
        } else {
          changedCount++;
        }
        
        PrintVerboseLine(labelWidth, beforeValueWidth, afterValueWidth,
                         regValue->valueName, regValue->comment, hasBefore,
                         beforeValue, valueIt->second);
      }
    }
    
    // Add blank line between sections in verbose/test mode
    if ((g_verbose || g_testMode) && !sectionValues.empty()) {
      std::cout << std::endl;
    }
  }

  CloseRegistryKey(hKey);

  if (!g_testMode) {
    std::cout << "Restore complete: " << successCount << " succeeded";
  } else {
    std::cout << "TEST MODE: Would change " << changedCount << " registry value(s)";
    if (unchangedCount > 0) {
      std::cout << " (" << unchangedCount << " unchanged)";
    }
  }
  if (skippedCount > 0) {
    std::cout << ", " << skippedCount << " skipped (different section)";
  }
  if (notFoundCount > 0) {
    std::cout << ", " << notFoundCount << " not found in file";
  }
  if (failCount > 0 && !g_testMode) {
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

  // Check for -v and -t flags in any position
  for (int i = 1; i < argc; i++) {
    std::string arg = ToLower(argv[i]);
    if (arg == "-v") {
      g_verbose = true;
    } else if (arg == "-t") {
      g_testMode = true;
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
  bool isUpdateAll = (option == "-uall");
  bool isUpdateSch = (option == "-usch");
  bool isUpdateWav = (option == "-uwav");
  bool isRestoreAll = (option == "-rall");
  bool isRestoreWav = (option == "-rwav");
  bool isRestoreSch = (option == "-rsch");

  if (!isSaveAll && !isSaveSch && !isSaveWav && !isUpdateAll &&
      !isUpdateSch && !isUpdateWav && !isRestoreAll && !isRestoreWav &&
      !isRestoreSch) {
    std::cerr << "Error: Invalid option '" << option << "'.\n\n";
    ShowUsage();
    return 1;
  }

  if (g_verbose) {
    std::cout << "Verbose mode enabled (showing per-value before/after "
                 "changes).\n";
  }

  if (g_testMode) {
    std::cout << "TEST MODE enabled (no changes will be made).\n";
  }

  if (g_verbose || g_testMode) {
    std::cout << "\n";
  }

  bool success;
  if (isSaveAll) {
    success = SavePreferences(filepath);
  } else if (isSaveSch) {
    success = SavePreferences(filepath, SCHSEC);
  } else if (isSaveWav) {
    success = SavePreferences(filepath, WAVSEC);
  } else if (isUpdateAll) {
    success = UpdatePreferences(filepath);
  } else if (isUpdateSch) {
    success = UpdatePreferences(filepath, SCHSEC);
  } else if (isUpdateWav) {
    success = UpdatePreferences(filepath, WAVSEC);
  } else if (isRestoreAll) {
    success = RestorePreferences(filepath);
  } else if (isRestoreWav) {
    success = RestorePreferences(filepath, WAVSEC);
  } else { // isRestoreSch
    success = RestorePreferences(filepath, SCHSEC);
  }

  return success ? 0 : 1;
}
