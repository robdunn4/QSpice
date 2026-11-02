#pragma once

// SchCodeGen's identity, for About boxes and the log. Defined in the .cpp,
// so it reports the library actually linked, not the headers a program
// was compiled against.
//
// The PATCH number is bumped with every delivered change to SchCodeGen.
namespace sch::codegen {

struct LibraryInfo {
  const char *name;      // "SchCodeGen"
  const char *version;   // "major.minor.patch"
};

LibraryInfo libraryInfo();

}   // namespace sch::codegen
