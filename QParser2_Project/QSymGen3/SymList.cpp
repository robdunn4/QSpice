//-----------------------------------------------------------------------------
// This file is part of the the QSymGen3 command-line tool contained in the
// QParser2 project.  You can find the complete project here:
// https://github.com/robdunn4/QSpice/
//-----------------------------------------------------------------------------
#include "SymList.h"
#include "GraphicSymbols.h"
#include <ItemAll.h>
#include <ItemTreeIO.h>
#include <cctype>
#include <format>

namespace GS = GraphicsSymbols;

// Sanitizes a pin/port name (as declared in the .qpindef) into a valid C++
// identifier for use in generated code.  Only affects identifier contexts --
// the original name must still be passed as-is (quoted) to addPinPortMap()
// so it matches the actual device pin name for JNI lookup.
//
// Invalid characters are encoded as 'x' + 2-digit uppercase hex of their
// ASCII value (e.g., '+' -> "x2B"), so "D+" becomes "Dx2B".  A leading digit
// is prefixed with '_' since that's also invalid in a C++ identifier.
static std::string safeIdentifier(const std::string &name) {
  std::string result;
  result.reserve(name.size());

  if (!name.empty() && std::isdigit(static_cast<unsigned char>(name[0])))
    result += '_';

  for (unsigned char c : name) {
    if (std::isalnum(c) || c == '_')
      result += static_cast<char>(c);
    else
      result += std::format("x{:02X}", c);
  }
  return result;
}

