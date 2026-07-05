#!/usr/bin/env python
import sys
import os
from getcp import getMplabIdeDir, isMac 
from getcp import isWindows
from getcp import isInternalBuild
usage="""
Usage:
   ./config.py path-to-installation-of-MPLAB-X

This utility must run from the mdbcs/utils directory. It will modify files in ../mdbcs so that Netbeans knows which version of MPLAB X to build mdbcs against.
You can run this utility anytime you want to 'point' mdbcs to a different version of MPLAB X.

 Example:
  
  ./config.py """ + r'"C:\Program Files (x86)\Microchip\MPLABX\v4.YY"' + """
  
  or
  
  ./config.py /opt/microchip/mplabx/v4.YY/mplab_ide
"""
LOCATION_PROP_FILE="../mdbcs/src/com/microchip/mdbcs/location.properties"
lockFileName="../mdbcs/configure_first"

class Template:
  def __init__(self, templateFileName, outputFileName, macroNames, useWindowsBackslash):
    self.templateFileName = templateFileName
    self.outputFileName = outputFileName
    self.macroNames= macroNames
    self.useWindowsBackslash = useWindowsBackslash


  def getTemplateFileName(self):
    return self.templateFileName

  def getOutputFileName(self):
    return self.outputFileName

  def getMacroNames(self):
    return self.macroNames

  def getUseWindowsBackslash(self):
    return self.useWindowsBackslash

templates = [   Template("../templates/build.xml.template",
                         "../mdbcs/build.xml",
                         ["__MPLAB_PACKS_FOLDER__"],
                         False),
                Template("../templates/location.properties.template",
                         LOCATION_PROP_FILE,
                         ["__MPLAB_PACKS_FOLDER__", "__MPLABX_THIRDPARTY_LIB_PATH__"],
                         False),
                Template("../templates/project.xml.template",
                         "../mdbcs/nbproject/project.xml",
                         ["__MPLAB_INSTALLATION_LIBRARIES__"],
                         True),
                Template("../templates/build-impl.xml.template",
                         "../mdbcs/nbproject/build-impl.xml",
                         ["__MPLAB_INSTALLATION_LIBRARIES__"],
                         True)
            ]


def sed(templateFileName, macros, destinationFile):
  fin  = open(templateFileName, "rt")
  fout = open(destinationFile, "wt") 
  for line in fin:
    for macro in macros.keys():
      if macro in line:
        line = line.replace(macro, macros[macro])
    fout.write(line)
  fin.close()
  fout.close()

def fixSlashes(d, useWindows):
  if isWindows() and useWindows:
    d = d.replace("/","\\")
  else:
    d = d.replace("\\","/")
  return d
  
def processTemplates(macros):
  for t in templates:
    macroNames = t.getMacroNames()
    for macroName in macroNames:
      macros[macroName] = fixSlashes(macros[macroName], t.getUseWindowsBackslash())
    sed(t.getTemplateFileName(), macros, t.getOutputFileName())
   

def getPacksFolder(d):
  if isInternalBuild(d):  
    return os.path.join("..","..","packs")
  else:
    return os.path.join(d,"packs")

def getLibrariesFile(d):
  if isInternalBuild(d):  
    return os.path.join(d,"lib","nblibraries.properties")
  else:
    return os.path.join(d,"mplab_platform", "lib","nblibraries.properties")

def getThirdpartyLibPath(d):
  if isInternalBuild(d):
    return os.path.join(d, "thirdparty", "")
  else:
    return os.path.join(d, "mplab_platform", "thirdparty", "")

def removeLockFile():
  if os.path.exists(lockFileName):
    os.remove(lockFileName)

def removeLocationFile():
  print("Removing " + LOCATION_PROP_FILE)
  if os.path.exists(LOCATION_PROP_FILE):
    os.remove(LOCATION_PROP_FILE)

def get_lib_extension_dir():
  home_dir = os.environ['HOME']
  return os.path.join(home_dir,"Library", "Java", "Extensions")

def calculate_and_create_user_lib_extensions_dir():
  user_lib_dir = get_lib_extension_dir()
  if os.path.exists(user_lib_dir) and os.path.isdir(user_lib_dir):
    print("{} exists".format(user_lib_dir))
  else:
    print("Creating {}".format(user_lib_dir))
    os.makedirs(user_lib_dir)
  return user_lib_dir

def create_dylib_links(ide_dir):
  user_lib_dir = calculate_and_create_user_lib_extensions_dir()
  bin_dir = os.path.join(ide_dir, "mplab_platform", "bin")
  links = [l for l in os.listdir(bin_dir) if l.endswith("dylib")]
  for link in links:
    real_dylib_location = os.path.realpath(os.path.join(bin_dir, link))
    local_link = os.path.join(user_lib_dir,link)
    if not os.path.exists(local_link):
      print("Linking {} to {}".format(real_dylib_location, user_lib_dir))
      os.symlink(real_dylib_location, local_link)

if __name__== "__main__":
  if len(sys.argv) == 2 and (sys.argv[1].lower() == "-h" or sys.argv[1].lower() == "--help"):
    print(usage)
    exit(0)
  if len(sys.argv) != 2:
    print(usage)
    exit(1)
  #try:
  d = sys.argv[1]
  mplab_dir = getMplabIdeDir(d)
  packsFolder = getPacksFolder(mplab_dir)
  librariesFile = getLibrariesFile(mplab_dir)
  thirdpartyFolder = getThirdpartyLibPath(mplab_dir)

  print("Configuring ../mdbcs to build against %s" % mplab_dir)
  processTemplates({"__MPLAB_INSTALLATION_LIBRARIES__": librariesFile,
                    "__MPLAB_PACKS_FOLDER__": packsFolder,
                    "__MPLABX_THIRDPARTY_LIB_PATH__": thirdpartyFolder})
  removeLockFile()
  if isInternalBuild(mplab_dir):
    removeLocationFile()

  if isMac() and not isInternalBuild(mplab_dir):
    create_dylib_links(mplab_dir)

  print("")
  print("You can now build ../mdbcs by either:")
  print("")
  print("Opening the mdbcs project in Netbeans and building from there")
  print("")
  print("or, if you have ant in your path by doing:")
  print("")
  print("$ cd ../mdbcs")
  print("$ ant jar")
  print("")
    
  #except Exception as e:
  #  print e
