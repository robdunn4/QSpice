#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable : 4996) // Qt headers reference APIs deprecated for Qt 7
#endif

#include "ViewerWin.h"

#include <QAction>
#include <QApplication>
#include <QCloseEvent>
#include <QCoreApplication>
#include <QDebug>
#include <QDesktopServices>
#include <QDir>
#include <QDragEnterEvent>
#include <QDragMoveEvent>
#include <QDropEvent>
#include <QFile>
#include <QFileDialog>
#include <QFileInfo>
#include <QIcon>
#include <QKeySequence>
#include <QLabel>
#include <QMenu>
#include <QMenuBar>
#include <QMessageBox>
#include <QObject>
#include <QPrinter>
#include <QProcess>
#include <QStandardPaths>
#include <QSize>
#include <QStatusBar>
#include <QToolBar>
#include <QTimer>
#include <QTransform>
#include <QUrl>

#ifdef _MSC_VER
#pragma warning(pop)
#endif

#include <string_view>

#include "AboutBox.h"
#include "AppIcon.h"
#include "DocView.h"
#include "HierOpen.h"
#include "ItemTreeIO.h"
#include "PrintDefaultsDlg.h"
#include "SchPrint.h"
#include "SchHeaderFooterDlg.h"
#include "SchItemBase.h"
#include "SchPrintDlg.h"
#include "SchHierBlock.h"
#include "SchRecordRules.h"
#include "SchSvgExport.h"
#include "SchTreeBuilder.h"
#include "Version.h"
#include "ViewerIcons.h"
#include "ViewerPrefs.h"

// SchCore names used in this file.
namespace recordrules = sch::core::recordrules;

// SchRender names used in this file.
namespace SchTreeBuilder = sch::render::SchTreeBuilder;

// SchOutput names used in this file.
using sch::output::SchPrint;
using sch::output::SchPrintSettings;
using sch::output::SchSvgExport;

// SchApp names used in this file.
using sch::app::appIcon;
using sch::app::showAbout;

namespace {

bool isSymbolFile(const QString &path) {
  return QFileInfo(path).suffix().compare(QStringLiteral("qsym"), Qt::CaseInsensitive) == 0;
}

} // namespace

ViewerWin::ViewerWin(ViewerPrefs &prefs, QWidget *parent) : QMainWindow(parent), prefs(prefs) {
  // The empty state: a hint, and the whole window a drop target.
  emptyLbl = new QLabel(tr("Open a schematic with File > Open, or drop a .qsch file here."), this);
  emptyLbl->setAlignment(Qt::AlignCenter);
  emptyLbl->setEnabled(false); // drawn greyed
  setCentralWidget(emptyLbl);
  setAcceptDrops(true);

  cursorPosLbl = new QLabel(this);
  statusBar()->addPermanentWidget(cursorPosLbl);

  schPrint = std::make_unique<SchPrint>(ViewerPrefs::iniFilePath(), ViewerPrefs::printDefaults());

  buildMenus();
  buildToolBar();

  const QByteArray geometry = prefs.windowGeometry();
  if (geometry.isEmpty() || !restoreGeometry(geometry)) resize(1000, 700);

  updtTitle();
  updtActions();
}

ViewerWin::~ViewerWin() = default;

