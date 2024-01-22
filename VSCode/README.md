# VSCode Configuration

For those who want to use the VSCode IDE for QSpice C-Block development and be able to compile directly from VSC, here is my build configuration file.  It contains build tasks for:

* The QSpice Digital Mars compiler (DMC).
* The free version of Microsoft Visual Studio (MSVC 2022).
* And the MinGW toolset (also free).

To use it, save the tasks.json file in the .vscode subfolder of your project.  From the Terminal menu in VSCode, select "Configure Default Build Task" and choose the desired compiler.

To build a C-Block DLL, make the *.cpp file the active editor item and select "Run Build Task" from the Terminal menu.  See the VSCode online help for more information about getting started and using VSCode.

Notes:

* The configuration assumes default install locations for the toolsets.  If you need to edit the *.json file for different installation locations, you can do that from within VSCode.

* See CBlockTemplates.pdf elsewhere in this repo and associated code if you want to compile C-Block DLLs using all three of the above toolsets and a single codebase.