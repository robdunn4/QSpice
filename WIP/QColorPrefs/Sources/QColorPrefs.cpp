/*
 * QColorPrefs.cpp -- A command-line utility to save/restore QSpice color
 *                    settings.
 *
 * Requires C++20 (MSVC /std:c++20 or later).
 *
 * The complete source code, documentation, and MSVS project files for the
 * current official version of this project is available at:
 *
 *   https://github.com/robdunn4/QSpice/
 *
 */
#define NOMINMAX
#include <algorithm>
#include <cassert>
#include <ctime>
#include <filesystem>
#include <format>
#include <fstream>
#include <iostream>
#include <map>
#include <optional>
#include <ranges>
#include <string>
#include <vector>
#include <windows.h>

#ifndef NDEBUG
#define DBGMSG "\n\n***** Debug Build *****"
#else
#define DBGMSG ""
#endif

namespace fs = std::filesystem;

const char *ProgName = "QColorPrefs";
const char *FileExt = ".qcolorpref";
// const char *ProgName = "QTheme";
// const char *FileExt  = ".qtheme";

const char *Version = "v1.6";
const char *BuildTimestamp = __TIMESTAMP__;

const std::string WAVSEC = "waveform";
const std::string SCHSEC = "schematic";

// Base registry path for all values
const char *REGISTRY_BASE_PATH =
    "Software\\Marcus Aurelius Software LLC\\QSPICE\\[Preferences]";

using Operation = unsigned char;
const Operation OpSecSch = 1, OpSecWav = 2, OpSecAll = OpSecSch | OpSecWav;
const Operation OpActSave = 4, OpActUpdate = 8, OpActRestore = 16,
                OpActMask = OpActSave | OpActUpdate | OpActRestore;

struct CmdArgs {
  // values set in ParseArgs()
  Operation op = 0;
  std::string filepath;
  bool verbose = false;
  bool testMode = false;
  bool silentMode = false;
};

// Structure to hold registry value configuration
struct PrefEntry {
  std::string section;
  std::string valueName;
  std::string comment;
};

// Structure to hold both registry and file values for a single key
struct PrefValue {
  std::string registryValue;
  std::string fileValue;
  std::string comment;
  bool inRegistry = false;
  bool inFile = false;
};

// Data structure: section -> key -> PrefValue
using PrefData = std::map<std::string, std::map<std::string, PrefValue>>;

// Results from AnalyzeOperation()
struct PrePassResult {
  int changedCount = 0;   // values that would change
  int unchangedCount = 0; // values already matching
  int skippedCount = 0;   // values excluded by section filter
  int notFoundCount = 0;  // keys absent from file (restore/update)
  int totalCount = 0;     // total values that would be written (save)
};

// Registry values to be saved/restored
const PrefEntry g_prefEntries[] = {
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
    {WAVSEC, "DataColor24", "Waveform Trace #24 Color"}};

// RAII wrapper for registry key handles
struct RegKeyGuard {
  HKEY key = nullptr;
  explicit RegKeyGuard(HKEY k = nullptr) : key(k) {}
  ~RegKeyGuard() {
    if (key) RegCloseKey(key);
  }
  RegKeyGuard(const RegKeyGuard &) = delete;
  RegKeyGuard &operator=(const RegKeyGuard &) = delete;
};

// Get current date and time as formatted string
std::string GetCurrentDateTime() {
  std::time_t now = std::time(nullptr);
  std::tm timeInfo;
  if (localtime_s(&timeInfo, &now) != 0) {
    return "Unknown Date";
  }
  char buffer[100];
  std::strftime(buffer, sizeof(buffer), "%B %d, %Y at %I:%M %p", &timeInfo);
  return std::string(buffer);
}

