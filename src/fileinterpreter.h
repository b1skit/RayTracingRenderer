// File interpreter object
// By Adam Badke


#ifndef FILEINTERPRETER_H
#define FILEINTERPRETER_H

#include <string>
#include <list>
#include "mesh.h"

#include "polygon.h"
#include "renderer.h"
#include <vector>

using std::string;
using std::list;
using std::vector;

class FileInterpreter
{
public:

    // No arg constructor
    FileInterpreter();

    // Constructor
    FileInterpreter(Renderer* newRenderer);

    // Read a file and assemble a mesh, given the filename. Invokes the recursive helper function
    // Return: A mesh object contstructed from the .simp file descriptions
    Mesh getMeshFromFile(string fileName);

private:
    // Pointer to the client's renderer object
    Renderer* theFileRenderer; // A renderer object, used to render the contents of the file

    // Recursive helper function: Extracts polygons
    vector<Polygon> getMeshHelper(string filename, bool currentDrawFilled, bool currentDepthFog, bool currentAmbientLighting, bool currentUseSurfaceColor, unsigned int currentSurfaceColor, ShadingModel currentShadingModel, double currentSpecCoef, double currentSpecExponent);

    // Interpret a string that has been read
    // Return: A list of split and cleansed tokens
    list<string> interpretTokenLine(string newString);

    // Split a string
    // Return: A list<string> containing each token as an individual element, in the order the file was read
    list<string> split(string line);

    // Cleanse a list of tokens of any formatting characters. Used after split has extracted tokens from a line
    // Return: A list<string> containing tokens cleansed of formatting characters
    list<string> cleanseTokens(list<string> tokens);

    // Read an obj file
    // Return: A vector<Polygon> containing all of the faces described by the obj
    vector<Polygon> getPolysFromObj(string filename);
};

#endif // FILEINTERPRETER_H
