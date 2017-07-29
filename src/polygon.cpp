// Polygon object
// By Adam Badke

#include <iostream>
#include "polygon.h"
#include "renderutilities.h"
#include <vector>
#include "normalvector.h"
#include <cmath>

using std::vector;
using std::cout;

// Constructor
Polygon::Polygon(){ //
    vertexArraySize = 3; // Allocate 3 vertices (for a triangle)

    vertices = new Vertex[vertexArraySize];
    currentVertices = 0;

    isAmbientLit = false;

    // Set the shading model to ambient only, by default:
    theShadingModel = ambientOnly;
}

// Triangle Constructor
Polygon::Polygon(Vertex p0, Vertex p1, Vertex p2){
    vertexArraySize = 3; // Allocate 3 vertices (for a triangle)

    vertices = new Vertex[vertexArraySize];
    vertices[0] = Vertex{p0.x, p0.y, p0.z, p0.color};
    vertices[1] = Vertex{p1.x, p1.y, p1.z, p1.color};
    vertices[2] = Vertex{p2.x, p2.y, p2.z, p2.color};

    for (unsigned int i = 0; i < vertexArraySize; i++) // Record the vertex numbers
        vertices[i].vertexNumber = i;

    currentVertices = 3;

    isAmbientLit = false;

    // Set the shading model to ambient only, by default:
    theShadingModel = ambientOnly;

    faceNormal = this->getFaceNormal();
}

// Copy constructor
Polygon::Polygon(const Polygon& currentPoly){
    this->vertexArraySize = currentPoly.vertexArraySize;
    this->currentVertices = currentPoly.currentVertices;

    this->isAmbientLit = currentPoly.isAmbientLit;

    this->theShadingModel = currentPoly.theShadingModel;
    this->specularCoefficient = currentPoly.specularCoefficient;
    this->specularExponent = currentPoly.specularExponent;
    this->reflectivity = currentPoly.reflectivity;

    this->vertices = new Vertex[vertexArraySize];
    for (unsigned int i = 0; i < vertexArraySize; i++){
        this->vertices[i] = currentPoly.vertices[i];
    }

    this->faceNormal = currentPoly.faceNormal;
}

// Overloaded assignment operator
Polygon& Polygon::operator=(const Polygon& rhs){
    if (this == &rhs)
        return *this;

    this->vertexArraySize = rhs.vertexArraySize;
    this->currentVertices = rhs.currentVertices;

    this->isAmbientLit = rhs.isAmbientLit;

    this->theShadingModel = rhs.theShadingModel;
    this->specularCoefficient = rhs.specularCoefficient;
    this->specularExponent = rhs.specularExponent;
    this->reflectivity = rhs.reflectivity;

    if (vertices != nullptr)
        delete[] vertices;

    this->vertices = new Vertex[vertexArraySize];
    for (unsigned int i = 0; i < currentVertices; i++)
        this->vertices[i] = rhs.vertices[i];

    this->faceNormal = rhs.faceNormal;

    return *this;
}

// Destructor;
Polygon::~Polygon(){
    // Handle polygons with no vertices
    if (vertices == nullptr)
        return;

    delete[] vertices;
}

// Remove all vertices from this polygon's vertice array
void Polygon::clearVertices(){
    if (vertices == nullptr)
        return;

    delete[] vertices;

    vertexArraySize = 3; // Allocate 3 vertices (for a triangle)

    vertices = new Vertex[vertexArraySize];
    currentVertices = 0;
}

// Add a vertex to the polygon.
// PreCondition: Vertices are always added in a Counter Clockwise order (vertices[i+1] = CCW, vertices[i-1] = CW
void Polygon::addVertex(Vertex newPoint){
    // If the vertex array still has space left:
    if (currentVertices < vertexArraySize){

        vertices[currentVertices] = Vertex(newPoint.x, newPoint.y, newPoint.z, newPoint.color, currentVertices);
        vertices[currentVertices].normal = newPoint.normal;

        currentVertices++;

        return; // We're done!
    }
    else{
        // Allocate a new array, then copy the existing vertices into it
        Vertex* newVertices = new Vertex[vertexArraySize + 1];
        for (unsigned int i = 0; i < vertexArraySize; i++){
            newVertices[i] = vertices[i];
        }
        // Deallocate the old array now that we've stored the values elsewhere
        delete [] vertices;

        newPoint.vertexNumber = vertexArraySize;
        newVertices[vertexArraySize] = Vertex(newPoint);

        vertexArraySize++;
        currentVertices++;

        vertices = newVertices;
    }

    return;
}

