#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable : 4996) // Qt headers reference APIs deprecated for Qt 7
#endif

#include "ViewerPrefs.h"

#include <QCoreApplication>
#include <QDir>
#include <QFileInfo>
#include <QList>
#include <QPair>
#include <QSettings>

#ifdef _MSC_VER
#pragma warning(pop)
#endif

#include "IniValues.h"
#include "QSpicePrefs.h"
#include "SchPrint.h"
#include "Version.h"

// SchApp names used in this file.
namespace ini = sch::app::ini;
using sch::app::ini::prefBool;
using sch::app::ini::prefDouble;
using sch::app::ini::prefValue;
using sch::app::ini::setPrefBool;
using sch::app::ini::setPrefValue;

const QString ViewerPrefs::kDefaultQInstallDirKey =
    QStringLiteral("HKEY_CURRENT_USER\\Software\\Marcus Aurelius Software LLC");

namespace {

constexpr const char *kQSpiceGroup        = "QSpice";
constexpr const char *kQInstallDirKeyKey  = "QInstallDirKey";
constexpr const char *kViewGroup          = "View";
constexpr const char *kMinZoomScaleKey    = "MinScale";
constexpr const char *kMaxZoomScaleKey    = "MaxScale";
constexpr const char *kNewViewWidthKey    = "NewViewWidth";

// The same key QSchEdit uses (there in [Display]).
constexpr const char *kShowResistorOrientKey = "ShowResistorOrientation";
constexpr const char *kExportGroup        = "Export";
constexpr const char *kSvgMinLineKey      = "SvgMinLineWidthMm";
constexpr const char *kColorsGroup        = "Colors";
constexpr const char *kFilesGroup         = "Files";
constexpr const char *kMaxRecentFilesKey  = "MaxRecentFiles";
constexpr const char *kRecentFilesGroup   = "RecentFiles";
constexpr const char *kWindowGroup        = "Window";
constexpr const char *kGeometryKey        = "Geometry";

} // namespace

QString ViewerPrefs::programFolder() {
  // ==================================================================
  // TEMPORARY -- FOR THE AUTHOR'S DEVELOPMENT ENVIRONMENT ONLY, as in
  // QSchEdit. Debug builds keep their files TWO FOLDER LEVELS ABOVE the
  // executable (<project>\x64\<Configuration>\QSchView.exe), so every
  // debug configuration shares one copy. Release builds are unaffected.
  // Remove before distribution.
  // ==================================================================
#ifdef _DEBUG
  return QDir::cleanPath(QCoreApplication::applicationDirPath() + "/../..");
#else
  return QCoreApplication::applicationDirPath();
#endif
}

QString ViewerPrefs::iniFilePath() {
  return QDir(programFolder())
      .filePath(QString::fromLatin1(Version::kProgramName) + QStringLiteral(".ini"));
}

QString ViewerPrefs::helpFilePath() {
  return QDir(programFolder())
      .filePath(QString::fromLatin1(Version::kProgramName) + QStringLiteral("_Help.pdf"));
}

void ViewerPrefs::load() {
  QSettings settings(iniFilePath(), QSettings::IniFormat);

  settings.beginGroup(kQSpiceGroup);
  qInstallDirKey_ = prefValue(settings, kQInstallDirKeyKey, kDefaultQInstallDirKey);
  settings.endGroup();

  const sch::render::ViewOptions defaults;

  // No grid: a viewer shows the drawing, nothing else.
  viewOptions_.gridVisible = false;

  settings.beginGroup(kViewGroup);
  viewOptions_.minZoomScale = prefDouble(settings, kMinZoomScaleKey, defaults.minZoomScale);
  viewOptions_.maxZoomScale = prefDouble(settings, kMaxZoomScaleKey, defaults.maxZoomScale);
  viewOptions_.newViewWidth = prefDouble(settings, kNewViewWidthKey, defaults.newViewWidth);
  showResistorOrient_ =
      prefBool(settings, kShowResistorOrientKey, kDefaultShowResistorOrient);
  settings.endGroup();

  settings.beginGroup(kExportGroup);
  svgMinLineMm_ = prefDouble(settings, kSvgMinLineKey, kDefaultSvgMinLineMm);
  settings.endGroup();

  // 1 to 99; anything else from a hand edit falls back to the default.
  settings.beginGroup(kFilesGroup);
  bool      ok    = false;
  const int count = prefValue(settings, kMaxRecentFilesKey,
                              QString::number(kDefaultMaxRecentFiles)).toInt(&ok);
  maxRecentFiles_ = (ok && count >= 1 && count <= kMaxMaxRecentFiles) ? count
                                                                      : kDefaultMaxRecentFiles;
  settings.endGroup();
  if (!(svgMinLineMm_ >= 0.0)) svgMinLineMm_ = kDefaultSvgMinLineMm;

  // Nonsense from a hand edit falls back rather than breaking the view.
  if (viewOptions_.minZoomScale <= 0.0) viewOptions_.minZoomScale = defaults.minZoomScale;
  if (viewOptions_.maxZoomScale <= viewOptions_.minZoomScale)
    viewOptions_.maxZoomScale = defaults.maxZoomScale;
  if (!(viewOptions_.newViewWidth > 0.0)) viewOptions_.newViewWidth = defaults.newViewWidth;

  // Only what the user changed: an empty value means "the default", and
  // the default is QSpice's own (or its stock value), not ours.
  colorValues_.clear();
  settings.beginGroup(kColorsGroup);
  for (const QPair<QString, QString> &stock : sch::link::prefs::stockDrawingValues()) {
    const QByteArray key   = stock.first.toLatin1();
    const QString    value = prefValue(settings, key.constData(), QString());
    if (!value.isEmpty()) colorValues_.insert(stock.first, value);
  }
  settings.endGroup();
}

