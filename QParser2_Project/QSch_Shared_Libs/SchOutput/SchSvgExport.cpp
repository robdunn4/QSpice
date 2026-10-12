#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable : 4996) // Qt headers reference APIs deprecated for Qt 7
#endif

#include "SchSvgExport.h"

#include <QBuffer>
#include <QCoreApplication>
#include <QFile>
#include <QGraphicsItem>
#include <QGraphicsScene>
#include <QPainter>
#include <QStyleOptionGraphicsItem>
#include <QSvgGenerator>

#ifdef _MSC_VER
#pragma warning(pop)
#endif

#include <memory>

#include "ItemSym.h"
#include "ItemText.h"
#include "ItemTree.h"
#include "SchItemArc3P.h"
#include "SchItemArc4P.h"
#include "SchItemCoil.h"
#include "SchItemComp.h"
#include "SchItemEllipse.h"
#include "SchItemJunction.h"
#include "SchItemLine.h"
#include "SchItemNet.h"
#include "SchItemPin.h"
#include "SchItemRect.h"
#include "SchItemTap.h"
#include "SchItemText.h"
#include "SchItemTriangle.h"
#include "SchItemWire.h"
#include "SchItemZigzag.h"
#include "SchPen.h"
#include "SchRefDes.h"
#include "SchStringDecode.h"
#include "SchRenderSource.h"
#include "SelHiLiteItem.h"

namespace sch::output {

// SchRender names used in this file.
using sch::render::SchItemBase;
using sch::render::SchItemArc3P;
using sch::render::SchItemArc4P;
using sch::render::SchItemCoil;
using sch::render::SchItemComp;
using sch::render::SchItemEllipse;
using sch::render::SchItemJunction;
using sch::render::SchItemLine;
using sch::render::SchItemNet;
using sch::render::SchItemPin;
using sch::render::SchItemRect;
using sch::render::SchItemTap;
using sch::render::SchItemText;
using sch::render::SchItemTriangle;
using sch::render::SchItemWire;
using sch::render::SchItemZigzag;
using sch::render::SelHiLiteStyle;
using sch::render::SchRenderSource;
namespace SchPen = sch::render::SchPen;

namespace {

QString tr(const char *text) {
  return QCoreApplication::translate("SchSvgExport", text);
}

// Device units: 1200 per inch. Fine enough that ordinary lines are real,
// SCALED widths rather than hairlines, and the thinnest-line minimum
// converts as it does for a printer.
constexpr int    kDpi       = 1200;
constexpr double kMmPerInch = 25.4;
constexpr double kMarginMm  = 2.0;

// The drawing part of one generator's output: after its header and
// <defs>, up to </svg>.
//
// No element should arrive with a cosmetic pen -- SchPen::vectorExport
// turns them into ordinary pens of the same width on the page, which
// Qt writes as plain stroke widths in the element's own (transformed)
// units. Anything that slips through comes out as a NON-SCALING stroke,
// a fixed number of the viewer's screen pixels; that marking is dropped
// as a safety net, leaving a stroke only slightly off in width.
QByteArray drawingOf(const QByteArray &svg) {
  qsizetype start = svg.indexOf("<svg");
  if (start < 0) return QByteArray();
  start = svg.indexOf('>', start);
  if (start < 0) return QByteArray();
  ++start;

  const qsizetype defsEnd = svg.indexOf("</defs>", start);
  if (defsEnd >= 0) start = defsEnd + 7;

  const qsizetype end = svg.lastIndexOf("</svg>");
  if (end < start) return QByteArray();

  QByteArray out = svg.mid(start, end - start).trimmed();
  out.replace("vector-effect=\"non-scaling-stroke\"", "");
  return out;
}

// Whether anything was actually drawn -- Qt writes its state groups
// either way.
bool drawsSomething(const QByteArray &body) {
  for (const char *tag : {"<path", "<polyline", "<polygon", "<rect", "<ellipse",
                          "<circle", "<line", "<text", "<image"})
    if (body.contains(tag)) return true;
  return false;
}

struct CompInfo {
  QString refdes;
  QString value;
  QString symbol;
};

// Designator, value (attribute text 1 -- positional, as everywhere else)
// and symbol name.
CompInfo compInfo(const SchItemComp *comp) {
  CompInfo out;
  out.refdes = sch::core::refdes::ofNode(comp->node());

  const NodePtr node = comp->node();
  if (node == nullptr) return out;

  const NodePtr symNode = node->findFirstOf(QPI::SYM);
  if (symNode == nullptr) return out;

  const ItemSymPtr sym = std::static_pointer_cast<ItemSym>(symNode->item());
  if (sym != nullptr) out.symbol = sch::core::decodeQSpiceString(sym->text.getValue()).trimmed();

  int textIndex = 0;
  for (const NodePtr &child : symNode->children()) {
    if (child->getEnumID() != QPI::TEXT) continue;
    if (textIndex++ != 1) continue;

    const ItemTextPtr text = std::static_pointer_cast<ItemText>(child->item());
    if (text != nullptr) out.value = sch::core::decodeQSpiceString(text->text.getValue()).trimmed();
    break;
  }
  return out;
}

QString firstLine(const QString &text) {
  QString line = text.section(QLatin1Char('\n'), 0, 0).trimmed();
  if (line.size() > 40) line = line.left(39) + QChar(0x2026);
  return line;
}

} // namespace

// ===========================================================================

bool SchSvgExport::write(const QString &path, const SchRenderSource *view, const QString &title,
                         double mmPer1000, double minLineMm, const QString &creator) {
  if (view == nullptr || view->renderScene() == nullptr || mmPer1000 <= 0.0) return false;

  const QRectF drawing = view->printableSceneRect();
  if (drawing.isEmpty()) return false;

  const double unitsPerMm = 1000.0 / mmPer1000;
  const double margin     = kMarginMm * unitsPerMm;
  const QRectF source     = drawing.adjusted(-margin, -margin, margin, margin);
  const double pxPerUnit  = mmPer1000 / 1000.0 * kDpi / kMmPerInch;

  SchSvgExport exporter(view, source, pxPerUnit);

  QByteArray data;
  if (!exporter.build(title, mmPer1000, minLineMm, creator, data)) return false;

  QFile file(path);
  if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) return false;
  if (file.write(data) == data.size()) return true;