// Get the vertex with the highest y value. Used by the renderer to draw this polygon.
// Return: The highest vertex in this polygon, in terms of y coordinates
// Precondition: It is assumed this polygon has at least 3 valid points.
Vertex* Polygon::getHighest(){
    // Find the vertex with the greatest y value
    double highestY = vertices[0].y;
    int highestVertex = 0;
    for (unsigned int i = 1; i < currentVertices; i++){
        if (vertices[i].y > highestY){
            highestY = vertices[i].y;
            highestVertex = i;
        }
    }
    return &vertices[highestVertex];
}

// Get the vertex with the lowest y value. Used by the renderer to draw this polygon.
// Return: The lowest vertex, int terms of y coordinates
// Precondition: It is assumed this polygon has at least 3 valid points.
Vertex* Polygon::getLowest(){
    // Find the vertex with the lowest y value
    double lowestY = vertices[0].y;
    int lowestVertex = 0;
    for (unsigned int i = 0; i < currentVertices; i++)
        if (vertices[i].y < lowestY){
            lowestY = vertices[i].y;
            lowestVertex = i;
        }
    return &vertices[lowestVertex];
}

// Get the next vertex in the vertex array
Vertex* Polygon::getNext(unsigned int currentVertex){
    if (currentVertex >=  vertexArraySize - 1)
        return &vertices[0];
    else
        return &vertices[currentVertex + 1];
}

// Get the previous vertex in the vertex array
Vertex* Polygon::getPrev(unsigned int currentVertex){
    if (currentVertex <= 0)
        return &vertices[vertexArraySize - 1];
    else
        return &vertices[currentVertex - 1];
}

// Get the last vertex in the vertex array
Vertex* Polygon::getLast(){
    return &vertices[currentVertices - 1];
}

// Set all of the vertices in this polygon to a single color
void Polygon::setSurfaceColor(unsigned int solidColor){
    for (unsigned int i = 0; i < currentVertices; i++)
            vertices[i].color = solidColor;
}

// Determine if this polygon's vertices contain more than 1 color
// Returns true if all vertices have the same color (or if polygon has 0 vertices)
bool Polygon::isSolidColor(){
    // Loop, checking each vertex:
    if (currentVertices > 0){
        for (unsigned int i = 1; i < currentVertices; i++){ // Compare each vertex to make sure it matches vertex 0
            if (vertices[i].color != vertices[0].color){
                return false;
            }
        }
    }
    return true;
}

// Determine if this Polygon is correctly initialized.
// Returns true if the Polygon has at least 2 vertices, false otherwise
bool Polygon::isValid(){
    return currentVertices >= 2;
}

// Determine if this Polygon is a line (ie. has exactly 2 vertices)
bool Polygon::isLine(){
    return (currentVertices == 2);
}

// Determine if this polygon is currently between hither and yon
bool Polygon::isInDepth(double hither, double yon){
    bool beforeHither = false; // Check if the polygon spans from before the near plane, out past the far plane
    bool afterYon = false;

    // Loop through each vertex, checking if any of them are in range
    for (unsigned int i = 0; i < currentVertices; i++){
        if (vertices[i].z >= hither && vertices[i].z <= yon){
            return true;
        }
        // Check if a polygon has an edge stretched between from before hither to after yon
        if (vertices[i].z < hither)
            beforeHither = true;
        if (vertices[i].z > yon)
            afterYon = true;
    }

    return beforeHither && afterYon;
}

