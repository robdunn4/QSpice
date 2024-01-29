#include "mainwindow.h"
#include "ui_mainwindow.h"

#include "appexception.h"
#include "parser.h"
#include <QFileDialog>
#include <QMessageBox>

QString fileName;
QFileDialog *fileDlg;
QMessageBox *msgBox;

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent), ui(new Ui::MainWindow) {
  ui->setupUi(this);

  fileDlg = new QFileDialog(this);
  fileDlg->setFileMode(QFileDialog::ExistingFile);
  fileDlg->setNameFilter("QSpice Schematic Files (*.qsch);;All Files (*.*)");
  fileDlg->setDirectory("C:/dev/qspice/projects");

  msgBox = new QMessageBox(this);
  msgBox->setStandardButtons(QMessageBox::Cancel);
}

MainWindow::~MainWindow() { delete ui; }

void MainWindow::on_actionExit_triggered() { close(); }

void MainWindow::on_actionOpen_triggered() {
  QStringList files;
  if (!fileDlg->exec())
    return;
  files = fileDlg->selectedFiles();
  fileName = files[0];

  statusBar()->showMessage("File: " + fileName);
  on_actionGenerate_triggered();
}

void MainWindow::on_actionGenerate_triggered() {
  Parser parser;

  ui->textBox->clear();

  try {
    parser.loadFile(fileName);
  } catch (const FileError &e) {
    QString msg(e.what());
    msg += ":\n" + fileName;
    msgBox->setText(msg);
    msgBox->setIcon(QMessageBox::Warning);
    msgBox->exec();
    return;
  }

  // QStringList list = parser.getLines();

  // for (int i = 0; i < list.count(); i++) {
  //   QString str(parser.lineTypeText(parser.lineType(i)).c_str());
  //   str += ": ";
  //   str += parser.getLine(i);
  //   ui->textBox->appendPlainText(str);
  // }

  parser.parseStrList(parser.getStrList());

  QStringList code = parser.genCode();

  QString sepStr("\n\n***** Code *****\n\n");
  ui->textBox->appendPlainText(sepStr);

  for (int i = 0; i < code.count(); i++) {
    ui->textBox->appendPlainText(code[i]);
  }

  // ui->textBox->appendPlainText(code.);
}
