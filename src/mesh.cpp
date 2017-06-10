// Mesh object: Contains a collection of polygon objects
// By Adam Badke

#include "mesh.h"
#include "polygon.h"

// Constructor
Mesh::Mesh(){
    isFilled = true; // Default to filled

    faces.reserve(3);
}

// Copy Constructor
Mesh::Mesh(const Mesh &existingMesh){
    faces = existingMesh.faces;
    isFilled = existingMesh.isFilled;
}

// Add a polygon face to the mesh
// Assumption: The face is not already contained in the mesh
void Mesh::addFace(Polygon newFace){
    faces.push_back(newFace);
}

// Get the vector of faces for this mesh. Used by the renderer
vector<Polygon> Mesh::getFaces(){
    return faces;
}

// Transform this polygon by a transformation matrix
void Mesh::transform(TransformationMatrix* theMatrix){
    transform(theMatrix, false);
}

// Transform this polygon by a transformation matrix
void Mesh::transform(TransformationMatrix* theMatrix, bool doRound){
    for(unsigned int i = 0; i < faces.size(); i++){
        faces[i].transform(theMatrix, doRound);
    }
}

// Debug this mesh
void Mesh::debug(){
    for (unsigned int i = 0; i < faces.size(); i++)
        faces[i].debug();
}
