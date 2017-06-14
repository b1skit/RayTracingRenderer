// File interpreter object
// By Adam Badke

#include "fileinterpreter.h"
#include <fstream>
#include <iostream>
#include <list>
#include <iterator>
#include <string>
#include "mesh.h"
#include "transformationmatrix.h"
#include <stack>
#include <vector>
#include "renderutilities.h"
#include "normalvector.h"
#include "light.h"
#include "polygon.h"

using std::ifstream;
using std::cout;
using std::list;
using std::stack;
using std::vector;

// No arg constructor
FileInterpreter::FileInterpreter(){
    // Does nothing
}

// Read a file and assemble a mesh, given the filename
// Return: A mesh object contstructed from the .simp file descriptions
Scene FileInterpreter::buildSceneFromFile(string filename){

    // Assemble the resulting polys into a scene, and return it
    Scene theScene;
    currentScene = &theScene; // Save the address of the scene being constructed

    Mesh newMesh;
    newMesh.faces = getMeshHelper(filename, true, false, false, false, 0xffffffff, phong , 0.3, 8); // Set the default values to start

    theScene.theMeshes.emplace_back( newMesh );

    currentScene = nullptr; // Remove the reference to the local object for safety

    return theScene;
}

// Recursive helper function: Extracts polygons
vector<Polygon> FileInterpreter::getMeshHelper(string filename, bool currentDrawFilled, bool currentDepthFog, bool currentAmbientLighting, bool currentUseSurfaceColor, unsigned int currentSurfaceColor, ShadingModel currentShadingModel, double currentSpecCoef, double currentSpecExponent){
    bool drawFilled = currentDrawFilled;                // Wireframe/filled flag
    bool usesDepthFog = currentDepthFog;                // Depth fog flag
    bool usesAmbientLighting = currentAmbientLighting;  // Ambient lighting flag

    bool usesSurfaceColor = currentUseSurfaceColor;
    unsigned int theSurfaceColor = currentSurfaceColor;

    ShadingModel theShadingModel = currentShadingModel;
    double theSpecCoefficient = currentSpecCoef;
    double theSpecExponent = currentSpecExponent;

    TransformationMatrix CTM;                   // Identity matrix
    stack<TransformationMatrix> theCTMStack;    // Stack of CTM's
    vector<Polygon> processedFaces;             // Vector of processed polygons, that will be inserted returned for insertion into a mesh
    vector<Polygon> currentFaces;               // A working vector of polygon faces, that will eventually be added to the processed faces vector

    // Open the file, and process it:
    ifstream* input = new ifstream(); // File reading object
    input->open(filename);
    if (input->is_open() ){

        // Catch errors caused by malformed files
        try {
            while(!input->eof()){ // Loop until the end of the file has been reached

                // Extract and cleanse the current line:
                string currentLine;
                getline(*input, currentLine); // Read the current line
                list<string>currentLineTokens = interpretTokenLine(currentLine); // Send the line for interpretation

                // Step through the extracted tokens, and process them:
                list<string>::iterator theIterator = currentLineTokens.begin(); // Point an iterator to the start of our list
                while (theIterator != currentLineTokens.end()){

                    // Handle wireframe/filled directive:
                    if (theIterator->compare("filled") == 0){
                        drawFilled = true;
                        theIterator++;
                    }
                    else if (theIterator->compare("wire") == 0){
                        drawFilled = false;
                        theIterator++;
                    }

                    // Handle open brace "{"
                    else if(theIterator->compare("{") == 0){
                        theIterator++;
                        theCTMStack.push(CTM); // Push the current CTM to the stack

                        CTM = TransformationMatrix(); // Set the transformation matrix back to the identity matrix
                    }

                    // Handle scale commands:
                    else if(theIterator->compare("scale") == 0){
                        theIterator++;

                        // Generate an identity matrix, apply the xform and push it to the stack
                        TransformationMatrix currentTransformation;
                        double scaleX = stod(*theIterator++);
                        double scaleY = stod(*theIterator++);
                        double scaleZ = stod(*theIterator++);
                        currentTransformation.addNonUniformScale(scaleX, scaleY, scaleZ);

                        // Multiply the new xForm into the CTM
                        CTM *= currentTransformation;
                    }

                    // Handle rotate commands:
                    else if(theIterator->compare("rotate") == 0){
                        theIterator++;

                        // Determine the axis of rotation:
                        string axis = *theIterator++;
                        Axis theAxis;
                        if (axis.compare("X") == 0){
                            theAxis = X;
                        } else if (axis.compare("Y") == 0){
                            theAxis = Y;
                        } else if (axis.compare("Z") == 0){
                            theAxis = Z;
                        }

                        // Get the angle, build a rotation transformation and push it to the local stack:
                        string angle = *theIterator++;
                        TransformationMatrix currentTransformation; // Generate an identity matrix
                        currentTransformation.addRotation(theAxis, stod(angle));

                        CTM *= currentTransformation;
                    }

                    // Handle translate commands:
                    else if(theIterator->compare("translate") == 0){
                        theIterator++;

                        // Build a transformation matrix and push it to the local stack:
                        TransformationMatrix currentTransformation;
                        string transVal = *theIterator++;
                        double transX = stod(transVal);
                        transVal = *theIterator++;
                        double transY = stod(transVal);
                        transVal = *theIterator++;
                        double transZ = stod(transVal);
                        currentTransformation.addTranslation(transX, transY, transZ);

                        CTM *= currentTransformation;
                    }

                    // Handle camera commands:
                    else if (theIterator->compare("camera") == 0){
                        theIterator++;

                        currentScene->xLow = stod(*theIterator++);
                        currentScene->yLow = stod(*theIterator++);

                        currentScene->xHigh = stod(*theIterator++);
                        currentScene->yHigh = stod(*theIterator++);

                        currentScene->camHither = stod(*theIterator++);
                        currentScene->camYon = stod(*theIterator++);

                        currentScene->cameraMovement = CTM;
                    }

                    // Handle ambient commands:
                    else if (theIterator->compare("ambient") == 0){
                        theIterator++;
                        currentScene->ambientRedIntensity = stod(*theIterator++);
                        currentScene->ambientGreenIntensity = stod(*theIterator++);
                        currentScene->ambientBlueIntensity = stod(*theIterator++);

                        // Update the ambient flag
                        usesAmbientLighting = true;

                    }

                    // Handle depth commands:
                    else if (theIterator->compare("depth") == 0){
                        theIterator++;
                        usesDepthFog = true;

                        // Update the renderer with the new parameters
                        currentScene->fogHither = stod(*theIterator++);
                        currentScene->fogYon = stod(*theIterator++);
                        currentScene->fogRedIntensity = stod(*theIterator++);
                        currentScene->fogGreenIntensity = stod(*theIterator++);
                        currentScene->fogBlueIntensity = stod(*theIterator++);

                        currentScene->fogColor = combineColorChannels(currentScene->fogRedIntensity, currentScene->fogGreenIntensity, currentScene->fogBlueIntensity);
                    }

                    // Handle light commands
                    else if (theIterator->compare("light") == 0){
                        theIterator++;

                        // Get the colors
                        double redIntensity = stod(*theIterator++);
                        double greenIntensity = stod(*theIterator++);
                        double blueIntensity = stod(*theIterator++);
                        double attenuationA = stod(*theIterator++);
                        double attenuationB = stod(*theIterator++);

                        // Create a light, transform it using the CTM and pass it to the renderer
                        Light newLight(redIntensity, greenIntensity, blueIntensity, attenuationA, attenuationB);
                        newLight.position.transform( &CTM );

                        currentScene->theLights.emplace_back(newLight);
                    }

                    // Handle surface commands:
                    else if (theIterator->compare("surface") == 0){
                        theIterator++;

                        usesSurfaceColor = true;

                        double red = stod(*theIterator++);
                        double green = stod(*theIterator++);
                        double blue = stod(*theIterator++);

                        theSurfaceColor = combineColorChannels(red, green, blue);

                        // Store the spec settings
                        theSpecCoefficient = stod(*theIterator++);
                        theSpecExponent = stod(*theIterator++);
                    }

                    // Handle flat shading:
                    else if (theIterator->compare("flat") == 0){
                        theIterator++;
                        theShadingModel = flat;
                    }

                    // Handle gouraud shading:
                    else if (theIterator->compare("gouraud") == 0){
                        theIterator++;
                        theShadingModel = gouraud;
                    }

                    // Handle phong shading:
                    else if (theIterator->compare("phong") == 0){
                        theIterator++;
                        theShadingModel = phong;
                    }

                    // Handle obj commands
                    else if (theIterator->compare("obj") == 0){
                        theIterator++;
                        vector<Polygon> objContents = getPolysFromObj("./" + *theIterator + ".obj");

                        for (unsigned int i = 0; i < objContents.size(); i++){
                            objContents[i].transform(&CTM);

                            // Set the various polygon drawing flags:
                            objContents[i].setFilled(drawFilled);

                            // Set the surface color instructions:
                            if (usesSurfaceColor)
                                objContents[i].setSurfaceColor(theSurfaceColor);

                            objContents[i].setSpecularCoefficient(theSpecCoefficient);
                            objContents[i].setSpecularExponent(theSpecExponent);
                            objContents[i].setShadingModel(theShadingModel);

                            // Set the fog and ambient lighting:
                            objContents[i].setShadingModel(theShadingModel);
                            objContents[i].setAffectedByDepthFog(usesDepthFog);
                            objContents[i].setAffectedByAmbientLight(usesAmbientLighting);
                        }
                        currentFaces.insert(currentFaces.end(), objContents.begin(), objContents.end() );
                    }

                    // Handle file commands
                    else if (theIterator->compare("file") == 0){

                        theIterator++; // Move the iterator to the filename element

                        vector<Polygon> loadedFaces = getMeshHelper("./" + *theIterator + ".simp", drawFilled, usesDepthFog, usesAmbientLighting, usesSurfaceColor, theSurfaceColor, theShadingModel, theSpecCoefficient, theSpecExponent);
                        theIterator++;

                        for (unsigned int i = 0; i < loadedFaces.size(); i++){
                            loadedFaces[i].transform(&CTM);
                        }
                        currentFaces.insert(currentFaces.end(), loadedFaces.begin(), loadedFaces.end() );
                    }

                    // Handle line commands:
                    else if (theIterator->compare("line") == 0){
                        theIterator++;

                        // Create a pair of vertices
                        Vertex v1, v2;

                        if (currentLineTokens.size() == 7){ // Handle lines with only XYZ tokens
                            v1 = {stod(*theIterator++), stod(*theIterator++), stod(*theIterator++)};
                            v2 = {stod(*theIterator++), stod(*theIterator++), stod(*theIterator)};
                        }

                        else { // Handle lines with XYZRGB tokens
                            v1 = {stod(*theIterator++), stod(*theIterator++), stod(*theIterator++)};
                            double red = stod(*theIterator++);
                            double green = stod(*theIterator++);
                            double blue = stod(*theIterator++);
                            v1.color = combineColorChannels(red, green, blue); // Combine the channels into a single unsigned int

                            v2 = {stod(*theIterator++), stod(*theIterator++), stod(*theIterator++)};
                            red = stod(*theIterator++);
                            green = stod(*theIterator++);
                            blue = stod(*theIterator);
                            v2.color = combineColorChannels(red, green, blue); // Combine the channels into a single unsigned int
                        }

                        // Set the surface color
                        if (usesSurfaceColor){
                            v1.color = theSurfaceColor;
                            v2.color = theSurfaceColor;
                        }

                        Polygon newFace;
                        newFace.addVertex(v1);
                        newFace.addVertex(v2);

                        newFace.setAffectedByDepthFog(usesDepthFog);
                        newFace.setAffectedByAmbientLight(usesAmbientLighting);

                        newFace.transform(&CTM); // Apply the CTM to the line

                        // Place the new line into the vector:
                        currentFaces.emplace_back(newFace);
                    }

                    // Handle "polygon" or "triangle" commands:
                    else if (theIterator->compare("polygon") == 0 || theIterator->compare("triangle") == 0){
                        theIterator++;
                        bool suppliedColor = false;
                        Vertex v1, v2, v3;
                        if (currentLineTokens.size() <= 10){ // Handle polygons without RGB tokens
                            v1.x = stod(*theIterator++);
                            v1.y = stod(*theIterator++);
                            v1.z = stod(*theIterator++);

                            v2.x = stod(*theIterator++);
                            v2.y = stod(*theIterator++);
                            v2.z = stod(*theIterator++);

                            v3.x = stod(*theIterator++);
                            v3.y = stod(*theIterator++);
                            v3.z = stod(*theIterator++);
                        }

                        else { // Handle polygons with RGB tokens
                            suppliedColor = true;

                            v1.x = stod(*theIterator++);
                            v1.y = stod(*theIterator++);
                            v1.z = stod(*theIterator++);
                            double red = stod(*theIterator++);
                            double green = stod(*theIterator++);
                            double blue = stod(*theIterator++);
                            v1.color = combineColorChannels(red, green, blue); // Combine the channels into a single unsigned int
                            v1.hasColor = true;

                            v2.x = stod(*theIterator++);
                            v2.y = stod(*theIterator++);
                            v2.z = stod(*theIterator++);
                            red = stod(*theIterator++);
                            green = stod(*theIterator++);
                            blue = stod(*theIterator++);
                            v2.color = combineColorChannels(red, green, blue); // Combine the channels into a single unsigned int
                            v2.hasColor = true;

                            v3.x = stod(*theIterator++);
                            v3.y = stod(*theIterator++);
                            v3.z = stod(*theIterator++);
                            red = stod(*theIterator++);
                            green = stod(*theIterator++);
                            blue = stod(*theIterator);
                            v3.color = combineColorChannels(red, green, blue); // Combine the channels into a single unsigned int
                            v3.hasColor = true;
                        }

                        // Handle surface color calls
                        if (usesSurfaceColor && !suppliedColor){
                            v1.color = theSurfaceColor;
                            v2.color = theSurfaceColor;
                            v3.color = theSurfaceColor;
                        }

                        // Build the polygon
                        Polygon newFace(v1, v2, v3);
                        newFace.transform(&CTM); // Apply the CTM

                        // Set the various polygon drawing flags:
                        newFace.setFilled(drawFilled);
                        newFace.setAffectedByDepthFog(usesDepthFog);
                        newFace.setAffectedByAmbientLight(usesAmbientLighting);

                        newFace.setSpecularCoefficient(theSpecCoefficient);
                        newFace.setSpecularExponent(theSpecExponent);
                        newFace.setShadingModel(theShadingModel);

                        // Set all .simp object polygon normals to be equivalent to face normals
                        normalVector faceNormal = newFace.getFaceNormal();
                        for (int i = 0; i < newFace.getVertexCount(); i++){
                            newFace.vertices[i].normal = faceNormal;
                        }


                        // Place the new face in the vector:
                        currentFaces.emplace_back(newFace);
                    }

                    // Handle closed brace "}": Pop the top of the matrix stack, overwriting the CTM with the popped matrix
                    else if(theIterator->compare("}") == 0 ){
                        theIterator++;
                        CTM = theCTMStack.top();
                        theCTMStack.pop();

                        // Apply the current CTM to the faces from the block
                        for (unsigned int i = 0; i < currentFaces.size(); i++){
                            currentFaces[i].transform(&CTM);
                        }
                        // Insert the processed faces into the final vector:
                        processedFaces.insert(processedFaces.end(), currentFaces.begin(), currentFaces.end() );
                        currentFaces.clear();

                    } else
                        theIterator++;

                } // end while: Iterator walk

            }// end while: !input.eof

        } catch (const std::exception &e){
            cout << "ERROR - File " << filename << ".simp is malformed!!! Resulting mesh is empty.\n";
        }
    } else
        cout << "ERROR - File " << filename << " not found!!! Please place it the working directory.\n";

    // Close the input stream
    input->close();

    // Insert the last faces into the vector of processed faces:
    processedFaces.insert(processedFaces.end(), currentFaces.begin(), currentFaces.end() );

    return processedFaces;
}

