#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable : 4996) // Qt headers reference APIs deprecated for Qt 7
#endif

#include "HierOpen.h"

#include <QDebug>
#include <QDir>
#include <QMessageBox>
#include <QObject>

#ifdef _MSC_VER
#pragma warning(pop)
#endif

#include "SchHierBlock.h"

namespace sch::app {

bool hierSchematicPath(QWidget *parent, const NodePtr &compNode, const QString &docPath,
                       QString &path) {
  using sch::core::hier::Result;

  const Result result = sch::core::hier::schematicPath(compNode, docPath, path);
  if (result == Result::Ok) return true;

  const QString nativePath = QDir::toNativeSeparators(path);

  QString why;
  switch (result) {
  case Result::Ok: return true;
  case Result::NotABlock:
    why = QObject::tr("This symbol is not a hierarchical schematic block.");
    break;
  case Result::NoName:
    why = QObject::tr("The block has no schematic name: its first string attribute is "
                      "missing or blank.");
    break;
  case Result::NoFolder:
    why = QObject::tr("The block names its schematic relative to this one, which has not "
                      "been saved yet. Save this schematic first.\n\n%1")
              .arg(nativePath);
    break;
  case Result::NotFound:
    why = QObject::tr("The block's schematic was not found:\n\n%1").arg(nativePath);
    break;
  case Result::NotAFile:
    why = QObject::tr("The block's schematic name is a folder, not a file:\n\n%1")
              .arg(nativePath);
    break;
  }

  qWarning().noquote() << "Open Schematic failed:" << why.simplified();
  QMessageBox::warning(parent, QObject::tr("Open Schematic"), why);
  return false;
}

} // namespace sch::app
