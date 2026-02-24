================================================================================
                            QColorPrefs v1.7
        QSpice Color Preferences Backup and Restore Utility
================================================================================

OVERVIEW
--------
QColorPrefs is a command-line utility that lets you save, restore, and manage
your QSpice schematic and waveform color preferences. Share color themes with
colleagues, back up your settings before experiments, or maintain multiple
color schemes for different projects.

Features:
  * Save all color settings to portable text files
  * Restore settings from saved files
  * Selective save/restore (schematic or waveform only)
  * Update existing preference files with current registry values
  * Test mode to preview changes before applying them
  * Verbose mode to see exactly what changes
  * Silent mode (-s) suppresses confirmation prompts for use in batch files and Windows shortcut files
  * Human-readable file format with inline comments


FULL DOCUMENTATION AND SOURCE CODE
-----------------------------------
Complete documentation, source code, build files, and sample color themes:
  https://github.com/robdunn4/QSpice


INSTALLATION
------------
1. Download QColorPrefs.exe and sample *.qcolorpref sample themes from the GitHub repository
2. Copy QColorPrefs.exe to a convenient folder, such as:
   - C:\Tools\QColorPrefs\
   - C:\Program Files\QSpice\Tools\
   - Or any folder in your PATH

That's it! No installer needed.


IMPORTANT: FIRST USE
--------------------
Before experimenting with QColorPrefs or applying downloaded color themes,
create a backup of your current QSpice registry settings:

  QColorPrefs -sall my_current_colors

This saves all your current QSpice color preferences to a file called
"my_current_colors.qcolorpref" which you can restore later if needed.


COMMAND LINE SYNTAX
-------------------
Usage: QColorPrefs  [options] <action> <filepath>

<action>:
  -sall  Save all QSpice registry color settings to file
  -ssch  Save only schematic color settings to file
  -swav  Save only waveform color settings to file

  -uall  Update all existing entries in file with current registry values
  -usch  Update only existing schematic entries in file with current registry values
  -uwav  Update only existing waveform entries in file with current registry values

  -rall  Restore all QSpice registry color settings from file
  -rwav  Restore only waveform color settings from file
  -rsch  Restore only schematic color settings from file

[options]:
     -v  (Verbose Mode) Show per-value before/after changes
     -t  (Test Mode)    Show what would happen without making changes
     -s  (Silent Mode)  No user confirmation prompt (for use in batch files)

Examples:
  QColorPrefs -sall mycolors.qcolorpref
  QColorPrefs -uwav mycolors
  QColorPrefs -t -rall mycolors
  QColorPrefs -t -v -sall mycolors
  QColorPrefs -v -rwav mycolors

Note: If no file extension is provided, '.qcolorpref' will be added automatically.
      The -v option can be combined with any other option for verbose output.
      The -t option shows actions without making changes (dry-run mode).


USAGE EXAMPLES
--------------

SAVE YOUR CURRENT COLORS:
  QColorPrefs -sall my_colors
  Creates: my_colors.qcolorpref with all current color settings

SAVE ONLY SCHEMATIC COLORS:
  QColorPrefs -ssch schematic_dark_mode
  Creates: schematic_dark_mode.qcolorpref with only schematic colors

SAVE ONLY WAVEFORM COLORS:
  QColorPrefs -swav waveform_bright
  Creates: waveform_bright.qcolorpref with only waveform colors

RESTORE ALL COLORS FROM A FILE:
  QColorPrefs -rall my_colors.qcolorpref
  Restores all color settings from the file

RESTORE ONLY WAVEFORM COLORS:
  QColorPrefs -rwav waveform_bright.qcolorpref
  Updates only waveform colors, leaves schematic colors unchanged

RESTORE ONLY SCHEMATIC COLORS:
  QColorPrefs -rsch schematic_dark_mode.qcolorpref
  Updates only schematic colors, leaves waveform colors unchanged

UPDATE AN EXISTING FILE:
  QColorPrefs -uall my_colors.qcolorpref
  Refreshes the file with current registry values
  Only updates keys already in the file (preserves your custom subset)

TEST BEFORE RESTORING (RECOMMENDED):
  QColorPrefs -t -rall downloaded_theme.qcolorpref
  Shows what would change without actually changing anything
  Review the output, then run without -t if acceptable

