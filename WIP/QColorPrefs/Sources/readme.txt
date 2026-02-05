================================================================================
QColorPrefs - A QSpice Color Settings Backup and Restore Utility
================================================================================

OVERVIEW
--------
QColorPrefs is a command-line utility that saves/restores QSpice color settings.
The preferences are saved in a human-readable text format that can be easily
edited, shared, or version controlled.


COMMAND LINE SYNTAX
-------------------
QColorPrefs <option> <filepath>

Options:
  -s, -save      Save registry values to a file
  -r, -restore   Restore registry values from a file

The filepath can be specified with or without an extension. If no extension
is provided, ".qcolorpref" will be automatically appended.


USAGE EXAMPLES
--------------
Save registry values to a file:
  QColorPrefs -save mycolors
  QColorPrefs -s C:\Backup\settings

Restore registry values from a file:
  QColorPrefs -restore mycolors.qcolorpref
  QColorPrefs -r C:\Backup\settings.qcolorpref


SAFETY FEATURES
---------------
* Save operation: If the target file already exists, you will be prompted to
  confirm whether you want to overwrite it.

* Restore operation: Before writing any values to the registry, you will be
  prompted to confirm that you want to proceed.


PREFERENCES FILE FORMAT
-----------------------
The preferences file is a simple text file with the following format:

  # Comment lines start with #
  # Registry path: HKEY_CURRENT_USER\Software\Marcus Aurelius Software LLC\QSPICE\Preferences
  
  [schematic]
  CADHighlightColor=0x326464
  CADBackgroundColor=0xe9ffff
  CADbomberSight=0x000000
  ...
  
  [waveform]
  NumberDataTraceColors=10
  DataColor1=0x008000
  DataColor2=0xff3f00
  ...

* Lines beginning with "#" are comments and are ignored
* Blank lines are ignored
* Key names correspond to string (REG_SZ) value names in the registry
* All values, regardless of section, are stored in the same registry location
* Sections are purely for organizational purposes in the preferences file


CONFIGURING REGISTRY VALUES
---------------------------
Before using QColorPrefs, you must configure which registry values to save
and restore by editing the source code file QColorPrefs.cpp.

All registry values are stored in:
  HKEY_CURRENT_USER\Software\Marcus Aurelius Software LLC\QSPICE\Preferences

Locate the g_registryValues vector (around line 22) and add the values you
want to backup, organized by section:

  std::vector<RegistryValue> g_registryValues = {
      // Schematic-related values
      {"schematic", "ColorBackground"},
      {"schematic", "ColorWire"},
      {"schematic", "ColorComponent"},
      
      // Waveform-related values
      {"waveform", "ColorTrace1"},
      {"waveform", "ColorTrace2"},
      {"waveform", "ColorGrid"},
  };

Each entry consists of:
  - Section name (e.g., "schematic", "waveform") - used to organize the 
    preferences file
  - Value name - the name of the REG_SZ (string) value in the registry

The section names are purely for organization in the preferences file and
help keep related settings grouped together. All values are still read from
and written to the same registry location.

After adding your values, recompile the program.


COMPILATION
-----------
To compile QColorPrefs, you need a C++11 compatible compiler. Using Microsoft
Visual C++ compiler:

  cl QColorPrefs.cpp /EHsc /std:c++11

Or using MinGW:

  g++ -std=c++11 QColorPrefs.cpp -o QColorPrefs.exe


SYSTEM REQUIREMENTS
-------------------
* Windows 11 (may work on Windows 10)
* Administrator privileges may be required for certain registry keys
* C++11 compatible compiler for building from source


TROUBLESHOOTING
---------------
"Error: Could not open registry key"
  - The QSPICE Preferences registry key does not exist, or you lack permission
    to access it. The key should be at:
    HKEY_CURRENT_USER\Software\Marcus Aurelius Software LLC\QSPICE\Preferences

"Error: Could not read registry value"
  - The value name is incorrect or the value does not exist in the registry.
    Make sure the value exists in the QSPICE Preferences key.

"Error: Registry value is not a string (REG_SZ)"
  - QColorPrefs only supports string (REG_SZ) registry values. The specified
    value is of a different type (such as DWORD, BINARY, etc.).

"Error: No registry values configured to save"
  - You need to add values to the g_registryValues vector in the source
    code and recompile.

"Warning: Section 'xyz' not found in file"
  - During restore, a section referenced in the code was not found in the
    preferences file. This may be normal if you're restoring an older or
    partial backup.


IMPORTANT NOTES
---------------
* BACKUP YOUR REGISTRY: Always create a full registry backup before using
  the restore function. Incorrect registry modifications can make your system
  unstable or unbootable.

* This utility works only with REG_SZ (string) registry values.

* Some registry changes may require logging out and back in, or restarting
  your computer to take effect.

* Administrative privileges may be required to modify certain registry keys.


LICENSE
-------
QColorPrefs is licensed under the GNU General Public License v3.0.
See license.txt for details.


VERSION
-------
Version 1.0
