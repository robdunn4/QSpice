#pragma once
#include <QSizeF>
#include "ItemNet.h"
#include "SchItemBase.h"

namespace sch::render {

// STUB: renders the net name at pt1; rotation not yet applied (same
// caveat as SchItemText). netDesc (optional description) isn't shown.
class SchItemNet : public SchItemBase {
public:
  explicit SchItemNet(ItemNetPtr item, QGraphicsItem *parent = nullptr);

  Rect  getRect() const noexcept override;
  Point getPosition() const noexcept override;
  void  moveTo(Point p) noexcept override;

  bool           isElectrical() const override { return true; }
  QList<QPointF> connectionPoints() const override;

  // Edits the net NAME specifically (its primary identifier) -- not
  // netDesc, the optional secondary description line, which live-edit
  // doesn't yet expose a way to reach.
  // Ground-family symbols (GND/EARTH/CHASSIS) orient themselves so
  // their base sits perpendicular to whatever connects at the anchor.
  // SchConnect::updtNetAnchors() drives this; see there.
  bool isGroundSymbol() const;

  // The same test on a decoded net name, for code working from records
  // (SchConnect's editing passes).
  static bool isGroundName(const QString &name);

  int  groundAlign() const;
  void setGroundAlign(int align);

  TextKind textKind() const override;

  // The label's net name, and whether it is one of the grounds.
  // Covers the DESCRIPTION as well as the label.
  //
  // getRect() deliberately does not: a description is documentation
  // hung below the net, and including it made the selection box -- and
  // the port border, which traces it -- reach far past the label. But
  // PAINTING is a different question: whatever is drawn has to sit
  // inside the bounding rect or Qt never repaints it, which left the old
  // description on screen after a drag until a zoom forced a full
  // redraw.
  QRectF boundingRect() const override;

  QString netName() const;

  // A PORT is a net whose scope is the whole netlist rather than this
  // sheet. Nothing about it differs here but the drawing: same record,
  // same connectivity, same naming -- only netType is non-zero.
  // ASSUMED: any non-zero netType is a port. QSpice appears to use the
  // field for nothing else, and ports have no direction -- so the only
  // value we ever WRITE is 1, while any non-zero value READS as a port
  // so a file carrying something else still draws correctly.
  bool isPort() const { return item_->netType.getValue() != 0; }

  // The description, as stored. Empty CLEARS the field rather than
  // storing an empty string, so adding a description and deleting it
  // leaves the record as it was.
  // Hides the description for the duration of a DRAG.
  //
  // The description is documentation hung below the label, with no
  // anchor of its own: while the label is being moved and re-anchored it
  // has nothing stable to hang from, so it is taken down until the move
  // settles rather than swinging about after it.
  void setDescHidden(bool hidden) {
    if (descHidden_ == hidden) return;

    prepareGeometryChange();
    descHidden_ = hidden;
    update();
  }

  QString netDesc() const;
  void    setNetDesc(const QString &text);

  void setPort(bool on) {
    prepareGeometryChange();
    item_->netType.setValue(on ? 1 : 0);
    update();
  }


  double fontSize() const override;
  void   setFontSize(double size) override;
  SchTextEditInfo       textEditInfo() const override;
  void                  applyTextEdit(const SchTextEditInfo &info) override;

protected:
  bool descHidden_ = false;

  void paint(QPainter *painter, const QStyleOptionGraphicsItem *option,
             QWidget *widget) override;

private:
  ItemNetPtr item_;
};

} // namespace sch::render
