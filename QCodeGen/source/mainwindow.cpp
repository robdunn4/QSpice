#include "mainwindow.h"
#include "ui_mainwindow.h"

#include "aboutdlg.h"
#include "actionbutton.h"
#include "appexception.h"
#include "appsettings.h"
#include "codegenerator.h"
#include "parser.h"
#include "prefsdlg.h"
#include "strings.h"
#include <QClipboard>
#include <QDesktopServices>
#include <QFileDialog>
#include <QMessageBox>
#include <QResource>
#include <stdio.h>

// for RAII/exception handling
class FilePtr {
public:
  FilePtr() : ptr(nullptr) {}
  FilePtr(FILE *ptr) : ptr(ptr) {}
  ~FilePtr() {
    if (ptr) fclose(ptr);
  }

  FILE *ptr;
};

QMessageBox *msgBox;
AboutDlg *aboutDlg;
QClipboard *clipbrd;
CodeGenerator codeGenerator;

TabBar *cTabs;
TabBar *dTabs;

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent), ui(new Ui::MainWindow) {
  ui->setupUi(this); // partial GUI

  setWindowTitle(AboutDlg::appNameVersion);
  aboutDlg = new AboutDlg(this);

  msgBox = new QMessageBox(QMessageBox::Warning,
      AboutDlg::appName + " - File I/O Error", "text", QMessageBox::Ok, this);

  clipbrd = QGuiApplication::clipboard();

  ::appSettings = new AppSettings(this);
  ::appSettings->loadSettings(this);
  ::appSettings->loadSettings(prefsSettings);

  ::aboutDlg->setIniFilePath(::appSettings->getIniFilePath());

  connectGui();

  StrList strs = loadStringResource(defaultCodeTemplate);
  codeGenerator.loadTmplFile(strs);
  // ui->loadTemplateBtn->setToolTip("[Default Template]");

  tmplFileName = prefsSettings.getTmplPath();
  reloadSlot(true);
  schFileName = prefsSettings.getSchPath();
}

MainWindow::~MainWindow() { delete ui; }

void MainWindow::connectGui() {
  // close/exit
  connect(ui->exitAct, &QAction::triggered, this, &MainWindow::closeSlot);
  ui->closeBtn->setAction(ui->exitAct);

  // open/load schematic
  connect(ui->loadSchAct, &QAction::triggered, this,
      &MainWindow::openSchematicSlot);
  ui->loadSchematicBtn->setAction(ui->loadSchAct);

  // load template
  connect(ui->loadTmplAct, &QAction::triggered, this,
      &MainWindow::loadTemplateSlot);
  ui->loadTemplateBtn->setAction(ui->loadTmplAct);

  // copy to clipboard
  connect(ui->copyAct, &QAction::triggered, this,
      &MainWindow::copyToClipbrdSlot);
  ui->copyBtn->setAction(ui->copyAct);
  ui->copyAct->setEnabled(false);

  // save
  connect(ui->saveAct, &QAction::triggered, this, &MainWindow::saveSlot);
  ui->saveBtn->setAction(ui->saveAct);
  ui->saveAct->setEnabled(false);

  // help
  ui->helpAboutAct->setText("About " + AboutDlg::appName);
  connect(ui->helpAboutAct, &QAction::triggered, this,
      &MainWindow::helpAboutSlot);
  ui->helpUsingAct->setText("Using " + AboutDlg::appName);
  connect(ui->helpUsingAct, &QAction::triggered, this,
      &MainWindow::helpUsingSlot);

  // show preferences dialog
  connect(ui->prefsAct, &QAction::triggered, this, &MainWindow::prefsSlot);

  // reload schematic & template
  connect(ui->reloadAct, &QAction::triggered, this, &MainWindow::reloadSlot);
  ui->reloadBtn->setAction(ui->reloadAct);
  ui->reloadAct->setEnabled(false);

  // tabs
  cTabs      = ui->componentTabs;
  QFont font = cTabs->font();
  font.setBold(true);
  font.setPointSize(10);
  cTabs->setFont(font);

  cTabs->setShape(QTabBar::RoundedNorth);
  cTabs->setDrawBase(true);
  cTabs->setExpanding(false);

  cTabs->addTab("No Components Loaded");
  connect(cTabs, &TabBar::currentChanged, this, &MainWindow::tabChangedSlot);

  dTabs = ui->detailTabs;
  dTabs->setShape(QTabBar::TriangularNorth);
  dTabs->setDrawBase(true);
  dTabs->setExpanding(false);

  dTabs->addTab("Summary");
  dTabs->addTab("C-Block Code");
  dTabs->setEnabled(false);
  dTabs->setCurrentIndex(0);
  connect(dTabs, &TabBar::currentChanged, this, &MainWindow::tabChangedSlot);

  // tooltips
  ui->loadSchAct->setToolTip("No schematic loaded");
  ui->loadTmplAct->setToolTip(tmplFileName);
}

