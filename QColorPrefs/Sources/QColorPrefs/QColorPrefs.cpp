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
// not needed for this utility, but left in place for easy reference if we want
// to add a GUI in the future
// #include "./resources/resource.h"

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
#define DBGMSG "  ***** Debug Build *****"
#else
#define DBGMSG ""
#endif

namespace fs = std::filesystem;

const char *ProgName = "QColorPrefs";
const char *FileExt = ".qcolorpref";
const char *VersionStr = "v1.7";
const char *BuildTimestamp = __TIMESTAMP__;

const std::string WAVSEC = "waveform";
const std::string SCHSEC = "schematic";

// Base registry path for all values
const char *REGISTRY_BASE_PATH =
    "Software\\Marcus Aurelius Software LLC\\QSPICE\\[Preferences]";

using Operation = unsigned char;
const Operation OpSecSch = 1, OpSecWav = 2, OpSecMask = OpSecSch | OpSecWav;
const Operation OpActSave = 4, OpActUpdate = 8, OpActRestore = 16,
                OpActMask = OpActSave | OpActUpdate | OpActRestore;

struct CmdArgs {
  // values set in ParseArgs()
  Operation op = 0;
  std::string filePath;
  bool verbMode = false;
  bool testMode = false;
  bool silentMode = false;
};

// Structure to hold registry value configuration
struct PrefEntry {
  std::string secName;
  std::string valName;
  std::string comment;
};

// Structure to hold both registry and file values for a single key
struct PrefValue {
  std::string regValue;
  std::string fileValue;
  std::string comment;
  bool inReg = false;
  bool inFile = false;
};

// Data structure: section -> key -> PrefValue
using PrefData = std::map<std::string, std::map<std::string, PrefValue>>;

// Results from AnalyzeOp()
struct PrePassResult {
  int changedCnt = 0;   // values that would change
  int unchangedCnt = 0; // values already matching
  int skippedCnt = 0;   // values excluded by section filter
  int notFoundCnt = 0;  // keys absent from file (restore/update)
  int totalCnt = 0;     // total values that would be written (save)
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

//  Built once on first call; returns section order and per-section entry lists.
struct SecData {
  std::vector<std::string> secOrder;
  std::map<std::string, std::vector<const PrefEntry *>> entries;
};

static const SecData &GetSecData() {
  static const SecData data = []() {
    SecData d;
    for (const PrefEntry &rv : g_prefEntries) {
      if (d.entries.find(rv.secName) == d.entries.end())
        d.secOrder.push_back(rv.secName);
      d.entries[rv.secName].push_back(&rv);
    }
    return d;
  }();
  return data;
}

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
std::string GetCurrDateTime() {
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
      "     -?  Show command-line syntax\n"
      "     -v  (Verbose Mode) Show per-value before/after changes\n"
      "     -t  (Test Mode)    Show what would happen without making changes\n"
      "     -s  (Silent Mode)  Suppress user confirmation prompts\n\n"

      "If no file extension is provided, '{1}' will be added automatically.\n"
      "The -v option can be combined with any other option for verbose output.\n"
      "The -t option shows actions without making changes (dry-run mode).\n"
      "The -s option suppresses confirmation prompts.  Save, update, and restore\n"
      "       operations will not prompt the user for permission before applying\n"
      "       the changes.  (Intended for use in batch scripts and Desktop shortcuts.\n"
      "       Use with caution.)\n",
      ProgName, FileExt);
}
// clang-format on

// Add default extension if the filepath has no extension.
void NormalizeFilePath(std::string &filePath) {
  size_t lastDot = filePath.find_last_of('.');
  size_t lastSlash = filePath.find_last_of("\\/");

  // If no dot found, or dot is before last slash (part of directory name)
  if (lastDot == std::string::npos ||
      (lastSlash != std::string::npos && lastDot < lastSlash)) {
    filePath += FileExt;
  }
}