void ViewerWin::buildMenus() {
  QMenu *fileMenu = menuBar()->addMenu(tr("&File"));

  actOpen = fileMenu->addAction(ViewerIcons::open(), tr("&Open..."), this, &ViewerWin::onOpen);
  actOpen->setShortcut(QKeySequence::Open);

  // Rebuilt each time it opens: the list is shared with other instances.
  recentMenu = fileMenu->addMenu(ViewerIcons::recent(), tr("Open &Recent"));
  connect(recentMenu, &QMenu::aboutToShow, this, &ViewerWin::rebuildRecentMenu);

  fileMenu->addSeparator();
  actPrint = fileMenu->addAction(ViewerIcons::print(), tr("&Print..."), this, &ViewerWin::onPrint);
  actPrint->setShortcut(QKeySequence::Print);

  actExportSvg = fileMenu->addAction(ViewerIcons::exportSvg(), tr("Export as S&VG..."), this,
                                     &ViewerWin::onExportSvg);

  fileMenu->addSeparator();
  actExit = fileMenu->addAction(ViewerIcons::exitApp(), tr("E&xit"), this, &QWidget::close);

  // Shortcuts as in QSchEdit.
  QMenu *viewMenu = menuBar()->addMenu(tr("&View"));

  actZoomIn = viewMenu->addAction(ViewerIcons::zoomIn(), tr("Zoom &In"), this,
                                  [this]() { view->zoomIn(); });
  actZoomIn->setShortcut(QKeySequence(QStringLiteral("Ctrl++")));

  actZoomOut = viewMenu->addAction(ViewerIcons::zoomOut(), tr("Zoom &Out"), this,
                                   [this]() { view->zoomOut(); });
  actZoomOut->setShortcut(QKeySequence(QStringLiteral("Ctrl+-")));

  actZoomToFit = viewMenu->addAction(ViewerIcons::zoomToFit(), tr("Zoom to &Fit"), this,
                                     [this]() { view->zoomToFit(); });
  actZoomToFit->setShortcut(QKeySequence(QStringLiteral("Ctrl+0")));

  // Preferences, kept in QSchView.ini. Available with or without a file.
  QMenu *optionsMenu = menuBar()->addMenu(tr("&Options"));

  actShowPolarity = optionsMenu->addAction(ViewerIcons::resistorPolarity(),
                                           tr("Show Resistor &Polarity"));
  actShowPolarity->setCheckable(true);
  actShowPolarity->setChecked(prefs.showResistorOrient());
  connect(actShowPolarity, &QAction::toggled, this, &ViewerWin::onShowPolarity);

  actColorTheme = optionsMenu->addAction(ViewerIcons::colorTheme(), tr("Select Color &Theme..."),
                                         this, &ViewerWin::onColorTheme);

  optionsMenu->addSeparator();
  actPrintDefaults = optionsMenu->addAction(ViewerIcons::printDefaults(),
                                            tr("Printing &Defaults..."), this,
                                            &ViewerWin::onPrintDefaults);
  actHeaderFooter = optionsMenu->addAction(ViewerIcons::headerFooter(),
                                           tr("&Headers/Footers..."), this,
                                           &ViewerWin::onHeaderFooter);

  QMenu *helpMenu = menuBar()->addMenu(tr("&Help"));
  actHelp = helpMenu->addAction(ViewerIcons::help(),
                                tr("%1 &Help").arg(QString::fromLatin1(Version::kProgramName)),
                                this, &ViewerWin::onHelp);
  actHelp->setShortcut(QKeySequence::HelpContents);
  helpMenu->addSeparator();

  actAbout = helpMenu->addAction(
      tr("&About %1").arg(QString::fromLatin1(Version::kProgramName)), this, &ViewerWin::onAbout);

  const QIcon icon = appIcon();
  if (!icon.isNull()) actAbout->setIcon(icon);
}

void ViewerWin::buildToolBar() {
  // Docked at the top for good: it cannot be moved, floated or hidden
  // (see createPopupMenu()).
  QToolBar *toolBar = addToolBar(tr("Main"));
  toolBar->setObjectName(QStringLiteral("mainToolBar"));
  toolBar->setMovable(false);
  toolBar->setFloatable(false);
  toolBar->setIconSize(QSize(24, 24));

  // A CHECKED button (Show Resistor Polarity) shows a pressed-in frame,
  // not the style's solid highlight fill, which hid the icon. Only the
  // checked state is styled; the rest stays native.
  toolBar->setStyleSheet(QStringLiteral(
      "QToolButton:checked {"
      "  background: transparent;"
      "  border: 1px solid palette(mid);"
      "  border-top-color: palette(dark);"
      "  border-left-color: palette(dark);"
      "  border-radius: 3px;"
      "}"
      "QToolButton:checked:hover { background: palette(midlight); }"));

  const QList<QAction *> groups[] = {
      {actOpen},
      {actPrint, actExportSvg},
      {actZoomIn, actZoomOut, actZoomToFit},
      {actShowPolarity},
  };

  bool first = true;
  for (const QList<QAction *> &group : groups) {
    if (!first) toolBar->addSeparator();
    first = false;

    for (QAction *action : group) {
      toolBar->addAction(action);

      // The button's tip: the menu text without its '&' and "...", and
      // the shortcut, if it has one.
      QString tip = action->text().remove(QLatin1Char('&'));
      if (tip.endsWith(QStringLiteral("..."))) tip.chop(3);
      if (!action->shortcut().isEmpty())
        tip += QStringLiteral(" (%1)").arg(action->shortcut().toString(QKeySequence::NativeText));
      action->setToolTip(tip);
    }
  }
}

