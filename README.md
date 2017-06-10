A Software 3D Renderer written in C++
By Adam Badke: www.AdamBadke.com
-------------------------------------
--------------------
Repository contents:
--------------------
README.txt						This file
src/							The project C++ source files and the QT 5 project file
WorkingDir/				Contains pre-configured scenes constructed from .simp and .obj files

---------
Overview:
---------
This repository contains the source code for a 3D renderer that can perfrom wireframe rendering, and flat, gouraud, and phong shading of 3D objects in the Wavefront .obj format. Scenes are described in a simple (.simp) format, allowing for objects to be placed in world space, surface colors and properties to be described, and point lights positioned.

Once compiled, this program accepts a command line filename argument, and will load and render the corresponding .simp file from the same directory as the executable and renders it.

---------------------
Instructions for use:
-----------------------------------------------
Compilation requires QT version 5.8 for Windows
Get it here:  https://www.qt.io/download/
-----------------------------------------------
1) Download the respository

2) Load the /src/qtqt.pro file to load the project into QT Creator

3) Configure your working dir to point at any .obj and .simp files you wish to render (ie. \workingDir\ )

3) Build the project in QT Creator (Release mode)

4) Launch the program directly from within QT
  -> The program will load the files included in the \workingDir of this repository (01.simp, 02.simp, ..., 07.simp)


Alternatively, if you would like to run the program directly using the compiled release executable:

1) Copy any required .dll files into the same directory as the compiled executable file (The program will inform you of any missing .dll files if they're not found at launch)
	
	-> Required files are dependent on your system configuration:
	
	-> Eg. For my system, using the 64-bit Microsoft Visual C++ compiler v14.0, I needed to copy the following files next to the compiled .exe:
		
		../Qt/5.8/msvc2015_64/bin/Qt5Core.dll
		../Qt/5.8/msvc2015_64/bin/Qt5Gui.dll
		../Qt/5.8/msvc2015_64/bin/Qt5Widgets.dll 
	
	
		-> Be sure to copy the matching 32 bit or 64 bit versions of these files that correspond with your compiler.
	
  
2) Copy any .simp files and associated .obj files you wish to render into the same directory as the compiled release executable. 
	-> ie. Move the files included in the \workingDir of this repository
			
3) From the command prompt, navigate to the directory containing the compiled executable and simp files

7) Run the compiled qtqt.exe, passing a .simp file as an argument. Do NOT include the ".simp" extension in the filename.
  -> Eg. To load the "page1.simp" file, you would use the following command:
		
			qtqt.exe mySimpFile
