// Polygon object
// By Adam Badke

#ifndef POLYGON_H
#define POLYGON_H

#include "vertex.h"
#include "transformationmatrix.h"
#include <vector>
#include "normalvector.h"

using std::vector;

// Shading model enumerator: Used for selecting/controlling an object's shading model
enum ShadingModel{
    ambientOnly = 0,
    flat = 1,
    gouraud = 2,
    phong = 3
};

class Polygon{
public:
    // Constructor
    Polygon();

    // Triangle Constructor
    Polygon(Vertex p0, Vertex p1, Vertex p2);

    // Copy constructor
    Polygon(const Polygon &existingPolygon);

    // Overloaded assignment operator
    Polygon& operator=(const Polygon& rhs);

    // Destructor;
    ~Polygon();

    // Remove all vertices from this polygon's vertice array
    void clearVertices();

    // Add a vertex to the polygon.
    // PreCondition: Vertices are always added in a Counter Clockwise order (vertices[i+1] = CCW, vertices[i-1] = CW
    void addVertex(Vertex newPoint);

    // Get the vertex with the highest y value. Used by the renderer to draw this polygon.
    // Return: The highest vertex in this polygon, in terms of y coordinates
    Vertex* getHighest();

    // Get the vertex with the lowest y value. Used by the renderer to draw this polygon.
    // Return: A pointer to a Vertex object
    Vertex* getLowest();

    // Get the next vertex in the vertex array
    Vertex* getNext(unsigned int currentVertex);

    // Get the previous vertex in the vertex array
    Vertex* getPrev(unsigned int currentVertex);

    // Get the last vertex in the vertex array
    Vertex* getLast();

    // Set all of the vertices in this polygon to a single color
    void setSurfaceColor(unsigned int solidColor);

    // Determine if this polygon's vertices contain more than 1 color
    // Returns true if all vertices have the same color (or if polygon has 0 vertices)
    bool isSolidColor();

    // Determine if this Polygon is correctly initialized.
    // Returns true if the Polygon has at least 2 vertices, false otherwise
    bool isValid();

    // Determine if this Polygon is a line (ie. has exactly 2 vertices)
    bool isLine();

    // Polygon frustum culling: Check if this polygon is in bounds of the view frustum
    bool isInFrustum(double xLow, double xHigh, double yLow, double yHigh);

    // Clip a polygon to the edges of the view plane
    // Note: Algorithm sourced from "Computer Graphics: Principals and Practice" Volume 3
    void clipToScreen(double xLow, double xHigh, double yLow, double yHigh);

    // Clip this polygon to the near/far planes
    void clipHitherYon(double hither, double yon);

    // Determine if this polygon is currently between hither and yon
    bool isInDepth(double hither, double yon);

    // Triangulate this polygon
    // Pre-condition: The polygon has >=4 vertices
    // Return: A mesh containing triangular faces only. Every triangle will contain the first vertex
    vector<Polygon>* getTriangulatedFaces();

    // Check Vertex Winding: Determine if we're looking at the front or the back of the polygon
    bool isFacingCamera();

    // Transform this polygon by a transformation matrix
    void transform(TransformationMatrix* theMatrix);

    // Transform this polygon by a transformation matrix, rounding its values
    void transform(TransformationMatrix* theMatrix, bool doRound);

    // Get a count of the number of vertices contained by this polygon
    int getVertexCount();

    // Check whether this polygon is affected by ambient lighting
    bool isAffectedByAmbientLight();

    // Set this polygon to be affected by ambient lighting
    void setAffectedByAmbientLight(bool newAmbientLit);

    // Update this polygon's vertex colors based on ambient light intensity
    void lightAmbiently(double redIntensity, double greenIntensity, double blueIntensity);

    // Get this polygon's shading model
    ShadingModel getShadingModel();

    // Set this polygon's shading model
    void setShadingModel(ShadingModel newShadingModel);

    // Get this polygon's specular coefficient
    double getSpecularCoefficient();

    // Set this polygon's specular coefficient
    void setSpecularCoefficient(double newSpecCoefficient);

    // Get this polygon's specular exponent
    double getSpecularExponent();

    // Set this polygon's specular exponent
    void setSpecularExponent(double newSpecExponent);

    // Get this polygon's reflectivity
    double getReflectivity();

    // Set this polygon's reflectivity
    void setReflectivity(double newReflectivity);

    // Get the center of this polygon, as a vertex
    Vertex getFaceCenter();

    // Get the (normalized) face normal of this polygon
    // Pre-condition: The polygon is a triangle
    NormalVector getFaceNormal();

    // Get the average of the normals of this polygon
    NormalVector getNormalAverage();


    // Debug this polygon
    void debug();

    // Public polygon attributes:
    //***************************

    Vertex* vertices = nullptr; // An array of points that describe this polygon

    NormalVector faceNormal;    // The pre-computed face normal of this polygon

private:
    unsigned int vertexArraySize; // Size of the vertex array in this polygon
    unsigned int currentVertices; // The number of vertices actually added to this polygon

    bool isAmbientLit; // Ambient lighting

    ShadingModel theShadingModel; // The shading model to be used for this polygon
    double specularCoefficient = 0.3;
    double specularExponent = 8;
    double reflectivity = 0.5;


    // Check if a vertex is in the positive half space of a plane. Used to clip polygons.
    bool inside(Vertex V, Vertex P, NormalVector n);

    // Calculate a vector intersection with a plane. Used to clip polygons.
    Vertex intersection(Vertex C, Vertex D, Vertex P, NormalVector n, bool doPerspectiveCorrect);

    // Helper function: Clips polygons using Sutherland-Hodgman 2D clipping algorithm
    Polygon clipHelper(Polygon source, Vertex P, NormalVector n, bool doPerspectiveCorrect);
};

#endif // POLYGON_H