std::string strToLower(const std::string &str) {
  std::string lower = str;
  std::ranges::transform(lower, lower.begin(),
                         [](unsigned char c) { return std::tolower(c); });
  return lower;
}

// open a registry key with specified access rights.  Returns std::nullopt on
// failure, with an error message printed.
std::optional<HKEY> OpenRegKey(HKEY hive, const std::string &subkey,
                               REGSAM access) {
  HKEY hKey;
  LONG result = RegOpenKeyExA(hive, subkey.c_str(), 0, access, &hKey);
  if (result != ERROR_SUCCESS) {
    std::cout << std::format("\n*** Error: Could not open registry key: {}\n",
                             subkey);
    return std::nullopt;
  }
  return hKey;
}

// Returns value on success, std::nullopt on failure.
// Caller decides whether to warn.
std::optional<std::string> ReadRegVal(HKEY hKey, const std::string &valName) {
  DWORD dataSize = 0;
  DWORD type;

  LONG result =
      RegQueryValueExA(hKey, valName.c_str(), NULL, &type, NULL, &dataSize);
  if (result != ERROR_SUCCESS || type != REG_SZ) {
    return std::nullopt;
  }

  std::vector<char> buffer(dataSize);
  result = RegQueryValueExA(hKey, valName.c_str(), NULL, &type,
                            reinterpret_cast<LPBYTE>(buffer.data()), &dataSize);
  if (result != ERROR_SUCCESS) {
    return std::nullopt;
  }

  return std::string(buffer.data());
}

bool WriteRegVal(HKEY hKey, const std::string &valName,
                 const std::string &value) {
  LONG result = RegSetValueExA(hKey, valName.c_str(), 0, REG_SZ,
                               reinterpret_cast<const BYTE *>(value.c_str()),
                               static_cast<DWORD>(value.size() + 1));
  if (result != ERROR_SUCCESS) {
    std::cout << std::format(
        "\n*** Error: Could not write registry value: {}\n", valName);
    return false;
  }
  return true;
}

// Parse a preferences file into a section->key->value map.
std::map<std::string, std::map<std::string, std::string>>
ParsePrefFile(const std::string &filePath) {
  std::map<std::string, std::map<std::string, std::string>> prefs;
  std::ifstream f(filePath);
  if (!f.is_open()) return prefs;

  std::string currSec;
  std::string line;
  while (std::getline(f, line)) {
    line.erase(0, line.find_first_not_of(" \t\r\n"));
    if (line.empty() || line.starts_with('#')) continue;
    if (line.starts_with('[')) {
      size_t end = line.find(']');
      if (end != std::string::npos) currSec = line.substr(1, end - 1);
      continue;
    }
    if (currSec.empty()) continue;
    size_t eq = line.find('=');
    if (eq == std::string::npos) continue;
    std::string key = line.substr(0, eq);
    std::string value = line.substr(eq + 1);

    // Strip inline comment -- we assume tht '#' only appears in comments, not
    // in values (e.g. not in hex color values).  This is a safe assumption for
    // the specific set of values we're working with, but could be revisited if
    // the utility is expanded to handle more general preferences in the future.
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
      value = strToLower(value);
    prefs[currSec][key] = value;
  }
  return prefs;
}

// Helper: get ordered list of unique section names from g_prefEntries
const std::vector<std::string> &GetSecOrder() { return GetSecData().secOrder; }

// Helper: get the preference entries belonging to a given section,
// in declaration order
const std::vector<const PrefEntry *> &
GetSecEntries(const std::string &section) {
  return GetSecData().entries.at(section);
}

