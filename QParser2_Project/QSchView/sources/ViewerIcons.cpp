#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable : 4996) // Qt headers reference APIs deprecated for Qt 7
#endif

#include "ViewerIcons.h"

#include <QByteArray>
#include <QPainter>
#include <QPixmap>
#include <QSvgRenderer>

#ifdef _MSC_VER
#pragma warning(pop)
#endif

namespace ViewerIcons {
namespace {

// The program icon's navy, and the white the shapes are filled with.
#define NAVY "#1A3F7A"
#define FILL "#FFFFFF"
#define PALE "#CFDBEE"

// Every icon is drawn on a 24-unit square.
#define SVG_OPEN                                                                               \
  "<svg xmlns='http://www.w3.org/2000/svg' viewBox='0 0 24 24' fill='none' stroke='" NAVY   \
  "' stroke-width='1.6' stroke-linejoin='round' stroke-linecap='round'>"
#define SVG_CLOSE "</svg>"

// The sizes, in device pixels, rendered into each icon.
constexpr int kSizes[] = {16, 20, 24, 32, 40, 48};

QIcon fromSvg(const char *svg) {
  QSvgRenderer renderer{QByteArray(svg)};
  QIcon        icon;
  for (int px : kSizes) {
    QPixmap pixmap(px, px);
    pixmap.fill(Qt::transparent);
    QPainter painter(&pixmap);
    painter.setRenderHint(QPainter::Antialiasing);
    renderer.render(&painter);
    painter.end();
    icon.addPixmap(pixmap);
  }
  return icon;
}

// A magnifying glass, with `inner` drawn in its lens.
#define LENS(inner)                                                                            \
  SVG_OPEN "<circle cx='10' cy='10' r='6.5' fill='" FILL "'/>"                               \
  "<line x1='14.8' y1='14.8' x2='21' y2='21' stroke-width='3'/>" inner SVG_CLOSE

} // namespace

QIcon open() {
  // An open folder.
  return fromSvg(SVG_OPEN
                 "<path d='M2.5 19.5V5.5h6l2 2h8v3' fill='" PALE "'/>"
                 "<path d='M2.5 19.5l3-9h16l-3 9z' fill='" FILL "'/>" SVG_CLOSE);
}

QIcon recent() {
  // A clock.
  return fromSvg(SVG_OPEN "<circle cx='12' cy='12' r='9' fill='" FILL "'/>"
                          "<path d='M12 6.5V12l3.8 2.4'/>" SVG_CLOSE);
}

QIcon print() {
  // A printer: paper in, body, paper out.
  return fromSvg(SVG_OPEN
                 "<rect x='6.5' y='2.5' width='11' height='6' fill='" FILL "'/>"
                 "<rect x='2.5' y='8.5' width='19' height='9' rx='1.5' fill='" PALE "'/>"
                 "<rect x='6.5' y='13.5' width='11' height='8' fill='" FILL "'/>"
                 "<line x1='9' y1='16.5' x2='15' y2='16.5' stroke-width='1.2'/>"
                 "<line x1='9' y1='19' x2='15' y2='19' stroke-width='1.2'/>"
                 "<circle cx='18.3' cy='11.3' r='0.6' fill='" NAVY "' stroke='none'/>" SVG_CLOSE);
}

QIcon exportSvg() {
  // A page with a folded corner, and an arrow leaving it.
  return fromSvg(SVG_OPEN
                 "<path d='M4.5 2.5h9l4 4v15h-13z' fill='" FILL "'/>"
                 "<path d='M13.5 2.5v4h4'/>"
                 "<path d='M9 14h12.5M18 10.5l3.5 3.5-3.5 3.5' stroke-width='1.9'/>" SVG_CLOSE);
}

QIcon exitApp() {
  // A door, and an arrow leaving through it.
  return fromSvg(SVG_OPEN
                 "<path d='M13.5 3.5h-9v17h9' fill='" FILL "'/>"
                 "<path d='M9.5 12h12M18 8.5l3.5 3.5-3.5 3.5' stroke-width='1.9'/>" SVG_CLOSE);
}

QIcon zoomIn() {
  return fromSvg(LENS("<path d='M7 10h6M10 7v6' stroke-width='1.9'/>"));
}

QIcon zoomOut() { return fromSvg(LENS("<path d='M7 10h6' stroke-width='1.9'/>")); }

QIcon zoomToFit() {
  // Four corners closing in on a drawing.
  return fromSvg(SVG_OPEN
                 "<path d='M3 8V3h5M16 3h5v5M21 16v5h-5M8 21H3v-5' stroke-width='1.9'/>"
                 "<rect x='7.5' y='8.5' width='9' height='7' fill='" PALE "' stroke-width='1.3'/>"
                 SVG_CLOSE);
}

QIcon help() {
  // A question mark in a circle.
  return fromSvg(SVG_OPEN "<circle cx='12' cy='12' r='9.5' fill='" FILL "'/>"
                          "<path d='M9.2 9.3a2.9 2.9 0 1 1 4.1 2.7c-.9.4-1.3 1-1.3 1.9v.8'"
                          " stroke-width='1.9'/>"
                          "<circle cx='12' cy='17.6' r='1.15' fill='" NAVY "' stroke='none'/>"
                          SVG_CLOSE);
}

QIcon resistorPolarity() {
  // A resistor's zigzag, with a "+" above its left lead.
  return fromSvg(SVG_OPEN
                 "<path d='M1.5 15h3l1.5-3 3 6 3-6 3 6 3-6 1.5 3h3' stroke-width='1.7'/>"
                 "<path d='M5 4v6M2 7h6' stroke-width='1.9'/>" SVG_CLOSE);
}

QIcon printDefaults() {
  // A printer, with a tick: the settings it starts with.
  return fromSvg(SVG_OPEN
                 "<rect x='6.5' y='2.5' width='11' height='5' fill='" FILL "'/>"
                 "<rect x='2.5' y='7.5' width='19' height='9' rx='1.5' fill='" PALE "'/>"
                 "<rect x='6.5' y='13.5' width='11' height='8' fill='" FILL "'/>"
                 "<path d='M9.2 17.4l1.9 1.9 3.8-3.8' stroke-width='1.7'/>" SVG_CLOSE);
}

QIcon colorTheme() {
  // A painter's palette.
  return fromSvg(SVG_OPEN
                 "<path d='M12 2.5a9.5 9.5 0 1 0 0 19c1.6 0 2-1.2 1.4-2.3-.7-1.2 0-2.7 1.5-2.7h2.1"
                 "a4.5 4.5 0 0 0 4.5-4.5c0-5.3-4.3-9.5-9.5-9.5z' fill='" FILL "'/>"
                 "<circle cx='7.5' cy='11' r='1.5' fill='#D04040' stroke='none'/>"
                 "<circle cx='10' cy='6.8' r='1.5' fill='#E0A020' stroke='none'/>"
                 "<circle cx='15' cy='6.8' r='1.5' fill='#30A050' stroke='none'/>"
                 "<circle cx='17.6' cy='11' r='1.5' fill='#3060C0' stroke='none'/>" SVG_CLOSE);
}

QIcon headerFooter() {
  // A page with a band across the top and the bottom.
  return fromSvg(SVG_OPEN
                 "<rect x='4.5' y='2.5' width='15' height='19' fill='" FILL "'/>"
                 "<rect x='4.5' y='2.5' width='15' height='4' fill='" PALE "'/>"
                 "<rect x='4.5' y='17.5' width='15' height='4' fill='" PALE "'/>"
                 "<path d='M8 11.5h8M8 14h5' stroke-width='1.2'/>" SVG_CLOSE);
}

} // namespace ViewerIcons
