// Mesh object: Contains a collection of polygon objects
// By Adam Badke


#ifndef MESH_H
#define MESH_H

#include "polygon.h"
#include <vector>

using std::vector;

class Mesh
{
public:
    // Constructor
    Mesh();

    // Copy Constructor
    Mesh(const Mesh &existingMesh);

    // Overloaded assignment operator
    Mesh& operator=(const Mesh& rhs);

    // Add a polygon face to the mesh
    // Assumption: The face is not already contained in the mesh
    void addFace(Polygon newFace);

    // Transform this Mesh by a transformation matrix
    void transform(TransformationMatrix* theMatrix);

    // Transform this Mesh by a transformation matrix
    void transform(TransformationMatrix* theMatrix, bool doRound);


    // Debug this mesh
    void debug();


    vector<Polygon> faces; // This mesh's collection of faces

private:

    bool isFilled; // Whether or not this mesh's polygons are to be rendered as a wireframes, or filled
};

#endif // MESH_H
