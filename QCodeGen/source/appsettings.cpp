#include "appsettings.h"

#include "aboutdlg.h"

AppSettings *appSettings;

AppSettings::AppSettings(QObject *parent)
    : QSettings(QSettings::IniFormat, QSettings::UserScope,
          AboutDlg::companyName, AboutDlg::appName, parent) {}

void AppSettings::saveSettings(MainWindow *mainWin) {
  beginGroup("MainWindow");
  setValue("geometry", mainWin->saveGeometry());
  endGroup();
}

void AppSettings::loadSettings(MainWindow *mainWin) {
  beginGroup("MainWindow");
  const auto geometry = value("geometry", QByteArray()).toByteArray();
  if (geometry.isEmpty()) mainWin->setGeometry(200, 200, 400, 400);
  else mainWin->restoreGeometry(geometry);
  endGroup();
}

QString AppSettings::getIniFilePath() const { return fileName(); }

// const char[] names to prevent mistyping save vs load
const char tmplBtnNdx[]   = "TemplateNdx";
const char tmplPath[]     = "TemplatePath";
const char authBtnNdx[]   = "AuthorNdx";
const char authText[]     = "Author";
const char userField1[]   = "UserField1";
const char userField2[]   = "UserField2";
const char userField3[]   = "UserField3";
const char tmplLastPath[] = "LastTemplate";
const char schBtnNdx[]    = "SchematicNdx";
const char schLastPath[]  = "LastSchematic";
const char schPath[]      = "SchematicPath";

void AppSettings::saveSettings(PrefsSettings &ps) {
  beginGroup("Preferences");
  setValue(tmplBtnNdx, ps.tmplActiveBtn);
  setValue(tmplPath, ps.tmplPathText);
  setValue(schPath, ps.schPathText);
  setValue(schBtnNdx, ps.schActiveBtn);
  setValue(authBtnNdx, ps.authActiveBtn);
  setValue(authText, ps.authText);
  setValue(userField1, ps.userFields[0]);
  setValue(userField2, ps.userFields[1]);
  setValue(userField3, ps.userFields[2]);

  // these two should be stored somewhere else, not in ps
  setValue(tmplLastPath, ps.tmplLastPath);
  setValue(schLastPath, ps.schLastPath);
  endGroup();
}

void AppSettings::loadSettings(PrefsSettings &ps) {
  beginGroup("Preferences");
  ps.tmplActiveBtn = value(tmplBtnNdx, 0).toInt();
  ps.tmplPathText  = value(tmplPath, "").toString();
  ps.schPathText   = value(schPath, "").toString();
  ps.authActiveBtn = value(authBtnNdx, 0).toInt();
  ps.authText      = value(authText, "").toString();
  ps.userFields[0] = value(userField1, "").toString();
  ps.userFields[1] = value(userField2, "").toString();
  ps.userFields[2] = value(userField3, "").toString();
  ps.tmplLastPath  = value(tmplLastPath, "").toString();
  ps.schActiveBtn  = value(schBtnNdx, 0).toInt();
  ps.schLastPath   = value(schLastPath, "").toString();
  ps.schPathText   = value(schPath, "").toString();
  endGroup();
}
