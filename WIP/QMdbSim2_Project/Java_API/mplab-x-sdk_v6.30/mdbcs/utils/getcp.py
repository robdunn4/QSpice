#!/usr/bin/env python
import inspect
import sys
import os
import platform

if platform.system().lower() == 'java':
  from java.net import URL, URLClassLoader
  from java.lang import ClassLoader
  from java.io import File
  from java.lang import System

usage=r"""
Usage:
 python getcp.py pathToMPLABXInstallation

Creates classpath and classpath.bat files in the current directory. 
The classpath and classpath.bat, contain the class path 
for all jars needed while working in mdbcs. They are a
shell script and a batch file respectively.

To use the classpath and classpath.bat files, from the command line in any of the native shells of your OS:

In Windows:
 
  c:>XXXX\mdbcs\utils> python getcp.py "c:\Program Files (x86)\Microchip\MPLABX\vX.YY\mplab_ide
  c:>XXXX\mdbcs\utils> classpath.bat
  c:>XXXX\mdbcs\utils> python
  (or java, groovysh, etc)

In Linux:
 
  $ python getcp.py /opt/microchip/mplabx/vX.YY/mplab_ide
  $ source classpath
  $ python
  (or java, groovysh, etc)

In MacOSX:
 
  $ python getcp.py /Applications/microchip/mplabx/vX.YY
  $ source classpath
  $ python
  (or java, groovy.sh, etc)


"""



gPlatformJars  = [ "mplab_platform/platform/lib/org-openide-util.jar", "mplab_platform/platform/lib/org-openide-modules.jar", "mplab_platform/platform/lib/org-openide-util-lookup.jar", "mplab_platform/platform/core/org-openide-filesystems.jar", "mplab_platform/ide/modules/org-netbeans-api-debugger.jar" ]

class XException(Exception):
  def __init__(self, value):
    self.value = value
  def __str__(self):
    return self.value

def isWindows():
  res = False
  if platform.system().lower() == "java":
    import java.lang.System as System
    if System.getProperty("os.name").lower().startswith("win"):
      res = True
  else:
    if platform.system().lower().startswith("win"):
      res = True
  return res 


def isMac():
  res = False
  if platform.system().lower() == "java":
    import java.lang.System as System
    if System.getProperty("os.name").lower().startswith("mac"):
      res = True
  else:
    if platform.system().lower() == "darwin":
      res = True
  return res 

def isInternalBuild(dd):
  return dd.endswith("netbeans") or dd.endswith("netbeans/")

explainBadDir= """ 
Error: directory %s does not point into an MPLAB X installation

In Windows and Linux, the passed dir should be the MPLAB X Installation directory.
In Mac, the passed dir should be the mplab_ide.app dir under the MPLAB Xinstallation directory
"""
def getMplabIdeDir(dd):
  """ returns the path to the mplab_ide directory """
  # for internal builds
  if isInternalBuild(dd):
    return dd

  l = os.listdir(dd)
  if isMac():
    mainDir = "Contents"
  else:
    mainDir = "mplab_platform"
  if not mainDir in l:
    raise XException(explainBadDir % dd)

  if isMac():
    dd = os.path.abspath(os.path.join(dd,".."))

  if not dd.endswith("/"):
    res = dd + "/"
  return res
    
def _walkOneDir(baseDir):
  cp = []
  for dirName, dirs, files in os.walk(baseDir, False):
    if dirName.endswith("docs"):
      continue
    for f in files:
      if f.endswith(".jar"):
        cp.append(os.path.join(dirName, "*"))
        break
  return cp

def getClassPath(d):
  # Add mdbcs.jar to the classpath based upon the location of
  # this source file.
  classPath = []
  thisFilePath = inspect.getfile(inspect.currentframe())
  if '__pyclasspath__' in thisFilePath:
          # We are running on Jython inside of *.jar file and
          # there is nothing we can do.
          pass
  else:
          parentFolderPath = os.path.abspath(os.path.dirname(thisFilePath))
          grandparentFolderPath = os.path.abspath(os.path.dirname(parentFolderPath))
          classPath.append(os.path.join(grandparentFolderPath, 'mdbcs', 'dist', 'mdbcs.jar'))

  d=getMplabIdeDir(d)
  classPath.extend(_walkOneDir(os.path.join(d,"mplab_platform","mdbcore")))
  classPath.extend(_walkOneDir(os.path.join(d,"mplab_platform","mplablibs")))
  for c in gPlatformJars:
    classPath.append(os.path.join(d,c))
  return classPath    

def getCLASSPATHValue(unix,d):
  res = "set CLASSPATH="
  sep = ";"
  if unix:
    res = "export CLASSPATH="
    sep = ":"
  cp=getClassPath(d)
  index = 0
  for c in cp:
    res += c
    index += 1
    if index < len(cp):
      res += sep
  return res


if platform.system().lower() == 'java':
  import glob
  def expandGlobs(cp):
    expandedCp = []
    for c in cp:
      expandedCp.extend(glob.glob(c))
    return expandedCp
      
  def loadClassPath(cp):
    cp=expandGlobs(cp)
    sys.path.extend(cp)
    m=URLClassLoader.getDeclaredMethod("addURL",[URL])
    m.setAccessible(1)
    for c in cp:
      m.invoke(ClassLoader.getSystemClassLoader(), File(c).toURL())  
  
if __name__== "__main__":
  if len(sys.argv) != 2:
    print(usage)
    exit(1)
  #try:
  d = sys.argv[1]
  dos=getCLASSPATHValue(False, d)
  unix=getCLASSPATHValue(True, d)
  f=open("classpath.bat","wt")
  f.write(dos)
  f.close()
  f=open("classpath","wt")
  f.write(unix)
  f.close()
  #except Exception as e:
  #  print e
