#pragma once

// SchLink's identity, for About boxes and the log. Defined in the .cpp, so
// it reports the library actually linked, not the headers a program was
// compiled against.
//
// The PATCH number is bumped with every delivered change to SchLink.
namespace sch::link {

struct LibraryInfo {
  const char *name;    // "SchLink"
  const char *version; // "major.minor.patch"
};

LibraryInfo libraryInfo();

} // namespace sch::link