// Polygon frustum culling: Check if this polygon is in bounds of the view frustum
bool Polygon::isInFrustum(double xLow, double xHigh, double yLow, double yHigh){

    // If all vertices are in the negative half-spaces of xMin/xMax/yMin/yMax, the polygon should be culled:
    bool belowXLow = true;
    bool aboveXHigh = true;
    bool belowYLow = true;
    bool aboveYHigh = true;
    // Check  each vertex:
    for (unsigned int i = 0; i < currentVertices; i++){
        if (vertices[i].x > xLow){
            belowXLow = false;
        }
        if (vertices[i].x < xHigh){
            aboveXHigh = false;
        }
        if (vertices[i].y > yLow){
            belowYLow = false;
        }
        if (vertices[i].y < yHigh){
            aboveYHigh = false;
        }
    }

    // If any of the booleans are still true, all vertices must be in that negative half space: Return false.
    return (!(belowXLow || aboveXHigh || belowYLow || aboveYHigh));
}

// Clip a polygon to the near/far planes
// Note: Algorithm modified from "Computer Graphics: Principals and Practice" Volume 3
void Polygon::clipHitherYon(double hither, double yon){
    Vertex hitherPlane(0, 0, hither);
    NormalVector hitherNormal(0, 0, 1);

    *this = clipHelper(*this, hitherPlane, hitherNormal, false);

    Vertex yonPlane(0, 0, yon);
    NormalVector yonNormal(0, 0, -1);

    if (this->currentVertices > 1) // Only try to clip if we've got at least 2 vertices left
        *this = clipHelper(*this, yonPlane, yonNormal, false);

    return;
}

// Clip a polygon to the view frustum
void Polygon::clipToScreen(double xLow, double xHigh, double yLow, double yHigh){

    // Create some vertices to define the clipping plane boundary:
    Vertex boundaryPlane[4];
    boundaryPlane[0] = Vertex (xLow, yHigh, 0);     // topLeft
    boundaryPlane[1] = Vertex (xLow, yLow, 0);      // botLeft
    boundaryPlane[2] = Vertex (xHigh, yLow, 0);     // botRight
    boundaryPlane[3] = Vertex (xHigh, yHigh, 0);    // topRight

    // Clip to each edge of the view plane, in turn
    for (int i = 0; i < 3; i++){
        if (this->currentVertices > 2)
            *this = clipHelper(*this, boundaryPlane[i], NormalVector(boundaryPlane[i].y - boundaryPlane[i + 1].y, boundaryPlane[i + 1].x - boundaryPlane[i].x, 0 ), true );

    }
    if (this->currentVertices > 2)
        *this = clipHelper(*this, boundaryPlane[3], NormalVector(boundaryPlane[3].y - boundaryPlane[0].y, boundaryPlane[0].x - boundaryPlane[3].x, 0 ), true );
}

// Helper function: Clips polygons using Sutherland-Hodgman 2D clipping algorithm
Polygon Polygon::clipHelper(Polygon source, Vertex planePoint, NormalVector planeNormal, bool doPerspectiveCorrect){

    // Create a result Polygon, and copy the source's key attributes
    Polygon result;
    result.isAmbientLit = source.isAmbientLit;

    result.theShadingModel = source.theShadingModel;
    result.specularCoefficient = source.specularCoefficient;
    result.specularExponent = source.specularExponent;
    result.reflectivity = source.reflectivity;

    result.faceNormal = source.faceNormal;

    bool dontAddLast = false; // Flag: Prevent adding extra vertices at the end
    int last = source.getVertexCount() - 1; // Last index: Calculated once here

    Vertex D = *source.getLast(); // Start at the last vertex
    bool Din = inside(D, planePoint, planeNormal);
    if (Din){
        result.addVertex(D);

        dontAddLast = true; // If  we've already added the last vertex, don't add it again
    }

    for (int i = 0; i < source.getVertexCount(); i++){
        Vertex C = D;
        bool Cin = Din;

        D = source.vertices[i];
        Din = inside(D, planePoint, planeNormal);

        if (Din != Cin)
            result.addVertex(intersection(C, D, planePoint, planeNormal, doPerspectiveCorrect) );

        if (Din && (i != last || dontAddLast == false)){
            result.addVertex(D);
        }
    }

    return result;
}