bool SymList::makeSymbol(const PinDefList &pinList, int rectWidth,
                         std::ostream &errStrm) {
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

  Point topLeft{-rectWidth / 2, 0};
  Point botRight{rectWidth / 2, -rectHeight};

  NodePtr nodePtr = root;
  nodePtr->addLast(ItemType::makePtr()); // hierarchical entry
  nodePtr->addLast(ItemDesc::makePtr(pinList.description));
  nodePtr->addLast(ItemShort::makePtr(false));

  // add component reference
  nodePtr->addLast(ItemText::makePtr(ArgPoint(0, topLeft.y + 150),
                                     ArgFontSize(), ArgRotAlign(),
                                     ArgTextFlags(), 0x1000000, -1, -1, "X1"));

  // add DLL reference
  nodePtr->addLast(ItemText::makePtr(ArgPoint(0, topLeft.y + 50),
                                     ArgFontSize(0.5), 15, 0, 0x1000000, -1, -1,
                                     pinList.partNbr));

  // add the multiple-choice device part number
  std::string mcDev;
  for (const std::string &part : pinList.partList) {
    std::string str = "\"" + part + "\"";
    if (!mcDev.length()) {
      // if there is only one choice, we can't include the "()"
      if (pinList.partList.size() > 1) str = "Device=<(" + str + ")";
      else str = "Device=<" + str;
    } else str = "," + str;
    mcDev += str;
  }
  mcDev += "> Select target device from list";
  nodePtr->addLast(ItemText::makePtr(
      ArgPoint(0, topLeft.y + 250), ArgFontSize(0.5f), ArgRotAlign(),
      ArgTextFlags().setHidden(), 0x1000000, -1, -1, mcDev));

  // add the user-editable *.elf/*.hex program attribute
  // let's use "Test" + DLL name + ".elf" as default for generated code
  std::string mcBin = "PgmPath=<\"Test_" + pinList.partNbr +
                      ".elf\"> Path to device code (*.hex/*.elf)";
  nodePtr->addLast(ItemText::makePtr(
      ArgPoint(0, botRight.y - 150), ArgFontSize(0.5f), ArgRotAlign(),
      ArgTextFlags().setHidden(), 0x1000000, -1, -1, mcBin));

  // add the user-editable QMdbCfg.ini path attribute
  std::string mcIni = "CfgPath=<\".\\QMdbCfg.ini\"> Path to QMdbCfg.ini file";
  nodePtr->addLast(ItemText::makePtr(
      ArgPoint(0, botRight.y - 250), ArgFontSize(0.5f), ArgRotAlign(),
      ArgTextFlags().setHidden(), 0x1000000, -1, -1, mcIni));

  // note:  we could add the hierarchical rect now if we knew the final
  // dimensions but we don't know them.  we have two alternatives:
  //
  // (1) create an ItemRectPtr here, save that pointer, and call
  // nodePtr->addFirst(rectPtr) now; later we can fix the dimensions using the
  // stored rectPtr
  //
  // (2) wait to add the ItemRect until we know the final dimensions, and then
  // add using nodePtr->addFirst() to ensure the correct Z-ordering
  //
  // for now, we'll go with (2)...

  PinDefList::const_iterator iter = pinList.cbegin();
  for (int col = 0; col < 2; col++) {
    int y    = topLeft.y - 100;
    int x    = topLeft.x;
    int xLbl = 170;
    int xSym = topLeft.x + 90;

    if (col) {
      x    = -x;
      xLbl = -xLbl;
      xSym = -xSym;
    }

    ArgRotAlign rotAlign = ArgRotAlign::create(
        ArgRotAlign::CENTER_V, col ? ArgRotAlign::EAST : ArgRotAlign::WEST);

    for (int row = 0; row < pinRows && iter != pinList.cend();
         row++, y -= pinRowHeight, iter++) {
      if (iter->type == 'X') continue;

      Point ptPin(x, y);
      Point ptLbl(xLbl, 0);
      Point ptSym(xSym, y);
      Point ptAltText(ptPin.x + ptLbl.x, y - 100);

      ItemPinPtr pinPtr = ItemPin::makePtr(
          ptPin, ptLbl, ArgFontSize(1.0), rotAlign, ArgPinInfo(), ArgColor(),
          ArgLookupNdx(), ArgString(iter->name));
      nodePtr->addLast(pinPtr);

      // add alternate text in tiny font
      if (iter->altText.length()) {
        ItemTextPtr txtPtr = ItemText::makePtr(
            ptAltText, ArgFontSize(0.25), rotAlign, ArgTextFlags::COMMENT_BIT,
            ArgColor(), ArgLookupNdx(), ArgPinNdx(), ArgString(iter->altText));
        nodePtr->addLast(txtPtr);
      }

      // add symbol
      GraphicsSymbols::Type symType;
      switch (iter->type) {
      case 'B':
        symType = GS::Type::CIRCLE;
        break;
      case 'I':
      case 'K':
        symType = col ? GS::Type::TRIANGLE_LEFT : GS::Type::TRIANGLE_RIGHT;
        break;
      case 'O':
        symType = col ? GS::Type::TRIANGLE_RIGHT : GS::Type::TRIANGLE_LEFT;
        break;
      case 'V':
        symType = GS::Type::TRIANGLE_UP;
        break;
      case 'G':
        symType = GS::Type::TRIANGLE_DOWN;
        break;
      }

      GS::addSymbol(nodePtr, symType, ptSym, 100, 0xFF0000, 0xFFFFFF);

      // expand rect
      if (row == pinRows - 1) botRight.y -= 50;
    }
  }

  // we add the rectangle using addFirst() to get the correct Z-order
  nodePtr->addFirst(ItemRect::makePtr(topLeft, botRight, 0, 0, 0, 0x1000000,
                                      0xFFFF, -1, 1 /* is hierarchical block */,
                                      -1));

  // TODO: this is a temporary bodge...
  StrList strList = ItemTreeIO::writeStrList(tree);
  for (const std::string &str : strList) push_back(str);

  return true;
}