QMenu *ViewerWin::createPopupMenu() {
  // No right-click menu over the toolbar or menu bar: QMainWindow's
  // default offers to hide the toolbar, and this one stays.
  return nullptr;
}

void ViewerWin::updtTitle() {
  QString title = QString::fromStdString(Version::winTitleStr());
  if (view != nullptr) title += QStringLiteral(" - ") + QFileInfo(view->filePath()).fileName();
  setWindowTitle(title);
}

void ViewerWin::updtActions() {
  // Empty: only Open, Open Recent, Exit, Help and About.
  const bool hasFile = (view != nullptr);
  actPrint->setEnabled(hasFile);
  actExportSvg->setEnabled(hasFile);
  actZoomIn->setEnabled(hasFile);
  actZoomOut->setEnabled(hasFile);
  actZoomToFit->setEnabled(hasFile);
}

// ---------------------------------------------------------------------------
// Opening files
// ---------------------------------------------------------------------------

void ViewerWin::openFiles(const QStringList &paths) {
  for (int i = 0; i < paths.size(); ++i) {
    if (i == 0 && view == nullptr) {
      loadHere(paths.at(i));
      continue;
    }
    launchInstance(paths.at(i));
  }
}

void ViewerWin::launchInstance(const QString &path) {
  const bool started =
      QProcess::startDetached(QCoreApplication::applicationFilePath(), QStringList() << path);
  if (started) {
    qInfo().noquote() << "Opened in a new instance:" << QDir::toNativeSeparators(path);
    return;
  }

  qWarning().noquote() << "Could not start a new instance for" << QDir::toNativeSeparators(path);
  QMessageBox::warning(this, tr("Open"),
                       tr("Could not start another %1 to show:\n%2")
                           .arg(QString::fromLatin1(Version::kProgramName),
                                QDir::toNativeSeparators(path)));
}

void ViewerWin::onComponentDoubleClicked(const NodePtr &compNode) {
  if (view == nullptr || !sch::core::hier::isBlock(compNode)) return;

  QString path;
  if (!sch::app::hierSchematicPath(this, compNode, view->filePath(), path)) return;

  // Always a new instance: this window keeps the schematic it shows.
  launchInstance(path);
}

bool ViewerWin::loadHere(const QString &path) {
  const QString nativePath = QDir::toNativeSeparators(path);

  if (isSymbolFile(path)) {
    qInfo().noquote() << "Symbol file not opened (not yet supported):" << nativePath;
    QMessageBox::information(this, tr("Open"),
                             tr("Symbol files (.qsym) are not supported yet.\n\n%1")
                                 .arg(nativePath));
    return false;
  }

  // As BYTES, not through ItemTreeIO::readFile(): a path does not survive
  // the round trip through std::string on Windows. Kept, to tell later
  // whether the file has changed.
  QFile file(path);
  if (!file.open(QIODevice::ReadOnly)) {
    qWarning().noquote() << "Open failed: could not read" << nativePath;
    QMessageBox::warning(this, tr("Open"), tr("Could not read:\n%1").arg(nativePath));
    prefs.removeRecentFile(path);
    return false;
  }
  const QByteArray bytes = file.readAll();
  file.close();

  ItemTree tree;
  int      omittedCount = 0;
  if (!parseSchematic(path, bytes, tr("Open"), tree, omittedCount)) return false;

  installTree(path, bytes, std::move(tree), omittedCount, false);

  qInfo().noquote() << "Opened" << nativePath;
  prefs.addRecentFile(path);
  return true;
}