VERBOSE MODE TO SEE DETAILS:
  QColorPrefs -v -sall my_backup
  Shows each value being saved with before/after comparison


TYPICAL WORKFLOWS
-----------------

WORKFLOW 1: Try a Downloaded Theme Safely
  1. Back up current settings:
     QColorPrefs -sall my_backup

  2. Preview the theme:
     QColorPrefs -t -rall cool_theme.qcolorpref

  3. If it looks good, apply it:
     QColorPrefs -rall cool_theme.qcolorpref

  4. If you don't like it, restore your backup:
     QColorPrefs -rall my_backup.qcolorpref


WORKFLOW 2: Experiment with Colors
  1. Save baseline:
     QColorPrefs -sall baseline

  2. Adjust colors in QSpice (Tools > Color Preferences)

  3. Save the experiment:
     QColorPrefs -sall experiment_01

  4. Try more changes, save again:
     QColorPrefs -sall experiment_02

  5. Restore whichever you prefer:
     QColorPrefs -rall experiment_01.qcolorpref


WORKFLOW 3: Maintain Custom Subset
  1. Save everything:
     QColorPrefs -sall full_backup

  2. Edit full_backup.qcolorpref in text editor
     Delete all lines except the 5-10 colors you actually customize
     Keep the header and section markers

  3. When you change colors in QSpice:
     QColorPrefs -uall full_backup.qcolorpref
     Only your custom subset gets updated

  4. Share your minimal file with colleagues
     Much smaller and easier to understand than full 45+ key files


WORKFLOW 4: Separate Schematic and Waveform Themes
  1. Save separately:
     QColorPrefs -ssch my_schematic_colors
     QColorPrefs -swav my_waveform_colors

  2. Mix and match:
     QColorPrefs -rsch dark_mode_schematic.qcolorpref
     QColorPrefs -rwav bright_waveforms.qcolorpref


UNDERSTANDING THE OUTPUT
-------------------------

SAVE OPERATIONS:
  "Saved 56 preference(s) to: mycolors.qcolorpref"
  All values written to file

UPDATE OPERATIONS:
  "Updated 3 preference(s) in: mycolors.qcolorpref"
  Shows how many values were written to the file

RESTORE OPERATIONS:
  "Restored 56 value(s) to registry"
  All values written to registry

TEST MODE:
  "TEST MODE: Would change 12 value(s) (33 unchanged)"
  Shows impact without making changes
  0 changed = no effect (registry already matches file)


FILE FORMAT
-----------
QColorPrefs files are simple text files you can view and edit:

  # This file was created by QColorPrefs v1.7 on February 18, 2026 at 05:00 PM
  # Generated automatically - do not edit unless you know what you're doing
  # Note that comments are not preserved when saving or updating from the registry
  # Registry path: HKEY_CURRENT_USER\Software\Marcus Aurelius Software LLC\QSPICE\[Preferences]

  [schematic]
  CADHighlightColor=0x326464            # Highlight Color
  CADBackgroundColor=0xe9ffff           # Background
  CADbomberSight=0x000000               # Schematic Grid Color
  JunctionSize=1.0                      # Junction Size
  CADlineWidth=1.5                      # CAD Line Width
  ...

  [waveform]
  NumberDataTraceColors=12              # Number of Trace Colors
  BackgroundColor=0x000000              # Background Color
  WaveFormViewerFontPoints=120          # Font Point Size
  WaveformPlotLineWidth=2.5             # Plot Line Thickness
  DataColor1=0x00ff00                   # Waveform Trace #1 Color
  DataColor2=0xff0000                   # Waveform Trace #2 Color
  ...

COLOR VALUES are in BGR (Blue-Green-Red) hexadecimal format:
  0x000000 = black
  0xffffff = white
  0x0000ff = red
  0x00ff00 = green
  0xff0000 = blue

NON-COLOR VALUES use decimal format:
  JunctionSize, CADlineWidth, BUSwidthMultiplier, DefaultTextSize  (e.g. 1.0, 1.5, 2.0)
  WaveFormViewerFontPoints                                          (e.g. 120)
  WaveformPlotLineWidth                                             (e.g. 2.5)
  NumberDataTraceColors                                             (e.g. 12, 24)

