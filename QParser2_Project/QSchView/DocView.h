#pragma once

#include <QStringList>

#include "ItemTree.h"
#include "SchViewBase.h"

class QDragEnterEvent;
class QDragMoveEvent;
class QDropEvent;
class QMimeData;
class QMouseEvent;

// The viewer's canvas: SchRender's read-only view, plus file drops and
// double-clicks on components.
//
// Dropped files are not opened here; the view reports them and the
// window decides (ViewerWin::openFiles()).
class DocView : public sch::render::SchViewBase {
  Q_OBJECT

public:
  explicit DocView(QWidget *parent = nullptr);

  // The files a drag carries, if EVERY one is a local .qsch or .qsym
  // (without regard to case); otherwise empty, and the drag is refused.
  // Shared with the window, which takes drops while it is empty.
  static QStringList droppableFiles(const QMimeData *mime);

signals:
  // Emitted from dropEvent(): the receiver must not delete this view
  // from inside the signal.
  void filesDropped(const QStringList &paths);

  // A left double-click on a component: its «component» record. Whether
  // it is a hierarchical block is the receiver's question.
  void componentDoubleClicked(const NodePtr &compNode);

protected:
  void dragEnterEvent(QDragEnterEvent *event) override;
  void dragMoveEvent(QDragMoveEvent *event) override;
  void dropEvent(QDropEvent *event) override;

  void mouseDoubleClickEvent(QMouseEvent *event) override;

  // The topmost component at `viewPos` -- anywhere on its body, its line
  // work or its labels -- or null.
  NodePtr componentAt(const QPoint &viewPos) const;
};
