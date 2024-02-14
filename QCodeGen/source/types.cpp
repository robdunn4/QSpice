#include "types.h"

QStringList convert(const StrList &strList) {
  QStringList qList;
  StrList::const_iterator begIter = strList.begin();
  StrList::const_iterator endIter = strList.end();

  while (begIter < endIter) {
    qList.append(begIter->c_str());
    begIter++;
  }

  return qList;
}

// QString convert(const String &str) { return QString(str.c_str()); }
QString convert(const String &str) { return QString::fromStdString(str); }