// Interpret a string that has been read
// Return: A list of split and cleansed tokens
list<string> FileInterpreter::interpretTokenLine(string currentLine){

    // Remove any whitespace from the front of the string
    unsigned int start = 0;
    while (start < currentLine.length() && (currentLine[start] == ' ' || currentLine[start] == '\t' || currentLine[start] == '\n' || currentLine[start] == '\r' ))
        start++;
    if (start != 0)
        currentLine = currentLine.substr(start, string::npos);

    // Handle empty lines or comments:
    if (currentLine.empty() || currentLine[0] == '#')
        return list<string>(); // Nothing to insert: Return an empty list

    // Split the line, then cleanse each token of unneccessary formatting characters:
    list<string> tokens = cleanseTokens( split(currentLine) );

    return tokens;
}

// Split a string
// Return: A list<string> containing each token as an individual element, in the order the file was read
list<string> FileInterpreter::split(string line){
    list<string> tokens; // Will store our cleansed instruction strings

    // Remove any whitespace from the front of the string
    unsigned int start = 0;
    while (line[start] == ' ' || line[start] == '\t' || line[start] == '\n' || line[start] == '\r')
        start++;
    line = line.substr(start, string::npos);

    // Handle single character lines:
    if (line.length() == 1 && (line[0] == '{' || line[0] == '}')){
        tokens.push_back(line);
        return tokens; // We're done - no point continuing
    }

    // Split the line at each space character:
    start = 0;
    unsigned int indexIterator = 1;
    while (indexIterator < line.length() ){

        if (line[indexIterator] == ' '){ // Split point found
            // Handle multiple spaces
            while ((indexIterator + 1) < line.length() && line[indexIterator + 1] == ' ')
                indexIterator++;

            // Extract the token and add it to our token list
            string newToken = line.substr(start, indexIterator - start);
            tokens.push_back(newToken);

            start = indexIterator + 1; // Move our starting position
            indexIterator+= 2;

        }

        // Handle '/' characters:
        else if (line[indexIterator] == '/'){ // Slash found: Used to seperate v/vt/vn commands when processing obj's
            // We want to leave the '/' in the list of tokens
            string newToken = line.substr(start, (indexIterator - start));
            tokens.push_back(newToken);
            tokens.push_back("/");

            indexIterator++;

            // Handle double slashes:
            if (line[indexIterator] == '/'){
                tokens.push_back("/");
                indexIterator++;
            }
            start = indexIterator;

        } else
            indexIterator++; // Advance the iterator

    } // end while

    // Add the last portion of the line, if there is anything left
    if (start <= line.length() - 1)
        tokens.push_back(line.substr(start, string::npos));


    return tokens;
}

