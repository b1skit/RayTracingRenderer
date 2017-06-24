// Mesh object: Contains a collection of polygon objects
// By Adam Badke

#include "mesh.h"
#include "polygon.h"
#include <iostream>

using std::cout;

// Constructor
Mesh::Mesh(){
    isWireframe = true; // Default to filled

    faces.reserve(3);
    boundingBoxFaces.reserve(6);
}

// Copy Constructor
Mesh::Mesh(const Mesh &existingMesh){
    faces = existingMesh.faces;
    isWireframe = existingMesh.isWireframe;
    isDepthFogged = existingMesh.isDepthFogged;

    boundingBoxFaces = existingMesh.boundingBoxFaces;
}

// Overloaded assignment operator
Mesh& Mesh::operator=(const Mesh& rhs){
    this->faces = rhs.faces;
    this->isWireframe = rhs.isWireframe;
    this->isDepthFogged = rhs.isDepthFogged;

    this->boundingBoxFaces = rhs.boundingBoxFaces;

    return *this;
}

// Transform this polygon by a transformation matrix
void Mesh::transform(TransformationMatrix* theMatrix){
    transform(theMatrix, false);
}

// Transform this polygon by a transformation matrix
void Mesh::transform(TransformationMatrix* theMatrix, bool doRound){

    // Transform the faces of the visible mesh:
    for(unsigned int i = 0; i < faces.size(); i++){
        faces[i].transform(theMatrix, doRound);
    }

    // Transform the faces of the bounding box:
    for (unsigned int i = 0; i < boundingBoxFaces.size(); i++){
        boundingBoxFaces[i].transform(theMatrix, doRound);
    }
}

// Generate/update a bounding box around the faces of this mesh
// Precondition: The mesh has at least 1 polygon
void Mesh::generateBoundingBox(){

    double xMin = faces[0].vertices[0].x;
    double xMax = faces[0].vertices[0].x;
    double yMin = faces[0].vertices[0].y;
    double yMax = faces[0].vertices[0].y;
    double zMin = faces[0].vertices[0].z;
    double zMax = faces[0].vertices[0].z;

    for (int i = 0; i < faces.size(); i++){

        // Get the # of vertices for the current polygon once:
        int numVertices = faces[i].getVertexCount();

        // Loop through each vertex in the polygon
        for (int j = 0; j < numVertices; j++){

            if (faces[i].vertices[j].x < xMin)
                xMin = faces[i].vertices[j].x;

            if (faces[i].vertices[j].x > xMax)
                xMax = faces[i].vertices[j].x;

            if (faces[i].vertices[j].y < yMin)
                yMin = faces[i].vertices[j].y;

            if (faces[i].vertices[j].y > yMax)
                yMax = faces[i].vertices[j].y;

            if (faces[i].vertices[j].z < zMin)
                zMin = faces[i].vertices[j].z;

            if (faces[i].vertices[j].z > zMax)
                zMax = faces[i].vertices[j].z;
        }
    }

    double swell = 0.001;

    // Ensure that the bounding box is not planar:
    if (xMin == xMax)
        xMin += swell;

    if (yMin == yMax)
        yMin += swell;

    if (zMin == zMax)
        zMin += swell;

    // Assemble the 6 sides of a bounding cube using our min/max coords
    Polygon left, right, front, back, top, bottom;

    Vertex frontLeftTop(xMin, yMax, zMin);
    Vertex frontLeftBot(xMin, yMin, zMin);
    Vertex frontRightTop(xMax, yMax, zMin);
    Vertex frontRightBot(xMax, yMin, zMin);

    Vertex backLeftTop(xMin, yMax, zMax);
    Vertex backLeftBot(xMin, yMin, zMax);
    Vertex backRightTop(xMax, yMax, zMax);
    Vertex backRightBot(xMax, yMin, zMax);

    left.addVertex( backLeftTop );
    left.addVertex( backLeftBot);
    left.addVertex( frontLeftBot );
    left.addVertex( frontLeftTop );

    right.addVertex( frontRightTop );
    right.addVertex( frontRightBot );
    right.addVertex( backRightBot );
    right.addVertex( backRightTop );

    front.addVertex( frontLeftTop );
    front.addVertex( frontLeftBot );
    front.addVertex( frontRightBot );
    front.addVertex( frontRightTop );

    back.addVertex( backRightTop );
    back.addVertex( backRightBot );
    back.addVertex( backLeftBot );
    back.addVertex( backLeftTop );

    top.addVertex( backLeftTop );
    top.addVertex( frontLeftTop );
    top.addVertex( frontRightTop );
    top.addVertex( backRightTop );

    bottom.addVertex( frontLeftBot );
    bottom.addVertex( backLeftBot );
    bottom.addVertex( backRightBot );
    bottom.addVertex( frontRightBot );

    // Generate face normals for each face of the bounding box:
    left.faceNormal = left.getFaceNormal();
    right.faceNormal = right.getFaceNormal();
    front.faceNormal = front.getFaceNormal();
    back.faceNormal = back.getFaceNormal();
    top.faceNormal = top.getFaceNormal();
    bottom.faceNormal = bottom.getFaceNormal();

    // Add the faces to the bounding box:
    boundingBoxFaces.emplace_back(left);
    boundingBoxFaces.emplace_back(right);
    boundingBoxFaces.emplace_back(front);
    boundingBoxFaces.emplace_back(back);
    boundingBoxFaces.emplace_back(top);
    boundingBoxFaces.emplace_back(bottom);
}

// Debug this mesh
void Mesh::debug(){

    // Debug bounding boxes:
    cout << "\nMesh bounding box faces:\n------------------\n";
    for (int i = 0; i < boundingBoxFaces.size(); i++){
        boundingBoxFaces[i].debug();
    }

    // Debug meshes:
    cout << "\nMesh visible faces:\n------------------\n";
    for (unsigned int i = 0; i < faces.size(); i++)
        faces[i].debug();
}
