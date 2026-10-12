#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable : 4996) // Qt headers reference APIs deprecated for Qt 7
#endif

#include <QApplication>
#include <QDebug>
#include <QStringList>

#ifdef _MSC_VER
#pragma warning(pop)
#endif

#include "AppLog.h"
#include "CliRun.h"
#include "QSpiceInstall.h"
#include "QSpicePrefs.h"
#include "QSpiceStyle.h"
#include "RenderStyle.h"
#include "SchItemBase.h"
#include "Version.h"
#include "ViewerPrefs.h"
#include "ViewerWin.h"

// SchRender names used in this file.
using sch::render::RenderStyle;

// SchApp names used in this file.
using sch::app::qspiceRenderStyle;

namespace {

// QSpice is OPTIONAL here, unlike in QSchEdit: QSchView only reads and
// draws, and the schematic carries its own symbols. So nothing is
// required of the installation; it is only where the colours come from.
//
// QSpice's own settings win, name by name. Any value they do not supply
// -- all of them, on a machine without QSpice -- comes from [Colors] in
// QSchView.ini, and failing that from a stock QSpice installation's.
void loadDrawingStyle(const ViewerPrefs &prefs) {
  const QStringList problems = sch::link::install::verify(prefs.qInstallDirKey());
  if (problems.isEmpty())
    qInfo("QSpice installation found");
  else
    qInfo().noquote() << "QSpice installation not found -- not needed:"
                      << problems.join(QStringLiteral("; "));

  sch::link::prefs::load(prefs.qInstallDirKey());
  sch::link::prefs::fillMissing(prefs.colorValues());
  RenderStyle::setCurrent(qspiceRenderStyle());
}

} // namespace

int main(int argc, char *argv[]) {
  QApplication application(argc, argv);

  sch::app::log::install(QString::fromLatin1(Version::kProgramName));

  // The log file only if the .ini asks for one ([Log] MaxSizeKB): off by
  // default in a release build, 1 MB in a debug build. Messages so far
  // were held, and are written or dropped now.
  constexpr int kDefaultLogKB = Version::kIsDebugBuild ? 1024 : 0;
  sch::app::log::configure(ViewerPrefs::iniFilePath(), kDefaultLogKB);
  qInfo().noquote() << QString::fromStdString(Version::winTitleStr()) << "started";

  ViewerPrefs prefs;
  prefs.load();

  // So every setting appears in the file the first time, where it can be
  // found and edited. Only missing ones: other instances may be running.
  prefs.writeMissingDefaults();

  loadDrawingStyle(prefs);

  // Before anything is drawn: the window, print and SVG export all paint
  // the same items, so all three show the marker or none does.
  sch::render::SchItemBase::setShowResistorOrient(prefs.showResistorOrient());

  // The arguments, minus the program name.
  const QStringList args = application.arguments().mid(1);

  // Options mean the command line: print or export, no window.
  if (CliRun::wanted(args)) return CliRun::run(args);

  ViewerWin window(prefs);
  window.show();

  // Files on the command line -- from Explorer, or another instance:
  // the first here, any others each in a new instance.
  window.openFiles(args);

  return application.exec();
}