void ViewerPrefs::writeMissingDefaults() {
  QSettings              settings(iniFilePath(), QSettings::IniFormat);
  const ini::OnlyMissing scope;

  settings.beginGroup(kQSpiceGroup);
  setPrefValue(settings, kQInstallDirKeyKey, qInstallDirKey_, kDefaultQInstallDirKey);
  settings.endGroup();

  const sch::render::ViewOptions defaults;

  settings.beginGroup(kViewGroup);
  setPrefValue(settings, kMinZoomScaleKey, QString::number(viewOptions_.minZoomScale),
               QString::number(defaults.minZoomScale));
  setPrefValue(settings, kMaxZoomScaleKey, QString::number(viewOptions_.maxZoomScale),
               QString::number(defaults.maxZoomScale));
  setPrefValue(settings, kNewViewWidthKey, QString::number(viewOptions_.newViewWidth),
               QString::number(defaults.newViewWidth));
  setPrefBool(settings, kShowResistorOrientKey, showResistorOrient_,
              kDefaultShowResistorOrient);
  settings.endGroup();

  settings.beginGroup(kFilesGroup);
  setPrefValue(settings, kMaxRecentFilesKey, QString::number(maxRecentFiles_),
               QString::number(kDefaultMaxRecentFiles));
  settings.endGroup();

  settings.beginGroup(kExportGroup);
  setPrefValue(settings, kSvgMinLineKey, QString::number(svgMinLineMm_),
               QString::number(kDefaultSvgMinLineMm));
  settings.endGroup();

  // The stock QSpice values, as defaults to override by hand. They apply
  // only to names QSpice's own settings do not supply.
  settings.beginGroup(kColorsGroup);
  for (const QPair<QString, QString> &stock : sch::link::prefs::stockDrawingValues()) {
    const QByteArray key = stock.first.toLatin1();
    setPrefValue(settings, key.constData(), colorValues_.value(stock.first, stock.second),
                 stock.second);
  }
  settings.endGroup();

  settings.sync();
}

void ViewerPrefs::setShowResistorOrient(bool show) {
  showResistorOrient_ = show;

  QSettings settings(iniFilePath(), QSettings::IniFormat);
  settings.beginGroup(kViewGroup);
  setPrefBool(settings, kShowResistorOrientKey, show, kDefaultShowResistorOrient);
  settings.endGroup();
  settings.sync();
}

sch::output::SchPrintSettings ViewerPrefs::printDefaults() {
  sch::output::SchPrintSettings defaults;
  defaults.mode            = sch::output::SchPrintSettings::Mode::WholeScale;
  defaults.autoOrient      = true;
  defaults.printBackground = false;
  return defaults;
}

sch::output::SchPrintSettings ViewerPrefs::storedPrintSettings() {
  sch::output::SchPrintSettings stored = printDefaults();
  stored.settingsFile                  = iniFilePath();
  stored.load();
  return stored;
}

QStringList ViewerPrefs::recentFiles() const {
  QSettings   settings(iniFilePath(), QSettings::IniFormat);
  QStringList list = ini::readPaths(settings, kRecentFilesGroup);

  // A limit lowered by hand shows at once; the file is trimmed at the
  // next addition.
  while (list.size() > maxRecentFiles_) list.removeLast();
  return list;
}

void ViewerPrefs::addRecentFile(const QString &path) {
  QSettings   settings(iniFilePath(), QSettings::IniFormat);
  QStringList list = ini::readPaths(settings, kRecentFilesGroup);
  ini::removePath(list, path);
  list.prepend(QDir::toNativeSeparators(QFileInfo(path).absoluteFilePath()));
  while (list.size() > maxRecentFiles_) list.removeLast();
  ini::writePaths(settings, kRecentFilesGroup, list);
}

void ViewerPrefs::removeRecentFile(const QString &path) {
  QSettings   settings(iniFilePath(), QSettings::IniFormat);
  QStringList list = ini::readPaths(settings, kRecentFilesGroup);
  ini::removePath(list, path);
  ini::writePaths(settings, kRecentFilesGroup, list);
}

QByteArray ViewerPrefs::windowGeometry() const {
  QSettings settings(iniFilePath(), QSettings::IniFormat);
  return settings.value(QStringLiteral("%1/%2").arg(kWindowGroup, kGeometryKey)).toByteArray();
}

void ViewerPrefs::setWindowGeometry(const QByteArray &geometry) {
  QSettings settings(iniFilePath(), QSettings::IniFormat);
  settings.setValue(QStringLiteral("%1/%2").arg(kWindowGroup, kGeometryKey), geometry);
  settings.sync();
}
