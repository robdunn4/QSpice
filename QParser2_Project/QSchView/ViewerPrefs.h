#pragma once

#include <QByteArray>
#include <QHash>
#include <QString>
#include <QStringList>

#include "SchViewBase.h"

namespace sch::output { struct SchPrintSettings; }

// QSchView's settings: QSchView.ini, beside the executable, separate from
// QSchEdit's.
//
// Every setting is written to the file even when it holds its default,
// so the file shows what exists and how each key is spelled:
//
//     MinScale=[] // default: 0.02
//     MinScale=0.05 // default: 0.02
//
// "[]" (or an empty value) means "the default"; anything else overrides
// it. Only MISSING keys are ever written by writeMissingDefaults(): the
// file is shared by every running instance and may be edited by hand.
//
// Sections:
//   [QSpice]       QInstallDirKey -- where QSpice's registry settings are
//   [View]         zoom, and the resistor orientation marker (no grid: a
//                  viewer shows the drawing alone)
//   [Print]        what the Print dialog opens with. Written ONLY by
//                  Options > Printing Defaults and Headers/
//                  Footers (and the Print dialog's Configure..., when
//                  asked); this program's defaults differ from SchPrint's
//                  -- see printDefaults()
//   [Export]       SVG export's own settings -- independent of printing
//   [Colors]       QSpice's colour and metric names, used only for names
//                  QSpice's own settings do not supply (QSpice wins)
//   [Log]          MaxSizeKB -- read by SchApp's log (sch::app::log::
//                  configure()), not here
//   [Files]        MaxRecentFiles -- how long the recent list may grow
//   [RecentFiles]  the shared recent list
//   [Window]       the window's size and position
class ViewerPrefs {
public:
  // The folder QSchView keeps its files in: beside the executable -- or,
  // in a DEBUG build, two folders above it (the project folder), so every
  // debug configuration shares one copy. TEMPORARY development
  // arrangement; see iniFilePath().
  static QString programFolder();

  static QString iniFilePath();

  // The help document, QSchView_Help.pdf, in programFolder(). In a release
  // build it is copied there by hand (or by the deploy step).
  static QString helpFilePath();

  // Reads everything from the file. Missing or "[]" values take their
  // defaults.
  void load();

  // Adds every setting the file does not have yet, at its default. Never
  // touches a key that is already there.
  void writeMissingDefaults();

  // --- [QSpice]
  const QString &qInstallDirKey() const noexcept { return qInstallDirKey_; }

  // --- [View]
  sch::render::ViewOptions viewOptions() const { return viewOptions_; }

  // A small "+" beside a resistor's first pin, as QSchEdit's option of the
  // same name. Shown on screen, in print and in exported SVG. On by
  // default here (off in QSchEdit). Options > Show Resistor Polarity
  // sets it, and writes it to the file at once.
  bool showResistorOrient() const noexcept { return showResistorOrient_; }
  void setShowResistorOrient(bool show);

  // --- [Print]
  //
  // The print settings this program starts from where the file has no
  // value: SchPrint's own, except the whole schematic at scale, the
  // orientation needing fewer pages, and WITHOUT the canvas background --
  // a viewer's prints are for paper.
  static sch::output::SchPrintSettings printDefaults();

  // What the file holds now, over printDefaults(): what every Print
  // dialog opens with. Read afresh each call -- another instance may have
  // saved since.
  static sch::output::SchPrintSettings storedPrintSettings();

  // --- [Export]
  //
  // The thinnest line in an exported SVG, in mm. SVG export takes nothing
  // from the print settings (its scale is fixed at 25.4 mm per 1000
  // units); this is its only setting.
  double svgMinLineMm() const noexcept { return svgMinLineMm_; }
  static constexpr double kDefaultSvgMinLineMm = 0.1;

  // --- [Colors]
  //
  // The values the user set, by QSpice name ("CADwireColor" ->
  // "0xff0000"); defaults ("[]") are left out. For
  // sch::link::prefs::fillMissing().
  const QHash<QString, QString> &colorValues() const noexcept { return colorValues_; }

  // --- [RecentFiles]
  //
  // Shared by every instance: each call re-reads the file, so a list
  // shown is never stale. At most maxRecentFiles() entries -- [Files]
  // MaxRecentFiles, not in [RecentFiles], which is rewritten whole.
  static constexpr int kDefaultMaxRecentFiles = 20;
  static constexpr int kMaxMaxRecentFiles     = 99;

  int maxRecentFiles() const noexcept { return maxRecentFiles_; }

  QStringList recentFiles() const;
  void        addRecentFile(const QString &path);
  void        removeRecentFile(const QString &path);

  // --- [Window]
  QByteArray windowGeometry() const;
  void       setWindowGeometry(const QByteArray &geometry);

  static const QString kDefaultQInstallDirKey;

  static constexpr bool kDefaultShowResistorOrient = true;

protected:
  QString                  qInstallDirKey_ = kDefaultQInstallDirKey;
  sch::render::ViewOptions viewOptions_;
  bool                     showResistorOrient_ = kDefaultShowResistorOrient;
  double                   svgMinLineMm_       = kDefaultSvgMinLineMm;
  int                      maxRecentFiles_     = kDefaultMaxRecentFiles;
  QHash<QString, QString>  colorValues_;
};
