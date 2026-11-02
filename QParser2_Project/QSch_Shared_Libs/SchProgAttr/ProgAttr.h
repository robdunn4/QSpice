#pragma once

#include <QString>
#include <QStringList>
#include <QtGlobal>

// Programmable Attributes: a symbol's text attribute written in one of
// QSpice's special forms, which changes how Symbol Properties and the
// schematic show it and what values it may take. QSpice's Help lists six
// forms:
//
//   1. Named Attribute   NAME=<VALUE>DESCRIPTION
//   2. Named Integer     NAME=<int[MIN:MAX](VALUE)>DESCRIPTION
//   3. Named Float       NAME=<float[MIN:MAX](VALUE)>DESCRIPTION
//   4. Multiple Choice   NAME=<V1,(V2),V3>DESCRIPTION
//   5. LOOKUP            LOOKUP<IDX1,IDX2;SYM1,SYM2...>
//   6. Essay             anything else -- ordinary text
//
// 1, 2, 3, 4 and 6 are recognized so far. A malformed 2, 3 or 4 parses
// as a Named Attribute whose value is everything between the angle
// brackets, so the text shows as it is and says something is wrong.
//
// The text is the record's DECODED text (sch::core::decodeQSpiceString()),
// with or without QSpice's "\n" turned into line feeds -- either works.
//
// THE RECORD ALWAYS HOLDS THE WHOLE FORM. Placing a symbol copies it as
// written, and changing the value in QSpice rewrites only what is between
// the angle brackets, leaving the name and description alone (confirmed
// with a placed resistor: "Value=<val>Resistor Value" became
// "Value=<2K>Resistor Value").
//
// HOW IT IS SHOWN depends on View > Raw Attributes, and on where the text
// is:
//   - a placed component's attribute, Raw Attributes off: "NAME=VALUE" on
//     the schematic; in Symbol Properties the row is named NAME, its value
//     cell holds VALUE, and the description pane shows NAME over
//     DESCRIPTION
//   - Raw Attributes on, or the symbol itself (QSpice's symbol editor has
//     no such switch and always shows the text as written): the whole
//     text, as stored
namespace sch::progattr {

enum class Kind {
  Essay,   // not a Programmable Attribute: ordinary text
  Named,   // NAME=<VALUE>DESCRIPTION
  Integer, // NAME=<int[MIN:MAX](VALUE)>DESCRIPTION
  Float,   // NAME=<float[MIN:MAX](VALUE)>DESCRIPTION
  Choice,  // NAME=<V1,(V2),V3>DESCRIPTION
};

// One attribute's text, taken apart.
struct ProgAttr {
  Kind    kind = Kind::Essay;
  QString name;        // "Value"
  QString value;       // "2K", Integer's "200", Float's "4.7k" -- may be
                       // empty
  QString description; // "Resistor Value" -- may be empty

  // The range of an Integer or a Float, inclusive -- MIN <= MAX always: a
  // form whose MIN is above its MAX is not one (see parse()). The text is
  // as written in the form ("1m"), and is what a clamped Float value
  // becomes; the numbers are what it is compared with.
  QString minText;
  QString maxText;
  qint64  minValue = 0;   // Integer
  qint64  maxValue = 0;   // Integer
  double  minReal  = 0.0; // Float
  double  maxReal  = 0.0; // Float

  // A Choice's items, trimmed, in order, and which is selected -- the one
  // in parentheses; -1 if none is. `value` is the selected item, empty if
  // none.
  QStringList choices;
  int         selected = -1;

  // Where the value sits in the text it was parsed from: the index of its
  // first character, and of the character just after it -- the '>' that
  // closes a Named value or a Choice's list (a Choice's span is the whole
  // list, which a new selection rewrites), the ')' that closes an Integer's
  // or a Float's. -1 for Essay.
  int valueStart = -1;
  int valueEnd   = -1;

  bool isProgrammable() const noexcept { return kind != Kind::Essay; }

  // What the schematic shows with Raw Attributes off: "NAME=VALUE" --
  // for a Choice with nothing selected, "NAME=<unselected>", as QSpice
  // shows it. Essay has none -- the caller shows its text as it is.
  QString displayText() const;

  // How many leading characters of displayText() are the name and its
  // '=' -- the part an editor of the display text must not change. 0 for
  // Essay.
  int displayPrefixLen() const;

  // What the description pane adds below the description: Integer's
  // "An integer value from MIN to MAX", Float's "A real number from MIN to
  // MAX" (MIN and MAX as written), Choice's "Choose a value from the
  // list". Empty for the other kinds.
  QString rangeHelp() const;

