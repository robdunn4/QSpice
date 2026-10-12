#pragma once

// SchRender's identity, for About boxes and the log. Defined in the .cpp, so
// it reports the library actually linked, not the headers a program was
// compiled against.
//
// The PATCH number is bumped with every delivered change to SchRender.
namespace sch::render {

struct LibraryInfo {
  const char *name;    // "SchRender"
  const char *version; // "major.minor.patch"
};

LibraryInfo libraryInfo();

} // namespace sch::render