bool ViewerWin::parseSchematic(const QString &path, const QByteArray &bytes,
                               const QString &action, ItemTree &tree, int &omittedCount) {
  const QString nativePath = QDir::toNativeSeparators(path);

  ItemTreeIO::ParseResult result = ItemTreeIO::readString(
      std::string_view(bytes.constData(), static_cast<size_t>(bytes.size())), true);
  if (!result.ok) {
    QString msg = tr("Could not read the schematic:\n%1\n\n%2")
                      .arg(nativePath, QString::fromStdString(result.error));
    if (result.line > 0) msg += tr("\n(line %1)").arg(result.line);
    qWarning().noquote() << "Read failed:" << nativePath << "-"
                         << QString::fromStdString(result.error);
    QMessageBox::warning(this, action, msg);
    return false;
  }

  // Records not allowed where they are are left out before anything is
  // built, as QSchEdit and printing do (SchRecordRules). Logged and
  // noted in the status bar; a viewer has nothing to save, so no dialog.
  const QList<recordrules::Omitted> omitted =
      recordrules::dropForbidden(result.tree.root(), recordrules::Dest::Schematic);
  recordrules::log(QFileInfo(path).fileName(), omitted);

  omittedCount = static_cast<int>(omitted.size());
  tree         = std::move(result.tree);
  return true;
}

void ViewerWin::installTree(const QString &path, const QByteArray &bytes, ItemTree tree,
                            int omittedCount, bool keepPosition) {
  // The old view's zoom and centre, for a reload.
  QTransform oldTransform;
  QPointF    oldCentre;
  const bool restore = keepPosition && view != nullptr;
  if (restore) {
    oldTransform = view->transform();
    oldCentre    = view->visibleSceneRect().center();
  }

  DocView *newView = new DocView(this);
  newView->setViewOptions(prefs.viewOptions());
  SchTreeBuilder::populateScene(tree, newView->scene());
  newView->setTree(std::move(tree));
  newView->setFilePath(QFileInfo(path).absoluteFilePath());
  loadedBytes = bytes;

  // Queued: the drop has to finish before anything here can show a
  // message box.
  connect(newView, &DocView::filesDropped, this, &ViewerWin::openFiles, Qt::QueuedConnection);
  connect(newView, &sch::render::SchViewBase::cursorPosChanged, this, &ViewerWin::onCursorPos);
  connect(newView, &DocView::componentDoubleClicked, this,
          &ViewerWin::onComponentDoubleClicked);

  // Replaces the empty-state label or the old view; Qt deletes either.
  setCentralWidget(newView);
  view     = newView;
  emptyLbl = nullptr;
  view->setFocus();

  // After the window has laid the view out -- fitted to a 0x0 viewport,
  // the transform would be degenerate -- and after the view's own first
  // fit, which was queued when it was constructed.
  if (restore) {
    QTimer::singleShot(0, view, [this, oldTransform, oldCentre]() {
      view->setTransform(oldTransform);
      view->ensurePanRoom();
      view->centerOn(oldCentre);
    });
  } else {
    QTimer::singleShot(0, view, &DocView::zoomToFit);
  }

  if (omittedCount > 0)
    statusBar()->showMessage(
        tr("%n record(s) not allowed where they are were left out -- see the log.", nullptr,
           omittedCount));

  updtTitle();
  updtActions();
}

bool ViewerWin::reloadIfChanged(const QString &action) {
  if (view == nullptr) return false;

  const QString path       = view->filePath();
  const QString nativePath = QDir::toNativeSeparators(path);

  QFile file(path);
  if (!file.open(QIODevice::ReadOnly)) {
    const QMessageBox::StandardButton answer = QMessageBox::question(
        this, action,
        tr("The file can no longer be read:\n%1\n\nGo on with what is shown?").arg(nativePath),
        QMessageBox::Yes | QMessageBox::Cancel, QMessageBox::Cancel);
    return answer == QMessageBox::Yes;
  }
  const QByteArray bytes = file.readAll();
  file.close();

  if (bytes == loadedBytes) return true;

  const QMessageBox::StandardButton answer = QMessageBox::question(
      this, action,
      tr("The file has changed since it was opened:\n%1\n\nReload it first?").arg(nativePath),
      QMessageBox::Yes | QMessageBox::No | QMessageBox::Cancel, QMessageBox::Yes);
  if (answer == QMessageBox::Cancel) return false;
  if (answer == QMessageBox::No) return true;

  ItemTree tree;
  int      omittedCount = 0;
  if (!parseSchematic(path, bytes, action, tree, omittedCount)) return false;

  installTree(path, bytes, std::move(tree), omittedCount, true);
  qInfo().noquote() << "Reloaded" << nativePath;
  return true;
}

