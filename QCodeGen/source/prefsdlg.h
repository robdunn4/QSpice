#ifndef PREFSDLG_H
#define PREFSDLG_H

#include "mainwindow.h"
#include <QButtonGroup>
#include <QDialog>

namespace Ui {
class PrefsDlg;
}

struct PrefsSettings {
  enum AuthType : int { AT_User = 0, AT_Custom };
  enum TmplType : int { TT_Internal = 0, TT_LastUsed, TT_File };
  enum SchType : int { ST_LastUsed = 0, ST_Folder };

  int tmplActiveBtn;
  QString tmplPathText;
  int authActiveBtn;
  QString authText;
  QString userFields[3];
  int schActiveBtn;

  QString tmplLastPath;
  QString schLastPath;
  QString schPathText;

  QString getSchPath() const;
  QString getTmplPath() const;
  bool useInternalTmpl() const;
  QString getAuthor() const;
  QString getUserField(int index) const;
};

extern PrefsSettings prefsSettings;

class PrefsDlg : public QDialog {
  Q_OBJECT

public:
  PrefsDlg(PrefsSettings &ps, MainWindow *parent = nullptr);
  ~PrefsDlg();

  int exec();

  void applyPrefsSettings();

protected:
  PrefsSettings &ps;

private slots:
  void startTmplRbChgdSlot();
  void authorRbChgdSlot();
  void selectTmplSlot();
  void startSchRbChgdSlot();
  void selectSchSlot();

private:
  Ui::PrefsDlg *ui;
};

#endif // PREFSDLG_H