// clang-format off
void ShowSyntax() {
  std::cout << std::format(
      "Usage: {0}  [options] <action> <filepath>\n\n"

      "<action>:\n"
      "  -sall  Save all QSpice registry color settings to file\n"
      "  -ssch  Save only schematic color settings to file\n"
      "  -swav  Save only waveform color settings to file\n\n"

      "  -uall  Update all existing entries in file with current registry values\n"
      "  -usch  Update only existing schematic entries in file with current registry values\n"
      "  -uwav  Update only existing waveform entries in file with current registry values\n\n"

      "  -rall  Restore all QSpice registry color settings from file\n"
      "  -rwav  Restore only waveform color settings from file\n"
      "  -rsch  Restore only schematic color settings from file\n\n"

      "[options]:\n"
      "     -v  (Verbose Mode) Show per-value before/after changes\n"
      "     -t  (Test Mode)    Show what would happen without making changes\n"
      "     -s  (Silent Mode)  No user confirmation prompt (for use in batch files)\n\n"

      "Examples:\n"
      "  {0} -sall mycolors{1}\n"
      "  {0} -uwav mycolors\n"
      "  {0} -t -rall mycolors\n"
      "  {0} -t -v -sall mycolors\n"
      "  {0} -v -rwav mycolors\n\n"

      "Note: If no file extension is provided, '{1}' will be added automatically.\n"
      "      The -v option can be combined with any other option for verbose output.\n"
      "      The -t option shows actions without making changes (dry-run mode).\n",

      ProgName, FileExt);
}
// clang-format on

// Add default extension if the filepath has no extension.
void NormalizeFilePath(std::string &filepath) {
  std::string normalized = filepath;
  size_t lastDot = normalized.find_last_of('.');
  size_t lastSlash = normalized.find_last_of("\\/");

  // If no dot found, or dot is before last slash (part of directory name)
  if (lastDot == std::string::npos ||
      (lastSlash != std::string::npos && lastDot < lastSlash)) {
    normalized += FileExt;
  }
  filepath = normalized;
}

std::string ToLower(const std::string &str) {
  std::string lower = str;
  std::ranges::transform(lower, lower.begin(),
                         [](unsigned char c) { return std::tolower(c); });
  return lower;
}

// Open a registry key. Caller must use RegKeyGuard for RAII cleanup.
bool OpenRegistryKey(HKEY hive, const std::string &subkey, REGSAM access,
                     HKEY &hKey) {
  LONG result = RegOpenKeyExA(hive, subkey.c_str(), 0, access, &hKey);
  if (result != ERROR_SUCCESS) {
    std::cerr << std::format("Error: Could not open registry key: {}\n",
                             subkey);
    return false;
  }
  return true;
}

// Returns value on success, std::nullopt on failure.
// Caller decides whether to warn.
std::optional<std::string> ReadRegistryValue(HKEY hKey,
                                             const std::string &valueName) {
  DWORD dataSize = 0;
  DWORD type;

  LONG result =
      RegQueryValueExA(hKey, valueName.c_str(), NULL, &type, NULL, &dataSize);
  if (result != ERROR_SUCCESS || type != REG_SZ) {
    return std::nullopt;
  }

  std::vector<char> buffer(dataSize);
  result = RegQueryValueExA(hKey, valueName.c_str(), NULL, &type,
                            reinterpret_cast<LPBYTE>(buffer.data()), &dataSize);
  if (result != ERROR_SUCCESS) {
    return std::nullopt;
  }

  return std::string(buffer.data());
}

bool WriteRegistryValue(HKEY hKey, const std::string &valueName,
                        const std::string &value) {
  LONG result = RegSetValueExA(hKey, valueName.c_str(), 0, REG_SZ,
                               reinterpret_cast<const BYTE *>(value.c_str()),
                               static_cast<DWORD>(value.size() + 1));
  if (result != ERROR_SUCCESS) {
    std::cerr << std::format("Error: Could not write registry value: {}\n",
                             valueName);
    return false;
  }
  return true;
}

