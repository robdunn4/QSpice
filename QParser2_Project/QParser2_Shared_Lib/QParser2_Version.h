//-----------------------------------------------------------------------------
// This file is part of the the QParser2 project.  You can find the complete
// project here:  https://github.com/robdunn4/QSpice/
//-----------------------------------------------------------------------------
/*
 * QParser2_Version.h -- QParser2_Shared_Lib's identity, for About boxes and
 * the log.
 *
 * Defined in the .cpp, so it reports the library actually linked, not the
 * headers a program was compiled against.
 *
 * The PATCH number is bumped with every delivered change to the library.
 *
 * Named QParser2_Version.h, not Version.h, so it cannot be confused with a
 * program's own Version.h on the include path.
 */
#pragma once

namespace qparser2 {

struct LibraryInfo {
  const char *name;    // "QParser2_Shared_Lib"
  const char *version; // "major.minor.patch"
};

LibraryInfo libraryInfo();

} // namespace qparser2
