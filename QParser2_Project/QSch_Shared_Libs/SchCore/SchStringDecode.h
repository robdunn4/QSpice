#pragma once
#include <QString>
#include <QStringDecoder>
#include <string>

namespace sch::core {

// Decodes a raw QSpice text field (as stored by ArgString -- tokenized
// directly from the file, quotes included, e.g. the 3 characters "R1"
// for a component named R1) into a proper QString, stripping a single
// leading/trailing quote pair. Doesn't touch the underlying stored
// std::string value, so round-trip serialization is unaffected.
//
// CONFIRMED by inspecting raw file bytes: QSpice's text fields use a
// MIXED encoding, not one consistent scheme --
//   - Some fields (SPICE directive lines, longer typed/pasted text)
//     start with a UTF-8 byte-order-mark (bytes 0xEF 0xBB 0xBF) followed
//     by UTF-8 content. Decoding these as Latin-1 (our first attempt)
//     turned the BOM into visible garbage: "\xef\xbb\xbf.tran 10m"
//     became "ï»¿.tran 10m".
//   - Other fields (e.g. the micro sign in a "1u" capacitor value) are
//     plain single-byte Windows-1252 with no BOM at all -- a lone
//     0xB5 byte is not valid UTF-8, so decoding THOSE as UTF-8 produces
//     a replacement-glyph instead of the intended character.
//
// QStringDecoder(QStringDecoder::Utf8) both validates (hasError() after
// decoding) and transparently consumes a leading BOM if it is literally
// the first byte of the input -- but here the opening quote character
// comes first, so the BOM instead decodes as a normal, valid embedded
// U+FEFF character (confirmed directly: decoding the real byte sequence
// produces "U+22, U+FEFF, U+2e, ..." -- quote, BOM, '.', ...). Stripped
// explicitly below.
// Windows-1252, decoded by hand.
//
// It differs from Latin-1 ONLY in 0x80-0x9F, where Latin-1 has control
// codes and Windows-1252 has printable characters -- including the euro
// sign, one of QSpice's symbol types. Everything else maps byte-for-code-
// point, identically in both.
//
// A table rather than QStringDecoder("windows-1252"): Qt resolves named
// encodings other than its built-in few through ICU, which a Windows Qt
// build may lack, and a decoder that silently fails to construct would
// quietly fall back to something else. Five undefined bytes (0x81, 0x8D,
// 0x8F, 0x90, 0x9D) keep their Latin-1 meaning so nothing is lost.
inline QString decodeCp1252(const QByteArray &bytes) {
  static constexpr char16_t kHigh[32] = {
      0x20AC, 0x0081, 0x201A, 0x0192, 0x201E, 0x2026, 0x2020, 0x2021,
      0x02C6, 0x2030, 0x0160, 0x2039, 0x0152, 0x008D, 0x017D, 0x008F,
      0x0090, 0x2018, 0x2019, 0x201C, 0x201D, 0x2022, 0x2013, 0x2014,
      0x02DC, 0x2122, 0x0161, 0x203A, 0x0153, 0x009D, 0x017E, 0x0178};

  QString out;
  out.reserve(bytes.size());

  for (const char c : bytes) {
    const unsigned char b = static_cast<unsigned char>(c);
    out.append((b >= 0x80 && b <= 0x9F) ? QChar(kHigh[b - 0x80]) : QChar(b));
  }

  return out;
}

inline QString decodeQSpiceString(const std::string &raw) {
  const QByteArray bytes = QByteArray::fromStdString(raw);

  // UTF-8 first; anything that is not valid UTF-8 is single-byte
  // WINDOWS-1252 -- not Latin-1, which is what this used to fall back to
  // and which has no euro sign.
  QStringDecoder utf8Decoder(QStringDecoder::Utf8);
  QString        s = utf8Decoder(bytes);
  if (utf8Decoder.hasError()) s = decodeCp1252(bytes);

  s.remove(QChar(0xFEFF));

  if (s.size() >= 2 && s.front() == QChar('"') && s.back() == QChar('"'))
    return s.mid(1, s.size() - 2);
  return s;
}

// --- Encoding, the reverse of the above ---

// Windows-1252 bytes for U+0080..U+009F's printable replacements. The
// mirror of kHigh in decodeCp1252(); kept adjacent so the two cannot
// drift apart.
inline bool cp1252ByteFor(char16_t code, unsigned char &out) {
  static constexpr char16_t kHigh[32] = {
      0x20AC, 0x0081, 0x201A, 0x0192, 0x201E, 0x2026, 0x2020, 0x2021,
      0x02C6, 0x2030, 0x0160, 0x2039, 0x0152, 0x008D, 0x017D, 0x008F,
      0x0090, 0x2018, 0x2019, 0x201C, 0x201D, 0x2022, 0x2013, 0x2014,
      0x02DC, 0x2122, 0x0161, 0x203A, 0x0153, 0x009D, 0x017E, 0x0178};

  // Everything outside 0x80-0x9F maps byte-for-code-point.
  if (code <= 0x7F || (code >= 0x00A0 && code <= 0x00FF)) {
    out = static_cast<unsigned char>(code);
    return true;
  }

  for (int i = 0; i < 32; ++i) {
    if (kHigh[i] != code) continue;

    out = static_cast<unsigned char>(0x80 + i);
    return true;
  }

  return false;
}

// Encodes as single-byte Windows-1252, or reports that it cannot.
//
// `ok` is false when any character has no 1252 byte -- Greek, CJK, an
// em space, anything outside the set. The bytes returned in that case
// are not usable.
inline QByteArray encodeCp1252(const QString &text, bool *ok = nullptr) {
  QByteArray out;
  out.reserve(text.size());

  bool all = true;
  for (const QChar ch : text) {
    unsigned char byte = 0;
    if (cp1252ByteFor(ch.unicode(), byte)) {
      out.append(static_cast<char>(byte));
      continue;
    }

    all = false;
    break;
  }

  if (ok != nullptr) *ok = all;
  return all ? out : QByteArray();
}

// Encodes a text field the way QSpice stores one.
//
// QSpice uses BOTH forms, which is why decoding has to try both:
//   - single-byte WINDOWS-1252 where every character fits
//   - UTF-8 WITH A BOM otherwise
//
// This used to write UTF-8 with NO BOM -- neither form. Text that
// happened to be plain ASCII survived, since all three agree there, but
// anything else came back wrong in QSpice: a typed micro sign or euro
// went out as two bytes and read back as two characters.
inline std::string encodeQSpiceString(const QString &text) {
  bool             fits  = false;
  const QByteArray bytes = encodeCp1252(text, &fits);

  // An OVERBAR marker (U+00AC) forces UTF-8 even though Windows-1252 has
  // it (0xAC). QSpice's own overbarred text is saved as UTF-8 with a BOM,
  // and whether it reads a bare 0xAC as an overbar code is unverified --
  // UTF-8 is the form known to work.
  const bool hasOverbar = text.contains(QChar(0x00AC));

  if (fits && !hasOverbar)
    return std::string(bytes.constData(), bytes.size());

  QByteArray utf8;
  utf8.append("\xEF\xBB\xBF"); // BOM, as QSpice writes it
  utf8.append(text.toUtf8());

  return std::string(utf8.constData(), utf8.size());
}

// --- Line breaks in text records ---
//
// A «text» record keeps a line break as the two characters BACKSLASH,
// 'n' -- "\n" in the file -- never as a real line feed, which would
// break the one-record-per-line file format. The GUI works with real
// line feeds; these convert at the record boundary, AFTER
// decodeQSpiceString() and BEFORE encodeQSpiceString().
//
// As QSpice reads it, any "\n" is a break -- so a typed backslash
// followed by 'n' becomes one on reload, in QSpice and here alike.
inline QString lineBreaksFromRecord(const QString &recordText) {
  QString text = recordText;
  text.replace(QStringLiteral("\\n"), QStringLiteral("\n"));
  return text;
}

inline QString lineBreaksToRecord(const QString &text) {
  QString recordText = text;
  recordText.remove(QLatin1Char('\r'));
  recordText.replace(QLatin1Char('\n'), QStringLiteral("\\n"));
  return recordText;
}

} // namespace sch::core
