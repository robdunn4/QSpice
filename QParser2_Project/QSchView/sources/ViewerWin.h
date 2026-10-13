#pragma once

#include <QByteArray>
#include <QMainWindow>
#include <QPointF>
#include <QString>
#include <QStringList>

#include <memory>

#include "ItemTree.h"

class DocView;
class QAction;
class QCloseEvent;
class QDragEnterEvent;
class QDragMoveEvent;
class QDropEvent;
class QLabel;
class QMenu;
class ViewerPrefs;
namespace sch::output { class SchPrint; }

// QSchView's one window. It shows ONE file, for its whole life; every
// other file opens in a new instance of the program.
//
// Empty -- started without a file, or the file could not be shown --
// it offers only File > Open, Open Recent, Exit, Help and About. Files dropped
// on it, empty or not, go through openFiles().
class ViewerWin : public QMainWindow {
  Q_OBJECT

public:
  explicit ViewerWin(ViewerPrefs &prefs, QWidget *parent = nullptr);
  ~ViewerWin() override;

  // From the command line, File > Open, Open Recent or a drop. The FIRST
  // is shown here if this window is still empty; every other file --
  // and the first too, when this window already shows one -- opens in a
  // new instance.
  void openFiles(const QStringList &paths);

protected:
  // Shows `path` here. False, reported, if it cannot be: a .qsym (not
  // yet supported), or a file that cannot be read or parsed.
  bool loadHere(const QString &path);

  // `bytes` (read from `path`) parsed into `tree`, with the records not
  // allowed where they are left out and logged. False, reported in a
  // message box titled `action`, if it cannot be parsed.
  bool parseSchematic(const QString &path, const QByteArray &bytes, const QString &action,
                      ItemTree &tree, int &omittedCount);

  // A new view for `tree`, replacing whatever is shown. With
  // `keepPosition`, the new view keeps the old one's zoom and centre --
  // for a reload; otherwise it fits the drawing.
  void installTree(const QString &path, const QByteArray &bytes, ItemTree tree,
                   int omittedCount, bool keepPosition);

  // Before printing or exporting (`action` names which): whether the
  // file on disk still matches what is shown, and if not, whether to
  // reload it first. False means do not go on -- cancelled, or the
  // reload failed.
  bool reloadIfChanged(const QString &action);

  // Starts another QSchView on `path`.
  void launchInstance(const QString &path);

  void buildMenus();

  // The toolbar: the main File and View actions, docked at the top.
  void buildToolBar();

  // None: no menu to hide the toolbar with.
  QMenu *createPopupMenu() override;
  void rebuildRecentMenu();
  void openRecent(const QString &path);
  void onOpen();
  void onAbout();

  // --- Options: preferences, each written to QSchView.ini at once.

  // The "+" beside a resistor's first pin, on screen and in output.
  void onShowPolarity(bool show);

  // What the Print dialog opens with: PrintDefaultsDlg.
  void onPrintDefaults();

  // Not yet: colour themes.
  void onColorTheme();

  // The header and footer text the Print dialog opens with: read from the
  // .ini, written back on OK. Never opens the printer -- the sample is
  // page 1 of 1 of the shown file, or of a made-up one.
  void onHeaderFooter();

  // Help > QSchView Help: QSchView_Help.pdf, from ViewerPrefs::helpFilePath().
  void onHelp();
  void onPrint();
  void onExportSvg();
  void onCursorPos(QPointF scenePos);

  // A double-click on a component: a hierarchical block's schematic opens
  // in a new instance, or a message says why it cannot. Other components
  // are ignored.
  void onComponentDoubleClicked(const NodePtr &compNode);

  void updtTitle();
  void updtActions();

  // Drops while EMPTY land on the window; once a file is shown, the
  // view takes them (DocView) and hands them to openFiles() as well.
  void dragEnterEvent(QDragEnterEvent *event) override;
  void dragMoveEvent(QDragMoveEvent *event) override;
  void dropEvent(QDropEvent *event) override;

  void closeEvent(QCloseEvent *event) override;

  ViewerPrefs &prefs;
  DocView     *view = nullptr; // null while empty
  QLabel      *emptyLbl;
  QLabel      *cursorPosLbl;

  // The file's bytes as loaded -- to tell, before printing or exporting,
  // whether the file has changed since.
  QByteArray loadedBytes;

  // Printer and page settings, kept in QSchView.ini.
  std::unique_ptr<sch::output::SchPrint> schPrint;

  QAction *actOpen;
  QMenu   *recentMenu;
  QAction *actPrint;
  QAction *actExportSvg;
  QAction *actExit;
  QAction *actZoomIn;
  QAction *actZoomOut;
  QAction *actZoomToFit;
  QAction *actShowPolarity;
  QAction *actPrintDefaults;
  QAction *actColorTheme;
  QAction *actHeaderFooter;
  QAction *actHelp;
  QAction *actAbout;
};
