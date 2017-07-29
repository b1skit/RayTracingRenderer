A Ray-tracing 3D Renderer written in C++
-------------------------------------
By Adam Badke: www.AdamBadke.com
-------------------------------------
--------------------
Repository contents:
--------------------
README.txt				This file

src/					The project C++ source files and the QT 5 project file

WorkingDir/				Contains pre-configured scenes constructed from .simp and .obj files

---------
Overview:
---------
This repository contains the source code for a ray tracing 3D renderer that can perform wireframe, flat, gouraud, and phong shaded rendering of objects described in the Wavefront .obj format. Scenes are constructed using a simple (.simp) format, allowing for objects to be placed and transformed in world space, as well as control of surface colors and propertie, point lighting, and environment variables.

Once compiled, this program accepts a command line filename argument and will load and render a .simp file (located in the same directory as the executable). Alternatively, if no command line argument is found the program will display a GUI window allowing users to cycle through 9 pre-configured scenes.

---------------------
Instructions for use:
------------------------------------------------------------------------------------
Preferred: Compilation using QT version 5.8 for Windows: https://www.qt.io/download/
------------------------------------------------------------------------------------
1) Download the respository

2) Open the /src/qtqt.pro file in QT Creator to load the project

3) Configure QT Creator's working directory to point at any .obj and .simp files you wish to render (ie. \workingDir\ ).
-> Alternatively, if command line rendering is to be used, place your .obj and .simp files alongside your compiled executable

3) Build the project in QT Creator (Release mode)

4) Launch the program from within QT Creator
  -> The program will load the files included in the \workingDir of this repository (01.simp, 02.simp, ..., 09.simp)


--------------------------------------------------------------------------------
Alternative: Running the program directly using the compiled release executable:
--------------------------------------------------------------------------------
1) Copy any required QT .dll files into the same directory as the compiled executable file (The program will inform you of any missing .dll files if they're not found at launch)
	
	-> Required files are dependent on your system configuration:
	
	Eg. For my system, using the 64-bit Microsoft Visual C++ compiler v14.0, I needed to copy the following files next to the compiled .exe:
		
		../Qt/5.8/msvc2015_64/bin/Qt5Core.dll
		../Qt/5.8/msvc2015_64/bin/Qt5Gui.dll
		../Qt/5.8/msvc2015_64/bin/Qt5Widgets.dll 
	
	
		-> Be sure to copy the matching 32 bit or 64 bit versions of these files that correspond with your compiler.
	
  
2) Copy any .simp files and associated .obj files you wish to render into the same directory as the compiled release executable. 
	-> ie. Move the files included in the \workingDir of this repository
			
3) From the command prompt, navigate to the directory containing the compiled executable and simp files

7) Run the compiled qtqt.exe, passing a .simp file as an argument. Do NOT include the ".simp" extension in the filename.
  -> Eg. To load the "page1.simp" file, you would use the following command:
		
			qtqt.exe page1