  file.close();
  file.remove();
  return false;
}

SchSvgExport::SchSvgExport(const SchRenderSource *schView, const QRectF &sourceRect,
                           double unitScale)
    : view(schView), source(sourceRect), pxPerUnit(unitScale) {
  viewBox = QRectF(0.0, 0.0, source.width() * pxPerUnit,
                   source.height() * pxPerUnit);
  sizePx  = viewBox.size().toSize();

  // Scene Y runs UP, SVG's down: the source's top edge (largest Y) is
  // the drawing's y = 0.
  page = QTransform(pxPerUnit, 0.0, 0.0, -pxPerUnit, -source.left() * pxPerUnit,
                    source.bottom() * pxPerUnit);
}

bool SchSvgExport::build(const QString &title, double mmPer1000, double minLineMm,
                         const QString &creator, QByteArray &out) {
  mapWireNets();

  QByteArray graphics, symbols, labels, junctions, texts;
  QList<QString>             netOrder;
  QHash<QString, QByteArray> netBodies;
  int counter = 0;

  // Print state for the duration: no highlights, the minimum line
  // width, and no cosmetic pens.
  SelHiLiteStyle::suppressed = true;
  SchPen::printMinDevicePx   = minLineMm * kDpi / kMmPerInch;
  SchPen::vectorExport       = true;

  // Bottom to top, so each layer keeps the elements' stacking.
  for (QGraphicsItem *item : view->renderScene()->items(Qt::AscendingOrder)) {
    if (item->parentItem() != nullptr) continue;
    if (!item->isVisible() || view->isPrintExcluded(item)) continue;
    if (dynamic_cast<const SchItemBase *>(item) == nullptr) continue;

    const QByteArray body = piece(item);
    if (!drawsSomething(body)) continue;
    ++counter;

    switch (layerOf(item)) {
    case Layer::Symbols: {
      if (const SchItemComp *comp = dynamic_cast<const SchItemComp *>(item)) {
        const CompInfo info  = compInfo(comp);
        const QString  label = info.refdes.isEmpty() ? tr("Component") : info.refdes;
        QString        tip   = label;
        if (!info.value.isEmpty()) tip += QStringLiteral(" – ") + info.value;

        symbols += group(body,
                         {{QStringLiteral("id"), uniqueId(label)},
                          {QStringLiteral("inkscape:label"), label},
                          {QStringLiteral("data-refdes"), info.refdes},
                          {QStringLiteral("data-value"), info.value},
                          {QStringLiteral("data-symbol"), info.symbol}},
                         tip);
        break;
      }
      symbols += group(body, {{QStringLiteral("id"),
                               uniqueId(QStringLiteral("pin-%1").arg(counter))},
                              {QStringLiteral("inkscape:label"), kindLabel(item)}});
      break;
    }

    case Layer::Wires: {
      const QString net = netAt(item);
      if (!netBodies.contains(net)) netOrder.append(net);
      netBodies[net] += group(
          body, {{QStringLiteral("id"),
                  uniqueId(QStringLiteral("%1-%2")
                               .arg(dynamic_cast<const SchItemTap *>(item) != nullptr
                                        ? QStringLiteral("tap")
                                        : QStringLiteral("wire"))
                               .arg(counter))},
                 {QStringLiteral("inkscape:label"), kindLabel(item)}});
      break;
    }

    case Layer::Labels: {
      const QString net = netAt(item);
      labels += group(body, {{QStringLiteral("id"), uniqueId(QStringLiteral("label-") + net)},
                             {QStringLiteral("inkscape:label"), net},
                             {QStringLiteral("data-net"), net}});
      break;
    }

    case Layer::Junctions: {
      const QString net = netAt(item);
      junctions += group(body, {{QStringLiteral("id"),
                                 uniqueId(QStringLiteral("junction-%1").arg(counter))},
                                {QStringLiteral("inkscape:label"), kindLabel(item)},
                                {QStringLiteral("data-net"), net}});
      break;
    }

    case Layer::Text: {
      const SchItemBase *schItem = dynamic_cast<const SchItemBase *>(item);
      const QString      text    = firstLine(schItem->textEditInfo().text);
      texts += group(body, {{QStringLiteral("id"),
                             uniqueId(QStringLiteral("text-%1").arg(counter))},
                            {QStringLiteral("inkscape:label"),
                             text.isEmpty() ? kindLabel(item) : text}});
      break;
    }

    case Layer::Graphics:
      graphics += group(body, {{QStringLiteral("id"),
                                uniqueId(QStringLiteral("graphic-%1").arg(counter))},
                               {QStringLiteral("inkscape:label"), kindLabel(item)}});
      break;
    }
  }

  SchPen::vectorExport       = false;
  SchPen::printMinDevicePx   = 0.0;
  SelHiLiteStyle::suppressed = false;

  // Wires, one sub-group per net, in order of first appearance.
  QByteArray wires;
  for (const QString &net : netOrder) {
    const QString shown = net.isEmpty() ? tr("(unnamed)") : net;
    wires += group(netBodies.value(net),
                   {{QStringLiteral("id"),
                     uniqueId(QStringLiteral("net-") +
                              (net.isEmpty() ? QStringLiteral("unnamed") : net))},
                    {QStringLiteral("inkscape:label"), shown},
                    {QStringLiteral("data-net"), net}});
  }

  // --- Assembly ----------------------------------------------------------
  const double widthMm  = source.width() * mmPer1000 / 1000.0;
  const double heightMm = source.height() * mmPer1000 / 1000.0;
  const QString vb      = QStringLiteral("0 0 %1 %2")
                         .arg(viewBox.width(), 0, 'f', 3)
                         .arg(viewBox.height(), 0, 'f', 3);

  out.clear();
  out += "<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"no\"?>\n";
  out += QStringLiteral(
             "<svg xmlns=\"http://www.w3.org/2000/svg\"\n"
             "     xmlns:xlink=\"http://www.w3.org/1999/xlink\"\n"
             "     xmlns:inkscape=\"http://www.inkscape.org/namespaces/inkscape\"\n"
             "     xmlns:sodipodi=\"http://sodipodi.sourceforge.net/DTD/sodipodi-0.dtd\"\n"
             "     width=\"%1mm\" height=\"%2mm\" viewBox=\"%3\" version=\"1.1\">\n")
             .arg(widthMm, 0, 'f', 3)
             .arg(heightMm, 0, 'f', 3)
             .arg(vb)
             .toUtf8();
  out += "<title>" + esc(title).toUtf8() + "</title>\n";
  out += "<desc>" +
         esc(tr("Exported by %1 at %2 mm per 1000 units")
                 .arg(creator)
                 .arg(mmPer1000, 0, 'f', 1))
             .toUtf8() +
         "</desc>\n";
  out += "<sodipodi:namedview id=\"namedview\" inkscape:document-units=\"mm\"/>\n";

  // The background: locked, so it cannot be grabbed by accident.
  const QColor canvas = view->renderBackground().color();
  out += QStringLiteral("<g inkscape:groupmode=\"layer\" id=\"layer-background\" "
                        "inkscape:label=\"%1\" sodipodi:insensitive=\"true\">\n"
                        "<rect x=\"0\" y=\"0\" width=\"%2\" height=\"%3\" fill=\"%4\"/>\n"
                        "</g>\n")
             .arg(esc(tr("Background")))
             .arg(viewBox.width(), 0, 'f', 3)
             .arg(viewBox.height(), 0, 'f', 3)
             .arg(canvas.name())
             .toUtf8();

  const QList<std::pair<QString, const QByteArray *>> layers = {
      {tr("Graphics"), &graphics},       {tr("Symbols"), &symbols},
      {tr("Wires"), &wires},             {tr("Net labels"), &labels},
      {tr("Junctions"), &junctions},     {tr("Text"), &texts},
  };
  for (const std::pair<QString, const QByteArray *> &layer : layers) {
    if (layer.second->isEmpty()) continue;

    QString id = QStringLiteral("layer-") + layer.first.toLower();
    id.replace(QLatin1Char(' '), QLatin1Char('-'));

    out += QStringLiteral("<g inkscape:groupmode=\"layer\" id=\"%1\" inkscape:label=\"%2\">\n")
               .arg(esc(uniqueId(id)), esc(layer.first))
               .toUtf8();
    out += *layer.second;
    out += "</g>\n";
  }

  out += "</svg>\n";
  return true;
}

