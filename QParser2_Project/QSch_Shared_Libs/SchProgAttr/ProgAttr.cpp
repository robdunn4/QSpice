#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable : 4996) // Qt headers reference APIs deprecated for Qt 7
#endif

#include "ProgAttr.h"

#include <QChar>
#include <QCoreApplication>
#include <QLatin1Char>
#include <QRegularExpression>

#ifdef _MSC_VER
#pragma warning(pop)
#endif

#include <limits>

namespace sch::progattr {
namespace {

// Qt 6's sizes and indices are qsizetype; the struct keeps plain ints.
int toInt(qsizetype n) { return static_cast<int>(n); }

bool validName(const QString &name) {
  if (name.isEmpty()) return false;

  for (const QChar c : name) {
    if (c.isSpace()) return false;
    if (c == QLatin1Char('<') || c == QLatin1Char('>')) return false;
  }

  return true;
}

// A whole number as written in a range: optional sign, then digits only.
bool parseWhole(const QString &text, qint64 &value) {
  bool ok = false;
  value   = text.toLongLong(&ok);
  return ok;
}

// The leading whole number of `text` (already trimmed): sign and digits,
// stopping at the first character that is neither. Saturates rather than
// overflowing, so a very long number clamps like any other. False if
// there are no digits at all.
bool leadingWhole(const QString &text, qint64 &value) {
  qsizetype  i        = 0;
  const bool negative = text.startsWith(QLatin1Char('-'));
  if (negative || text.startsWith(QLatin1Char('+'))) ++i;

  constexpr qint64 kLimit  = std::numeric_limits<qint64>::max() / 10 - 1;
  qint64           result  = 0;
  bool             digits  = false;
  bool             toolong = false;

  for (; i < text.size(); ++i) {
    const QChar c = text.at(i);
    if (c < QLatin1Char('0') || c > QLatin1Char('9')) break;

    digits = true;
    if (result > kLimit) {
      toolong = true;
      continue;
    }

    result = result * 10 + (c.unicode() - u'0');
  }

  if (!digits) return false;

  if (toolong) result = std::numeric_limits<qint64>::max();
  value = negative ? -result : result;
  return true;
}

// Whether `text` has a decimal digit at `at`.
bool digitAt(const QString &text, qsizetype at) {
  if (at >= text.size()) return false;
  return text.at(at) >= QLatin1Char('0') && text.at(at) <= QLatin1Char('9');
}

// Whether `text` from `from` on is letters only (units, "Ohm") -- or
// nothing.
bool lettersOnly(const QString &text, int from) {
  for (qsizetype i = from; i < text.size(); ++i)
    if (!text.at(i).isLetter()) return false;
  return true;
}

// A Float's MIN or MAX: a SPICE number and nothing else.
bool parseBound(const QString &text, double &value) {
  int understood = 0;
  if (!spiceNumber(text, value, understood)) return false;
  return understood == toInt(text.size());
}

// The Float form inside a Named value. On a match, `attr` becomes a
// Float, with its value narrowed to what is between the parentheses.
void matchFloat(ProgAttr &attr) {
  static const QRegularExpression floatRe(
      QStringLiteral(R"(^float\[([^:\]]*):([^\]]*)\]\((.*)\)$)"),
      QRegularExpression::DotMatchesEverythingOption);

  const QRegularExpressionMatch match = floatRe.match(attr.value);
  if (!match.hasMatch()) return;

  double low  = 0.0;
  double high = 0.0;
  if (!parseBound(match.captured(1), low)) return;
  if (!parseBound(match.captured(2), high)) return;
  if (low > high) return;

  const int inner = toInt(match.capturedStart(3));

  attr.kind       = Kind::Float;
  attr.minText    = match.captured(1);
  attr.maxText    = match.captured(2);
  attr.minReal    = low;
  attr.maxReal    = high;
  attr.valueEnd   = attr.valueStart + inner + toInt(match.capturedLength(3));
  attr.valueStart = attr.valueStart + inner;
  attr.value      = match.captured(3);
}

// A Choice item, already trimmed: non-empty, and nothing in it that
// would make the list ambiguous.
bool validChoice(const QString &item) {
  if (item.isEmpty()) return false;

  for (const QChar c : item) {
    if (c.isSpace()) return false;
    if (c == QLatin1Char('(') || c == QLatin1Char(')')) return false;
    if (c == QLatin1Char('<') || c == QLatin1Char('>')) return false;
  }

  return true;
}

// The Choice form inside a Named value. On a match, `attr` becomes a
// Choice; its value span stays the whole list.
void matchChoice(ProgAttr &attr) {
  if (attr.kind != Kind::Named) return;
  if (!attr.value.contains(QLatin1Char(','))) return;

  QStringList items;
  int         selected = -1;

  for (const QString &part : attr.value.split(QLatin1Char(','))) {
    QString item = part.trimmed();

    const bool marked = item.startsWith(QLatin1Char('(')) &&
                        item.endsWith(QLatin1Char(')')) && item.size() >= 2;
    if (marked) {
      if (selected >= 0) return; // two selected
      item     = item.mid(1, item.size() - 2);
      selected = toInt(items.size());
    }

    if (!validChoice(item)) return;
    if (items.contains(item, Qt::CaseInsensitive)) return; // duplicate

    items.append(item);
  }

  if (items.size() < 2) return;

  attr.kind     = Kind::Choice;
  attr.choices  = items;
  attr.selected = selected;
  attr.value    = (selected >= 0) ? items.at(selected) : QString();
}

// The Integer form inside a Named value. On a match, `attr` becomes an
// Integer, with its value narrowed to what is between the parentheses.
void matchInteger(ProgAttr &attr) {
  static const QRegularExpression intRe(
      QStringLiteral(R"(^int\[([+-]?\d+):([+-]?\d+)\]\((.*)\)$)"),
      QRegularExpression::DotMatchesEverythingOption);

  const QRegularExpressionMatch match = intRe.match(attr.value);
  if (!match.hasMatch()) return;

  qint64 low  = 0;
  qint64 high = 0;
  if (!parseWhole(match.captured(1), low)) return;
  if (!parseWhole(match.captured(2), high)) return;
  if (low > high) return;

  const int inner = static_cast<int>(match.capturedStart(3));

  attr.kind       = Kind::Integer;
  attr.minText    = QString::number(low);
  attr.maxText    = QString::number(high);
  attr.minValue   = low;
  attr.maxValue   = high;
  attr.valueEnd   = attr.valueStart + inner + toInt(match.capturedLength(3));
  attr.valueStart = attr.valueStart + inner;
  attr.value      = match.captured(3);
}

} // namespace

bool spiceNumber(const QString &text, double &value, int &understood) {
  value      = 0.0;
  understood = 0;

  const qsizetype size = text.size();
  qsizetype       i    = 0;

  if (i < size &&
      (text.at(i) == QLatin1Char('+') || text.at(i) == QLatin1Char('-')))
    ++i;

  // The mantissa: digits, a point, digits -- at least one digit in all.
  bool digits = false;
  while (digitAt(text, i)) {
    ++i;
    digits = true;
  }

  // Whether the number is a plain whole number so far: only then may a
  // multiplier stand in for its decimal point ("4k7").
  bool plain = true;

  if (i < size && text.at(i) == QLatin1Char('.')) {
    plain = false;
    ++i;
    while (digitAt(text, i)) {
      ++i;
      digits = true;
    }
  }

  if (!digits) return false;

  // An exponent only if digits follow it: "1e" is 1 with units "e".
  if (i < size &&
      (text.at(i) == QLatin1Char('e') || text.at(i) == QLatin1Char('E'))) {
    qsizetype after = i + 1;
    if (after < size && (text.at(after) == QLatin1Char('+') ||
                         text.at(after) == QLatin1Char('-')))
      ++after;

    if (digitAt(text, after)) {
      plain = false;
      i     = after;
      while (digitAt(text, i)) ++i;
    }
  }

  const qsizetype numberEnd = i;

  bool ok = false;
  value   = text.left(numberEnd).toDouble(&ok);
  if (!ok) return false;

  // The multiplier, longest first: "meg" and "mil" before "m".
  struct Multiplier {
    const char16_t *suffix;
    double          scale;
  };

  static const Multiplier kMultipliers[] = {
      {u"meg", 1e6},     {u"mil", 25.4e-6}, {u"f", 1e-15},     {u"p", 1e-12},
      {u"n", 1e-9},      {u"u", 1e-6},      {u"\u00B5", 1e-6}, // micro sign
      {u"\u03BC", 1e-6},                                       // Greek mu
      {u"m", 1e-3},      {u"k", 1e3},       {u"g", 1e9},       {u"t", 1e12},
  };

  const QString rest = text.mid(i);
  for (const Multiplier &multiplier : kMultipliers) {
    const QString suffix = QString::fromUtf16(multiplier.suffix);
    if (!rest.startsWith(suffix, Qt::CaseInsensitive)) continue;

    i += suffix.size();

    // The multiplier as the decimal point -- "4k7" is 4.7k, "2u2" 2.2u,
    // "1MEG5" 1.5MEG -- as resistor, capacitor and inductor values are
    // often written. Only after a plain whole number: "4.7k7" is 4.7k
    // with "7" left over.
    if (plain && digitAt(text, i)) {
      const qsizetype fracStart = i;
      while (digitAt(text, i)) ++i;

      value = (text.left(numberEnd) + QLatin1Char('.') +
               text.mid(fracStart, i - fracStart))
                  .toDouble(&ok);
      if (!ok) return false;
    }

    value *= multiplier.scale;
    break;
  }

  understood = toInt(i);
  return true;
}

QString ProgAttr::rangeHelp() const {
  if (kind == Kind::Integer)
    return QCoreApplication::translate("sch::progattr",
                                       "An integer value from %1 to %2")
        .arg(minText, maxText);

  if (kind == Kind::Float)
    return QCoreApplication::translate("sch::progattr",
                                       "A real number from %1 to %2")
        .arg(minText, maxText);

  if (kind == Kind::Choice)
    return QCoreApplication::translate("sch::progattr",
                                       "Choose a value from the list");

  return QString();
}

QString ProgAttr::helpText() const {
  const QString range = rangeHelp();
  if (range.isEmpty()) return description;
  if (description.isEmpty()) return range;
  return description + QLatin1Char('\n') + range;
}

namespace {

// ProgAttr::acceptValue() for a Float.
QString acceptReal(const ProgAttr &attr, const QString &typed, bool *adjusted) {
  const QString trimmed = typed.trimmed();

  QString result;
  double  number     = 0.0;
  int     understood = 0;

  if (!spiceNumber(trimmed, number, understood)) result = attr.minText;
  else if (number < attr.minReal) result = attr.minText;
  else if (number > attr.maxReal) result = attr.maxText;
  else if (lettersOnly(trimmed, understood)) result = trimmed;
  else result = trimmed.left(understood);

  if (adjusted != nullptr) *adjusted = (result != trimmed);
  return result;
}

} // namespace

QString ProgAttr::acceptValue(const QString &typed, bool *adjusted) const {
  if (adjusted != nullptr) *adjusted = false;
  if (kind == Kind::Float) return acceptReal(*this, typed, adjusted);

  if (kind == Kind::Choice) {
    for (const QString &item : choices)
      if (item.compare(typed.trimmed(), Qt::CaseInsensitive) == 0) return item;
    return value;
  }

  if (kind != Kind::Integer) return typed;

  const QString trimmed = typed.trimmed();

  qint64 number = minValue;
  if (leadingWhole(trimmed, number))
    number = qBound(minValue, number, maxValue);

  const QString result = QString::number(number);
  if (adjusted != nullptr) *adjusted = (result != trimmed);
  return result;
}

QString ProgAttr::displayText() const {
  if (!isProgrammable()) return QString();

  if (kind == Kind::Choice && selected < 0)
    return name + QStringLiteral("=<unselected>");

  return name + QLatin1Char('=') + value;
}

int ProgAttr::displayPrefixLen() const {
  if (!isProgrammable()) return 0;
  return toInt(name.size()) + 1;
}

ProgAttr parse(const QString &text) {
  ProgAttr attr;

  const qsizetype equals = text.indexOf(QLatin1Char('='));
  if (equals <= 0) return attr;

  const qsizetype open = equals + 1;
  if (open >= text.size() || text.at(open) != QLatin1Char('<')) return attr;

  const QString name = text.left(equals);
  if (!validName(name)) return attr;

  // The '>' that balances the opening '<'.
  int       depth = 0;
  qsizetype close = -1;

  for (qsizetype i = open; i < text.size(); ++i) {
    if (text.at(i) == QLatin1Char('<')) {
      ++depth;
      continue;
    }

    if (text.at(i) != QLatin1Char('>')) continue;

    --depth;
    if (depth > 0) continue;

    close = i;
    break;
  }

  if (close < 0) return attr;

  attr.kind        = Kind::Named;
  attr.name        = name;
  attr.value       = text.mid(open + 1, close - open - 1);
  attr.description = text.mid(close + 1);
  attr.valueStart  = toInt(open + 1);
  attr.valueEnd    = toInt(close);

  matchInteger(attr);
  matchFloat(attr);
  matchChoice(attr);
  return attr;
}

QString withValue(const QString &text, const QString &value) {
  const ProgAttr attr = parse(text);
  if (!attr.isProgrammable()) return text;

  // A Choice: the list rewritten, trimmed, with the parentheses moved.
  if (attr.kind == Kind::Choice) {
    int chosen = -1;
    for (int i = 0; i < attr.choices.size() && chosen < 0; ++i)
      if (attr.choices.at(i).compare(value.trimmed(), Qt::CaseInsensitive) == 0)
        chosen = i;

    if (chosen < 0) return text;

    QStringList items = attr.choices;
    items[chosen]     = QLatin1Char('(') + items.at(chosen) + QLatin1Char(')');

    return text.left(attr.valueStart) + items.join(QLatin1Char(',')) +
           text.mid(attr.valueEnd);
  }

  return text.left(attr.valueStart) + value + text.mid(attr.valueEnd);
}

} // namespace sch::progattr
