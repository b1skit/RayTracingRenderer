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

    // Add a polygon face to the mesh
    // Assumption: The face is not already contained in the mesh
    void addFace(Polygon newFace);

    // Get the vector of faces for this mesh. Used by the renderer
    vector<Polygon> getFaces();

    // Transform this Mesh by a transformation matrix
    void transform(TransformationMatrix* theMatrix);

    // Transform this Mesh by a transformation matrix
    void transform(TransformationMatrix* theMatrix, bool doRound);


    // Debug this mesh
    void debug();

private:
    vector<Polygon> faces; // This mesh's collection of faces
    bool isFilled; // Whether or not this mesh's polygons are to be rendered as a wireframes, or filled
};

#endif // MESH_H