void MainWindow::closeSlot() { this->close(); }

void MainWindow::closeEvent(QCloseEvent * /* event */) {
  prefsSettings.schLastPath  = schFileName;
  prefsSettings.tmplLastPath = tmplFileName;
  ::appSettings->saveSettings(this);
  ::appSettings->saveSettings(prefsSettings);
}

// load the text from a file into a string list -- returns false on fail
// and leaves strList unchanged
bool MainWindow::loadFile(QString filePath, StrList &strList) {
  StrList inStrings; // don't overwrite strList until finished without error
  FilePtr file;
  char buf[2048]; // file read buffer

  try {
    // open file for binary read
    if (fopen_s(&file.ptr, filePath.toStdString().c_str(), "rb"))
      throw FileError("Error opening file");

    // read file into stringlist with error checking
    char *s;
    while ((s = fgets(buf, sizeof(buf), file.ptr)) && !ferror(file.ptr)) {
      // trim trailing whitespace
      size_t i = strlen(s) - 1;
      while (::isspace(s[i] & 0x7f) && i >= 0) s[i--] = 0;

      // add to stringlist
      inStrings.push_back(String(s));
    }

    // a final (probably unnecessary) error check
    if (ferror(file.ptr)) throw FileError("Unexpected file error state");
  } catch (const FileError &e) {
    QString msg(e.what());
    msg += ":\n" + filePath;
    msgBox->setText(msg);
    msgBox->setIcon(QMessageBox::Warning);
    msgBox->exec();
    return false;
  }

  strList = inStrings;
  return true;
}

bool MainWindow::selectFileOpen(QString &filePath, QString fileType,
    QString filter, bool folder) {
  QString path = QFileInfo(filePath).absolutePath();

  QFileDialog openDlg(this, AboutDlg::appName + " - Open " + fileType, path,
      filter);
  openDlg.setFileMode(
      folder ? QFileDialog::Directory : QFileDialog::ExistingFile);

  if (!openDlg.exec()) return false;

  filePath = openDlg.selectedFiles().constFirst();
  return true;
}

// reset component tabs from parser cblks
void MainWindow::refreshGui() {
  // reset component tabs from parser

  { // disable gui tabChangedSlot signals temporariliy
    QSignalBlocker blocker(ui->componentTabs);

    ui->componentTabs->clear();
    int cnt = parser.getCblkCnt();

    if (!cnt) {
      ui->componentTabs->addTab("No Components Loaded");
      ui->componentTabs->setCurrentIndex(0);
      ui->detailTabs->setEnabled(false);
    } else {
      for (int i = 0; i < cnt; i++) {
        CBlockData cblk = parser.getCblk(i);

        // add component tab
        QString tabTitle;
        tabTitle += cblk.getSymName() + ":" + cblk.getCblkName();
        cTabs->addTab(tabTitle);
      }
      ui->detailTabs->setCurrentIndex(0);
      ui->detailTabs->setEnabled(true);
      ui->componentTabs->setCurrentIndex(0);
      ui->componentTabs->setEnabled(true);
    }
  } // enable gui tabChangedSlot signals

  // add set tooltips for load buttons to file names here?

  // send tabChangedSlot signal
  tabChangedSlot();
}

// for reading default template from embedded resource
StrList MainWindow::loadStringResource(const QString &resPath) {
  StrList strs;

  QFile file(resPath);
  if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) return strs;

  char buf[2048];
  qint64 len;

  while (!file.atEnd()) {
    len = file.readLine(buf, sizeof(buf));
    if (len < 1) continue;
    while (::isspace(buf[len - 1] & 0x7f) && len > 0) buf[--len] = 0;
    strs.push_back(String(buf));
  }

  return strs;
}

void MainWindow::openSchematicSlot() {
  QString fpath = schFileName;

  bool res = selectFileOpen(fpath, "Schematic",
      "QSpice Schematic Files (*.qsch);;All Files (*.*)");
  if (!res) return;

  StrList fileStrs;

  if (!loadFile(fpath, fileStrs)) return;
  parser.reset();
  if (!parser.loadFile(fileStrs)) {
    msgBox->setText("Unable to parse schematic:\n" + fpath);
    msgBox->exec();
    return;
  }
  parser.genCblkCode(codeGenerator);

  schFileName = fpath;
  ui->loadSchematicBtn->setToolTip(fpath);

  refreshGui();

  ui->reloadAct->setEnabled(true);
}

void MainWindow::loadTemplateSlot() {
  QString fpath = tmplFileName;

  bool res = selectFileOpen(fpath, "Template",
      AboutDlg::appName + " Template Files (*." + tmplFileSuffix +
          ");;All Files (*.*)");
  if (!res) return;

  StrList fileStrs;

  if (!loadFile(fpath, fileStrs)) return;
  codeGenerator.loadTmplFile(fileStrs);
  parser.genCblkCode(codeGenerator);

  tmplFileName = fpath;
  ui->loadTemplateBtn->setToolTip(fpath);
  ui->reloadBtn->setEnabled(true);

  tabChangedSlot();
}