// Parse a preferences file into a section->key->value map.
std::map<std::string, std::map<std::string, std::string>>
ParsePreferencesFile(const std::string &filepath) {
  std::map<std::string, std::map<std::string, std::string>> prefs;
  std::ifstream f(filepath);
  if (!f.is_open()) return prefs;

  std::string currentSection;
  std::string line;
  while (std::getline(f, line)) {
    line.erase(0, line.find_first_not_of(" \t\r\n"));
    if (line.empty() || line.starts_with('#')) continue;
    if (line.starts_with('[')) {
      size_t end = line.find(']');
      if (end != std::string::npos) currentSection = line.substr(1, end - 1);
      continue;
    }
    if (currentSection.empty()) continue;
    size_t eq = line.find('=');
    if (eq == std::string::npos) continue;
    std::string key = line.substr(0, eq);
    std::string value = line.substr(eq + 1);
    // Strip inline comment
    size_t cp = value.find('#');
    if (cp != std::string::npos) value = value.substr(0, cp);
    // Trim
    key.erase(0, key.find_first_not_of(" \t"));
    key.erase(key.find_last_not_of(" \t") + 1);
    value.erase(0, value.find_first_not_of(" \t"));
    value.erase(value.find_last_not_of(" \t") + 1);
    // Normalize hex color values to lowercase for consistent comparison
    if (value.size() >= 2 && value[0] == '0' &&
        (value[1] == 'x' || value[1] == 'X'))
      value = ToLower(value);
    prefs[currentSection][key] = value;
  }
  return prefs;
}

// Helper: get ordered list of unique section names from g_prefEntries
std::vector<std::string> GetSectionOrder() {
  std::vector<std::string> sections;
  for (const PrefEntry &rv : g_prefEntries) {
    if (std::ranges::find(sections, rv.section) == sections.end()) {
      sections.push_back(rv.section);
    }
  }
  return sections;
}

// Helper: get the preference entries belonging to a given section,
// in declaration order
std::vector<const PrefEntry *> GetSectionEntries(const std::string &section) {
  std::vector<const PrefEntry *> entries;
  for (const PrefEntry &rv : g_prefEntries) {
    if (rv.section == section) {
      entries.push_back(&rv);
    }
  }
  return entries;
}

// Load all registry values into prefData at startup.
bool LoadRegistryValues(const CmdArgs &args, PrefData &prefData) {
  HKEY hKey;
  if (!OpenRegistryKey(HKEY_CURRENT_USER, REGISTRY_BASE_PATH, KEY_READ, hKey)) {
    return false;
  }
  RegKeyGuard guard(hKey);

  for (const PrefEntry &entry : g_prefEntries) {
    PrefValue &pv = prefData[entry.section][entry.valueName];
    pv.comment = entry.comment;

    std::optional<std::string> val = ReadRegistryValue(hKey, entry.valueName);
    if (val) {
      // Normalize hex color values to lowercase for consistent comparison
      if (val->size() >= 2 && (*val)[0] == '0' &&
          ((*val)[1] == 'x' || (*val)[1] == 'X'))
        *val = ToLower(*val);
      pv.registryValue = *val;
      pv.inRegistry = true;
    } else {
      pv.inRegistry = false;
      if (args.verbose) {
        std::cerr << std::format("Warning: Registry value not found: {}\n",
                                 entry.valueName);
      }
    }
  }

  return true;
}

// Load file values into prefData.
void LoadFileValues(const std::string &filepath, PrefData &prefData) {
  std::map<std::string, std::map<std::string, std::string>> filePrefs =
      ParsePreferencesFile(filepath);

  for (const std::pair<const std::string, std::map<std::string, std::string>>
           &secPair : filePrefs) {
    const std::string &secName = secPair.first;
    const std::map<std::string, std::string> &keys = secPair.second;

    std::map<std::string, std::map<std::string, PrefValue>>::iterator secIter =
        prefData.find(secName);
    if (secIter == prefData.end()) continue;

    for (const std::pair<const std::string, std::string> &keyPair : keys) {
      std::map<std::string, PrefValue>::iterator keyIter =
          secIter->second.find(keyPair.first);
      if (keyIter == secIter->second.end()) continue;
      keyIter->second.fileValue = keyPair.second;
      keyIter->second.inFile = true;
    }
  }
}

// Column-width constants for aligned output.
// Assumes max display width of 12 characters for any value.
const int MAX_VALUE_DISPLAY_WIDTH = 12;

struct ColumnWidths {
  size_t maxKeyLen = 0;
  size_t maxCommentLen = 0;
  size_t labelWidth = 0;
  size_t maxKeyValueLen = 0;
};

