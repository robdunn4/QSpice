# VSCode Configuration

For those who want to use the VSCode IDE for QSpice C-Block development and be able to compile directly from VSC, here is my build configuration file.  It contains build tasks for:

* The QSpice Digital Mars compiler (DMC).
* The free version of Microsoft Visual Studio (MSVC 2022).
* And the MinGW toolset (also free -- see below).

To use it, save the tasks.json file in the .vscode subfolder of your project.  From the Terminal menu in VSCode, select "Configure Default Build Task" and choose the desired compiler.

To build a C-Block DLL, make the *.cpp file the active editor item and select "Run Build Task" from the Terminal menu.  See the VSCode online help for more information about getting started and using VSCode.

Notes:

* Visual Studio Code (VSCode/VSC) and Microsoft Visual Studio (MSVS) are not the same thing.  VSCode is a Microsoft-supported open source IDE.  It does not contain a compiler so you'll need a compiler toolset.  VSCode executables, sources, and support are available [here](https://code.visualstudio.com/).

* The configuration assumes default install locations for the toolsets.  If you need to edit the *.json file for different installation locations, you can do that from within VSCode.

* See CBlockTemplates.pdf elsewhere in this repo and associated code if you want to compile C-Block DLLs using all three of the above toolsets and a single codebase.

## About MinGW

Download and install MSYS2 from [MSYS2.org](https://www.msys2.org).  This installs the 64-bit toolset in C:\msys64.  It creates empty 32-bit toolset sub-folders in C:\msys64\mingw32.

For QSpice DLL components, you'll need to install the 32-bit tools.  If you can find good instructions, follow those.  Otherwise, try the following:

* Run msys2.exe from the installation folder.  This will open a bash shell.
* Enter the following command:  `pacman -S mingw-w64-i686-toolchain`
* Enter twice to install all 32-bit MinGW tools.
* When the install completes, enter `exit` to close the bash shell.

Hopefully, that will get you up an running.