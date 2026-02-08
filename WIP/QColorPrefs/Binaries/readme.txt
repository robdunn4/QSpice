================================================================================
QColorPrefs - QSpice Color Preferences Backup and Restore Utility
================================================================================

OVERVIEW
--------
QColorPrefs is a command-line utility to save and restore QSpice color 
preferences.  You can save the current QSpice schematic and waveform color
settings in a text-based preferences file and restore them to the Windows
Registry.  The preferences are saved in a human-readable text format that can 
be easily edited, shared, or version controlled.

The utility supports selective restore operations, allowing you to restore
only schematic colors or only waveform colors independently, making it easy
to apply specific color schemes without affecting other settings.


GETTING STARTED
---------------
IMPORTANT: Before experimenting with QColorPrefs or restoring any color 
schemes, you should first create a backup of your current settings:

  QColorPrefs -sall my_current_settings

This will save all your current QSpice color preferences to a file called
"my_current_settings.qcolorpref" that you can restore later if needed.


COMMAND LINE SYNTAX
-------------------
QColorPrefs [options] <filepath>

Options:
  -sall      Save all registry values to a *.qcolorpref text file
  -ssch      Save only schematic registry values to a *.qcolorpref text file
  -swav      Save only waveform registry values to a *.qcolorpref text file
  -rall      Restore all registry saved color settings from *.qcolorpref file 
             (both schematic and waveform)
  -rwav      Restore only waveform color settings from *.qcolorpref file
  -rsch      Restore only schematic color settings from *.qcolorpref file
  -v         Verbose mode - show detailed warnings and messages

The filepath can be specified with or without an extension. If no extension
is provided, ".qcolorpref" will be automatically appended.

The -v option can be combined with any other option for detailed output.


USAGE EXAMPLES
--------------
Save all registry values to a file:
  QColorPrefs -sall mycolors
  QColorPrefs -sall C:\Backup\settings

Save only schematic values to a file:
  QColorPrefs -ssch schematic_colors
  
Save only waveform values to a file:
  QColorPrefs -swav waveform_colors

Restore all registry values from a file:
  QColorPrefs -rall mycolors.qcolorpref
  QColorPrefs -rall C:\Backup\settings.qcolorpref

Restore only waveform values from a file:
  QColorPrefs -rwav mycolors.qcolorpref

Restore only schematic values from a file:
  QColorPrefs -rsch mycolors.qcolorpref

Use verbose mode to see detailed warnings:
  QColorPrefs -v -sall mycolors
  QColorPrefs -v -rall mycolors.qcolorpref


SAFETY FEATURES
---------------
* Save operation: If the target file already exists, you will be prompted to
  confirm whether you want to overwrite it.

* Restore operation: Before writing any values to the registry, you will be
  prompted to confirm that you want to proceed. You will also be reminded to
  close QSpice before continuing.

* Selective save: You can save only schematic colors (-ssch) or only
  waveform colors (-swav) to create specialized backup files.

* Selective restore: You can restore only schematic colors (-rsch) or only
  waveform colors (-rwav) without affecting the other section's settings.

* Verbose mode: Use the -v option to see detailed warnings about missing
  registry values or preferences file entries. Without -v, the program runs
  quietly and only reports errors.


PREFERENCES FILE FORMAT
-----------------------
The preferences file is a simple text file with the following format:

  # Comment lines start with #
  
  [schematic]
  CADHighlightColor=0x326464       # Highlight Color
  CADBackgroundColor=0xe9ffff      # Background
  CADbomberSight=0x000000          # Schematic Grid Color
  ...

  [waveform]
  NumberDataTraceColors=12         # Number of Trace Colors
  CursorBackFore=0xffffff          # Attached Cursor Text Color
  CursorBackGnd=0x0000ff           # Attached Cursor Background
  ...

* Lines beginning with "#" are comments and are ignored
* Blank lines are ignored
* Section names (like [schematic] and [waveform]) organize related settings
* Inline comments (after #) provide descriptions and are ignored during restore
* Colors are specified as three-byte hex values in BGR order.  For example, 
  0x000000 is black, 0xffffff is white, and 0x0000ff is pure red.


SYSTEM REQUIREMENTS
-------------------
* Windows 11 (may work on Windows 10)
* Administrator privileges may be required for certain registry keys


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

"Warning: Section 'xyz' not found in file"
  - During restore, a section referenced in the code was not found in the
    preferences file. This may be normal if you're restoring an older or
    partial backup.


PROJECT INFORMATION
-------------------
Complete documentation, source code, and Microsoft Visual Studio build files 
for this project are available at:  https://github.com/robdunn4/QSpice.


LICENSE
-------
QColorPrefs is licensed under the GNU General Public License v3.0.
See license.txt for details.


VERSION
-------
Version 1.2
