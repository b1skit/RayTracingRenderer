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

    // Transform this Mesh by a transformation matrix
    void transform(TransformationMatrix* theMatrix);

    // Transform this Mesh by a transformation matrix
    void transform(TransformationMatrix* theMatrix, bool doRound);


    // Debug this mesh
    void debug();


    // Mesh attributes:
    vector<Polygon> faces; // This mesh's collection of faces

    bool isWireframe = false; // Whether or not this mesh's polygons are to be rendered in wireframe, or filled
    bool isDepthFogged = false;  // Whether or not this mesh is occluded by atmospheric fog. Default = false

private:


};

#endif // MESH_H