Note: NumberDataTraceColors controls how many trace colors QSpice cycles through.
Even if set to 12, all 24 DataColor entries are saved and restored by QColorPrefs.


EDITING .qcolorpref FILES
--------------------------
You can safely edit .qcolorpref files in any text editor. This is useful for:
  * Creating custom color themes by hand
  * Removing settings you don't want to change
  * Combining settings from multiple files
  * Adding comments to document your choices

COMMENTING OUT ENTRIES:
To prevent a setting from being restored to the registry, add # at the
beginning of the line to comment it out:

  [schematic]
  CADHighlightColor=0xff0000       # Highlight Color
  # CADBackgroundColor=0xffffff    # Background Color - KEEP MY CURRENT VALUE
  CADbomberSight=0x000000          # Schematic Grid Color

When you restore this file, CADBackgroundColor will be skipped and your
current registry value will be preserved.

COMMON SETTINGS TO COMMENT OUT:
Many users prefer to keep their personal preferences for these settings:

  # Font sizes (you may prefer different sizes):
  # WaveFormViewerFontPoints=10
  # DefaultTextSize=12

  # Line widths (personal preference):
  # WaveformPlotLineWidth=2
  # CADlineWidth=2

  # Junction/bus settings (may vary by use case):
  # JunctionSize=4
  # BUSwidthMultiplier=3

  # Number of trace colors (depends on your typical simulation size):
  # NumberDataTraceColors=24

By commenting these out, you can share a color theme that only affects
actual colors, not sizes or other non-color settings.


COMPLETE REGISTRY VALUE REFERENCE
----------------------------------
QColorPrefs manages the following registry values in:
HKEY_CURRENT_USER\Software\Marcus Aurelius Software LLC\QSPICE\[Preferences]


SCHEMATIC SECTION (24 values):

COLOR VALUES:
  CADHighlightColor          Highlight Color
  CADBackgroundColor         Background
  CADbomberSight             Schematic Grid Color
  CADgrapicAnnotation        Graphic Annotation Color
  CADsolidAnnotation         Solid Annotation Fill Color
  CADgrapicText              Symbol Graphic Text Color
  CADnormalText              Component Text Color
  CADnormalSchText           Schematic Text Color
  CADcommentText             Comment Text Color
  CADnetLabelText            Net Label Color
  CADwireColor               Wire Color
  CADhighlightWireColor      Highlight Wire Color
  CADpinDotColor             Pin Dot Color
  CADspritedAnchor           Sprited Anchor Color
  CADpinColor                Pin Color Color [sic]
  CADportBkColor             Port Background Color
  CADhierarchalBlock         Hierarchical Box Color
  CADhierarchalFill          Hierarchical Fill Color
  CADunconnectedPin          Unconnected Pin Color
  CADbusTapError             Incorrect Bus Tap Color

SIZE/DIMENSION VALUES:
  JunctionSize               Junction Size (numeric)
  CADlineWidth               CAD Line Width (numeric)
  BUSwidthMultiplier         Bus Width Multiplier (numeric)
  DefaultTextSize            Default Text Size (numeric)


WAVEFORM SECTION (32 values):

COLOR VALUES:
  BackgroundColor            Background Color
  FrameColor                 Ticks & Axis Color
  CursorLineColor            Cursor Color
  CursorBackFore             Attached Cursor Text Color
  CursorBackGnd              Attached Cursor Background
  
  DataColor1                 Waveform Trace #1 Color
  DataColor2                 Waveform Trace #2 Color
  DataColor3                 Waveform Trace #3 Color
  DataColor4                 Waveform Trace #4 Color
  DataColor5                 Waveform Trace #5 Color
  DataColor6                 Waveform Trace #6 Color
  DataColor7                 Waveform Trace #7 Color
  DataColor8                 Waveform Trace #8 Color
  DataColor9                 Waveform Trace #9 Color
  DataColor10                Waveform Trace #10 Color
  DataColor11                Waveform Trace #11 Color
  DataColor12                Waveform Trace #12 Color
  DataColor13                Waveform Trace #13 Color
  DataColor14                Waveform Trace #14 Color
  DataColor15                Waveform Trace #15 Color
  DataColor16                Waveform Trace #16 Color
  DataColor17                Waveform Trace #17 Color
  DataColor18                Waveform Trace #18 Color
  DataColor19                Waveform Trace #19 Color
  DataColor20                Waveform Trace #20 Color
  DataColor21                Waveform Trace #21 Color
  DataColor22                Waveform Trace #22 Color
  DataColor23                Waveform Trace #23 Color
  DataColor24                Waveform Trace #24 Color

