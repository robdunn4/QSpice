#pragma once

#include <QString>


// A TEXT description of a
// sheet's connectivity, from the record-tree model (NetModel). QSchEdit
// shows it as Tools > Dump Connectivity (Debug builds).
//
// Written for the move of connectivity from scene items to the record
// tree, and kept as a diagnostic: dump a sheet, change something, dump
// again, compare the text. So the format describes RESULTS, never how
// they were found, and is fully deterministic:
//   - integer schematic coordinates;
//   - every list sorted, every net numbered in sorted order;
//   - no date, time, path or pointer -- dumps of the same file under
//     two builds compare equal byte for byte.
//
// Sections: a header of counts; NETS (each connected group of points
// with its members -- wires, buses, component pins, net labels, taps --
// and the names its wires carry and its labels give it); JUNCTIONS
// required (three or more non-bus wire endpoints) and present (records
// in the sheet), with any mismatch; UNCONNECTED PINS and LOOSE WIRE ENDS
// (exactly what the on-screen marks show); BUSES; DESIGNATORS, with
// duplicates.
//
// The format is the one the step-0 reference dumps (Connectivity_Tests)
// were written in, so those files still compare.
//
// HISTORY (2026-10-07): there was also a SCENE producer, dump(), and a
// Debug shadow check comparing the two after every edit. Both were
// retired with the scene-side connectivity code once the logs had been
// clean through the step-5 testing.
//
// No moc.
namespace sch::core {

class NetModel;

namespace conndump {

// The dump of the sheet `model` was built from. `fileName` (no path)
// heads it.
QString dumpModel(const sch::core::NetModel &model, const QString &fileName);

// The first line at which two dumps differ, as `line N: "a" | "b"`;
// empty if they are the same.
QString firstDifference(const QString &a, const QString &b);

} // namespace conndump
} // namespace sch::core