bool SymList::makeCppSnippet(const PinDefList &pinList, std::ostream &errStrm) {
  // delimiter strings -- const so they can be changed in one place
  static const char *beginDelim = "/*** Begin QSymGen3 Code Snippet ***/";
  static const char *endDelim   = "/*** End QSymGen3 Code Snippet ***/";

  push_back(beginDelim);

  // emit lambda code to restore canonical case from QSpice-downcased device
  // string
  push_back(
      "// resolves QSpice-downcased device name to canonical case for MDBCS");
  push_back("inst->deviceName = [DevPartName]() -> std::string {");
  push_back("    static constexpr std::string_view deviceNames[] = {");
  for (const std::string &dev : pinList.partList)
    push_back("        \"" + dev + "\",");
  push_back("    };");
  push_back("    for (const auto &name : deviceNames)");
  push_back("        if (_stricmp(DevPartName, name.data()) == 0) return "
            "std::string(name);");
  push_back("    Display(std::format(\"Unknown device \\\"{}\\\"; check P "
            "record in pin defs.\", DevPartName).c_str());");
  push_back("    gAbortFlg = true;");
  push_back("    return \"\";");
  push_back("}();");
  push_back("");
  push_back("// register pin/port/name mappings");

  // emit code to wrap/map pins to port maps
  for (const PinDef &pinDef : pinList) {
    // identifier form for use in declarations/references; the quoted
    // literal passed to addPinPortMap() must remain the name as declared
    // in the .qpindef, since that's what's matched against the actual
    // device pin for JNI lookup
    const std::string ident = safeIdentifier(pinDef.name);

    switch (pinDef.type) {
    case 'I':
      push_back("inst->mdb.addPinPortMap(\"" + pinDef.name + "\", &" + ident +
                ");");
      break;
    case 'B':
      push_back("inst->mdb.addPinPortMap(\"" + pinDef.name + "\", &" + ident +
                "_I, &" + ident + "_O, &" + ident + "_C);");
      break;
    case 'O':
      // TBD:  Are output-only pins used in uCs?  If so, need to implement
      push_back("// TODO: output-only pin \"" + pinDef.name +
                "\" -- review addPinPortMap() signature");
      break;
    case 'V':
      push_back("inst->vddName = \"" + pinDef.name + "\";");
      break;
    case 'G': // no entry in snippet
    case 'K': // no entry in snippet
    case 'X': // no entry in snippet
      break;
    default:
      errStrm << "Unexpected pin type '" << pinDef.type << "' for pin \""
              << pinDef.name << "\" -- skipped.\n";
      break;
    }
  }

  push_back(endDelim);

  return true;
}