void MainWindow::reloadSlot(bool inInit) {
  StrList schStrs;
  StrList saveSchStrs = parser.getStrList();
  StrList tmplStrs;
  StrList saveTmplStrs = codeGenerator.getStrList();

  if (!tmplFileName.isEmpty()) {
    if (!loadFile(tmplFileName, tmplStrs)) {
      msgBox->setText("Unable to reload template:\n" + schFileName);
      msgBox->exec();
      tmplStrs = saveTmplStrs;
    }
    codeGenerator.loadTmplFile(tmplStrs);
  }

  parser.reset();
  if (!inInit) {
    if (!loadFile(schFileName, schStrs)) {
      msgBox->setText("Unable to reload schematic:\n" + schFileName);
      msgBox->exec();
      schStrs = saveSchStrs;
    }
    if (!parser.loadFile(schStrs)) {
      msgBox->setText("Unable to parse schematic:\n" + schFileName);
      msgBox->exec();
      parser.reset();
    }
    parser.genCblkCode(codeGenerator);
  }

  refreshGui();
}

void MainWindow::saveSlot() {
  QString path   = QFileInfo(schFileName).absolutePath();
  QString filter = "C/C++ Files (*.cpp);;Text Files (*.txt);;All Files (*.*)";
  int detailNdx  = dTabs->currentIndex();
  QString sfx;

  // using switch for possible additional tabs -- currently only two
  switch (detailNdx) {
  case 0: // summary
    sfx = "txt";
    break;
  case 1: // c-block code from template
    sfx = "cpp";
    break;
  default: Q_ASSERT(0);
  }
  path += "/*." + sfx;

  QFileDialog saveDlg(this, AboutDlg::appName + " - Save To File", path,
      filter);
  // QFileDialog saveDlg(this, AboutDlg::appName + " - Save To File", path,
  //     getFileFilters());
  saveDlg.setFileMode(QFileDialog::AnyFile);
  saveDlg.setDefaultSuffix(sfx);
  // apparently setting accept mode confirms overwrites for us...
  // it also checks for write-only status and forces user to select
  // a different filename...
  saveDlg.setAcceptMode(QFileDialog::AcceptSave);

  if (!saveDlg.exec()) return;

  QString filePath = saveDlg.selectedFiles().constFirst();

  QFile outFile;
  outFile.setFileName(filePath);
  outFile.open(QIODevice::WriteOnly);
  QTextStream outStrm(&outFile);
  outStrm << ui->textBox->toPlainText();
  outFile.close();
}

void MainWindow::copyToClipbrdSlot() {
  QString str = ui->textBox->textCursor().selectedText();
  str.replace(QChar(0x2029), QChar('\n'));

  // if no selection, use entire contents
  if (!str.length()) str = ui->textBox->toPlainText();

  // put it on the clipboard
  clipbrd->setText(str);
}

void MainWindow::helpAboutSlot() { aboutDlg->show(); }

void MainWindow::helpUsingSlot() {
  QFileInfo fInfo("./doc/" + helpUserDocName);

  if (!QDesktopServices::openUrl(fInfo.absoluteFilePath())) {
    QString msg = "Unable to locate/open help file:\n\n      " +
                  helpUserDocName + "\n\nPlease check your installation files.";
    QMessageBox(QMessageBox::Warning, AboutDlg::appName, msg, QMessageBox::Ok,
        this)
        .exec();
  };
}

void MainWindow::prefsSlot() {
  PrefsDlg prefsDlg(prefsSettings, this);

  if (prefsDlg.exec() == QDialog::Accepted) {
    if (dTabs->isEnabled()) reloadSlot();
  }
}

void MainWindow::tabChangedSlot() {
  ui->textBox->clear();

  int componentNdx = cTabs->currentIndex();
  int detailNdx    = dTabs->currentIndex();
  if (componentNdx < 0) componentNdx = 0;

  ui->copyAct->setEnabled(false);
  ui->saveAct->setEnabled(false);

  if (!parser.getCblkCnt()) return;

  CBlockData cblk = parser.getCblk(componentNdx);
  StrList strList;

  // using switch for possible additional tabs -- currently only two
  switch (detailNdx) {
  case 0: // summary
    strList = cblk.getCblkSummary();
    break;
  case 1: // c-block code from template
    strList = cblk.getCblkCode();
    break;
  default: Q_ASSERT(0);
  }

  for (int i = 0; i < strList.size(); i++) {
    ui->textBox->appendPlainText(strList[i].c_str());
  }

  ui->copyAct->setEnabled(strList.size());
  ui->saveAct->setEnabled(strList.size());

  ui->textBox->moveCursor(QTextCursor::Start);
  ui->textBox->ensureCursorVisible();
}