// Check if a vertex is in the positive half space of a plane. Used to clip polygons.
bool Polygon::inside(Vertex theVertex, Vertex thePlane, NormalVector planeNormal){
    return (theVertex - thePlane).dot(planeNormal) >= 0; // Compare vector pointing from plane towards point against the face normal
}

// Calculate a vector intersection with a plane. Used to clip polygons.
Vertex Polygon::intersection(Vertex prevVertex, Vertex currentVertex, Vertex planePoint, NormalVector planeNormal, bool doPerspectiveCorrect){

    // Vector from current to previous vertex
    Vertex distance = currentVertex - prevVertex;

    // Calculate the ratio of the cosine angles between the line and the plane
    double ratio = (planePoint - prevVertex).dot(planeNormal)/(double)((distance).dot(planeNormal));

    Vertex intersectionPoint = prevVertex + (distance * ratio);

    // Handle side frustum: Use the perspective correct Z value for the point of intersection
    if (doPerspectiveCorrect)
        intersectionPoint.z = getPerspCorrectLerpValue(prevVertex.z, prevVertex.z, currentVertex.z, currentVertex.z, ratio);

    // Interpolate the color:
    intersectionPoint.color = addColors( multiplyColorChannels(prevVertex.color, 1 - ratio), multiplyColorChannels(currentVertex.color, ratio) );

    // Interpolate the normal:
    intersectionPoint.normal.xn = (prevVertex.normal.xn * (1 - ratio)) + (currentVertex.normal.xn * ratio);
    intersectionPoint.normal.yn = (prevVertex.normal.yn * (1 - ratio)) + (currentVertex.normal.yn * ratio);
    intersectionPoint.normal.zn = (prevVertex.normal.zn * (1 - ratio)) + (currentVertex.normal.zn * ratio);

    // Normalize the interpolated normal
    intersectionPoint.normal.normalize();

    return intersectionPoint;
}

// Check vertex winding: Determine if we're looking at the front or the back of the polygon
bool Polygon::isFacingCamera(){

    //  Assumes Polygon is only visible if the vertices are counter clockwise with relation to the RASTER
    double sum = 0;
    for (unsigned int i = 0; i < currentVertices - 1; i++){
        sum += (vertices[i + 1].x - vertices[i].x) * (vertices[i + 1].y + vertices[i].y);
    }
    sum += (vertices[0].x - vertices[currentVertices - 1].x) * (vertices[0].y + vertices[currentVertices - 1].y);

    if (sum > 0){ // Assuming CCW vertex winding is relative to cartesian plane (and not the raster)
        return false;
    }

    return true;
}

// Transform this polygon by a transformation matrix
void Polygon::transform(TransformationMatrix* theMatrix){
    transform(theMatrix, false);
}

// Transform this polygon by a transformation matrix, rounding its values
void Polygon::transform(TransformationMatrix* theMatrix, bool doRound){
    // Loop through each Vertex, instructing it to transform itself
    for (unsigned int i = 0; i < currentVertices; i++){
        vertices[i].transform(theMatrix, doRound);
    }

    // Transform the face normal
    if (!doRound)
        faceNormal.transform(theMatrix);
}

// Get a count of the number of vertices contained by this polygon
int Polygon::getVertexCount(){
    return currentVertices;
}

// Triangulate this polygon
// Pre-condition: The polygon has >=4 vertices
// Return: A mesh containing triangular faces only. Every triangle will contain the first vertex
vector<Polygon>* Polygon::getTriangulatedFaces(){
    vector<Polygon>* result = new vector<Polygon>(); // Create an empty mesh object to contain our resulting faces

    // Handle polygons with 3 or less vertices: Return the whole polygon
    if (currentVertices < 4){
        result->emplace_back( *this );
        return result;
    }

    // Create a common vertex
    Vertex v1 = vertices[0];

    int index = 1;
    int lastIndex = currentVertices - 1;
    while (index < lastIndex){
        Polygon newFace(*this); // Copy the existing polygon to copy its essential drawing attributes
        newFace.clearVertices();    // Remove the vertices from the copy

        newFace.addVertex(v1); // Add the common vertex

        newFace.addVertex(vertices[ index ]);
        newFace.addVertex(vertices[ index + 1 ]);
        index++;

        result->emplace_back(newFace); // Add the new face
    }

    return result;
}