// Compute column widths based on g_prefEntries at runtime.
ColumnWidths ComputeColumnWidths() {
  ColumnWidths w;
  for (const PrefEntry &rv : g_prefEntries) {
    w.maxKeyLen = std::max(w.maxKeyLen, rv.valueName.size());
    w.maxCommentLen = std::max(w.maxCommentLen, rv.comment.size());
  }
  // "  key (comment):" = 2 + maxKeyLen + 2 + maxCommentLen + 2
  w.labelWidth = 2 + w.maxKeyLen + 2 + w.maxCommentLen + 2;
  // "key=value" = maxKeyLen + 1 + MAX_VALUE_DISPLAY_WIDTH
  w.maxKeyValueLen = w.maxKeyLen + 1 + MAX_VALUE_DISPLAY_WIDTH;
  return w;
}

const ColumnWidths g_colWidths = ComputeColumnWidths();

// Print one verbose change line with aligned columns.
// hasBefore:  true when a prior state exists (file for save, registry for
// restore). When hasBefore is false (save to new file only), shows just the
// value. When hasBefore is true and values are equal, shows "==" instead of
// "->". When hasBefore is true but beforeValue is empty, shows "(new)" for
// that key.
void PrintVerboseLine(const std::string &key, const std::string &comment,
                      bool hasBefore, const std::string &beforeValue,
                      const std::string &afterValue) {
  std::string label = std::format("  {} ({}):", key, comment);
  std::cout << std::format("{:<{}} ", label, g_colWidths.labelWidth);

  if (hasBefore) {
    std::string displayBefore = beforeValue.empty() ? "(new)" : beforeValue;
    const char *arrow = (beforeValue == afterValue) ? "==" : "->";
    std::cout << std::format("{:>{}} {} {}", displayBefore,
                             MAX_VALUE_DISPLAY_WIDTH, arrow, afterValue);
  } else {
    // No prior state — just show the value being written
    std::cout << std::format("{:>{}}", afterValue, MAX_VALUE_DISPLAY_WIDTH);
  }
  std::cout << "\n";
}

// Write the standard file header
void WriteFileHeader(std::ofstream &file, const char *verb,
                     const std::string &sectionFilter) {
  file << std::format("# This file was {} by {} {} on {}\n", verb, ProgName,
                      Version, GetCurrentDateTime());
  file << "# Generated automatically - do not edit unless you know what "
          "you're doing\n";
  file << std::format("# Registry path: HKEY_CURRENT_USER\\{}\n",
                      REGISTRY_BASE_PATH);
  if (!sectionFilter.empty()) {
    file << std::format("# Section filter: [{}] only\n", sectionFilter);
  }
  file << "\n";
}

// Write one key=value line with aligned comment.
void WriteKeyValue(std::ofstream &file, const std::string &key,
                   const std::string &value, const std::string &comment) {
  if (comment.empty()) {
    file << std::format("{}={}\n", key, value);
  } else {
    size_t kvLen = key.size() + 1 + value.size();
    size_t pad = (kvLen < g_colWidths.maxKeyValueLen)
                     ? g_colWidths.maxKeyValueLen - kvLen + 1
                     : 1;
    file << std::format("{}={}{:>{}}# {}\n", key, value, "", pad, comment);
  }
}

// Derive section filter string from the operation enum
std::string GetSectionFilter(const CmdArgs &args) {
  switch (args.op & OpSecAll) {
  case OpSecSch:
    return SCHSEC;
  case OpSecWav:
    return WAVSEC;
  }
  return ""; // no section filter
}