SIZE/CONFIGURATION VALUES:
  NumberDataTraceColors      Number of Trace Colors (numeric)
  WaveFormViewerFontPoints   Font Point Size (numeric)
  WaveformPlotLineWidth      Plot Line Thickness (numeric)

TOTAL: 56 registry values (24 schematic + 32 waveform)


SAFETY FEATURES
---------------
* Prompts before overwriting existing files (unless the -s switch is used)
* Prompts before modifying registry (unless the -s switch is used)
* Test mode (-t) previews changes without applying them
* Selective restore prevents accidental changes to wrong section


TIPS AND BEST PRACTICES
------------------------

1. ALWAYS CREATE A BACKUP FIRST
   Before trying new themes or experimenting:
   QColorPrefs -sall my_backup

2. USE TEST MODE
   Preview changes before applying:
   QColorPrefs -t -rall theme.qcolorpref

3. CLOSE QSPICE BEFORE RESTORE
   QSpice must be closed for registry changes to take effect

4. USE DESCRIPTIVE FILENAMES
   Good: dark_mode_green_traces.qcolorpref
   Bad: colors1.qcolorpref

5. VERSION CONTROL YOUR THEMES
   Store .qcolorpref files in Git for history tracking

6. SHARE WITH COLLEAGUES
   Email .qcolorpref files to share your color schemes

7. KEEP A "DEFAULTS" FILE
   Save QSpice's default colors before customizing:
   QColorPrefs -sall qspice_defaults


TROUBLESHOOTING
---------------

"Error: Could not open registry key"
  → QSpice may not be installed, or registry permissions issue
  → Try running as administrator

"Error: File not found" (on restore or update)
  → Restore and update require an existing file
  → Use -sall to create a new file first

"Restored 56 value(s) to registry" but colors didn't change
  → Close and reopen QSpice
  → Registry changes take effect when QSpice starts

Test mode shows "Would change 0 value(s)"
  → File already matches current registry
  → No restore needed (already up to date)


SYSTEM REQUIREMENTS
-------------------
* Windows 10 or later
* QSpice installed
* No administrator rights required (for normal use)


VERSION HISTORY
---------------
v1.7 - February 2026
  * Updated save/restore counts to reflect all 56 managed registry values
  * Minor output and documentation improvements

v1.6 - February 2026
  * Improved file-not-found error handling for restore and update operations
  * Consolidated error output to stdout
  * Simplified GetOperationDescription() logic
  * Internal refactoring and code cleanup

v1.5 - February 2026
  * Added silent mode (-s) for batch file use
  * Performance improvements

v1.4 - February 2026
  * Added update operations (-uall, -usch, -uwav)
  * Added test mode (-t) for safe previewing
  * Added verbose mode (-v) for detailed output
  * Improved header with version and timestamp
  * Per-section alignment in verbose output
  * Changed-count reporting in test mode

v1.2 - February 2026
  * Added selective save (-ssch, -swav)
  * Added inline comments in preference files
  * Verbose mode improvements

v1.1 - February 2026
  * Added selective restore (-rwav, -rsch)
  * Improved user feedback

v1.0 - February 2026
  * Initial release
  * Basic save and restore functionality


LICENSE
-------
QColorPrefs is licensed under the GNU General Public License v3.0.


SUPPORT AND CONTRIBUTIONS
--------------------------
Report issues, request features, or contribute improvements:
  https://github.com/robdunn4/QSpice

Questions or suggestions? Open an issue on GitHub.


QUICK REFERENCE
---------------
First time:    QColorPrefs -sall my_backup
Preview:       QColorPrefs -t -rall theme.qcolorpref
Apply theme:   QColorPrefs -rall theme.qcolorpref
Undo:          QColorPrefs -rall my_backup.qcolorpref
Update file:   QColorPrefs -uall my_backup.qcolorpref
Details:       Add -v flag to any command


================================================================================
Happy color customizing!
================================================================================
