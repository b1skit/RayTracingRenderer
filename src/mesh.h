// Mesh object: Contains a collection of polygon objects
// By Adam Badke


#ifndef MESH_H
#define MESH_H

#include "polygon.h"
#include <vector>

using std::vector;
class Mesh;

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

    // Generate/update a bounding box around the faces of this mesh
    // Precondition: The mesh has at least 1 polygon
    void generateBoundingBox();

    // Debug this mesh
    void debug();

    // Mesh attributes:
    //*****************
    vector<Polygon> faces; // This mesh's collection of faces
    vector<Polygon> boundingBoxFaces;    // A collection of 6 faces that make up a bounding box surrounding this polygon
    bool isWireframe = false; // Whether or not this mesh's polygons are to be rendered in wireframe, or filled
};

#endif // MESH_H
