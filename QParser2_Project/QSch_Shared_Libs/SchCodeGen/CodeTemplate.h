#pragma once

#include <QByteArray>
#include <QDateTime>
#include <QHash>
#include <QList>
#include <QSet>
#include <QString>
#include <QStringList>

// Code templates: the text a "Create ... Template" command writes, kept in
// files a user can read and edit rather than in the program.
//
// One template per file, extension kTemplateExtension, in a folder. A
// template is the output text, line for line, with a few DIRECTIVES --
// lines whose first character is '@'. No C or C++ line starts with one;
// a Verilog line can (an event control, "@(posedge clk)"), and is
// written with "@@":
//
//   @# anything                 a comment: dropped
//   @format 1                   the file format; required, first
//   @template "Title"           its name, shown in menus and dialogs and
//                               written into the output; required
//   @kind cpp                   which command offers it: cpp, verilog,
//                               socket; required
//   @compiler MSVC              the compiler its output is written for,
//                               by the program's name for it ("DMC",
//                               "MSVC"); optional, empty if absent
//   @option id "Label" [default=on|off] [requires=id]
//                               a choice offered before generating; the
//                               label may hold {{placeholders}}; one with
//                               requires= is available only while that
//                               option is on, and counts as off otherwise
//   @if id / @else / @endif     lines kept only when option `id` is on
//                               (or off); may nest
//   @@...                       a line starting with "@...": the first '@'
//                               is dropped
//
// Everywhere else, {{NAME}} is replaced by a value from the Context -- an
// unknown name is an error, never left in the output. A placeholder whose
// value holds several lines yields several lines.
//
// Qt Core only; no dialogs, no moc.
namespace sch::codegen {

// The extension of a template file -- the one place it is written.
inline constexpr const char *kTemplateExtension = ".qschedtmpl";

// The @format this program reads.
inline constexpr int kFormatVersion = 1;

struct Option {
  QString id;      // "struct"
  QString label;   // "Include a struct {{STRUCT}} template:"
  bool    defaultOn = false;
  QString needs;   // the option it requires (requires=), or empty -- "requires"
                   // is a C++20 keyword
};

// What a template is rendered with: placeholder values, and the options
// that are on.
struct Context {
  QHash<QString, QString> values;   // by placeholder name, upper case
  QSet<QString>           on;       // option ids

  // Whether `id` counts as on: set, and -- when it requires another --
  // that one counts as on too.
  bool isOn(const QString &id, const QList<Option> &options) const;
};

class Template {
public:
  // Reads a template from `text`, the contents of the file at `path`
  // (for messages). False, with the reason and line number in `error`,
  // if it is not a valid template.
  static bool parse(
      const QString &text, const QString &path, Template &tmpl, QString &error);

  // Reads one from a file: UTF-8 if valid, else Windows-1252.
  static bool load(const QString &path, Template &tmpl, QString &error);

  const QString       &title() const { return title_; }
  const QString       &kind() const { return kind_; }
  const QString       &compiler() const { return compiler_; }
  const QString       &path() const { return path_; }
  const QList<Option> &options() const { return options_; }

  // The output for `context`, LF line ends. Empty, with the reason in
  // `error`, if a placeholder has no value.
  QString render(const Context &context, QString &error) const;

  // `text` -- an option's label, say -- with its placeholders replaced.
  // Unknown ones are left as they are.
  static QString substitute(const QString &text, const Context &context);

protected:
  // One body line or directive, as parsed.
  struct Line {
    enum class Type { Text, If, Else, EndIf };
    Type    type = Type::Text;
    QString text;   // Text: the line; If: the option id
    int     number = 0;
  };

  QString       title_;
  QString       kind_;
  QString       compiler_;
  QString       path_;
  QList<Option> options_;
  QList<Line>   lines_;
};

// Every template in `folders`, read in order: a template whose title
// matches one already read (ignoring case) replaces it -- so a user's
// folder, read last, overrides the shipped one. Files that cannot be read
// are skipped, the reasons in `problems`.
QList<Template> loadFolders(const QStringList &folders, QStringList &problems);

// Those of `templates` of `kind`, in title order.
QList<Template> ofKind(const QList<Template> &templates, const QString &kind);

// `text` as a generated file is written: LF line ends, Windows-1252 --
// QSpice's own -- or UTF-8 when a character does not fit.
QByteArray fileBytes(const QString &text);

// Writes fileBytes(text) to `path`. False, with the reason in `error`.
bool writeFile(const QString &path, const QString &text, QString &error);

// `when` as C's asctime() writes it, which QSpice's templates use: "Tue
// Oct 20 12:42:40 2026" -- the day of the month padded with a SPACE to
// two places ("Oct  5"), English names whatever the locale.
QString asctimeText(const QDateTime &when);

}   // namespace sch::codegen