// Get human-readable description of what the operation will do
std::string GetOperationDescription(const CmdArgs &args) {
  switch (args.op) {
  case OpActSave | OpSecAll:
    return std::format("Save all settings from registry to {}", args.filepath);
  case OpActSave | OpSecSch:
    return std::format("Save only schematic settings from registry to {}",
                       args.filepath);
  case OpActSave | OpSecWav:
    return std::format("Save only waveform settings from registry to {}",
                       args.filepath);
  case OpActUpdate | OpSecAll:
    return std::format(
        "Update all existing entries in {} with current registry values",
        args.filepath);
  case OpActUpdate | OpSecSch:
    return std::format(
        "Update only schematic entries in {} with current registry values",
        args.filepath);
  case OpActUpdate | OpSecWav:
    return std::format(
        "Update only waveform entries in {} with current registry values",
        args.filepath);
  case OpActRestore | OpSecAll:
    return std::format("Restore all settings from {} to registry",
                       args.filepath);
  case OpActRestore | OpSecSch:
    return std::format("Restore only schematic settings from {} to registry",
                       args.filepath);
  case OpActRestore | OpSecWav:
    return std::format("Restore only waveform settings from {} to registry",
                       args.filepath);
  }

  // should not get here
  assert(0 && "Invalid operation enum value");
  return "Unknown operation";
}

// ---------------------------------------------------------------------------
// AnalyzeOperation
// Performs a read-only pre-pass over prefData to count what would change,
// what would be skipped, etc.  Also prints verbose per-value lines if
// args.verbose is set.  Returns a PrePassResult for use by
// PrintPrePassSummary() and the confirmation prompt.
// ---------------------------------------------------------------------------
PrePassResult AnalyzeOperation(const CmdArgs &args, const PrefData &prefData) {
  PrePassResult result;
  const std::string sectionFilter = GetSectionFilter(args);
  const Operation action = args.op & OpActMask;
  const bool fileExists = fs::exists(args.filepath);

  for (const std::string &secName : GetSectionOrder()) {

    // Section excluded by filter: count skipped entries and move on.
    // Count all canonical entries for this section; every one of them is
    // excluded regardless of its registry/file presence state.
    if (!sectionFilter.empty() && secName != sectionFilter) {
      result.skippedCount +=
          static_cast<int>(GetSectionEntries(secName).size());
      continue;
    }

    PrefData::const_iterator secIter = prefData.find(secName);
    if (secIter == prefData.end()) continue;

    bool any = false;
    for (const PrefEntry *rv : GetSectionEntries(secName)) {
      std::map<std::string, PrefValue>::const_iterator keyIter =
          secIter->second.find(rv->valueName);
      if (keyIter == secIter->second.end()) continue;
      const PrefValue &pv = keyIter->second;

      if (action == OpActSave) {
        if (!pv.inRegistry) continue;
        result.totalCount++;
        if (fileExists && pv.inFile) {
          if (pv.fileValue == pv.registryValue) result.unchangedCount++;
          else result.changedCount++;
        } else {
          result.changedCount++;
        }
        if (args.verbose)
          PrintVerboseLine(rv->valueName, pv.comment, fileExists, pv.fileValue,
                           pv.registryValue);

      } else if (action == OpActUpdate) {
        if (!pv.inFile) {
          if (pv.inRegistry) result.notFoundCount++;
          continue;
        }
        if (pv.fileValue == pv.registryValue) result.unchangedCount++;
        else result.changedCount++;
        if (args.verbose)
          PrintVerboseLine(rv->valueName, pv.comment, true, pv.fileValue,
                           pv.registryValue);

      } else if (action == OpActRestore) {
        if (!pv.inFile) {
          result.notFoundCount++;
          continue;
        }
        if (!pv.inRegistry || pv.registryValue != pv.fileValue)
          result.changedCount++;
        else result.unchangedCount++;
        if (args.verbose)
          PrintVerboseLine(rv->valueName, pv.comment, true, pv.registryValue,
                           pv.fileValue);
      }
      any = true;
    }
    if (args.verbose && any) std::cout << "\n";
  }
  return result;
}

