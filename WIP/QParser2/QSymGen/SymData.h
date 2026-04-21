//-----------------------------------------------------------------------------
// This file is part of the the QParser2 project.  You can find the complete
// project here:  https://github.com/robdunn4/QSpice/
//-----------------------------------------------------------------------------
#pragma once
#include "PinItems.h"
#include <ItemPin.h>
#include <QArgUtils.h>
#include <cassert>

/*
 * Plan:  We need the class to contain a vector of ItemPinPtr elements.  This
 * vector will be populated by data from a PinItemList.  Each PinItem will be
 * converted to one or more ItemPin objects.  For normal input/output pins
 * there will be one symbol pin for each PinItem.
 *
 * GPIO pins will be added in the future.  For GPIO pins, three DLL block pins
 * will be generated for each PinItem (Input, Output, and Control).  For now,
 * we'll simply put them aside.
 *
 * The class will eventually split the PinItems list into two columns.  It will
 * calculate appropriate number of rows for the two columns of pins, calculate
 * appropriate coordinates for the pins, generate the symbol coordinates and
 * arguments, and generate code for the symbol file.  It will also generate code
 * for a symbol rectangle, ItemSym, ItemType, etc.  Finally, it will generate
 * the *.qsym file.
 *
 * When GPIO pins are added, we may also need to generate a *.qsch file and
 * change the symbol block type to a hierarchical block with a netlist that
 * incorporates a GPIO symbol.  Again, we'll put this off for now.
 */

class SymPin {
public:
  SymPin(const PinItemPtr pinItem);

  // protected:
  ItemPinPtr itemPin;
};

using SymPinList = std::vector<SymPin>;

class SymData : public SymPinList {
public:
  SymData(const PinItemList &pinList);

  int calcRows() const;

  void fixCoords();

  int   width      = 1000; // default for now
  int   rowSpacing = 200;
  Point topLeft    = {0, 0}; // calculated in fixCoords()
  Point botRight   = {0, 0}; // calculated in fixCoords()
};