// makeSchematic() is similar to makeSymbol() but a bit more complicated....
// we need to create a schematic which contains ports for each pin (whether
// GPIO or not) and include a unique GPIO symbol instance for each actual GPIO
// pin.... for now, we'll tie the GPIO pins to nets (as opposed to connecting
// wires) so we also don't need to lay the elements out in an orderly way --
// just a column of ports, a column of GPIO symbols, and the DLL block.  the
// DLL block will, of course, require multiple pins for GPIO pins tied to the
// GPIO symbols.  for now, we'll hard-code the GPIO symbol to use a net file,
// e.g., "GPIO.net" or similar.
//
// Note:  apparently some things -- pins in particular -- must land on 100
// point boundaries.  I assume this is to ensure that wires and pins match up
// exactly in the coordinate system for GUI purposes....  I discovered this
// when trying to make the GPIO symbol small with pins spaced 50 units apart;
// the GUI was moving the pins to be on 100-unit boundaries....
//
bool SymList::makeSchematic(const PinDefList &pinList, std::ostream &errStrm) {
  // root stuff
  ItemTree   tree;
  ItemSchPtr sch  = ItemSch::makePtr();
  NodePtr    root = tree.setRoot(sch);

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
  symNode->addLast(ItemShort::makePtr(false));

  // instance name
  symNode->addLast(ItemText::makePtr(
      Point(rectCntrH, 250), ArgFontSize(), ArgRotAlign(15), ArgTextFlags(),
      ArgColor(), ArgLookupNdx(), ArgPinNdx(), "X1"));

  // DLL name
  symNode->addLast(ItemText::makePtr(
      Point(rectCntrH, 100), ArgFontSize(), ArgRotAlign(15), ArgTextFlags(),
      ArgColor(), ArgLookupNdx(), ArgPinNdx(), pinList.partNbr));

  // create DevPart attribute
  symNode->addLast(
      ItemText::makePtr(Point(rectCntrH, 350), ArgFontSize(), ArgRotAlign(15),
                        ArgTextFlags().setHidden(), ArgColor(), ArgLookupNdx(),
                        ArgPinNdx(), ArgString("char* DevPartName=Device")));

  // create DevPgmPath attribute
  symNode->addLast(
      ItemText::makePtr(Point(rectCntrH, 350), ArgFontSize(), ArgRotAlign(15),
                        ArgTextFlags().setHidden(), ArgColor(), ArgLookupNdx(),
                        ArgPinNdx(), ArgString("char* DevPgmPath=PgmPath")));

  // create CfgIniPath attribute
  symNode->addLast(
      ItemText::makePtr(Point(rectCntrH, 350), ArgFontSize(), ArgRotAlign(15),
                        ArgTextFlags().setHidden(), ArgColor(), ArgLookupNdx(),
                        ArgPinNdx(), ArgString("char* CfgIniPath=CfgPath")));

  // create ROUT attribute
  symNode->addLast(
      ItemText::makePtr(Point(rectCntrH, 350), ArgFontSize(), ArgRotAlign(15),
                        ArgTextFlags().setHidden(), ArgColor(), ArgLookupNdx(),
                        ArgPinNdx(),
                        ArgString("ROUT=" + std::to_string(pinList.rout))));

  // make a new PinList that contains all of the DLL pins
  PinDefList dllPinList;
  for (const PinDef &pinDef : pinList) {
    if (pinDef.type != 'B') {
      dllPinList.push_back(pinDef);
      continue;
    } // replace GPIO with three pins (_I, _O, _C); pin name is also pin net to
    // connect to GPIO pin
    PinDef pd = pinDef;
    pd.type   = 'I';
    pd.name   = pinDef.name + "_I";
    dllPinList.push_back(pd);
    pd.type = 'O';
    pd.name = pinDef.name + "_O";
    dllPinList.push_back(pd);
    pd.type = 'C';
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
    case 'V':
      pinInfo.setInfo(ArgPinInfo::TYPE_INPORT, ArgPinInfo::DATA_FLOAT);
      symNode->addLast(ItemPin::makePtr(Point(pinX, pinY), ptLbl, fntLbl,
                                        rotAlign, pinInfo, pinClr,
                                        ArgLookupNdx(),
                                        safeIdentifier(pinDef.name),
                                        pinDef.name));
      break;
    case 'C': // GPIO control pin (synthesized from 'B'); DLL drives direction
      pinInfo.setInfo(ArgPinInfo::TYPE_OUTPORT, ArgPinInfo::DATA_BOOL);
      symNode->addLast(ItemPin::makePtr(Point(pinX, pinY), ptLbl, fntLbl,
                                        rotAlign, pinInfo, pinClr,
                                        ArgLookupNdx(),
                                        safeIdentifier(pinDef.name),
                                        pinDef.name));
      break;
    case 'K': // SimClock pin; DLL receives clock signal
      pinInfo.setInfo(ArgPinInfo::TYPE_INPORT, ArgPinInfo::DATA_BOOL);
      symNode->addLast(ItemPin::makePtr(Point(pinX, pinY), ptLbl, fntLbl,
                                        rotAlign, pinInfo, pinClr,
                                        ArgLookupNdx(),
                                        safeIdentifier(pinDef.name),
                                        pinDef.name));
      break;
    case 'G':
      // TODO: GND pin schematic handling to be determined
      // pinInfo.setInfo(ArgPinInfo::TYPE_INPORT, ArgPinInfo::DATA_FLOAT);
      pinInfo.setInfo(ArgPinInfo::TYPE_DLLGND, ArgPinInfo::DATA_FLOAT);
      symNode->addLast(ItemPin::makePtr(Point(pinX, pinY), ptLbl, fntLbl,
                                        rotAlign, pinInfo, pinClr,
                                        ArgLookupNdx(),
                                        safeIdentifier(pinDef.name),
                                        pinDef.name));
      break;
    case 'O':
      pinInfo.setInfo(ArgPinInfo::TYPE_OUTPORT, ArgPinInfo::DATA_FLOAT);
      symNode->addLast(ItemPin::makePtr(Point(pinX, pinY), ptLbl, fntLbl,
                                        rotAlign, pinInfo, pinClr,
                                        ArgLookupNdx(),
                                        safeIdentifier(pinDef.name),
                                        pinDef.name));
      break;
    case 'X':
      // no extra space
      continue;
    default:
      // should not happen
      return false;
    }

    pinY -= 200;
  }

  // add the hierarchical block rect
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
