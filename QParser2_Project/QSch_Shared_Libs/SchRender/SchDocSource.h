#pragma once

#include <QBrush>
#include <QList>
#include <QRectF>
#include <QString>

#include <memory>

#include "ItemTree.h"
#include "SchRecordRules.h"
#include "SchRenderSource.h"

namespace sch::render {

class SchScene;

// A schematic or symbol FILE as a render source, with no editor: read,
// checked against the record rules, built into a scene, and handed to
// printing or export as is.
//
// For a program that prints or exports without editing -- the planned
// print/export tool. The editor's own source is its view (SchView).
//
// Nothing is excluded (there is nothing transient), and with no viewer
// the "visible area" is the whole drawing. Drawn with the current
// RenderStyle; the caller sets that, and the items' static display
// options, as it wants them before loading.
//
// Goes to SchRender at the library split. No moc.
class SchDocSource : public SchRenderSource {
public:
  SchDocSource();
  ~SchDocSource() override;

  SchDocSource(const SchDocSource &)            = delete;
  SchDocSource &operator=(const SchDocSource &) = delete;

  // Reads `path` -- a .qsch strictly, a .qsym with its file header
  // optional -- leaves out records the rules forbid (listed in
  // `omitted`), and builds the scene. False, with `error` set, if the
  // file cannot be read or parsed; the previous contents are kept.
  bool load(const QString &path, QString &error, QList<sch::core::recordrules::Omitted> &omitted);

  bool isLoaded() const { return scene_ != nullptr; }

  // --- SchRenderSource.
  QGraphicsScene *renderScene() const override;
  QBrush          renderBackground() const override;
  bool            isPrintExcluded(const QGraphicsItem *item) const override;
  QRectF          visibleSceneRect() const override;
  QString         sourcePath() const override { return path_; }

protected:
  // The tree outlives nothing it should not: the items hold references
  // into its records, so it is kept for the scene's lifetime -- and the
  // scene is destroyed FIRST (declared after).
  ItemTree                  tree_;
  std::unique_ptr<SchScene> scene_;
  QString                   path_;
};

} // namespace sch::render
