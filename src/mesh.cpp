// Mesh object: Contains a collection of polygon objects
// By Adam Badke

#include "mesh.h"
#include "polygon.h"

// Constructor
Mesh::Mesh(){
    isWireframe = true; // Default to filled

    faces.reserve(3);
}

// Copy Constructor
Mesh::Mesh(const Mesh &existingMesh){
    faces = existingMesh.faces;
    isWireframe = existingMesh.isWireframe;
}

// Overloaded assignment operator
Mesh& Mesh::operator=(const Mesh& rhs){
    this->faces = rhs.faces;
    this->isWireframe = rhs.isWireframe;

    return *this;
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