QByteArray SchSvgExport::piece(QGraphicsItem *item) const {
  QBuffer buffer;
  if (!buffer.open(QIODevice::WriteOnly)) return QByteArray();

  QSvgGenerator generator;
  generator.setOutputDevice(&buffer);
  generator.setResolution(kDpi);
  generator.setSize(sizePx);
  generator.setViewBox(viewBox);

  QPainter painter;
  if (!painter.begin(&generator)) return QByteArray();
  paintTree(painter, item);
  painter.end();

  return drawingOf(buffer.data());
}

void SchSvgExport::paintTree(QPainter &painter, QGraphicsItem *item) const {
  if (!item->isVisible() || view->isPrintExcluded(item)) return;

  // childItems() is already in stacking order; those stacked behind the
  // parent go first, as the scene paints them.
  const QList<QGraphicsItem *> kids = item->childItems();
  for (QGraphicsItem *kid : kids)
    if (kid->flags() & QGraphicsItem::ItemStacksBehindParent || kid->zValue() < 0.0)
      paintTree(painter, kid);

  painter.save();
  painter.setTransform(item->sceneTransform() * page);
  QStyleOptionGraphicsItem option;
  option.exposedRect = item->boundingRect();
  item->paint(&painter, &option, nullptr);
  painter.restore();

  for (QGraphicsItem *kid : kids)
    if (!(kid->flags() & QGraphicsItem::ItemStacksBehindParent) && kid->zValue() >= 0.0)
      paintTree(painter, kid);
}

