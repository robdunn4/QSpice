//-----------------------------------------------------------------------------
// This file is part of the the QParser2 project.  You can find the complete
// project here:  https://github.com/robdunn4/QSpice/
//-----------------------------------------------------------------------------
#include "QItemNet.h"
#include "StrUtils.h"

QItemNet::QItemNet(std::string typeStr, std::string argStr)
    : QItemBase(typeStr, argStr) {}

QItemNet::QItemNet(const QItemNet &other)
    : QItemBase(other), pt1(other.pt1), fontSize(other.fontSize), rotateAlign(other.rotateAlign), netType(other.netType),
      netName(other.netName) {}

QItemBasePtr QItemNet::clone() const {
  return std::make_shared<QItemNet>(*this);
}

void QItemNet::parseItem() {
  StrUtils::StrList strList = StrUtils::tokenize(argStr);

  if (strList.size() != 5) {
    std::string str = "Unexpected content in " + typeStr + " " + argStr;
    throw std::invalid_argument(str);
  }

  pt1 = ArgPoint(strList[0]);
  fontSize = ArgFontSize(strList[1]);
  rotateAlign = ArgRotAlign(strList[2]);
  netType = ArgInt(strList[3]);
  netName = strList[4];
  netDesc = std::string();

  // reparse for net description if present
  size_t pos = strList[4].find_first_of(" ");
  if (pos != std::string::npos) {
    netName = strList[4].substr(0, pos);
    netDesc = strList[4].substr(pos + 1);
  }
}

std::string QItemNet::toString() const {
  std::string str = typeStr + " " + pt1.toString() + " " + fontSize.toString() + " " +
                    rotateAlign.toString() + " " + netType.toString() + " " + netName.toString();
  if (netDesc.getValue().length()) str += " " + netDesc.toString();
  return str;
}
