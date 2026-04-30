//-----------------------------------------------------------------------------
// This file is part of the the QParser2 project.  You can find the complete
// project here:  https://github.com/robdunn4/QSpice/
//-----------------------------------------------------------------------------
#include "QtDemo.h"
#include <QStringDecoder>
#include <QtWidgets>

QtDemo::QtDemo(QWidget *parent) : QMainWindow(parent) {
  ui.setupUi(this);

  // connect GUI
  connect(ui.actionExit, &QAction::triggered, this, &QtDemo::exit_clicked);
  connect(ui.exitBtn, &QPushButton::clicked, this, &QtDemo::exit_clicked);
  connect(ui.loadBtn, &QPushButton::clicked, this, &QtDemo::load_clicked);
  connect(ui.saveBtn, &QPushButton::clicked, this, &QtDemo::save_clicked);
  connect(ui.textEdit, &QTextEdit::textChanged,
          [this]() { textChanged = true; });

  // set text editor font to monospace, etc.
  QFont font("monospace", 12);
  font.setStyleHint(QFont::Monospace);
  font.setFixedPitch(true);
  ui.textEdit->setFont(font);
}

QtDemo::~QtDemo() {}

// for now, keep unused editor ptr -- may later have multiple tabs for multiple
// symbols in a *.qsch.  if so, that will require moving the "changed" flag into
// an editor subclass
void QtDemo::loadFile(const QString &fileName, QTextEdit *editor) {
  ui.textEdit->clear();
  textChanged = false;

  int res = parser.load(fileName);
  if (res) {
    // failed
    throw QException::QException(); // TODO
  }
  ItemLibPtr    libPtr = parser.getLibItem();
  ArgLibString &libStr = libPtr->libText;
  if (!libStr.isSubckt()) {
    // not a netlist
    throw QException::QException(); // TODO
  }

  // convert from std::string to QString with system encoding (Windows-1252)
  // note:  seems simple once you see it but took a while to sort
  QByteArray raw(libStr.getValue().c_str() + 1); // skip pipe char
  auto       toUtf16 = QStringDecoder(QStringDecoder::System);
  QString    string  = toUtf16(raw);

  // replace newline indicators with newlines and display in text editor
  string.replace("\\n", "\n");
  ui.textEdit->setPlainText(string);
}

bool QtDemo::discardChanges() {
  if (!textChanged) return true;

  QMessageBox::StandardButton reply;
  reply = QMessageBox::question(
      this, "Unsaved Changes",
      "The text has been modified.\nDo you want to discard the changes?",
      QMessageBox::Yes | QMessageBox::No);
  return reply == QMessageBox::Yes;
}

void QtDemo::load_clicked() {
  // confirm if changed
  if (!discardChanges()) return;

  // move this to class level for selection persistence? or at least
  // set to last selection before showing...
  QString fileName = QFileDialog::getOpenFileName(
      this, tr("Open File"), ".",
      tr("QSpice Symbol Files (*.qsym);;All Files (*.*)"));

  if (fileName.isEmpty()) {
    return;
  }

  this->filePath = fileName;
  loadFile(fileName, ui.textEdit);
  textChanged = false;
}

void QtDemo::save_clicked() {
  QMessageBox::information(this, "Save",
                           "Save functionality is not implemented yet.");
}

void QtDemo::exit_clicked() {
  // confirm if changed
  if (!discardChanges()) return;
  close();
}