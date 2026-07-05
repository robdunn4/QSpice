//------------------------------------------------------------------------------
// This file is part of the QMdbSim2 project, a Microchip Simulator framework
// for QSpice C-Block components.  See the GitHub repository at
// https://github.com/robdunn4/QSpice/ for the complete project, current
// sources, documentation, and demonstration code.
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
// PinState.h -- Pin state type shared between MdbcsDebugger and MdbSim.
//
// Extracted from MdbSim.h to avoid a circular include dependency:
//   MdbSim.h        includes MdbcsDebugger.h
//   MdbcsDebugger.h needs PinState (parameter of getPinState())
// Both headers include this file instead.
//------------------------------------------------------------------------------
#pragma once

// pin state constants
const bool PIN_ANALOG  = false;
const bool PIN_DIGITAL = true;
const bool PIN_OUTPUT  = false;
const bool PIN_INPUT   = true;

// structure for pin state
struct PinState {
  PinState() : daState(PIN_ANALOG), ioState(PIN_INPUT), voltage(0.0) {}

  bool   daState;
  bool   ioState;
  double voltage;

  bool isDigital() const { return daState == PIN_DIGITAL; }
  bool isAnalog()  const { return !isDigital(); }
  bool isInput()   const { return ioState == PIN_INPUT; }
  bool isOutput()  const { return !isInput(); }
};