// ---------------------------------------------------------------------------
// Printing and export
// ---------------------------------------------------------------------------

void ViewerWin::onPrint() {
  // Always through the preview: what prints is what was seen. The file
  // name is the print job's name.
  if (!reloadIfChanged(tr("Print"))) return;

  const QString title = QFileInfo(view->filePath()).fileName();

  // Every Print starts from the .ini -- Options > Printing Defaults --
  // whatever the last one changed, and with the printer opened afresh
  // (one that was asleep last time gets another try).
  schPrint->reloadDefaults();
  schPrint->ensurePrinter()->setDocName(title);

  SchPrintDlg dialog(schPrint.get(), view, title, this);
  dialog.exec();
}

void ViewerWin::onExportSvg() {
  // Asks where, beside the schematic by default, then writes; failures
  // are reported here. The writing is SchSvgExport's.
  if (!reloadIfChanged(tr("Export as SVG"))) return;

  const QString schPath = view->filePath();
  const QString title   = QFileInfo(schPath).fileName();

  if (view->printableSceneRect().isEmpty()) {
    QMessageBox::information(this, tr("Export as SVG"), tr("There is nothing to export."));
    return;
  }

  // Beside the schematic, under its own name.
  const QString suggested = QDir(QFileInfo(schPath).absolutePath())
                                .filePath(QFileInfo(schPath).completeBaseName() +
                                          QStringLiteral(".svg"));

  QString path = QFileDialog::getSaveFileName(this, tr("Export as SVG"), suggested,
                                              tr("SVG Files (*.svg)"));
  if (path.isEmpty()) return;
  if (!path.endsWith(QStringLiteral(".svg"), Qt::CaseInsensitive)) path += QStringLiteral(".svg");

  // Nothing from the print settings: the fixed export scale, and the
  // thinnest line from [Export].
  if (SchSvgExport::write(path, view, title, SchPrintSettings::kDefaultMmPer1000,
                          prefs.svgMinLineMm(), QString::fromLatin1(Version::kProgramName)))
    return;

  QMessageBox::warning(this, tr("Export as SVG"),
                       tr("Could not write %1.").arg(QDir::toNativeSeparators(path)));
}

void ViewerWin::onOpen() {
  // Starts where the most recent file is, if there is one.
  QString           startDir = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation);
  const QStringList recent   = prefs.recentFiles();
  if (!recent.isEmpty()) startDir = QFileInfo(recent.first()).absolutePath();

  const QStringList paths = QFileDialog::getOpenFileNames(
      this, tr("Open"), startDir,
      tr("QSpice schematics (*.qsch);;QSpice symbols (*.qsym);;All files (*.*)"));
  if (paths.isEmpty()) return;

  openFiles(paths);
}

void ViewerWin::rebuildRecentMenu() {
  recentMenu->clear();

  const QStringList recent = prefs.recentFiles();
  if (recent.isEmpty()) {
    QAction *none = recentMenu->addAction(tr("(none)"));
    none->setEnabled(false);
    return;
  }

  for (int i = 0; i < recent.size(); ++i) {
    const QString path = recent.at(i);

    // "&1" .. "&9", then "1&0": a mnemonic for each of the first ten.
    // The rest are numbered without one.
    QString number = QString::number(i + 1);
    if (i < 9) number = QStringLiteral("&%1").arg(i + 1);
    if (i == 9) number = QStringLiteral("1&0");
    const QString text   = QStringLiteral("%1  %2").arg(number, QDir::toNativeSeparators(path));

    recentMenu->addAction(text, this, [this, path]() { openRecent(path); });
  }
}

void ViewerWin::openRecent(const QString &path) {
  if (QFileInfo::exists(path)) {
    openFiles(QStringList() << path);
    return;
  }

  const QMessageBox::StandardButton answer = QMessageBox::question(
      this, tr("Open Recent"),
      tr("This file no longer exists:\n%1\n\nRemove it from the list?")
          .arg(QDir::toNativeSeparators(path)));
  if (answer == QMessageBox::Yes) prefs.removeRecentFile(path);
}