// ---------------------------------------------------------------------------
// PrintPrePassSummary
// Reports what the operation will do (or would do in test mode).
// ---------------------------------------------------------------------------
void PrintPrePassSummary(const CmdArgs &args, const PrePassResult &result) {
  const Operation action = args.op & OpActMask;
  const bool fileExists = fs::exists(args.filepath);
  const std::string prefix = args.testMode ? "TEST MODE: Would" : "Will";

  if (action == OpActSave) {
    if (fileExists) {
      std::cout << std::format("{} change {} value(s) in: {}", prefix,
                               result.changedCount, args.filepath);
      if (result.unchangedCount > 0)
        std::cout << std::format(" ({} unchanged)", result.unchangedCount);
    } else {
      std::cout << std::format("{} create new file with {} value(s): {}",
                               prefix, result.totalCount, args.filepath);
    }

  } else if (action == OpActUpdate) {
    std::cout << std::format("{} change {} value(s)", prefix,
                             result.changedCount);
    if (result.unchangedCount > 0 || result.notFoundCount > 0) {
      std::cout << " (";
      if (result.unchangedCount > 0) {
        std::cout << std::format("{} unchanged", result.unchangedCount);
        if (result.notFoundCount > 0) std::cout << ", ";
      }
      if (result.notFoundCount > 0)
        std::cout << std::format("{} not in file", result.notFoundCount);
      std::cout << ")";
    }

  } else if (action == OpActRestore) {
    std::cout << std::format("{} change {} registry value(s)", prefix,
                             result.changedCount);
    if (result.unchangedCount > 0)
      std::cout << std::format(" ({} unchanged)", result.unchangedCount);
    if (result.notFoundCount > 0 && args.verbose)
      std::cout << std::format(", {} not found in file", result.notFoundCount);
  }

  if (result.skippedCount > 0)
    std::cout << std::format(", {} skipped (section filter)",
                             result.skippedCount);
  std::cout << "\n";
}

// ---------------------------------------------------------------------------
// ConfirmOperation
// Prompts the user to confirm before making changes.  Returns true if the
// user confirms, false if they cancel.
// ---------------------------------------------------------------------------
bool ConfirmOperation(const CmdArgs &args) {
  const Operation action = args.op & OpActMask;
  const std::string sectionFilter = GetSectionFilter(args);

  if (action == OpActRestore) {
    std::cout << "This will write values to the Windows registry.\n";
    if (!sectionFilter.empty())
      std::cout << std::format("Only restoring [{}] section.\n", sectionFilter);
    std::cout << "If QSpice is open, please close it before continuing.\n";
  } else if (action == OpActSave && fs::exists(args.filepath)) {
    std::cout << std::format("File '{}' already exists.\n", args.filepath);
  }

  std::cout << std::format("{}? (y/N): ", GetOperationDescription(args));
  std::string response;
  std::getline(std::cin, response);
  return !response.empty() && ToLower(response)[0] == 'y';
}

// ---------------------------------------------------------------------------
// SavePreferences  (write pass only)
// ---------------------------------------------------------------------------
bool SavePreferences(const CmdArgs &args, const PrefData &prefData) {
  const std::string sectionFilter = GetSectionFilter(args);

  std::ofstream file(args.filepath);
  if (!file.is_open()) {
    std::cerr << std::format("Error: Could not create file: {}\n",
                             args.filepath);
    return false;
  }
  WriteFileHeader(file, "created", sectionFilter);

  int totalSaved = 0;
  for (const std::string &secName : GetSectionOrder()) {
    if (!sectionFilter.empty() && secName != sectionFilter) continue;
    PrefData::const_iterator secIter = prefData.find(secName);
    if (secIter == prefData.end()) continue;

    bool any = false;
    for (const PrefEntry *rv : GetSectionEntries(secName)) {
      std::map<std::string, PrefValue>::const_iterator keyIter =
          secIter->second.find(rv->valueName);
      if (keyIter == secIter->second.end()) continue;
      if (!keyIter->second.inRegistry) continue;
      if (!any) file << std::format("[{}]\n", secName); // lazy header
      WriteKeyValue(file, rv->valueName, keyIter->second.registryValue,
                    rv->comment);
      any = true;
      totalSaved++;
    }
    if (any) file << "\n";
  }

  file.close();
  std::cout << std::format("Saved {} preference(s) to: {}\n", totalSaved,
                           args.filepath);
  return true;
}

