//-----------------------------------------------------------------------------
// This file is part of the the QParser2 project.  You can find the complete
// project here:  https://github.com/robdunn4/QSpice/
//-----------------------------------------------------------------------------
#include "SymList.h"
#include <ItemAll.h>
#include <ItemTree.h>
#include <ItemTreeIO.h>

bool SymList::makeSymbol(const PinDefList &pinList, std::ostream &errStrm) {
  ItemTree   tree;
  ItemSymPtr sym  = ItemSym::makePtr(pinList.partNbr);
  auto       root = tree.setRoot(sym);

  if (!root) {
    errStrm << "Failed to create root node.\n";
    return false;
  }

  // some preliminary calcs for graphics
  int       pinRows      = (pinList.totPinCnt + 1) / 2;
  const int pinRowHeight = 200;
  int       rectHeight   = pinRows * pinRowHeight;

  Point topLeft{-500, 0};
  Point botRight{+500, -rectHeight};

  NodePtr nodePtr = root;
  nodePtr->addLast(ItemType::makePtr()); // hierarchical entry
  nodePtr->addLast(ItemDesc::makePtr(pinList.description));
  nodePtr->addLast(ItemShort::makePtr(false));

  // TODO:  finish using ArgXXX construtors
  nodePtr->addLast(ItemText::makePtr(ArgPoint(0, topLeft.y + 150),
                                     ArgFontSize(), ArgRotAlign(),
                                     ArgTextFlags(), 0x1000000, -1, -1, "X1"));
  nodePtr->addLast(ItemText::makePtr(ArgPoint(0, topLeft.y + 50),
                                     ArgFontSize(0.5), 15, 0, 0x1000000, -1, -1,
                                     pinList.partNbr));

  PinDefList::const_iterator iter = pinList.cbegin();
  for (int col = 0; col < 2; col++) {
    int x = col == 0 ? -500 : 500;
    int y = -pinRowHeight / 2;

    int   xLbl = col == 0 ? 20 : -20;
    Point ptLbl(xLbl, 0);

    ArgRotAlign rotAlign = ArgRotAlign::create(
        ArgRotAlign::CENTER_V, col ? ArgRotAlign::EAST : ArgRotAlign::WEST);

    for (int row = 0; row < pinRows && iter != pinList.cend();
         row++, y -= pinRowHeight, iter++) {
      if (iter->type == 'X') continue;
      Point      pt(x, y);
      ItemPinPtr pinPtr =
          ItemPin::makePtr(pt, ptLbl, ArgFontSize(1.0), rotAlign, ArgPinInfo(),
                           ArgColor(), ArgLookupNdx(), ArgString(iter->name));
      nodePtr->addLast(pinPtr);

      // add alternate text in tiny font
      if (!iter->altText.length()) continue;
      Point altTxtPt(x + 25, y - 100);
      if (col) altTxtPt.x -= 50;

      ItemTextPtr txtPtr = ItemText::makePtr(
          altTxtPt, ArgFontSize(0.25), rotAlign, ArgTextFlags::COMMENT_BIT,
          ArgColor(), ArgLookupNdx(), ArgPinNdx(), ArgString(iter->altText));
      nodePtr->addLast(txtPtr);

      // expand rect?
      if (row == pinRows - 1) botRight.y -= 50;
    }
  }

  // we can add the rectangle last because the library sorts items into required
  // order
  nodePtr->addLast(ItemRect::makePtr(topLeft, botRight, 0, 0, 0, 0x1000000,
                                     0xFFFF, -1, 1 /* is hierarchical block */,
                                     -1));

  // TODO: this is a temporary bodge...
  StrList strList = ItemTreeIO::writeStrList(tree);
  for (const std::string &str : strList) push_back(str);

  return true;
}