QString SchSvgExport::uniqueId(const QString &wanted) {
  QString id;
  for (const QChar ch : wanted) {
    const bool keep = (ch.unicode() < 128 && ch.isLetterOrNumber()) ||
                      ch == QLatin1Char('-') || ch == QLatin1Char('_') ||
                      ch == QLatin1Char('.');
    id += keep ? ch : QLatin1Char('_');
  }

  // An XML id starts with a letter or an underscore.
  if (id.isEmpty() || !(id.at(0).isLetter() || id.at(0) == QLatin1Char('_')))
    id.prepend(QLatin1Char('_'));

  QString candidate = id;
  for (int n = 2; usedIds.contains(candidate); ++n)
    candidate = QStringLiteral("%1-%2").arg(id).arg(n);

  usedIds.insert(candidate);
  return candidate;
}

QByteArray SchSvgExport::group(const QByteArray &body,
                               const QList<std::pair<QString, QString>> &attrs,
                               const QString &title) {
  QByteArray out = "<g";
  for (const std::pair<QString, QString> &attr : attrs) {
    if (attr.second.isEmpty()) continue;
    out += ' ' + attr.first.toUtf8() + "=\"" + esc(attr.second).toUtf8() + '"';
  }
  out += ">\n";
  if (!title.isEmpty()) out += "<title>" + esc(title).toUtf8() + "</title>\n";
  out += body;
  out += "\n</g>\n";
  return out;
}

