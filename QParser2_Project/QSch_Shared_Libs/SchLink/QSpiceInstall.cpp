#include "QSpiceInstall.h"

#include <QDir>
#include <QFileInfo>
#include <QObject>
#include <QSettings>

namespace sch::link::install {
namespace {

QString theInstallPath;
QString theQuxExePath;
QString theBehavioralPath;
QString theQorvoPath;

// A key's (Default) value -- the unnamed one regedit shows as
// "(Default)". QSettings addresses it as "Default" or "."; both are
// tried, since which one a given Qt version honours has varied.
QString defaultValueOf(const QString &key) {
  QSettings settings(key, QSettings::NativeFormat);

  const QString viaDefault =
      settings.value(QStringLiteral("Default")).toString().trimmed();
  if (!viaDefault.isEmpty()) return viaDefault;

  return settings.value(QStringLiteral(".")).toString().trimmed();
}

} // namespace

QStringList verify(const QString &vendorKey) {
  QStringList problems;

  theInstallPath.clear();
  theQuxExePath.clear();
  theBehavioralPath.clear();
  theQorvoPath.clear();

  const QString installDirSubkey = vendorKey + QStringLiteral("\\InstallDir");
  const QString installPath      = defaultValueOf(installDirSubkey);

  if (installPath.isEmpty()) {
    problems.append(
        QObject::tr("The QSpice installation folder is not recorded in the "
                    "registry. Expected it as the default value of:\n  %1")
            .arg(installDirSubkey));
    return problems;
  }

  if (!QFileInfo(installPath).isDir()) {
    problems.append(
        QObject::tr("The QSpice installation folder does not exist:\n  %1")
            .arg(QDir::toNativeSeparators(installPath)));
    return problems;
  }

  theInstallPath = installPath;

  // Joined with QDir rather than by string concatenation, so a registry
  // value with or without a trailing separator gives the same result.
  const QDir installDir(installPath);

  const QString quxExe = installDir.filePath(QStringLiteral("QUX.exe"));
  if (QFileInfo(quxExe).isFile())
    theQuxExePath = quxExe;
  else
    problems.append(QObject::tr("QUX.exe was not found in:\n  %1")
                        .arg(QDir::toNativeSeparators(installPath)));

  // Optional: recorded if present, never reported if absent.
  const QString qorvo = installDir.filePath(QStringLiteral("Qorvo"));
  if (QFileInfo(qorvo).isDir()) theQorvoPath = qorvo;

  const QString behavioral = installDir.filePath(QStringLiteral("Behavioral"));
  if (QFileInfo(behavioral).isDir())
    theBehavioralPath = behavioral;
  else
    problems.append(
        QObject::tr("The QSpice Behavioral folder was not found:\n  %1")
            .arg(QDir::toNativeSeparators(behavioral)));

  return problems;
}

const QString &installPath() { return theInstallPath; }
const QString &quxExePath() { return theQuxExePath; }
const QString &behavioralPath() { return theBehavioralPath; }
const QString &qorvoPath() { return theQorvoPath; }

QString filePath(const QString &fileName) {
  if (theInstallPath.isEmpty()) return QString();
  return QDir(theInstallPath).absoluteFilePath(fileName);
}

SimPrograms simPrograms(bool fastMath) {
  SimPrograms programs;
  programs.quxExe   = theQuxExePath;
  programs.simExe   = filePath(fastMath ? QStringLiteral("QSPICE64.exe")
                                        : QStringLiteral("QSPICE80.exe"));
  programs.qpostExe = filePath(QStringLiteral("QPOST.exe"));

  if (programs.quxExe.isEmpty() || !QFileInfo::exists(programs.quxExe))
    programs.missing << QStringLiteral("QUX.exe");

  for (const QString &exe : {programs.simExe, programs.qpostExe}) {
    if (!exe.isEmpty() && QFileInfo::exists(exe)) continue;
    programs.missing << (exe.isEmpty() ? QStringLiteral("(installation folder unknown)")
                                       : QDir::toNativeSeparators(exe));
  }

  return programs;
}

} // namespace sch::link::install
