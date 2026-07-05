//-----------------------------------------------------------------------------
// This file is part of the the QSymGen3 command-line tool contained in the
// QParser2 project.  You can find the complete project here:
// https://github.com/robdunn4/QSpice/
//-----------------------------------------------------------------------------
#pragma once
#include "StrList.h"
#include <filesystem>
#include <iostream>

// TODO:  Compare to shared libs FileUtil.*; maybe consolidate?

namespace Fs {
using std::filesystem::absolute;
using std::filesystem::exists;
using std::filesystem::path;

int loadFile(const Fs::path &filePath, StrList &lines);

int loadFile(std::istream &istrm, StrList &lines);

int writeFile(const Fs::path &filePath, const StrList &lines);

int writeFile(std::ostream &ostrm, const StrList &lines);

} // namespace Fs