// ---------------------------------------------------------------------------
// View and Help
// ---------------------------------------------------------------------------

void ViewerWin::onCursorPos(QPointF scenePos) {
  cursorPosLbl->setText(
      tr("X: %1, Y: %2").arg(scenePos.x(), 0, 'f', 2).arg(scenePos.y(), 0, 'f', 2));
}

// ---------------------------------------------------------------------------
// Options
// ---------------------------------------------------------------------------

void ViewerWin::onShowPolarity(bool show) {
  prefs.setShowResistorOrient(show);

  // One switch for every item; the window repaints, and print and export
  // read it when they draw.
  sch::render::SchItemBase::setShowResistorOrient(show);
  if (view != nullptr) view->viewport()->update();
}

void ViewerWin::onPrintDefaults() {
  PrintDefaultsDlg dialog(view != nullptr ? view->filePath() : QString(), this);
  if (dialog.exec() != QDialog::Accepted) return;

  // Read again by the next Print -- see onPrint().
  dialog.save();
}

void ViewerWin::onColorTheme() {
  QMessageBox::information(this, tr("Select Color Theme"),
                           tr("Color themes are not available yet.\n\n"
                              "Colors come from QSpice's settings when it is installed, "
                              "and from [Colors] in %1.ini otherwise.")
                               .arg(QString::fromLatin1(Version::kProgramName)));
}

void ViewerWin::onHeaderFooter() {
  SchPrintSettings stored = ViewerPrefs::storedPrintSettings();

  SchHeaderFooterDlg dialog(
      stored,
      PrintDefaultsDlg::sampleContext(view != nullptr ? view->filePath() : QString(), stored),
      this);
  if (dialog.exec() != QDialog::Accepted) return;

  dialog.apply(stored);
  stored.save();
}

void ViewerWin::onHelp() {
  // The help is one PDF, shown in whatever reads PDFs on this machine:
  // beside the program, or in a debug build two folders above it -- see
  // ViewerPrefs::helpFilePath().
  const QString path       = ViewerPrefs::helpFilePath();
  const QString nativePath = QDir::toNativeSeparators(path);
  const QString title = tr("%1 Help").arg(QString::fromLatin1(Version::kProgramName));

  if (!QFileInfo::exists(path)) {
    qInfo().noquote() << "Help not found:" << nativePath;
    QMessageBox::information(this, title,
                             tr("The help document was not found. It should be:\n\n%1")
                                 .arg(nativePath));
    return;
  }

  if (QDesktopServices::openUrl(QUrl::fromLocalFile(path))) return;

  // Found, but nothing on this machine opened it.
  qWarning().noquote() << "Help could not be opened:" << nativePath;
  QMessageBox::warning(this, title,
                       tr("Could not open the help document. Is a PDF viewer "
                          "installed?\n\n%1")
                           .arg(nativePath));
}

void ViewerWin::onAbout() {
  // The name, version, linked libraries and icon -- SchApp's, shared
  // with every program.
  showAbout(this, QString::fromLatin1(Version::kProgramName),
            QString::fromStdString(Version::versionStr()));
}

// ---------------------------------------------------------------------------
// Drops on the empty window, and closing
// ---------------------------------------------------------------------------

void ViewerWin::dragEnterEvent(QDragEnterEvent *event) {
  if (DocView::droppableFiles(event->mimeData()).isEmpty()) {
    event->ignore();
    return;
  }
  event->acceptProposedAction();
}

void ViewerWin::dragMoveEvent(QDragMoveEvent *event) { event->acceptProposedAction(); }

void ViewerWin::dropEvent(QDropEvent *event) {
  const QStringList paths = DocView::droppableFiles(event->mimeData());
  if (paths.isEmpty()) {
    event->ignore();
    return;
  }
  event->acceptProposedAction();

  // After the drop has finished: a message box cannot interrupt a drag.
  QTimer::singleShot(0, this, [this, paths]() { openFiles(paths); });
}

void ViewerWin::closeEvent(QCloseEvent *event) {
  prefs.setWindowGeometry(saveGeometry());
  QMainWindow::closeEvent(event);
}