// Check whether this polygon is affected by ambient lighting
bool Polygon::isAffectedByAmbientLight(){
    return this->isAmbientLit;
}

// Update this polygon's vertex colors based on ambient light intensity
void Polygon::lightAmbiently(double redIntensity, double greenIntensity, double blueIntensity){
    for (unsigned int i = 0; i < currentVertices; i++){
        vertices[i].color = multiplyColorChannels(vertices[i].color, 1, redIntensity, greenIntensity, blueIntensity);
    }
}

// Set this polygon to be affected by ambient lighting
void Polygon::setAffectedByAmbientLight(bool newAmbientLit){
    isAmbientLit = newAmbientLit;
}

// Get this polygon's shading model
ShadingModel Polygon::getShadingModel(){
    return theShadingModel;
}

// Set this polygon's shading model
void Polygon::setShadingModel(ShadingModel newShadingModel){
    theShadingModel = newShadingModel;
}

// Get this polygon's specular coefficient
double Polygon::getSpecularCoefficient(){
    return specularCoefficient;
}

// Set this polygon's specular coefficient
void Polygon::setSpecularCoefficient(double newSpecCoefficient){
    specularCoefficient = newSpecCoefficient;
}

// Get this polygon's specular exponent
double Polygon::getSpecularExponent(){
    return specularExponent;
}

// Set this polygon's specular exponent
void Polygon::setSpecularExponent(double newSpecExponent){
    specularExponent = newSpecExponent;
}

// Get this polygon's reflectivity
double Polygon::getReflectivity(){
    return reflectivity;
}

// Set this polygon's reflectivity
void Polygon::setReflectivity(double newReflectivity){
    reflectivity = newReflectivity;
}

// Get the center of this polygon, as a vertex
Vertex Polygon::getFaceCenter(){
    double xTotal = 0;
    double yTotal = 0;
    double zTotal = 0;
    for (unsigned int i = 0; i < currentVertices; i++){
        xTotal += vertices[i].x;
        yTotal += vertices[i].y;
        zTotal += vertices[i].z;
    }
    // Return a vertex built from the average of the values:
    return Vertex(xTotal/currentVertices, yTotal/currentVertices, zTotal/currentVertices);
}

// Get the (normalized) face normal of this polygon
// Pre-condition: The polygon is a triangle (ie. vertex_i != vertex_j)
NormalVector Polygon::getFaceNormal(){

    // Calculate vectors originating at vertex 0, and pointing towards vertices 1 & 2
    NormalVector lhs(vertices[1].x - vertices[0].x, vertices[1].y - vertices[0].y, vertices[1].z - vertices[0].z);  // 0 to 1
    NormalVector rhs(vertices[2].x - vertices[0].x, vertices[2].y - vertices[0].y, vertices[2].z - vertices[0].z);  // 0 to 2

    // Perform a cross product, and normalize the result:
    lhs = lhs.crossProduct(rhs);
    lhs.normalize();

    return lhs;
}

// Get the average of the normals of this polygon
NormalVector Polygon::getNormalAverage(){
    NormalVector average;

    for (unsigned int i = 0; i < currentVertices; i++){
        average.xn += vertices[i].normal.xn;
        average.yn += vertices[i].normal.yn;
        average.zn += vertices[i].normal.zn;
    }

    // Average the normals:
    average.xn /= currentVertices;
    average.yn /= currentVertices;
    average.zn /= currentVertices;

    // Normalize the average:
    average.normalize();

    return average;
}

// Debug this polygon
void Polygon::debug(){

    cout << "Polygon:\n";
    for (unsigned int i = 0; i < currentVertices; i++){
        vertices[i].debug();
    }
    cout << "Face ";
    faceNormal.debug();
    cout << "End Polygon.\n\n";
}

