#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(                                                               \
    disable : 4996)   // Qt headers reference APIs deprecated for Qt 7
#endif

#include "CodeTemplate.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QLocale>
#include <QRegularExpression>
#include <QStringDecoder>

// Inside the block too: it brings in Qt headers of its own.
#include "SchStringDecode.h"

#ifdef _MSC_VER
#pragma warning(pop)
#endif

#include <algorithm>
#include <iterator>

namespace sch::codegen {

namespace {

// Options requiring options requiring ...: deep enough for any real
// template, shallow enough that a loop ends.
constexpr int kMaxRequiresDepth = 16;

// "NAME" in {{NAME}}.
const QRegularExpression &placeholderRe() {
  static const QRegularExpression re(
      QStringLiteral(R"(\{\{([A-Za-z_][A-Za-z0-9_]*)\}\})"));
  return re;
}

// A file's text: UTF-8 if it is valid UTF-8, else Windows-1252.
QString decodeFile(const QByteArray &bytes) {
  QStringDecoder utf8(QStringDecoder::Utf8);
  QString        text = utf8(bytes);
  if (utf8.hasError()) text = sch::core::decodeCp1252(bytes);
  text.remove(QChar(0xFEFF));
  return text;
}

// The quoted string starting at `at` in `text` -- no escapes, a label
// cannot hold a double quote. False if there is none; `at` moves past it.
bool takeQuoted(const QString &text, qsizetype &at, QString &out) {
  while (at < text.size() && text.at(at).isSpace()) ++at;
  if (at >= text.size() || text.at(at) != QLatin1Char('"')) return false;

  const qsizetype close = text.indexOf(QLatin1Char('"'), at + 1);
  if (close < 0) return false;

  out = text.mid(at + 1, close - at - 1);
  at  = close + 1;
  return true;
}

// The word starting at `at`; empty if none. `at` moves past it.
QString takeWord(const QString &text, qsizetype &at) {
  while (at < text.size() && text.at(at).isSpace()) ++at;
  const qsizetype start = at;
  while (at < text.size() && !text.at(at).isSpace()) ++at;
  return text.mid(start, at - start);
}

// Whether every open @if keeps its lines.
bool allKept(const QList<bool> &keep) {
  for (const bool k : keep) {
    if (!k) return false;
  }
  return true;
}

bool isIdentifier(const QString &word) {
  static const QRegularExpression re(
      QStringLiteral(R"(^[A-Za-z_][A-Za-z0-9_]*$)"));
  return re.match(word).hasMatch();
}

}   // namespace

bool Context::isOn(const QString &id, const QList<Option> &options) const {
  QString current = id;

  for (int depth = 0; depth < kMaxRequiresDepth; ++depth) {
    if (!on.contains(current)) return false;

    QString needs;
    for (const Option &option : options) {
      if (option.id == current) needs = option.needs;
    }
    if (needs.isEmpty()) return true;

    current = needs;
  }

  return false;
}

bool Template::parse(
    const QString &text, const QString &path, Template &tmpl, QString &error) {
  tmpl       = Template();
  tmpl.path_ = path;

  const QString where = QFileInfo(path).fileName();

  QStringList lines =
      QString(text).remove(QLatin1Char('\r')).split(QLatin1Char('\n'));

  // The file's own last line end is not an empty line of output.
  if (!lines.isEmpty() && lines.last().isEmpty()) lines.removeLast();

  bool sawFormat  = false;
  int  depth      = 0;
  bool inElse[64] = {};

  for (int i = 0; i < lines.size(); ++i) {
    const QString &line   = lines.at(i);
    const int      number = i + 1;

    const QString lineError =
        QStringLiteral("%1, line %2: ").arg(where).arg(number);

    if (!line.startsWith(QLatin1Char('@'))) {
      if (!sawFormat) {
        error = lineError + QStringLiteral("text before \"@format\"");
        return false;
      }
      tmpl.lines_.append({Line::Type::Text, line, number});
      continue;
    }

    if (line.startsWith(QStringLiteral("@#"))) continue;

    if (line.startsWith(QStringLiteral("@@"))) {
      tmpl.lines_.append({Line::Type::Text, line.mid(1), number});
      continue;
    }

    qsizetype     at        = 1;
    const QString directive = takeWord(line, at);

    if (directive == QLatin1String("format")) {
      bool      ok      = false;
      const int version = takeWord(line, at).toInt(&ok);
      if (!ok || version != kFormatVersion) {
        error = lineError +
            QStringLiteral(
                "\"@format %1\" is the only format this program "
                "reads")
                .arg(kFormatVersion);
        return false;
      }
      sawFormat = true;
      continue;
    }

    if (!sawFormat) {
      error = lineError + QStringLiteral("\"@format\" must come first");
      return false;
    }

    if (directive == QLatin1String("template")) {
      if (!takeQuoted(line, at, tmpl.title_) ||
          tmpl.title_.trimmed().isEmpty()) {
        error =
            lineError + QStringLiteral("\"@template\" needs a title in quotes");
        return false;
      }
      tmpl.title_ = tmpl.title_.trimmed();
      continue;
    }

    if (directive == QLatin1String("kind")) {
      tmpl.kind_ = takeWord(line, at).toLower();
      if (tmpl.kind_.isEmpty()) {
        error = lineError +
            QStringLiteral("\"@kind\" needs a kind: cpp, verilog or socket");
        return false;
      }
      continue;
    }

    if (directive == QLatin1String("compiler")) {
      tmpl.compiler_ = takeWord(line, at);
      if (tmpl.compiler_.isEmpty()) {
        error =
            lineError + QStringLiteral("\"@compiler\" needs a compiler's name");
        return false;
      }
      continue;
    }

    if (directive == QLatin1String("option")) {
      Option option;
      option.id = takeWord(line, at);
      if (!isIdentifier(option.id)) {
        error = lineError +
            QStringLiteral("\"@option\" needs an id (letters, digits, _)");
        return false;
      }

      for (const Option &existing : tmpl.options_) {
        if (existing.id != option.id) continue;
        error = lineError +
            QStringLiteral("option \"%1\" is declared twice").arg(option.id);
        return false;
      }

      if (!takeQuoted(line, at, option.label)) {
        error = lineError +
            QStringLiteral("option \"%1\" needs a label in quotes")
                .arg(option.id);
        return false;
      }

      for (QString word = takeWord(line, at); !word.isEmpty();
          word          = takeWord(line, at)) {
        if (word == QLatin1String("default=on")) {
          option.defaultOn = true;
          continue;
        }
        if (word == QLatin1String("default=off")) {
          option.defaultOn = false;
          continue;
        }
        if (word.startsWith(QStringLiteral("requires="))) {
          option.needs = word.mid(9);
          bool known   = false;
          for (const Option &existing : tmpl.options_) {
            if (existing.id == option.needs) known = true;
          }
          if (!known) {
            error = lineError +
                QStringLiteral(
                    "option \"%1\" requires \"%2\", which is not "
                    "declared above it")
                    .arg(option.id, option.needs);
            return false;
          }
          continue;
        }
        error = lineError + QStringLiteral("unknown setting \"%1\"").arg(word);
        return false;
      }

      tmpl.options_.append(option);
      continue;
    }

    if (directive == QLatin1String("if")) {
      const QString id    = takeWord(line, at);
      bool          known = false;
      for (const Option &option : tmpl.options_) {
        if (option.id == id) known = true;
      }
      if (!known) {
        error = lineError +
            QStringLiteral(
                "\"@if %1\": no such option is declared above "
                "it")
                .arg(id);
        return false;
      }
      if (depth >= static_cast<int>(std::size(inElse))) {
        error = lineError + QStringLiteral("\"@if\" nested too deeply");
        return false;
      }
      inElse[depth++] = false;
      tmpl.lines_.append({Line::Type::If, id, number});
      continue;
    }

    if (directive == QLatin1String("else")) {
      if (depth == 0 || inElse[depth - 1]) {
        error = lineError + QStringLiteral("\"@else\" without its \"@if\"");
        return false;
      }
      inElse[depth - 1] = true;
      tmpl.lines_.append({Line::Type::Else, QString(), number});
      continue;
    }

    if (directive == QLatin1String("endif")) {
      if (depth == 0) {
        error = lineError + QStringLiteral("\"@endif\" without its \"@if\"");
        return false;
      }
      --depth;
      tmpl.lines_.append({Line::Type::EndIf, QString(), number});
      continue;
    }

    error = lineError +
        QStringLiteral(
            "unknown directive \"@%1\" (\"@@\" starts a line "
            "with '@')")
            .arg(directive);
    return false;
  }

  if (!sawFormat) {
    error = where + QStringLiteral(": no \"@format\" line -- not a template");
    return false;
  }
  if (depth != 0) {
    error = where + QStringLiteral(": an \"@if\" has no \"@endif\"");
    return false;
  }
  if (tmpl.title_.isEmpty()) {
    error = where + QStringLiteral(": no \"@template\" title");
    return false;
  }
  if (tmpl.kind_.isEmpty()) {
    error = where + QStringLiteral(": no \"@kind\"");
    return false;
  }

  return true;
}

bool Template::load(const QString &path, Template &tmpl, QString &error) {
  QFile file(path);
  if (!file.open(QIODevice::ReadOnly)) {
    error = QStringLiteral("%1: %2").arg(
        QFileInfo(path).fileName(), file.errorString());
    return false;
  }

  return parse(decodeFile(file.readAll()), path, tmpl, error);
}

QString Template::render(const Context &context, QString &error) const {
  QStringList out;

  // One entry per open @if: whether its lines are kept.
  QList<bool> keep;

  for (const Line &line : lines_) {
    switch (line.type) {
    case Line::Type::If:
      keep.append(context.isOn(line.text, options_));
      continue;
    case Line::Type::Else:
      keep.last() = !keep.last();
      continue;
    case Line::Type::EndIf:
      keep.removeLast();
      continue;
    case Line::Type::Text:
      break;
    }

    if (!allKept(keep)) continue;

    QString                         result;
    qsizetype                       from = 0;
    QRegularExpressionMatchIterator it = placeholderRe().globalMatch(line.text);

    while (it.hasNext()) {
      const QRegularExpressionMatch m    = it.next();
      const QString                 name = m.captured(1).toUpper();

      if (!context.values.contains(name)) {
        error = QStringLiteral("%1, line %2: no value for {{%3}}")
                    .arg(QFileInfo(path_).fileName())
                    .arg(line.number)
                    .arg(name);
        return QString();
      }

      result += line.text.mid(from, m.capturedStart(0) - from) +
          context.values.value(name);
      from = m.capturedEnd(0);
    }

    result += line.text.mid(from);
    out.append(result);
  }

  return out.join(QLatin1Char('\n')) + QLatin1Char('\n');
}

QString Template::substitute(const QString &text, const Context &context) {
  QString                         result;
  qsizetype                       from = 0;
  QRegularExpressionMatchIterator it   = placeholderRe().globalMatch(text);

  while (it.hasNext()) {
    const QRegularExpressionMatch m    = it.next();
    const QString                 name = m.captured(1).toUpper();

    result += text.mid(from, m.capturedStart(0) - from);
    result += context.values.contains(name) ? context.values.value(name)
                                            : m.captured(0);
    from = m.capturedEnd(0);
  }

  return result + text.mid(from);
}

QList<Template> loadFolders(const QStringList &folders, QStringList &problems) {
  QList<Template> found;

  for (const QString &folder : folders) {
    if (folder.isEmpty()) continue;

    const QDir dir(folder);
    if (!dir.exists()) continue;

    const QStringList files = dir.entryList(
        {QStringLiteral("*") + QString::fromLatin1(kTemplateExtension)},
        QDir::Files, QDir::Name | QDir::IgnoreCase);

    for (const QString &name : files) {
      Template tmpl;
      QString  error;
      if (!Template::load(dir.filePath(name), tmpl, error)) {
        problems.append(QDir::toNativeSeparators(dir.filePath(name)) +
            QStringLiteral(" -- ") + error);
        continue;
      }

      bool replaced = false;
      for (Template &existing : found) {
        if (existing.title().compare(tmpl.title(), Qt::CaseInsensitive) != 0)
          continue;
        existing = tmpl;
        replaced = true;
        break;
      }
      if (!replaced) found.append(tmpl);
    }
  }

  return found;
}

QList<Template> ofKind(const QList<Template> &templates, const QString &kind) {
  QList<Template> matching;
  for (const Template &tmpl : templates) {
    if (tmpl.kind().compare(kind, Qt::CaseInsensitive) == 0)
      matching.append(tmpl);
  }

  std::sort(matching.begin(), matching.end(),
      [](const Template &a, const Template &b) {
        return a.title().compare(b.title(), Qt::CaseInsensitive) < 0;
      });
  return matching;
}

QByteArray fileBytes(const QString &text) {
  const QString lf = QString(text).remove(QLatin1Char('\r'));

  bool             fits  = false;
  const QByteArray bytes = sch::core::encodeCp1252(lf, &fits);
  return fits ? bytes : lf.toUtf8();
}

bool writeFile(const QString &path, const QString &text, QString &error) {
  QFile file(path);
  if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
    error = file.errorString();
    return false;
  }

  const QByteArray bytes = fileBytes(text);
  if (file.write(bytes) != bytes.size()) {
    error = file.errorString();
    return false;
  }

  file.close();
  return true;
}

QString asctimeText(const QDateTime &when) {
  const QLocale c = QLocale::c();
  const QDate   d = when.date();
  const QTime   t = when.time();

  return QStringLiteral("%1 %2 %3 %4 %5")
      .arg(c.dayName(d.dayOfWeek(), QLocale::ShortFormat),
          c.monthName(d.month(), QLocale::ShortFormat),
          QStringLiteral("%1").arg(d.day(), 2, 10, QLatin1Char(' ')),
          t.toString(QStringLiteral("HH:mm:ss")), QString::number(d.year()));
}

}   // namespace sch::codegen
