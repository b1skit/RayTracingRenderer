// Vectors used for defining points
// By Adam Badke

#ifndef VERTEX_H
#define VERTEX_H

#include "transformationmatrix.h"
#include "math.h"
#include "normalvector.h"

class Vertex{
public:
    // No arg constructor: Sets everything to default values
    Vertex();

    // 3 arg constructor: Sets x, y, z, and uses the default (debug) color
    Vertex(double newX, double newY, double newZ);

    // 4 arg constructor: Sets x, y, z and color
    Vertex(double newX, double newY, double newZ, unsigned int newColor);

    // 4 arg constructor: Sets x, y, z, w
    Vertex(double newX, double newY, double newZ, double newW);

    // 5 arg constructor: Sets x, y, z, color and vertex number. For use by polygon object when inreasing its vertex count
    Vertex(double newX, double newY, double newZ, unsigned int newColor, int vertexNumber);

    // Copy Constructor
    Vertex(const Vertex &currentVertex);

    // Overloaded assignment operator
    Vertex& operator=(const Vertex& rhs);

    // Checks vertices for spacial equality
    // Returns true if these vertices occupy the same relative position in 3D space, false otherwise
    bool operator==(Vertex &otherVertex);

    // Checks vertices for spacial non-equality
    // Returns true if these vertices do NOT occupy the same relative position in 3D space, false otherwise
    bool operator!=(Vertex &otherVertex);

    // Overloaded subtraction operator: Perform componenent-wise subtraction of 2 vertices
    Vertex& operator-(const Vertex& rhs) const;

    // Overloaded addition operator: Perform componenent-wise addition of 2 vertices
    Vertex& operator+(const Vertex& rhs) const;

    // Multiply the components of this vertex by a scalar
    Vertex& operator*(double scale);

    // Perform a dot product: Vertex vector dot product normal vector.
    double dot(normalVector theNormal);

    // Get the vector length of this vertex (ie its distance from the origin)
    double length();

    // Set the vertex number
    void setVertexNumber(int newVertexNumber);

    // Transform the vertex by a transformation matrix
    void transform(TransformationMatrix* theMatrix);

    // Transform the vertex by a transformation matrix, and round the x/y components. Used for transforming to screenspace to prevent rounding errors
    void transform(TransformationMatrix* theMatrix, bool doRound);

    // Update the W component of this vertex
    void setW(double newW);

    // Debug this vertex:
    void debug();

    // Vertex properties:
    double x;
    double y;
    double z;
    unsigned int color;
    unsigned int vertexNumber; // This vertex's index in its containing polygon vertex array

    bool hasColor = false; // Whether or not this vertex has been assigned a custom color, or initialized to a default color. Used to assist the "surface" command.

    // Vertex normal:
    normalVector normal;

//    // Texture coordinates: Not currently used
//    double xt;
//    double yt;
//    double zt;

private:
    static const unsigned int DEFAULT_COLOR = 0xffffffff; // Default color for all vertices, if none is assigned
    double w; // Common divisor

    // // Divide this vector by the W coordinate. Resets the common denomintor as 1. Used when adding perspective transformations
    void divideByW();
};


#endif // VERTEX_H
