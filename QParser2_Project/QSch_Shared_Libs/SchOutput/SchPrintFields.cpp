#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable : 4996) // Qt headers reference APIs deprecated for Qt 7
#endif

#include "SchPrintFields.h"

#include <QCoreApplication>
#include <QDir>
#include <QFileInfo>
#include <QLocale>
#include <QtGlobal>

#ifdef _MSC_VER
#pragma warning(pop)
#endif

namespace sch::output {

namespace {

QString tr(const char *text) {
  return QCoreApplication::translate("SchPrintFields", text);
}

constexpr double kMmPerInch = 25.4;

// A date/time by preset or Qt pattern; empty for an invalid one.
QString formatWhen(const QDateTime &when, const QString &format) {
  if (!when.isValid()) return QString();

  const QLocale locale = QLocale::system();
  const QString fmt    = format.trimmed();

  if (fmt.isEmpty() || fmt == QLatin1String("short"))
    return locale.toString(when, QLocale::ShortFormat);
  if (fmt == QLatin1String("long")) return locale.toString(when, QLocale::LongFormat);
  if (fmt == QLatin1String("iso"))
    return when.toString(QStringLiteral("yyyy-MM-dd HH:mm:ss"));
  if (fmt == QLatin1String("date"))
    return locale.toString(when.date(), QLocale::ShortFormat);
  if (fmt == QLatin1String("time"))
    return locale.toString(when.time(), QLocale::ShortFormat);

  // A pattern -- the locale supplies month and day names.
  return locale.toString(when, format);
}

QString pageInfo(const SchPrintContext &ctx) {
  if (ctx.pages <= 1) return QString();
  if (ctx.tiled)
    return tr("Page %1 of %2 (row %3, column %4)")
        .arg(ctx.page)
        .arg(ctx.pages)
        .arg(ctx.row)
        .arg(ctx.col);
  return tr("Page %1 of %2").arg(ctx.page).arg(ctx.pages);
}

// One field's value; `known` false for a name not recognised.
QString fieldValue(const QString &name, const QString &format,
                   const SchPrintContext &ctx, bool &known) {
  known = true;
  const QString key = name.trimmed().toLower();

  if (key == QLatin1String("path"))
    return ctx.path.isEmpty() ? tr("Untitled - %1").arg(ctx.title) : ctx.path;
  if (key == QLatin1String("name"))
    return ctx.path.isEmpty() ? ctx.title : QFileInfo(ctx.path).fileName();
  if (key == QLatin1String("folder"))
    return ctx.path.isEmpty()
               ? QString()
               : QDir::toNativeSeparators(QFileInfo(ctx.path).absolutePath());
  if (key == QLatin1String("title")) return ctx.title;
  if (key == QLatin1String("modified")) return formatWhen(ctx.modified, format);
  if (key == QLatin1String("printed")) return formatWhen(ctx.printed, format);
  if (key == QLatin1String("page")) return QString::number(ctx.page);
  if (key == QLatin1String("pages")) return QString::number(ctx.pages);
  if (key == QLatin1String("row")) return QString::number(ctx.row);
  if (key == QLatin1String("col")) return QString::number(ctx.col);
  if (key == QLatin1String("pageinfo")) return pageInfo(ctx);
  if (key == QLatin1String("printer")) return ctx.printer;
  if (key == QLatin1String("user")) return qEnvironmentVariable("USERNAME");

  if (key == QLatin1String("scale")) {
    if (ctx.mmPer1000 <= 0.0) return QString();
    return tr("%1 mm (%2 inch) per 1000 units")
        .arg(ctx.mmPer1000, 0, 'f', 1)
        .arg(ctx.mmPer1000 / kMmPerInch, 0, 'f', 2);
  }

  known = false;
  return QString();
}

} // namespace

namespace SchPrintFields {

QString expand(const QString &templ, const SchPrintContext &ctx) {
  QString out;
  int     i      = 0;
  int     fields = 0; // known fields seen
  int     filled = 0; // ... that expanded to something

  while (i < templ.size()) {
    const QChar ch = templ.at(i);

    // Doubled braces are literal.
    if ((ch == QLatin1Char('{') || ch == QLatin1Char('}')) && i + 1 < templ.size() &&
        templ.at(i + 1) == ch) {
      out += ch;
      i += 2;
      continue;
    }

    if (ch != QLatin1Char('{')) {
      out += ch;
      ++i;
      continue;
    }

    // An unclosed brace is literal, as is everything after it.
    const qsizetype close = templ.indexOf(QLatin1Char('}'), i + 1);
    if (close < 0) {
      out += templ.mid(i);
      break;
    }

    const QString   inner = templ.mid(i + 1, close - i - 1);
    const qsizetype colon = inner.indexOf(QLatin1Char(':'));
    const QString   name  = (colon < 0) ? inner : inner.left(colon);
    const QString   fmt   = (colon < 0) ? QString() : inner.mid(colon + 1);

    bool          known = false;
    const QString value = fieldValue(name, fmt, ctx, known);
    out += known ? value : templ.mid(i, close - i + 1);
    if (known) {
      ++fields;
      if (!value.isEmpty()) ++filled;
    }

    i = static_cast<int>(close) + 1;
  }

  // Fields, and every one of them empty: the slot says nothing, so its
  // own text goes too -- "Modified: {modified}" on an untitled document
  // prints nothing rather than a dangling "Modified:".
  if (fields > 0 && filled == 0) return QString();
  return out;
}

QList<std::pair<QString, QString>> menuEntries() {
  return {
      {tr("Full path"), QStringLiteral("{path}")},
      {tr("File name"), QStringLiteral("{name}")},
      {tr("Folder"), QStringLiteral("{folder}")},
      {tr("Document title"), QStringLiteral("{title}")},
      {QString(), QString()},
      {tr("Modified (short)"), QStringLiteral("{modified}")},
      {tr("Modified (long)"), QStringLiteral("{modified:long}")},
      {tr("Modified (ISO)"), QStringLiteral("{modified:iso}")},
      {tr("Modified date"), QStringLiteral("{modified:date}")},
      {tr("Modified, custom format..."), QStringLiteral("{modified:yyyy-MM-dd HH:mm}")},
      {QString(), QString()},
      {tr("Printed (short)"), QStringLiteral("{printed}")},
      {tr("Printed (long)"), QStringLiteral("{printed:long}")},
      {tr("Printed (ISO)"), QStringLiteral("{printed:iso}")},
      {tr("Printed date"), QStringLiteral("{printed:date}")},
      {tr("Printed time"), QStringLiteral("{printed:time}")},
      {tr("Printed, custom format..."), QStringLiteral("{printed:yyyy-MM-dd HH:mm}")},
      {QString(), QString()},
      {tr("Page info (several pages only)"), QStringLiteral("{pageinfo}")},
      {tr("Page number"), QStringLiteral("{page}")},
      {tr("Page count"), QStringLiteral("{pages}")},
      {tr("Page row"), QStringLiteral("{row}")},
      {tr("Page column"), QStringLiteral("{col}")},
      {QString(), QString()},
      {tr("Printer"), QStringLiteral("{printer}")},
      {tr("Scale"), QStringLiteral("{scale}")},
      {tr("User name"), QStringLiteral("{user}")},
  };
}

} // namespace SchPrintFields

} // namespace sch::output