SchSvgExport::Layer SchSvgExport::layerOf(const QGraphicsItem *item) {
  if (dynamic_cast<const SchItemComp *>(item) != nullptr) return Layer::Symbols;
  if (dynamic_cast<const SchItemPin *>(item) != nullptr) return Layer::Symbols;
  if (dynamic_cast<const SchItemWire *>(item) != nullptr) return Layer::Wires;
  if (dynamic_cast<const SchItemTap *>(item) != nullptr) return Layer::Wires;
  if (dynamic_cast<const SchItemNet *>(item) != nullptr) return Layer::Labels;
  if (dynamic_cast<const SchItemJunction *>(item) != nullptr) return Layer::Junctions;
  if (dynamic_cast<const SchItemText *>(item) != nullptr) return Layer::Text;
  return Layer::Graphics;
}

QString SchSvgExport::kindLabel(const QGraphicsItem *item) {
  if (dynamic_cast<const SchItemWire *>(item) != nullptr) return tr("Wire");
  if (dynamic_cast<const SchItemTap *>(item) != nullptr) return tr("Bus tap");
  if (dynamic_cast<const SchItemJunction *>(item) != nullptr) return tr("Junction");
  if (dynamic_cast<const SchItemPin *>(item) != nullptr) return tr("Pin");
  if (dynamic_cast<const SchItemText *>(item) != nullptr) return tr("Text");

  if (const SchItemRect *rect = dynamic_cast<const SchItemRect *>(item))
    return rect->hasImage() ? tr("Image") : tr("Rectangle");
  if (dynamic_cast<const SchItemEllipse *>(item) != nullptr) return tr("Ellipse");
  if (dynamic_cast<const SchItemTriangle *>(item) != nullptr) return tr("Triangle");
  if (dynamic_cast<const SchItemLine *>(item) != nullptr) return tr("Line");
  if (dynamic_cast<const SchItemArc3P *>(item) != nullptr ||
      dynamic_cast<const SchItemArc4P *>(item) != nullptr)
    return tr("Arc");
  if (dynamic_cast<const SchItemZigzag *>(item) != nullptr) return tr("Zigzag");
  if (dynamic_cast<const SchItemCoil *>(item) != nullptr) return tr("Coil");
  return tr("Element");
}

void SchSvgExport::mapWireNets() {
  wireNetAt.clear();

  for (QGraphicsItem *item : view->renderScene()->items()) {
    const SchItemWire *wire = dynamic_cast<const SchItemWire *>(item);
    if (wire == nullptr) continue;

    const QString net = wire->netName().trimmed();
    if (net.isEmpty()) continue;

    for (const QPointF &p : wire->connectionPoints())
      wireNetAt.insert(pointKey(wire->mapToScene(p)), net);
  }
}

QString SchSvgExport::netAt(const QGraphicsItem *item) const {
  if (const SchItemWire *wire = dynamic_cast<const SchItemWire *>(item))
    return wire->netName().trimmed();
  if (const SchItemNet *label = dynamic_cast<const SchItemNet *>(item))
    return label->netName().trimmed();

  // Taps and junctions carry no name: whichever wire meets them.
  const SchItemBase *schItem = dynamic_cast<const SchItemBase *>(item);
  if (schItem == nullptr) return QString();

  for (const QPointF &p : schItem->connectionPoints()) {
    const QString net = wireNetAt.value(pointKey(item->mapToScene(p)));
    if (!net.isEmpty()) return net;
  }
  return QString();
}

QString SchSvgExport::pointKey(const QPointF &p) {
  return QStringLiteral("%1,%2").arg(qRound(p.x())).arg(qRound(p.y()));
}

QString SchSvgExport::esc(const QString &text) { return text.toHtmlEscaped(); }

} // namespace sch::output
