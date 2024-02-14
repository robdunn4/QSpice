#ifndef TYPES_H
#define TYPES_H

// std::string and QString types & templates to convert

#include <QString>
#include <QStringList>
#include <string>
#include <vector>

typedef std::string String;

// class String : public std::string {
//   friend QString &operator=(QString &lhs, const String &rhs);
//   // QString &operator=(QString &lhs, String &rhs) {
//   //   lhs = QString(rhs.c_str());
//   //   return &lhs;
//   // }
// };

// QString &operator=(QString &lhs, const String &rhs) {
//   return QString(rhs.c_str());
// }

typedef std::vector<String> StrList;

// class StrList : public std::vector<String> {
//   StrList &append(StrList strList) {
//     for (String &str : strList)
//       push_back(str);
//     return *this;
//   }

//   // friend QStringList operator=(QStringList lhs, StrList rhs);
//   // // {
//   // //   lhs.clear();
//   // //   for (String &str : rhs)
//   // //     lhs.append(str);
//   // //   return &lhs
//   // // }
// };

QStringList convert(const StrList &strList);
QString convert(const String &str);

// QString operator=(const String &str) { return QString(str.c_str()); }

// for RAII/exception handling
class FilePtr {
public:
  FilePtr() : ptr(nullptr) {}
  FilePtr(FILE *ptr) : ptr(ptr) {}
  ~FilePtr() {
    if (ptr)
      fclose(ptr);
  }

  FILE *ptr;
};

#endif // TYPES_H
