#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable : 4996) // Qt headers reference APIs deprecated for Qt 7
#endif

#include "SchDocSource.h"

#include <QByteArray>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QObject>

#ifdef _MSC_VER
#pragma warning(pop)
#endif

#include <string_view>
#include <utility>

#include "ItemTreeIO.h"
#include "RenderStyle.h"
#include "SchScene.h"
#include "SchTreeBuilder.h"

namespace sch::render {

SchDocSource::SchDocSource() = default;

SchDocSource::~SchDocSource() {
  // Items first: they hold references into the tree's records.
  scene_.reset();
}

bool SchDocSource::load(const QString &path, QString &error,
                        QList<sch::core::recordrules::Omitted> &omitted) {
  omitted.clear();

  // As BYTES, not through ItemTreeIO::readFile(): a path does not
  // survive the round trip through std::string on Windows.
  QFile file(path);
  if (!file.open(QIODevice::ReadOnly)) {
    error = QObject::tr("Could not read %1").arg(QDir::toNativeSeparators(path));
    return false;
  }
  const QByteArray bytes = file.readAll();
  file.close();

  // Case-insensitive, as the Windows file system is.
  const bool isSym =
      QFileInfo(path).suffix().compare(QStringLiteral("qsym"), Qt::CaseInsensitive) == 0;

  // A .qsym's header is optional -- QSpice writes it, the symbol files
  // in its install folders have none. A schematic's is required.
  ItemTreeIO::ParseResult result = ItemTreeIO::readString(
      std::string_view(bytes.constData(), static_cast<size_t>(bytes.size())), !isSym);
  if (!result.ok) {
    error = QObject::tr("Could not parse %1: %2")
                .arg(QDir::toNativeSeparators(path), QString::fromStdString(result.error));
    return false;
  }

  omitted = sch::core::recordrules::dropForbidden(
      result.tree.root(), isSym ? sch::core::recordrules::Dest::Symbol : sch::core::recordrules::Dest::Schematic);

  // The old scene goes before the old tree it refers to.
  std::unique_ptr<SchScene> scene = std::make_unique<SchScene>();
  SchTreeBuilder::populateScene(result.tree, scene.get());
  scene->setRoot(result.tree.root());

  scene_.reset();
  tree_  = std::move(result.tree);
  scene_ = std::move(scene);
  path_  = path;
  return true;
}

QGraphicsScene *SchDocSource::renderScene() const {
  return scene_.get();
}

QBrush SchDocSource::renderBackground() const { return RenderStyle::current().background; }

bool SchDocSource::isPrintExcluded(const QGraphicsItem *item) const {
  Q_UNUSED(item);
  return false;
}

QRectF SchDocSource::visibleSceneRect() const { return printableSceneRect(); }

} // namespace sch::render