// ---------------------------------------------------------------------------
// UpdatePreferences  (write pass only)
// ---------------------------------------------------------------------------
bool UpdatePreferences(const CmdArgs &args, const PrefData &prefData) {
  const std::string sectionFilter = GetSectionFilter(args);

  std::ofstream file(args.filepath);
  if (!file.is_open()) {
    std::cerr << std::format("Error: Could not open file for writing: {}\n",
                             args.filepath);
    return false;
  }
  WriteFileHeader(file, "modified", sectionFilter);

  int updatedCount = 0;
  for (const std::string &secName : GetSectionOrder()) {
    if (!sectionFilter.empty() && secName != sectionFilter) continue;
    PrefData::const_iterator secIter = prefData.find(secName);
    if (secIter == prefData.end()) continue;

    bool any = false;
    for (const PrefEntry *rv : GetSectionEntries(secName)) {
      std::map<std::string, PrefValue>::const_iterator keyIter =
          secIter->second.find(rv->valueName);
      if (keyIter == secIter->second.end()) continue;
      if (!keyIter->second.inFile) continue;
      if (!any) file << std::format("[{}]\n", secName); // lazy header
      WriteKeyValue(file, rv->valueName, keyIter->second.registryValue,
                    rv->comment);
      any = true;
      updatedCount++;
    }
    if (any) file << "\n";
  }

  file.close();
  std::cout << std::format("Updated {} preference(s) in: {}\n", updatedCount,
                           args.filepath);
  return true;
}

// ---------------------------------------------------------------------------
// RestorePreferences  (write pass only)
// ---------------------------------------------------------------------------
bool RestorePreferences(const CmdArgs &args, const PrefData &prefData) {
  const std::string sectionFilter = GetSectionFilter(args);

  RegKeyGuard guard;
  {
    HKEY hKey;
    if (!OpenRegistryKey(HKEY_CURRENT_USER, REGISTRY_BASE_PATH,
                         KEY_READ | KEY_WRITE, hKey))
      return false;
    guard.key = hKey;
  }

  int successCount = 0, failCount = 0;
  for (const std::string &secName : GetSectionOrder()) {
    if (!sectionFilter.empty() && secName != sectionFilter) continue;
    PrefData::const_iterator secIter = prefData.find(secName);
    if (secIter == prefData.end()) continue;

    for (const PrefEntry *rv : GetSectionEntries(secName)) {
      std::map<std::string, PrefValue>::const_iterator keyIter =
          secIter->second.find(rv->valueName);
      if (keyIter == secIter->second.end()) continue;
      if (!keyIter->second.inFile) continue;

      if (WriteRegistryValue(guard.key, rv->valueName,
                             keyIter->second.fileValue))
        successCount++;
      else failCount++;
    }
  }

  std::cout << std::format("Restored {} value(s) to registry", successCount);
  if (failCount > 0) std::cout << std::format(", {} failed", failCount);
  std::cout << "\n";

  return failCount == 0;
}

// ---------------------------------------------------------------------------
// ParseArgs
// ---------------------------------------------------------------------------
bool ParseArgs(int argc, char *argv[], CmdArgs &args) {
  // Action map for converting strings to Operation enum
  static const std::map<std::string, Operation> actionMap = {
      {"-sall", OpActSave | OpSecAll},    {"-ssch", OpActSave | OpSecSch},
      {"-swav", OpActSave | OpSecWav},    {"-uall", OpActUpdate | OpSecAll},
      {"-usch", OpActUpdate | OpSecSch},  {"-uwav", OpActUpdate | OpSecWav},
      {"-rall", OpActRestore | OpSecAll}, {"-rsch", OpActRestore | OpSecSch},
      {"-rwav", OpActRestore | OpSecWav},
  };

  std::string action;

  int i = 0;
  while (++i < argc) {
    std::string arg = ToLower(argv[i]);

    if (arg == "-v") {
      if (args.verbose) {
        std::cerr << "Error: -v (verbose) option specified multiple times.\n\n";
        return false;
      }
      args.verbose = true;
      continue;
    }

    if (arg == "-s") {
      if (args.silentMode) {
        std::cerr
            << "Error: -s (silent mode) option specified multiple times.\n\n";
        return false;
      }
      args.silentMode = true;
      continue;
    }

    if (arg == "-t") {
      if (args.testMode) {
        std::cerr
            << "Error: -t (test mode) option specified multiple times.\n\n";
        return false;
      }
      args.testMode = true;
      continue;
    }

    if (arg.starts_with('-')) {
      if (!action.empty()) {
        std::cerr << std::format(
            "Error: Too many actions specified: '{}' and '{}'.\n\n", action,
            arg);
        return false;
      }
      action = arg;
      continue;
    }

    if (!args.filepath.empty()) {
      std::cerr << std::format(
          "Error: Too many files specified: '{}' and '{}'.\n\n", args.filepath,
          argv[i]);
      return false;
    }
    args.filepath = argv[i]; // use raw argv for filepath to preserve case
  }

  if (action.empty()) {
    std::cerr << "Error: Missing required action argument.\n\n";
    return false;
  }

  if (args.filepath.empty()) {
    std::cerr << "Error: Missing required filepath argument.\n\n";
    return false;
  }
  NormalizeFilePath(args.filepath);

  // Convert action string to operation
  std::map<std::string, Operation>::const_iterator actIter =
      actionMap.find(action);
  if (actIter == actionMap.end()) {
    std::cerr << std::format("Error: Invalid option '{}'.\n\n", action);
    return false;
  }
  args.op = actIter->second;

  return true;
}

