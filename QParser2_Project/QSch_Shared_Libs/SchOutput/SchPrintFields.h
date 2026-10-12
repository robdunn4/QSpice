#pragma once

#include <QDateTime>
#include <QList>
#include <QString>

#include <utility>

namespace sch::output {

// Header and footer FIELDS: the {name} and {name:format} tokens a
// header/footer slot may contain, expanded for one printed page.
//
// Syntax (see PrintHeaderFooterFields.md for the user's version):
//   {name}          a field
//   {name:format}   a field with a format -- dates and times only, for
//                   now: a preset (short, long, iso, date, time) or a
//                   Qt date/time pattern ("yyyy-MM-dd HH:mm"). Only the
//                   FIRST colon separates, so "HH:mm" works.
//   {{  }}          a literal brace
//
// An unknown field is left exactly as typed, braces and all, so a typo
// shows on the page instead of vanishing. An unclosed "{" is literal.
//
// A slot whose fields ALL come out empty is dropped whole, its own text
// included: "Modified: {modified}" prints nothing for an unsaved
// document, "{pageinfo}" nothing for a single page.
//
// No moc.

// Everything a field can refer to, for one page.
struct SchPrintContext {
  QString   path;           // full native path; empty if never saved
  QString   title;          // the document's display name
  QDateTime modified;       // invalid if there is no file
  QDateTime printed;
  int       page    = 1;
  int       pages   = 1;
  int       row     = 1;    // 1-based
  int       col     = 1;
  bool      tiled   = false;
  QString   printer;
  double    mmPer1000 = 0.0; // the scale IN EFFECT; 0 if unknown
};

namespace SchPrintFields {

// `templ` with every field replaced.
QString expand(const QString &templ, const SchPrintContext &ctx);

// The Insert menu: (menu text, token) pairs, in menu order. An empty
// token is a separator.
QList<std::pair<QString, QString>> menuEntries();

// The defaults -- what printing showed before templates existed.
inline QString defaultHeaderLeft() { return QStringLiteral("{path}"); }
inline QString defaultHeaderCenter() { return QString(); }
inline QString defaultHeaderRight() { return QStringLiteral("Modified: {modified}"); }
inline QString defaultFooterLeft() { return QStringLiteral("{pageinfo}"); }
inline QString defaultFooterCenter() { return QString(); }
inline QString defaultFooterRight() { return QStringLiteral("Printed: {printed}"); }

} // namespace SchPrintFields

} // namespace sch::output