  // The description pane's text: the description, then rangeHelp() on a
  // line of its own, either of which may be absent.
  QString helpText() const;

  // What a typed value becomes when stored, as QSpice does it. Named
  // takes the text as typed. Integer:
  //   - spaces round it are trimmed
  //   - the leading whole number is taken, sign and all, and anything
  //     after it ignored: "5k" is 5 (no metric multipliers -- those are
  //     Float's), "12.7" is 12
  //   - no number at all ("abc", nothing) is MIN
  //   - then it is clamped to MIN..MAX
  // Float (see spiceNumber() for how a number is read):
  //   - spaces round it are trimmed
  //   - a number in range is kept AS TYPED, multiplier, units and all:
  //     "4.7k", "4k7", "4.7kOhm", "1e3", "2.2MEG"
  //   - a number followed by anything but letters keeps only what was
  //     understood: "4.7k7" is "4.7k", "2.5.3" is "2.5" -- cut so the
  //     stored text says what is actually read
  //   - no number at all ("abc", nothing) is MIN, as written
  //   - below MIN or above MAX is MIN or MAX, as written
  // Choice: one of the items, matched without regard to case and returned
  // as the item is written; anything else leaves the selection as it is
  // (the current value, empty if none). Picked from a closed list, so
  // never `adjusted`.
  // `adjusted`, if given, is set when the result is not what was typed
  // (spaces aside) -- for the beep that tells the user so. Never for
  // Named. Essay returns `typed` unchanged.
  QString acceptValue(const QString &typed, bool *adjusted = nullptr) const;
};

// Takes `text` apart. Anything not EXACTLY one of the recognized forms is
// Essay, as in QSpice, which simply does not match a near miss:
//
//   - NAME: everything before the first '=', at least one character, with
//     no white space and no '<' or '>'
//   - then '<' at once -- "Value = <val>" is Essay
//   - VALUE: up to the '>' that closes that '<'. A '<' inside the value
//     opens a nested pair that needs its own '>', so a value of "<1k>"
//     round-trips. With no closing '>' the text is Essay
//   - DESCRIPTION: whatever follows, possibly nothing
//
// A Named value that is EXACTLY int[MIN:MAX](VALUE) -- MIN and MAX whole
// numbers, optionally signed, MIN <= MAX, no spaces -- is an Integer, its
// value what is between the parentheses. Anything else, "int[2000:2](5)"
// or "int[2:2000]" included, stays Named.
//
// Likewise float[MIN:MAX](VALUE) is a Float, MIN and MAX each a whole
// SPICE number with nothing after it ("1m", "10k", "2.2MEG", "1e-3"),
// MIN <= MAX.
//
// A Named value that is a list is a Choice: two or more items separated
// by commas, spaces round each ignored, at most one in parentheses -- the
// selected one. Each item is non-empty, with no space, comma, parenthesis
// or angle bracket in it, and no two are the same, case aside. Anything
// else stays Named: one item ("<a>", "<(a)>"), two selected, "a,,b",
// "a b,c", "a,(b),A".
ProgAttr parse(const QString &text);

// The SPICE number at the start of `text`, as a value: an optionally
// signed decimal ("4.7", ".5", "-3"), an optional exponent ("e-3"), then
// an optional metric multiplier, not case sensitive:
//
//   f 1e-15   p 1e-12   n 1e-9   u or µ 1e-6   mil 25.4e-6   m 1e-3
//   k 1e3     meg 1e6   g 1e9    t 1e12
//
// "m" is milli and "meg" mega, whatever the case -- "1M" is 1e-3.
//
// After a plain whole number, a multiplier may also stand for the decimal
// point, as component values are often written: "4k7" is 4.7k, "2u2"
// 2.2u, "1MEG5" 1.5MEG. Not after a number that already has a point or
// an exponent: "4.7k7" is 4.7k, with "7" left over.
// `understood` is how many characters that took; what follows (units,
// say) is not part of the number. False, with `understood` 0, if `text`
// does not start with a number.
bool spiceNumber(const QString &text, double &value, int &understood);

// `text` with its value replaced by `value`: only what is between the
// angle brackets changes -- for an Integer or a Float, only what is
// between the parentheses -- as QSpice does it. For a Choice, `value`
// must be one of its items (case aside): the list is rewritten, trimmed,
// with the parentheses on that item ("a,(b),c"); anything else leaves
// the text unchanged. The value is taken as given,
// angle brackets included -- QSpice accepts them, and so does this; an
// editor wanting QSpice's rules for the kind passes it through
// ProgAttr::acceptValue() first. Essay text is returned unchanged.
QString withValue(const QString &text, const QString &value);

} // namespace sch::progattr
