#include "mainwindow.h"
#include "ui_mainwindow.h"

#include "aboutdlg.h"
#include "actionbutton.h"
#include "appexception.h"
#include "appsettings.h"
#include "codegenerator.h"
#include "parser.h"
#include "types.h"
#include <QClipboard>
#include <QFileDialog>
#include <QMessageBox>

QString tmplFileSuffix("qcgt"); // change this once decided
QString schFileName("./test_schematics/");
QString tmplFileName("./templates/qspice_default." + tmplFileSuffix);
// QString tmplFileName(":/templates/templates/qspice_default.qcgt");

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

  // ultimately should be this...
  // schFileName = "./test_schematics/"; // need to load from settings
  // schFileName = "C:/Dev/QSpice/Qt/QCodeGen/test_schematics/";

  // ultimately should be this...
  // tmplFileName = "./templates/qspice_base." + tmplFileSuffix;
  // tmplFileName =
  //     "C:/Dev/QSpice/Qt/QCodeGen/templates/qspice_base." + tmplFileSuffix;

  msgBox = new QMessageBox(QMessageBox::Warning,
                           AboutDlg::appName + " - File I/O Error", "text",
                           QMessageBox::Ok, this);

  clipbrd = QGuiApplication::clipboard();

  connectGui();

  ::appSettings = new AppSettings(this);
  ::appSettings->loadSettings(this);

  qDebug() << QDir::currentPath();

  // need to move this to settings load
  StrList inStrings;
  ui->loadTemplateBtn->setToolTip("No Template Loaded");
  if (loadFile(tmplFileName, inStrings)) {
    codeGenerator.loadTmplFile(inStrings);
    ui->loadTemplateBtn->setToolTip(tmplFileName);
  }
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

  // show about dialog
  connect(ui->helpAboutAct, &QAction::triggered, this,
          &MainWindow::helpAboutSlot);

  // tabs
  cTabs = ui->componentTabs;
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

  // connect buttons
  ui->saveBtn->setAction(ui->saveAct);
  ui->saveAct->setEnabled(false);

  ui->reloadBtn->setAction(ui->reloadAct);
  ui->reloadAct->setEnabled(false);

  // tooltips
  ui->loadSchAct->setToolTip("No schematic loaded");
  ui->loadTmplAct->setToolTip(tmplFileName);
}

void MainWindow::closeSlot() { this->close(); }

void MainWindow::closeEvent(QCloseEvent * /* event */) {
  ::appSettings->saveSettings(this);
}

// load the text from a file into a string list -- returns false on fail
// and leaves strList unchanged
bool MainWindow::loadFile(QString filePath, StrList &strList) {
  StrList inStrings; // don't overwrite strList until finished without error
  FilePtr file;
  char buf[2048]; // file read buffer

  try {
    // open file for binary read
    file.ptr = fopen(filePath.toStdString().c_str(), "rb");
    if (!file.ptr)
      throw FileError("Error opening file");

    // read file into stringlist with error checking
    char *s; // start of text
    s = fgets(buf, sizeof(buf), file.ptr);
    while (!feof(file.ptr) && !ferror(file.ptr)) {
      Q_ASSERT(s); // shouldn't happen

      // ensure buffer is null-terminated (not sure about fgets())
      size_t len = strlen(buf);
      if (len >= sizeof(buf))
        throw FileError("Internal buffer overflow on file read");

      // trim trailing whitespace
      size_t i = strlen(s) - 1;
      while (::isspace(s[i] & 0x7f) && i > 0)
        s[i--] = 0;

      // add to stringlist
      // add test/throw error on unexpected empty line (if not last line)?
      inStrings.push_back(String(s));

      // get next line
      s = fgets(buf, sizeof(buf), file.ptr);
    }

    // a final (probably unnecessary) error check
    if (ferror(file.ptr))
      throw FileError("Unexpected file error state");
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
                                QString filter) {
  QString path = QFileInfo(filePath).absolutePath();

  QFileDialog openDlg(this, AboutDlg::appName + " - Open " + fileType, path,
                      filter);
  openDlg.setFileMode(QFileDialog::ExistingFile);

  if (!openDlg.exec())
    return false;

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
  tabChangedSlot(0);
}

void MainWindow::openSchematicSlot() {
  QString fpath = schFileName;

  bool res = selectFileOpen(fpath, "Schematic",
                            "QSpice Schematic Files (*.qsch);;All Files (*.*)");
  if (!res)
    return;

  StrList fileStrs;

  if (!loadFile(fpath, fileStrs))
    return;
  parser.reset();
  parser.loadFile(fileStrs);
  parser.genCblkCode(codeGenerator);

  schFileName = fpath;
  ui->loadSchematicBtn->setToolTip(fpath);
  ui->reloadBtn->setEnabled(true);

  refreshGui();
}

void MainWindow::loadTemplateSlot() {
  QString fpath = tmplFileName;
  bool res = selectFileOpen(fpath, "Template",
                            AboutDlg::appName + " Template Files (*." +
                                tmplFileSuffix + ");;All Files (*.*)");
  if (!res)
    return;

  StrList fileStrs;

  if (!loadFile(fpath, fileStrs))
    return;
  codeGenerator.loadTmplFile(fileStrs);
  parser.genCblkCode(codeGenerator);

  tmplFileName = fpath;
  ui->loadTemplateBtn->setToolTip(fpath);
  ui->reloadBtn->setEnabled(true);

  tabChangedSlot(0);
}

void MainWindow::reloadSlot() {
  // // todo
  // try {
  //   // throws error...
  //   loadFile(fpath, fileStrs);
  //   parser.loadFile(fileStrs);
  //   codeGenerator.loadTmplFile(tmplFileName);
  //   parser.genCblkCode(codeGenerator);
  // } catch (const FileError &e) {
  //   QString msg(e.what());
  //   msg += ":\n" + fpath;
  //   msgBox->setText(msg);
  //   msgBox->setIcon(QMessageBox::Warning);
  //   msgBox->exec();
  //   return;
  // }

  // refreshGui();
}

void MainWindow::saveSlot() {
  // todo
}

void MainWindow::copyToClipbrdSlot() {
  QString str = ui->textBox->textCursor().selectedText();
  str.replace(QChar(0x2029), QChar('\n'));

  // if no selection, use entire contents
  if (!str.length())
    str = ui->textBox->toPlainText();

  // put it on the clipboard
  clipbrd->setText(str);
}

void MainWindow::helpAboutSlot() { aboutDlg->show(); }

void MainWindow::helpUsingSlot() {
  // todo
}

void MainWindow::tabChangedSlot(int /*index*/) {
  // todo
  QObject *who = sender(); // temp for debug

  ui->textBox->clear();

  int componentNdx = cTabs->currentIndex();
  int detailNdx = dTabs->currentIndex();
  if (componentNdx < 0)
    componentNdx = 0;

  if (!parser.getCblkCnt())
    return;

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
  default:
    Q_ASSERT(0);
  }

  for (int i = 0; i < strList.size(); i++) {
    ui->textBox->appendPlainText(strList[i].c_str());
  }

  ui->textBox->moveCursor(QTextCursor::Start);
  ui->textBox->ensureCursorVisible();
}
