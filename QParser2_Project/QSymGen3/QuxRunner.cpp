//-----------------------------------------------------------------------------
// This file is part of the the QSymGen3 command-line tool contained in the
// QParser2 project.  You can find the complete project here:
// https://github.com/robdunn4/QSpice/
//-----------------------------------------------------------------------------
#include "QuxRunner.h"
#include <cstdio>
#include <format>

StrList QuxRunner::run(const Fs::path &quxExePath, const Fs::path &qschPath,
                       std::ostream &errStrm) {
  StrList lines;

  if (!Fs::exists(quxExePath)) {
    errStrm << std::format("QuxRunner: QUX.exe not found at \"{}\".\n",
                            quxExePath.string());
    return lines;
  }

  // cmd.exe requires the entire command string to be wrapped in an extra outer
  // pair of quotes when it contains multiple quoted arguments (e.g. paths with
  // spaces).  The resulting format is: "\"<exe>\" <args> \"<path>\""
  std::string cmd = std::format("\"\"{}\" -DLLvariables \"{}\" -stdout\"",
                                quxExePath.string(), qschPath.string());

  FILE *pipe = _popen(cmd.c_str(), "r");
  if (!pipe) {
    errStrm << "QuxRunner: failed to launch: " << cmd << "\n";
    return lines;
  }

  char buf[4096];
  while (fgets(buf, sizeof(buf), pipe)) {
    std::string line(buf);

    // strip trailing newline(s)
    while (!line.empty() && (line.back() == '\n' || line.back() == '\r'))
      line.pop_back();

    if (line.substr(0, 2) == "//") continue;
    lines.push_back(std::move(line));
  }

  int exitCode = _pclose(pipe);
  if (exitCode != 0) {
    errStrm << std::format(
        "QuxRunner: QUX.exe exited with code {}; output may be incomplete.\n",
        exitCode);
    lines.clear();
  }

  return lines;
}