// Cleanse a list of tokens of any formatting characters. Used after split has extracted tokens from a line
// Return: A list<string> containing tokens cleansed of formatting characters
list<string> FileInterpreter::cleanseTokens(list<string> tokens){
    list<string>::iterator theIterator = tokens.begin();
    while (theIterator != tokens.end()){
        string currentLine = *theIterator;

        // Find a start position past any formatting characters at the beginning of the string
        unsigned int start = 0;
        while ( start < currentLine.length() && ( currentLine[start] == ' ' || currentLine[start] == '(' || currentLine[start] == '\t' || currentLine[start] == '\"' ) ){
            start++;
        }

        // Find an end position beefore any formatting characters at the end of the string
        int end = (int)currentLine.length() - 1;
        while ( end >= 0 && ( currentLine[ end ] == ' ' || currentLine[ end ] == ',' || currentLine[ end ] == ')' || currentLine[ end ] == '\t' || currentLine[ end ] == '\"' || currentLine[ end ] == '\n' || currentLine[ end ] == '\r') ) {
            end--;
        }

        // If a string has nothing but formatting characters, remove it from the token array
        if ( end < (int)start){
            list<string>::iterator temp = theIterator;
            theIterator++;
            tokens.erase(temp);
        } else{ // Otherwise, strip the formatting characters and add the modified substring back into the list and move on:
            *theIterator = currentLine.substr(start, end - start + 1);
            theIterator++;
        }
    } // End while

    return tokens;
}