// makeSchematic() is similar to makeSymbol() but a bit more complicated....  we
// need to create a schematic which contains ports for each pin (whether GPIO or
// not) and include a unique GPIO symbol instance for each actual GPIO pin....
// for now, we'll tie the GPIO pins to nets (as opposed to connecting wires) so
// we also don't need to lay the elements out in an orderly way -- just a column
// of ports, a column of GPIO symbols, and the DLL block.  the DLL block will,
// of course, require multiple pins for GPIO pins tied to the GPIO symbols.  for
// now, we'll hard-code the GPIO symbol to use a net file, e.g., "GPIO.net" or
// similar.
//
// Note:  apparently some things -- pins in particular -- must land on 100 point
// boundaries.  I assume this is to ensure that wires and pins match up exactly
// in the coordinate system for GUI purposes....  I discovered this when trying
// to make the GPIO symbol small with pins spaced 50 units apart; the GUI was
// (AFAICT) moving the pins to be on 100-unit boundaries....
//
bool SymList::makeSchematic(const PinDefList &pinList, std::ostream &errStrm) {
  // root stuff
  ItemTree   tree;
  ItemSchPtr sch  = ItemSch::makePtr();
  auto       root = tree.setRoot(sch);

  if (!root) {
    errStrm << "Failed to create root node.\n";
    return false;
  }
  NodePtr nodePtr = root;

  // let's start with "easy stuff" -- just the ports for now....
  int       x        = 0;
  int       y        = 0;
  int       portCnt  = 0;
  const int yInc     = 200;
  const int yIncGpio = 600;
  const int portX    = 0;
  const int gpioX    = 300;

  for (const PinDef &pinDef : pinList) {
    if (pinDef.type == 'X') {
      continue;
    }
    ItemNetPtr netPtr = ItemNet::makePtr(
        ArgPoint(x, y), ArgFontSize(),
        ArgRotAlign(ArgRotAlign::SOUTH | ArgRotAlign::CENTER_H),
        ArgInt(1 /*port*/), pinDef.name);
    nodePtr->addLast(netPtr);

    y -= pinDef.type == 'B' ? yIncGpio : yInc;
  }

  // add GPIO symbols
  x = 700;
  y = 200;

  for (const PinDef &pinDef : pinList) {
    if (pinDef.type != 'B') {
      if (pinDef.type != 'X') y -= yInc;
      continue;
    }

    // create component and symbol
    ItemCmpPtr  cmpPtr  = ItemCmp::makePtr(Point(x, y), ArgRot(), ArgStuff());
    NodePtr     cmpNode = nodePtr->addLast(cmpPtr);
    ItemSymPtr  symPtr  = ItemSym::makePtr("GPIO_IMPL");
    NodePtr     symNode = cmpNode->addLast(symPtr);
    ItemTypePtr typePtr = ItemType::makePtr(); // hierarchical block
    symNode->addLast(typePtr);
    symNode->addLast(ItemDesc::makePtr("GPIO Symbol for QSymGen2"));
    // symNode->addLast(ItemLib::makePtr()); // QSpice removes this???
    symNode->addLast(ItemShort::makePtr(false));

    symNode->addLast(ItemRect::makePtr(
        Point(-300, 0), Point(200, -200), ArgRot(), ArgLineWidth(),
        ArgLineType(), ArgColor(0x1000000), ArgColor(0xffff), ArgLookupNdx(),
        ArgInt(1 /* hierarchical */), ArgPinNdx()));
    symNode->addLast(ItemText::makePtr(
        Point(-50, -50), ArgFontSize(0.35f), ArgRotAlign(15), ArgTextFlags(),
        ArgColor(), ArgLookupNdx(), ArgPinNdx(), "G_" + pinDef.name));

    //  GPIO implementation schematic
    symNode->addLast(ItemText::makePtr(
        Point(-50, -120), ArgFontSize(0.25f), ArgRotAlign(15), ArgTextFlags(),
        ArgColor(), ArgLookupNdx(), ArgPinNdx(), ArgString("GPIO_IMPL")));

    // add pins
    symNode->addLast(ItemPin::makePtr(Point(-200, -150), Point(0, 20),
                                      ArgFontSize(0.2f), ArgRotAlign(14),
                                      ArgPinInfo(), ArgColor(), ArgLookupNdx(),
                                      ArgString("G"), ArgString(pinDef.name)));

    symNode->addLast(ItemPin::makePtr(
        Point(-100, -150), Point(0, 20), ArgFontSize(0.2f), ArgRotAlign(14),
        ArgPinInfo(), ArgColor(), ArgLookupNdx(), ArgString("I"),
        ArgString(pinDef.name + "_I")));

    symNode->addLast(ItemPin::makePtr(
        Point(0, -150), Point(0, 20), ArgFontSize(0.2f), ArgRotAlign(14),
        ArgPinInfo(), ArgColor(), ArgLookupNdx(), ArgString("O"),
        ArgString(pinDef.name + "_O")));

    symNode->addLast(ItemPin::makePtr(
        Point(100, -150), Point(0, 20), ArgFontSize(0.2f), ArgRotAlign(14),
        ArgPinInfo(), ArgColor(), ArgLookupNdx(), ArgString("C"),
        ArgString(pinDef.name + "_C")));

    y -= yIncGpio;
  }

  // finally, the schematic DLL block
  Point cmpOrg    = Point(1500, 300);
  int   rectWidth = 2000;
  int   rectCntrH = rectWidth / 2;
  Point rectTL    = Point(0, -100);

  ItemCmpPtr  cmpPtr  = ItemCmp::makePtr(cmpOrg, ArgRot(), ArgStuff());
  NodePtr     cmpNode = nodePtr->addLast(cmpPtr);
  ItemSymPtr  symPtr  = ItemSym::makePtr("DLL");
  NodePtr     symNode = cmpNode->addLast(symPtr);
  ItemTypePtr typePtr = ItemType::makePtr(ItemType::DLL_TYPE);
  symNode->addLast(typePtr);
  symNode->addLast(ItemDesc::makePtr("DLL for " + pinList.description));
  // symNode->addLast(ItemLib::makePtr()); // not needed, QSpice strips???
  symNode->addLast(ItemShort::makePtr(false));

  // instance name
  symNode->addLast(ItemText::makePtr(
      Point(rectCntrH, 250), ArgFontSize(), ArgRotAlign(15), ArgTextFlags(),
      ArgColor(), ArgLookupNdx(), ArgPinNdx(), "X1"));

  // DLL name
  symNode->addLast(ItemText::makePtr(
      Point(rectCntrH, 100), ArgFontSize(), ArgRotAlign(15), ArgTextFlags(),
      ArgColor(), ArgLookupNdx(), ArgPinNdx(), pinList.partNbr + "_DLL"));

  // make a new PinList that contains all of the DLL pins
  PinDefList dllPinList;
  for (const PinDef &pinDef : pinList) {
    if (pinDef.type != 'B') {
      dllPinList.push_back(pinDef);
      continue;
    }
    // replace GPIO with three pins (_I, _O, _C); pin name is also pin net to
    // connect to GPIO pin
    PinDef pd = pinDef;
    pd.type   = 'I';
    pd.name   = pinDef.name + "_I";
    dllPinList.push_back(pd);
    pd.type = 'O';
    pd.name = pinDef.name + "_O";
    dllPinList.push_back(pd);
    pd.type = 'O';
    pd.name = pinDef.name + "_C";
    dllPinList.push_back(pd);
  }

  int pinX = 0;
  int pinY = -200;

  ArgPinInfo  pinInfo;
  ArgRotAlign rotAlign = ArgRotAlign(ArgRotAlign::CENTER_V | ArgRotAlign::WEST);
  Point       ptLbl    = Point(20, 0);
  ArgFontSize fntLbl   = ArgFontSize();
  ArgColor    pinClr   = ArgColor();

  for (const PinDef &pinDef : dllPinList) {
    switch (pinDef.type) {
    case 'I':
      pinInfo.setInfo(ArgPinInfo::TYPE_INPORT, ArgPinInfo::DATA_FLOAT);
      symNode->addLast(
          ItemPin::makePtr(Point(pinX, pinY), ptLbl, fntLbl, rotAlign, pinInfo,
                           pinClr, ArgLookupNdx(), pinDef.name, pinDef.name));
      break;
    case 'O':
      pinInfo.setInfo(ArgPinInfo::TYPE_OUTPORT, ArgPinInfo::DATA_FLOAT);
      symNode->addLast(
          ItemPin::makePtr(Point(pinX, pinY), ptLbl, fntLbl, rotAlign, pinInfo,
                           pinClr, ArgLookupNdx(), pinDef.name, pinDef.name));
      break;
    case 'X':
      break;
    default:
      // should not happen
      return false;
    }

    pinY -= 200;
  }

  // rect (move later)
  Point rectBR(rectTL.x + rectWidth, rectTL.y + pinY + 200);

  symNode->addLast(
      ItemRect::makePtr(rectTL, rectBR, ArgRot(), ArgLineWidth(), ArgLineType(),
                        ArgColor(0x1000000), ArgColor(0xffff), ArgLookupNdx(),
                        ArgInt(1 /* hierarchical */), ArgPinNdx()));

  // TODO: this is a temporary bodge...
  StrList strList = ItemTreeIO::writeStrList(tree);
  for (const std::string &str : strList) push_back(str);

  return true;
}
