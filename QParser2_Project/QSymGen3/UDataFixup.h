//-----------------------------------------------------------------------------
// This file is part of the the QSymGen3 command-line tool contained in the
// QParser2 project.  You can find the complete project here:
// https://github.com/robdunn4/QSpice/
//-----------------------------------------------------------------------------
#pragma once
#include "StrList.h"

// UDataFixup -- post-processes QUX.exe's raw generated uData declaration lines 
// (before any other template processing touches them): converts every INPUT 
// declaration from a value copy to a reference, e.g.
//     double  RA0_I = data[1].d; // input
// 
// becomes
//     double& RA0_I = data[1].d; // input
//
// WHY: QMdbSim2 requires that the input variable be a reference to the uData
// value and not a stack variable copy (as created by QSpice).
namespace UDataFixup {
	// Returns a new StrList with input declarations converted to references and
	// a banner comment inserted before the first one. If uDataLines contains no
	// line tagged exactly "// input", returns it unchanged (no banner emitted).
	StrList apply(const StrList& uDataLines);
} // namespace UDataInputRefFixup