// ---------------------------------------------------------------------------
// main
// ---------------------------------------------------------------------------
int main(int argc, char *argv[]) {
  std::cout << std::format("\n{} {} [{}]   " DBGMSG "\n\n", ProgName, Version,
                           BuildTimestamp);

  CmdArgs args;
  PrefData prefData;

  if (!ParseArgs(argc, argv, args)) {
    ShowSyntax();
    return 1;
  }

  std::cout << std::format("SELECTED OPERATION:  {}\n\n",
                           GetOperationDescription(args));

  if (args.testMode)
    std::cout << "TEST MODE enabled (no changes will be made).\n\n";

  // For restore operations, the source file must exist
  if ((args.op & OpActMask) == OpActRestore && !fs::exists(args.filepath)) {
    std::cerr << std::format(
        "Error: Preferences file not found: {}\n"
        "       Cannot restore registry values without a source file.\n",
        args.filepath);
    return 1;
  }

  // Load all data upfront
  if (!LoadRegistryValues(args, prefData)) {
    std::cerr << "Error: Could not load registry values.\n";
    return 1;
  }

  if (fs::exists(args.filepath)) {
    LoadFileValues(args.filepath, prefData);
    if (args.verbose) {
      int fileKeyCount = 0;
      for (const std::pair<const std::string, std::map<std::string, PrefValue>>
               &secPair : prefData)
        for (const std::pair<const std::string, PrefValue> &kvPair :
             secPair.second)
          if (kvPair.second.inFile) fileKeyCount++;
      std::cout << std::format("Loaded {} value(s) from: {}\n\n", fileKeyCount,
                               args.filepath);
    }
  } else if (args.verbose) {
    std::cout << std::format("File does not exist and will be created: {}\n\n",
                             args.filepath);
  }

  if (args.verbose)
    std::cout << "VERBOSE MODE enabled (showing per-value before/after "
                 "changes).\n\n";

  // Pre-pass: analyze what would happen, print verbose lines if requested
  PrePassResult result = AnalyzeOperation(args, prefData);

  // Report what will/would happen
  PrintPrePassSummary(args, result);

  if (args.testMode) return 0;

  // If nothing would change, skip prompt and exit cleanly
  if (result.changedCount == 0) {
    std::cout << "No changes to make.\n";
    return 0;
  }

  // Confirm with user before making any changes
  if (!args.silentMode && !ConfirmOperation(args)) {
    std::cout << "Operation cancelled.\n";
    return 0;
  }

  // Dispatch — write passes only from here
  bool success = false;
  switch (args.op & OpActMask) {
  case OpActSave:
    success = SavePreferences(args, prefData);
    break;
  case OpActUpdate:
    success = UpdatePreferences(args, prefData);
    break;
  case OpActRestore:
    success = RestorePreferences(args, prefData);
    break;
  default:
    assert(0 && "Invalid operation");
  }

  if (!success) std::cerr << "Operation failed.\n";

  return success ? 0 : 1;
}