// Read an obj file
// Return: A vector<Polygon> containing all of the faces described by the obj
vector<Polygon> FileInterpreter::getPolysFromObj(string filename){

    vector<Polygon> theFaces; // A vector of Polygon objects, extracted face by face

    vector<Vertex> theVertices; // We burn the first index with a dummy vertex to maintain vertex # to vector index equivalence
    theVertices.emplace_back( Vertex() ); // Dummy vertex: All vertices from index 1 onward are valid!

    // Use a vector of Vertex's to hold our normals: Bit of a workaround with a tiny memory overhead but good enough for now.
    vector<normalVector> theNormals; // We burn the first index with a dummy vertex to maintain vertex # to vector index equivalence
    theNormals.emplace_back( normalVector() ); // Dummy vertex: All vertices from index 1 onward are valid!

    ifstream* input = new ifstream(); // File reading object
    input->open(filename);
    if (input->is_open() ){
        while(!input->eof()){
            // Extract and cleanse the current line:
            string currentLine;
            getline(*input, currentLine); // Read the current line
            list<string>currentLineTokens = interpretTokenLine(currentLine); // Send the line for interpretation

            // Step through the extracted tokens, and process them:
            list<string>::iterator theIterator = currentLineTokens.begin(); // Point an iterator to the start of our list
            while (theIterator != currentLineTokens.end()){

                // Handle "v" vertice commands
                if (theIterator->compare("v") == 0){
                    theIterator++;

                    // Create an XYZ vertex:
                    Vertex v1;
                    double x = stod(*theIterator++);
                    double y = stod(*theIterator++);
                    double z = stod(*theIterator++);
                    v1.x = x;
                    v1.y = y;
                    v1.z = z;

                    // Vertex: XYZW
                    if (currentLineTokens.size() == 5){
                        double w = stod(*theIterator++);
                        v1.setW(w);
                    }

                    // Vertex: XYZRGB
                    else if (currentLineTokens.size() == 7){
                        double red = stod(*theIterator++);
                        double green = stod(*theIterator++);
                        double blue = stod(*theIterator++);
                        v1.color = combineColorChannels( red, green, blue );
                        v1.hasColor = true;
                    }
                    // Vertex: XYZWRGB
                    else if (currentLineTokens.size() == 8){
                        double w = stod(*theIterator++);
                        v1.setW(w);

                        double red = stod(*theIterator++);
                        double green = stod(*theIterator++);
                        double blue = stod(*theIterator++);
                        v1.color = combineColorChannels( red, green, blue );
                        v1.hasColor = true;
                    }

                    // Convert to LHS coordinate system:
                    v1.z *= -1;

                    theVertices.emplace_back(v1); // Add the assembled vertex to our vertex vector
                }

                // Handle "vn" vertex normals
                else if(theIterator->compare("vn") == 0){
                    theIterator++;

                    normalVector newNormal;
                    newNormal.xn = stod(*theIterator++);
                    newNormal.yn = stod(*theIterator++);
                    newNormal.zn = stod(*theIterator++);

                    // Convert to LHS coordinate system:
                    newNormal.zn *= -1;

                    theNormals.emplace_back(newNormal);
                }

                // Handle "vt" vertex texture coordinates
                else if(theIterator->compare("vt") == 0){
                    theIterator++; // Rest of the elements will be iterated past....

                }

                // Handle "f" face details: f v/vt/vn
                else if(theIterator->compare("f") == 0){
                    theIterator++;

                    Polygon newFace; // A polygon, that will be initialized with the indicated vertices

                    // Loop, reading tokens in order until the end of the line
                    while (theIterator != currentLineTokens.end() ){

                        Vertex newVertex; // Create a vertex object

                        // Retrieve the stored vertex
                        int index = stoi( *theIterator++ ); // Move past the vertex index
                        if (index < 1)
                            newVertex = theVertices[ theVertices.size() + index]; // Add our negative value to the size to get the correct offset
                        else
                            newVertex = theVertices[ index ];

                        // Read the rest of the line:
                        if (theIterator != currentLineTokens.end() && theIterator->compare("/") == 0){ // Slash: Must be a vt or / next
                            theIterator++;

                            if (theIterator != currentLineTokens.end() && !theIterator->compare("/") == 0){ // Not a slash: Must be a vt
                                theIterator++; // Skip handling vt's (for now...)

                                // Handle vn trailing vt, if it exists: f v/vt/vn
                                if (theIterator != currentLineTokens.end() && theIterator->compare("/") == 0){ // Another slash: must be vn
                                    theIterator++; // Move past the slash
                                    normalVector newNormal;
                                    index = stoi( *theIterator++ );
                                    if (index < 1)
                                        newNormal = theNormals[ theNormals.size() + index]; // Add our negative value to the size to get the correct offset
                                    else
                                        newNormal = theNormals[ index ];

                                    // Store the normal in the vertex
                                    newVertex.normal = newNormal;
                                }
                            }
                            else { // IS a slash: must be a vn: f v//vn
                                theIterator++; // Move past the "/" character

                                normalVector newNormal;
                                index = stoi( *theIterator++ );
                                if (index < 1)
                                    newNormal = theNormals[ theNormals.size() + index]; // Add our negative value to the size to get the correct offset
                                else
                                    newNormal = theNormals[ index ];

                                // Store the normal in the vertex
                                newVertex.normal = newNormal;
                            }
                        } // End vertex "/" handling: Move to next vertex in line

                        newFace.addVertex(newVertex);

                    } // End while


                    // Check each vertex to make sure it has a valid normal. Set it to the face normal if it does not
                    normalVector faceNormal = newFace.getFaceNormal();
                    for (int i = 0; i < newFace.getVertexCount(); i++){
                        if (newFace.vertices[i].normal.isZero() ) {
                            newFace.vertices[i].normal = faceNormal; // Set 0,0,0 normals to be the face normal
                        }
                    }

                    // Triangulate, if neccessary:
                    if (newFace.getVertexCount() > 3){
                        vector<Polygon>* triangulatedFaces = newFace.getTriangulatedFaces();
                        // Add the triangulated faces to the mesh:
                        for (unsigned int i = 0; i < triangulatedFaces->size(); i++){
                            theFaces.emplace_back(triangulatedFaces->at(i) );
                        }
                        // Cleanup:
                        delete triangulatedFaces;

                    } else // Otherwise, add the new face to our vector of extracted faces:
                        theFaces.emplace_back(newFace);

                } // End "f" face command handling
                else
                    theIterator++;

            } // End iterator while

        } //end input.eof

    }// end if input.is_open
    else
        cout << "ERROR - File " << filename << " not found!!! Please place it the working directory.\n";

    return theFaces;
}