// Load all registry values into prefData at startup.
bool LoadRegVals(const CmdArgs &args, PrefData &prefData) {
  auto hKey = OpenRegKey(HKEY_CURRENT_USER, REGISTRY_BASE_PATH, KEY_READ);
  if (!hKey) return false;
  RegKeyGuard guard(*hKey);

  for (const PrefEntry &entry : g_prefEntries) {
    PrefValue &pv = prefData[entry.secName][entry.valName];
    pv.comment = entry.comment;

    std::optional<std::string> val = ReadRegVal(*hKey, entry.valName);
    if (val) {
      // Normalize hex color values to lowercase for consistent comparison
      if (val->size() >= 2 && (*val)[0] == '0' &&
          ((*val)[1] == 'x' || (*val)[1] == 'X'))
        *val = strToLower(*val);
      pv.regValue = *val;
      pv.inReg = true;
    } else {
      pv.inReg = false;
      if (args.verbMode) {
        std::cout << std::format("Warning: Registry value not found: {}\n",
                                 entry.valName);
      }
    }
  }

  return true;
}

// Load file values into prefData.
void LoadFileVals(const std::string &filePath, PrefData &prefData) {
  std::map<std::string, std::map<std::string, std::string>> filePrefs =
      ParsePrefFile(filePath);

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

struct ColWidths {
  size_t maxSecLen = std::max(WAVSEC.length(), SCHSEC.length());
  size_t maxKeyLen = 0;
  size_t maxCommentLen = 0;
  size_t labelWidth = 0;
  size_t maxKeyValLen = 0;
};

// Compute column widths based on g_prefEntries at runtime.
ColWidths CalcColWidths() {
  ColWidths w;
  for (const PrefEntry &rv : g_prefEntries) {
    w.maxKeyLen = std::max(w.maxKeyLen, rv.valName.size());
    w.maxCommentLen = std::max(w.maxCommentLen, rv.comment.size());
  }
  // "  key (comment):" = 2 + maxKeyLen + 2 + maxCommentLen + 2
  w.labelWidth = 2 + w.maxKeyLen + 2 + w.maxCommentLen + 2;
  // "key=value" = maxKeyLen + 1 + MAX_VALUE_DISPLAY_WIDTH
  w.maxKeyValLen = w.maxKeyLen + 1 + MAX_VALUE_DISPLAY_WIDTH;
  return w;
}

const ColWidths g_colWidths = CalcColWidths();

// Print one verbose change line with aligned columns.
// hasBefore:  true when a prior state exists (file for save, registry for
// restore). When hasBefore is false (save to new file only), shows just the
// value. When hasBefore is true and values are equal, shows "==" instead of
// "->". When hasBefore is true but beforeValue is empty, shows "(new)" for
// that key.
void PrintVerboseLine(const std::string &sec, const std::string &key,
                      const std::string &comment, bool hasBefore,
                      const std::string &beforeVal,
                      const std::string &afterVal) {
  std::string label = std::format("  {:<{}}  {} ({}):", "[" + sec + "]",
                                  g_colWidths.maxSecLen + 2, key, comment);
  std::cout << std::format("{:<{}} ", label, g_colWidths.labelWidth + 7);

  if (hasBefore) {
    std::string displayBefore = beforeVal.empty() ? "(new)" : beforeVal;
    const char *arrow = (beforeVal == afterVal) ? "==" : "->";
    std::cout << std::format("{:>{}} {} {}", displayBefore,
                             MAX_VALUE_DISPLAY_WIDTH, arrow, afterVal);
  } else {
    // No prior state — just show the value being written
    std::cout << std::format("{:>{}}", afterVal, MAX_VALUE_DISPLAY_WIDTH);
  }
  std::cout << "\n";
}

// Write the standard file header
void WriteFileHdr(std::ofstream &file, const char *verb,
                  const std::string &secFilter) {
  file << std::format("# This file was {} by {} {} on {}\n", verb, ProgName,
                      VersionStr, GetCurrDateTime());
  file << "# Generated automatically - do not edit unless you know what "
          "you're doing\n";
  file << "# Save operations are complete replacements and comments are "
          "not preserved\n";
  file << "# Update operations attempt to preserve line comments within header "
          "sections\n";
  if (!secFilter.empty()) {
    file << std::format("# Section filter: [{}] only\n", secFilter);
  }
  file << "\n";
}

// Write one key=value line with aligned comment.
void WriteKeyVal(std::ofstream &file, const std::string &key,
                 const std::string &val, const std::string &comment) {
  if (comment.empty()) {
    file << std::format("{}={}\n", key, val);
  } else {
    size_t kvLen = key.size() + 1 + val.size();
    size_t pad = (kvLen < g_colWidths.maxKeyValLen)
                     ? g_colWidths.maxKeyValLen - kvLen + 1
                     : 1;
    file << std::format("{}={}{:>{}}# {}\n", key, val, "", pad, comment);
  }
}

// Derive section filter string from the operation enum
std::string GetSecFilter(const CmdArgs &args) {
  switch (args.op & OpSecMask) {
  case OpSecSch:
    return SCHSEC;
  case OpSecWav:
    return WAVSEC;
  }
  return ""; // no section filter
}

// Get human-readable description of what the operation will do
std::string GetOpDesc(const CmdArgs &args) {
  const std::string sec = GetSecFilter(args);
  const std::string scope = sec.empty() ? "all" : "only " + sec;

  switch (args.op & OpActMask) {
  case OpActSave:
    return std::format("Save {} settings from registry to {}", scope,
                       args.filePath);
  case OpActUpdate:
    return std::format("Update {} entries in {} with current registry values",
                       sec.empty() ? "all existing" : scope, args.filePath);
  case OpActRestore:
    return std::format("Restore {} settings from {} to registry", scope,
                       args.filePath);
  }

  assert(0 && "\n*** Invalid operation enum value");
  return "Unknown operation";
}

// ---------------------------------------------------------------------------
// AnalyzeOp
// Performs a read-only pre-pass over prefData to count what would change,
// what would be skipped, etc.  Also prints verbose per-value lines if
// args.verbose is set.  Returns a PrePassResult for use by
// PrintPrePassSummary() and the confirmation prompt.
// ---------------------------------------------------------------------------
PrePassResult AnalyzeOp(const CmdArgs &args, const PrefData &prefData) {
  PrePassResult result;
  const std::string secFilter = GetSecFilter(args);
  const Operation action = args.op & OpActMask;
  const bool fileExists = fs::exists(args.filePath);

  for (const std::string &secName : GetSecOrder()) {

    // Section excluded by filter: count skipped entries and move on.
    // Count all canonical entries for this section; every one of them is
    // excluded regardless of its registry/file presence state.
    if (!secFilter.empty() && secName != secFilter) {
      result.skippedCnt += static_cast<int>(GetSecEntries(secName).size());
      continue;
    }

    PrefData::const_iterator secIter = prefData.find(secName);
    if (secIter == prefData.end()) continue;

    bool any = false;
    for (const PrefEntry *rv : GetSecEntries(secName)) {
      std::map<std::string, PrefValue>::const_iterator keyIter =
          secIter->second.find(rv->valName);
      if (keyIter == secIter->second.end()) continue;
      const PrefValue &pv = keyIter->second;

      switch (action) {
      case OpActSave:
        if (!pv.inReg) continue;
        result.totalCnt++;
        if (fileExists && pv.inFile) {
          if (pv.fileValue == pv.regValue) result.unchangedCnt++;
          else result.changedCnt++;
        } else {
          result.changedCnt++;
        }
        if (args.verbMode)
          PrintVerboseLine(secName, rv->valName, pv.comment, fileExists,
                           pv.fileValue, pv.regValue);
        break;
      case OpActUpdate:
        if (!pv.inFile) {
          if (pv.inReg) result.notFoundCnt++;
          continue;
        }
        if (pv.fileValue == pv.regValue) result.unchangedCnt++;
        else result.changedCnt++;
        if (args.verbMode)
          PrintVerboseLine(secName, rv->valName, pv.comment, true, pv.fileValue,
                           pv.regValue);
        break;
      case OpActRestore:
        if (!pv.inFile) {
          result.notFoundCnt++;
          continue;
        }
        if (!pv.inReg || pv.regValue != pv.fileValue) result.changedCnt++;
        else result.unchangedCnt++;
        if (args.verbMode)
          PrintVerboseLine(secName, rv->valName, pv.comment, true, pv.regValue,
                           pv.fileValue);
        break;
      default:
        assert(0 && "\n*** Invalid operation enum value in AnalyzeOperation");
        break;
      }

      any = true;
    }
    if (args.verbMode && any) std::cout << "\n";
  }
  return result;
}

// ---------------------------------------------------------------------------
// PrintPrePassSummary
// Reports what the operation will do (or would do in test mode).
// ---------------------------------------------------------------------------
void PrintPrePassSummary(const CmdArgs &args, const PrePassResult &result) {
  const Operation action = args.op & OpActMask;
  const bool fileExists = fs::exists(args.filePath);
  const std::string prefix = args.testMode ? "TEST MODE: Would" : "Will";

  switch (action) {
  case OpActSave:
    if (fileExists) {
      std::cout << std::format("{} change {} value(s) in {}", prefix,
                               result.changedCnt, args.filePath);
      if (result.unchangedCnt > 0)
        std::cout << std::format(" ({} unchanged)", result.unchangedCnt);
    } else
      std::cout << std::format("{} create new file with {} value(s): {}",
                               prefix, result.totalCnt, args.filePath);
    break;
  case OpActUpdate:
    std::cout << std::format("{} change {} value(s)", prefix,
                             result.changedCnt);
    if (result.unchangedCnt > 0 || result.notFoundCnt > 0) {
      if (result.unchangedCnt > 0) {
        std::cout << std::format(", {} unchanged", result.unchangedCnt);
      }
      if (result.notFoundCnt > 0)
        std::cout << std::format(", {} not in file", result.notFoundCnt);
    }
    break;
  case OpActRestore:
    std::cout << std::format("{} change {} registry value(s)", prefix,
                             result.changedCnt);
    if (result.unchangedCnt > 0)
      std::cout << std::format(" ({} unchanged)", result.unchangedCnt);
    if (result.notFoundCnt > 0 && args.verbMode)
      std::cout << std::format(", {} not found in file", result.notFoundCnt);
    break;
  default:
    assert(0 && "\n*** Invalid operation enum value in PrintPrePassSummary");
    break;
  }

  if (result.skippedCnt > 0)
    std::cout << std::format(", {} skipped (section filter)",
                             result.skippedCnt);
  std::cout << "\n";
}

// ---------------------------------------------------------------------------
// ConfirmOp
// Prompts the user to confirm before making changes.  Returns true if the
// user confirms, false if they cancel.
// ---------------------------------------------------------------------------
bool ConfirmOp(const CmdArgs &args) {
  const Operation action = args.op & OpActMask;
  const std::string secFilter = GetSecFilter(args);

  if (action == OpActRestore) {
    std::cout << "This will write values to the Windows registry\n";
    if (!secFilter.empty())
      std::cout << std::format("Only restoring [{}] section\n", secFilter);
    std::cout << "If QSpice is open, please close it before continuing\n";
  } else if (action == OpActSave && fs::exists(args.filePath)) {
    std::cout << std::format("File '{}' already exists\n", args.filePath);
  }

  std::cout << std::format("\n{}? (y/N): ", GetOpDesc(args));
  std::string response;
  std::getline(std::cin, response);
  return !response.empty() && strToLower(response)[0] == 'y';
}

// ---------------------------------------------------------------------------
// SavePrefs  (write pass only)
// ---------------------------------------------------------------------------
bool SavePrefs(const CmdArgs &args, const PrefData &prefData) {
  const std::string secFilter = GetSecFilter(args);

  std::ofstream file(args.filePath);
  if (!file.is_open()) {
    std::cout << std::format("\n*** Error: Could not create file: {}\n",
                             args.filePath);
    return false;
  }
  WriteFileHdr(file, "created", secFilter);

  int totalSaved = 0;
  for (const std::string &secName : GetSecOrder()) {
    if (!secFilter.empty() && secName != secFilter) continue;
    PrefData::const_iterator secIter = prefData.find(secName);
    if (secIter == prefData.end()) continue;

    bool any = false;
    for (const PrefEntry *rv : GetSecEntries(secName)) {
      std::map<std::string, PrefValue>::const_iterator keyIter =
          secIter->second.find(rv->valName);
      if (keyIter == secIter->second.end()) continue;
      if (!keyIter->second.inReg) continue;
      if (!any) file << std::format("[{}]\n", secName); // lazy header
      WriteKeyVal(file, rv->valName, keyIter->second.regValue, rv->comment);
      any = true;
      totalSaved++;
    }

    if (any) file << "\n";
  }

  file.close();
  std::cout << std::format("Saved {} preference(s) to: {}\n", totalSaved,
                           args.filePath);
  return true;
}

// ---------------------------------------------------------------------------
// UpdatePrefs  (write pass only)
// ---------------------------------------------------------------------------
bool UpdatePrefs(const CmdArgs &args, const PrefData &prefData) {
  const std::string secFilter = GetSecFilter(args);

  // Read the existing file into memory before opening it for writing.
  std::vector<std::string> lines;
  {
    std::ifstream in(args.filePath);
    if (!in.is_open()) {
      std::cout << std::format(
          "\n*** Error: Could not open file for reading: {}\n", args.filePath);
      return false;
    }
    std::string ln;
    while (std::getline(in, ln))
      lines.push_back(ln);
  }

  std::ofstream file(args.filePath);
  if (!file.is_open()) {
    std::cout << std::format(
        "\n*** Error: Could not open file for writing: {}\n", args.filePath);
    return false;
  }

  // Write the refreshed file header (timestamp, verb, optional section filter).
  WriteFileHdr(file, "modified", secFilter);

  // Stream through the saved lines.  Skip the old header block (all leading
  // '#' and blank lines before the first '[section]') — WriteFileHdr already
  // wrote a fresh one above.  From the first section header onward, pass every
  // line through verbatim except key=value lines that belong to the update set,
  // which get rewritten with the current registry value.
  std::string currSec;
  bool pastHeader = false;
  int updatedCnt = 0;

  for (const std::string &rawLine : lines) {
    // Trim leading whitespace for classification only; preserve raw line
    // otherwise.
    std::string trimmed = rawLine;
    trimmed.erase(0, trimmed.find_first_not_of(" \t\r\n"));

    // Still in the leading header block?
    if (!pastHeader) {
      if (trimmed.empty() || trimmed.starts_with('#'))
        continue;        // skip old header
      pastHeader = true; // first non-comment, non-blank line — section header
    }

    // Section header: track current section, pass through.
    if (trimmed.starts_with('[')) {
      size_t end = trimmed.find(']');
      if (end != std::string::npos) currSec = trimmed.substr(1, end - 1);
      file << rawLine << "\n";
      continue;
    }

    // Blank or comment line: pass through verbatim.
    if (trimmed.empty() || trimmed.starts_with('#')) {
      file << rawLine << "\n";
      continue;
    }

    // key=value line: check whether this key should be updated.
    size_t eq = trimmed.find('=');
    if (eq == std::string::npos || currSec.empty()) {
      file << rawLine << "\n"; // unrecognised line — pass through
      continue;
    }

    std::string key = trimmed.substr(0, eq);
    key.erase(0, key.find_first_not_of(" \t"));
    key.erase(key.find_last_not_of(" \t") + 1);

    // Section excluded by filter — pass line through unchanged.
    if (!secFilter.empty() && currSec != secFilter) {
      file << rawLine << "\n";
      continue;
    }

    // Look up the key in prefData.
    PrefData::const_iterator secIter = prefData.find(currSec);
    if (secIter == prefData.end()) {
      file << rawLine << "\n";
      continue;
    }
    std::map<std::string, PrefValue>::const_iterator keyIter =
        secIter->second.find(key);
    if (keyIter == secIter->second.end() || !keyIter->second.inFile) {
      file << rawLine << "\n";
      continue;
    }

    // Rewrite the line with the current registry value and canonical comment.
    WriteKeyVal(file, key, keyIter->second.regValue, keyIter->second.comment);
    updatedCnt++;
  }

  file.close();
  std::cout << std::format("Updated {} preference(s) in: {}\n", updatedCnt,
                           args.filePath);
  return true;
}

// ---------------------------------------------------------------------------
// RestorePrefs  (write pass only)
// ---------------------------------------------------------------------------
bool RestorePrefs(const CmdArgs &args, const PrefData &prefData) {
  const std::string secFilter = GetSecFilter(args);

  auto hKey =
      OpenRegKey(HKEY_CURRENT_USER, REGISTRY_BASE_PATH, KEY_READ | KEY_WRITE);
  if (!hKey) return false;
  RegKeyGuard guard(*hKey);

  int successCnt = 0, failCnt = 0;
  for (const std::string &secName : GetSecOrder()) {
    if (!secFilter.empty() && secName != secFilter) continue;
    PrefData::const_iterator secIter = prefData.find(secName);
    if (secIter == prefData.end()) continue;

    for (const PrefEntry *rv : GetSecEntries(secName)) {
      std::map<std::string, PrefValue>::const_iterator keyIter =
          secIter->second.find(rv->valName);
      if (keyIter == secIter->second.end()) continue;
      if (!keyIter->second.inFile) continue;

      if (WriteRegVal(guard.key, rv->valName, keyIter->second.fileValue))
        successCnt++;
      else failCnt++;
    }
  }

  std::cout << std::format("Restored {} value(s) to registry", successCnt);
  if (failCnt > 0) std::cout << std::format(", {} failed", failCnt);
  std::cout << "\n";

  return failCnt == 0;
}

// ---------------------------------------------------------------------------
// ParseArgs
// ---------------------------------------------------------------------------
bool ParseArgs(int argc, char *argv[], CmdArgs &args) {
  // Action map for converting strings to Operation enum
  static const std::map<std::string, Operation> actionMap = {
      {"-sall", OpActSave | OpSecMask},    {"-ssch", OpActSave | OpSecSch},
      {"-swav", OpActSave | OpSecWav},     {"-uall", OpActUpdate | OpSecMask},
      {"-usch", OpActUpdate | OpSecSch},   {"-uwav", OpActUpdate | OpSecWav},
      {"-rall", OpActRestore | OpSecMask}, {"-rsch", OpActRestore | OpSecSch},
      {"-rwav", OpActRestore | OpSecWav},
  };

  std::string action;

  int i = 0;
  while (++i < argc) {
    std::string arg = strToLower(argv[i]);

    // if help flag, show syntax and exit immediately
    if (arg == "-?") {
      ShowSyntax();
      exit(0);
    }

    if (arg == "-v") {
      if (args.verbMode) {
        std::cout
            << "\n*** Error: -v (verbose) option specified multiple times\n\n";
        return false;
      }
      args.verbMode = true;
      continue;
    }

    if (arg == "-s") {
      if (args.silentMode) {
        std::cout << "\n*** Error: -s (silent mode) option specified multiple "
                     "times\n\n";
        return false;
      }
      args.silentMode = true;
      continue;
    }

    if (arg == "-t") {
      if (args.testMode) {
        std::cout << "\n*** Error: -t (test mode) option specified multiple "
                     "times\n\n";
        return false;
      }
      args.testMode = true;
      continue;
    }

    if (arg.starts_with('-')) {
      if (!action.empty()) {
        std::cout << std::format(
            "\n*** Error: Too many actions specified: '{}' and '{}'\n\n",
            action, arg);
        return false;
      }
      action = arg;
      continue;
    }

    if (!args.filePath.empty()) {
      std::cout << std::format(
          "\n*** Error: Too many files specified: '{}' and '{}'\n\n",
          args.filePath, argv[i]);
      return false;
    }
    args.filePath = argv[i]; // use raw argv for filepath to preserve case
  }

  if (action.empty()) {
    std::cout << "\n*** Error: Missing required action argument\n\n";
    return false;
  }

  if (args.filePath.empty()) {
    std::cout << "\n*** Error: Missing required filepath argument\n\n";
    return false;
  }
  NormalizeFilePath(args.filePath);

  // Convert action string to operation
  std::map<std::string, Operation>::const_iterator actIter =
      actionMap.find(action);
  if (actIter == actionMap.end()) {
    std::cout << std::format("\n*** Error: Invalid option '{}'\n\n", action);
    return false;
  }
  args.op = actIter->second;

  return true;
}

// ---------------------------------------------------------------------------
// main
// ---------------------------------------------------------------------------
int main(int argc, char *argv[]) {
  std::cout << std::format("\n{} {} [{}]   " DBGMSG "\n\n", ProgName, VersionStr,
                           BuildTimestamp);

  CmdArgs args;
  PrefData prefData;

  if (!ParseArgs(argc, argv, args)) {
    ShowSyntax();
    return 1;
  }

  std::cout << std::format("SELECTED OPERATION:  {}\n", GetOpDesc(args));

  if (args.testMode) std::cout << "TEST MODE: No changes will be made\n";

  // For restore and update operations, the source file must exist
  bool fileExists = fs::exists(args.filePath);
  if (!fileExists && (args.op & (OpActRestore | OpActUpdate))) {
    std::cout << std::format("\n*** Error: File not found: {}\n"
                             "           Cannot perform restore or update "
                             "operations without a source file\n",
                             args.filePath);
    return 1;
  }

  // Load all data upfront
  if (!LoadRegVals(args, prefData)) {
    std::cout << "\n*** Error: Could not load registry values\n";
    return 1;
  }

  if (fileExists) {
    LoadFileVals(args.filePath, prefData);
    if (args.verbMode) {
      int fileKeyCnt = 0;
      for (const std::pair<const std::string, std::map<std::string, PrefValue>>
               &secPair : prefData)
        for (const std::pair<const std::string, PrefValue> &kvPair :
             secPair.second)
          if (kvPair.second.inFile) fileKeyCnt++;
      std::cout << std::format("\nLoaded {} value(s) from {}\n", fileKeyCnt,
                               args.filePath);
    }
  } else if (args.verbMode) {
    std::cout << std::format("\nFile does not exist and will be created: {}\n",
                             args.filePath);
  }

  if (args.verbMode)
    std::cout << "\nVERBOSE MODE enabled (showing per-value before/after "
                 "changes):\n\n";

  // Pre-pass: analyze what would happen, print verbose lines if requested
  PrePassResult result = AnalyzeOp(args, prefData);

  // Report what will/would happen
  PrintPrePassSummary(args, result);

  if (args.testMode) return 0;

  // If nothing would change, skip prompt and exit cleanly
  if (result.changedCnt == 0) {
    std::cout << "\nNo changes to make -- exiting...\n";
    return 0;
  }

  // Confirm with user before making any changes
  if (!args.silentMode && !ConfirmOp(args)) {
    std::cout << "*** Operation cancelled ***\n";
    return 0;
  }

  // Dispatch — write passes only from here
  bool success = false;
  switch (args.op & OpActMask) {
  case OpActSave:
    success = SavePrefs(args, prefData);
    break;
  case OpActUpdate:
    success = UpdatePrefs(args, prefData);
    break;
  case OpActRestore:
    success = RestorePrefs(args, prefData);
    break;
  default:
    assert(0 && "\n*** Invalid operation ***\n");
  }

  if (!success) std::cout << "\n*** Operation failed ***\n";

  return success ? 0 : 1;
}
