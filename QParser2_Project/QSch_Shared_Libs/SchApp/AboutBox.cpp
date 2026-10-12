#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable : 4996) // Qt headers reference APIs deprecated for Qt 7
#endif

#include "AboutBox.h"

#include <QMessageBox>
#include <QObject>
#include <QPixmap>

#ifdef _MSC_VER
#pragma warning(pop)
#endif

#include "AppIcon.h"

// Each shared library's name and version.
#include "QParser2_Version.h"
#include "SchAppVersion.h"
#include "SchCoreVersion.h"
#include "SchLinkVersion.h"
#include "SchOutputVersion.h"
#include "SchRenderVersion.h"

namespace sch::app {
namespace {

// The icon beside the text, in logical pixels.
constexpr int kAboutIconPx = 96;

} // namespace

QString libraryVersionsHtml() {
  struct Row {
    const char *name;
    const char *version;
  };

  const qparser2::LibraryInfo    parser = qparser2::libraryInfo();
  const sch::core::LibraryInfo   core   = sch::core::libraryInfo();
  const sch::link::LibraryInfo   link   = sch::link::libraryInfo();
  const sch::render::LibraryInfo render = sch::render::libraryInfo();
  const sch::output::LibraryInfo output = sch::output::libraryInfo();
  const sch::app::LibraryInfo    app    = sch::app::libraryInfo();

  const Row rows[] = {
      {parser.name, parser.version}, {core.name, core.version},     {link.name, link.version},
      {render.name, render.version}, {output.name, output.version}, {app.name, app.version},
  };

  // Starts with a tag, so QMessageBox takes it as rich text.
  QString html = QStringLiteral("<p>%1</p><table>").arg(QObject::tr("Shared libraries:"));
  for (const Row &row : rows)
    html += QStringLiteral("<tr><td>%1</td><td style=\"padding-left:16px\">%2</td></tr>")
                .arg(QString::fromLatin1(row.name).toHtmlEscaped(),
                     QString::fromLatin1(row.version).toHtmlEscaped());
  html += QStringLiteral("</table>");
  return html;
}

void showAbout(QWidget *parent, const QString &programName, const QString &version) {
  QMessageBox box(parent);
  box.setWindowTitle(QObject::tr("About %1").arg(programName));
  box.setText(QObject::tr("%1\nVersion %2").arg(programName, version));
  box.setInformativeText(libraryVersionsHtml());

  // The standard information icon if the program's own cannot be loaded.
  const QPixmap icon = appIconPixmap(kAboutIconPx);
  if (icon.isNull())
    box.setIcon(QMessageBox::Information);
  else
    box.setIconPixmap(icon);

  box.exec();
}

} // namespace sch::app
