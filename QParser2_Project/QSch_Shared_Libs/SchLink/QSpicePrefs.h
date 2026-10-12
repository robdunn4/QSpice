#pragma once

#include <QHash>
#include <QList>
#include <QPair>
#include <QString>
#include <QStringList>
#include <QtGlobal>

// QSpice's own colours and CAD settings, READ from the Windows registry
// at startup. Read-only: the one value ever written back goes through
// PrefsWriter (QSpicePrefsWriter.h), deliberately a separate header.
//
// These are the values QSpice itself draws with, under
//   HKEY_CURRENT_USER\Software\Marcus Aurelius Software LLC\QSPICE\[Preferences]
//
// Why they matter: an element's ArgColor can say "use this element
// type's default colour" (the 0x01 high byte -- see SchColorDecode). The
// real defaults live here.
//
// COLOURS ARE RETURNED RAW, as 0xBBGGRR integers exactly as QSpice
// stores them, so this library needs Qt Core only. Turning them into
// QColor is the drawing side's job.
//
// BYTE ORDER is 0xBBGGRR -- the same convention ArgColor uses, NOT the
// 0xRRGGBB an HTML-coloured eye reads. Confirmed three ways against
// QSpice screenshots that had already been measured pixel-by-pixel:
//   CADBackgroundColor 0xe9ffff -> RGB(255,255,233), exactly the
//     background measured from QSpice's own captures
//   CADwireColor       0xff0000 -> pure blue, matching its blue wires
//   CADnetLabelText    0x0000ff -> pure red, matching its red net labels
// Read as 0xRRGGBB, all three would be wrong (and two of them
// spectacularly so), so this is not a coin flip.
//
// Values are stored in the registry as STRINGS ("0xff0000", "1.5",
// "true"), not as DWORDs.
namespace sch::link::prefs {

// Reads QSpice's [Preferences] key, built from the VENDOR key
// ("HKEY_CURRENT_USER\Software\Marcus Aurelius Software LLC" on a stock
// machine). Call once, before anything is drawn.
//
// Safe when the values are absent (a machine without QSpice installed,
// or a non-Windows build): every accessor then returns the documented
// fallback, which is the value from a stock QSpice installation.
void load(const QString &vendorKey);

// True if the registry key was found and read. Worth checking before
// telling the user their QSpice colours are in use.
bool loaded();

// For a program that may run WITHOUT QSpice: values for any name the
// registry did NOT supply, in the registry's own names and text forms
// ("CADwireColor" = "0xff0000", "CADlineWidth" = "1.5") -- read, say,
// from the program's own settings file. Call after load().
//
// The registry always wins: a name it supplied is left alone. A name in
// neither keeps the stock fallback. loaded() is unchanged -- it still
// says whether the REGISTRY was read.
void fillMissing(const QHash<QString, QString> &fallbackValues);

// Every value the drawing side reads -- the colours and metrics below,
// not FastMath or the symbol folders -- as (registry name, stock text),
// in the order listed below. The stock text is what a stock QSpice
// installation holds, and what each accessor falls back to. For writing
// a settings file's defaults.
QList<QPair<QString, QString>> stockDrawingValues();

// The full registry path load() read, "<vendor key>\QSPICE\[Preferences]";
// empty before load().
const QString &keyPath();

// The [Preferences] key under a vendor key.
QString keyPathFor(const QString &vendorKey);

// --- Schematic colours, 0xBBGGRR ---
quint32 highlightColor();     // CADHighlightColor
quint32 backgroundColor();    // CADBackgroundColor
quint32 gridColor();          // CADbomberSight
quint32 graphicAnnotation();  // CADgrapicAnnotation  -- symbol line work
quint32 solidAnnotationFill();// CADsolidAnnotation   -- shape fill
quint32 graphicText();        // CADgrapicText        -- text inside symbols
quint32 componentText();      // CADnormalText        -- refdes/value labels
quint32 schematicText();      // CADnormalSchText     -- top-level text
quint32 commentText();        // CADcommentText
quint32 netLabelText();       // CADnetLabelText
quint32 wireColor();          // CADwireColor
quint32 highlightWireColor(); // CADhighlightWireColor
quint32 pinDotColor();        // CADpinDotColor
quint32 spritedAnchor();      // CADspritedAnchor
quint32 pinColor();           // CADpinColor
quint32 portBkColor();        // CADportBkColor
quint32 hierarchicalBlock();  // CADhierarchalBlock   [sic -- QSpice's spelling]
quint32 hierarchicalFill();   // CADhierarchalFill    [sic]
quint32 unconnectedPin();     // CADunconnectedPin
quint32 busTapError();        // CADbusTapError

// --- Schematic metrics ---
double junctionSize();       // JunctionSize
double cadLineWidth();       // CADlineWidth
double busWidthMultiplier(); // BUSwidthMultiplier
double defaultTextSize();    // DefaultTextSize

// --- Simulation ---

// FastMath, "true" or "false": which simulator QSpice runs -- QSPICE64.exe
// for true, QSPICE80.exe (80-bit extended precision) for false. True when
// the value is absent, as in a stock installation.
bool fastMath();

// --- Symbol browser ---

// UserSymbolDirectories: the folders the user added to QSpice's Symbol
// & IP browser, in the order QSpice lists them.
//
// Stored as ONE string, each path enclosed in vertical bars:
//   |C:\Dev\QSpice\TAE_Libs\Symbols||C:\Program Files\QSPICE\EPC|
// Empty pieces between bars are skipped, and a path listed twice is kept
// only the first time -- see sameDirectory().
//
// Read from the registry ONCE, at load(). After that this returns OUR
// copy, as last set through PrefsWriter; changes QSpice makes while we
// are running are not seen.
QStringList userSymbolDirectories();

// Whether two folder paths name the same folder, as far as the text
// shows: case-insensitive, either separator, trailing separator ignored.
// Does not resolve links or relative paths.
bool sameDirectory(const QString &a, const QString &b);

// Parses one "0xBBGGRR" string. False, and bgr untouched, if it is not
// one. Exposed for values this module doesn't wrap in a named accessor.
bool bgrFromString(const QString &text, quint32 &bgr);

} // namespace sch::link::prefs
