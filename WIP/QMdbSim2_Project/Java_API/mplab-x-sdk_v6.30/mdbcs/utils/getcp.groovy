#!/usr/bin/env groovy
import groovy.io.FileType
def usage="""
Usage:
  groovy getcp.groovy pathToMPLABXInstallation

Creates classpath and classpath.bat files in the current directory. 
The classpath and classpath.bat, contain the class path 
for all jars needed while working in mdbcs. They are a
shell script and a batch file respectively.

To use the classpath and classpath.bat files, from the command line in any of the native shells of your OS:

In Windows:
 
  c:>XXXX\\mdbcs\\utils> groovy getcp.groovy "c:\\Program Files (x86)\\Microchip\\MPLABX\\vX.YY\\mplab_ide
  c:>XXXX\\mdbcs\\utils> classpath.bat
  c:>XXXX\\mdbcs\\utils> groovy
  (or java, jython, etc)

In Linux:
 
  \$ groovy getcp.groovy /opt/microchip/mplabx/vX.YY/mplab_ide
  \$ source classpath
  \$ groovy
  (or java, jython, etc)

In MacOSX:
 
  \$ groovy getcp.groovy /Applications/microchip/mplabx/vX.YY
  \$ source classpath
  \$ groovy
  (or java, jython, etc)


"""

// *************** Functions *************************

def isMac() {
	return System.getProperty("os.name").toLowerCase().contains("mac")
}

def checkForInstallation(dir) {
	def isInstallation = false
	lookFor = "mplab_platform"
	if (isMac()) {
		lookFor = "mplab_ide.app"
	}
	dir.eachFile {f->
		if (f.getName() == lookFor) {
			isInstallation = true
		}
	}
	return isInstallation
}

def getlistOfDirectoriesWithJars(dir) {
	def d = new File(dir)
	def l = []
	d.eachDirRecurse {dd-> 
      def containsJars = false
      dd.eachFile {f->
          if (f.getName().endsWith(".jar")) {
             containsJars = true;
          }
      }
      if (containsJars && !dd.getName().endsWith("docs")) {
          l.add(dd)
      }      
	}
	return l
}


// ******** Hardcoded data ******************
// List of NetBeans platform jars we need to use MDBCore
def platformJars  = [ "mplab_platform/platform/lib/org-openide-util.jar", "mplab_platform/platform/lib/org-openide-modules.jar", "mplab_platform/platform/lib/org-openide-util-lookup.jar", "mplab_platform/platform/core/org-openide-filesystems.jar", "ide/modules/org-netbeans-api-debugger.jar" ]

// ******** Main code ***********************

// Check we have one arg
if (args.length != 1) {
    print usage
    System.exit(2)
}
def installation = new File(args[0])
def baseDir=""
if (checkForInstallation(installation)) {
	println "Processing MPLAB X installation"
	if (isMac()) {
		baseDir = "/mplab_ide.app/Contents/Resources/mplab_ide/"
	} else {
		baseDir = "/"
	}
} else {
  println "Error: directory" + args[0] + " does not point into an MPLAB X installation"
  println ""
  println "In Windows and Linux, the passed dir should be the mplab_ide under a MPLAB X Installation"
  println "In Mac, the passed dir should be the mplab_ide.app dir under the MPLAB Xinstallation directory"
	System.exit(4)
}


def listUnix = ""
def listWindows = ""

def listOfDirectoriesWithJars = []
print args[0] + baseDir 
listOfDirectoriesWithJars += getlistOfDirectoriesWithJars(args[0] + baseDir + "mplab_platform/mdbcore") 
listOfDirectoriesWithJars += getlistOfDirectoriesWithJars(args[0] + baseDir + "mplab_platform/mplablibs") 
finalList = []
finalList.add(0, new File("../mdbcs/dist/mdbcs.jar").getAbsolutePath())
mdbcsAbsPath =  new File("../mdbcs/dist/mdbcs.jar").getAbsolutePath()

listUnix = mdbcsAbsPath
listWindows = mdbcsAbsPath
for (f in listOfDirectoriesWithJars) {
        listUnix += ":"
        listUnix += f.getAbsolutePath() + "/*"
        listWindows += ";"
        listWindows += f.getAbsolutePath()  + "/*" 
}

for (f in platformJars) {
        listUnix += ":"
        listUnix += args[0] + baseDir + f
        listWindows += ";"
        listWindows += args[0] + baseDir + f
}    
// Finally create claspath, classpath.bat
def classpathWindows = new File("classpath.bat")
classpathWindows.write("set CLASSPATH=" + listWindows)

def classpathUnix = new File("classpath")
classpathUnix.write("export CLASSPATH=" + listUnix)

println "Created classpath, classpath.bat in the current directory"
