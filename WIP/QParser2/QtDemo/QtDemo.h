//-----------------------------------------------------------------------------
// This file is part of the the QParser2 project.  You can find the complete
// project here:  https://github.com/robdunn4/QSpice/
//-----------------------------------------------------------------------------
#pragma once

#include "QParserIO.h"
#include "ui_QtDemo.h"

class QtDemo : public QMainWindow {
  Q_OBJECT

public:
  QtDemo(QWidget *parent = nullptr);
  ~QtDemo();

  QString filePath;
  bool    textChanged = false;

  void loadFile(const QString &fileName, QTextEdit *editor);
  bool discardChanges();

  QParserIO parser;

private:
  Ui::QtDemoClass ui;

public slots:
  void exit_clicked();
  void load_clicked();
  void save_clicked();
};
