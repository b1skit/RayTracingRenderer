// Vectors used for defining points
// By Adam Badke

#include "vertex.h"
#include "transformationmatrix.h"
#include "renderutilities.h"
#include "normalvector.h"

#include <iostream>

using std::cout;

// No arg constructor: Sets everything to default values
Vertex::Vertex(){
    x = 0;
    y = 0;
    z = 0;
    color = DEFAULT_COLOR;
    w = 1;
}

// 3 arg constructor: Sets x, y, z: Assigns the default (debug) color
Vertex::Vertex(double newX, double newY, double newZ){
    x = newX;
    y = newY;
    z = newZ;
    color = DEFAULT_COLOR;

    w = 1; // Set the common divisor to 1;
}

// 4 arg constructor: Sets x, y, z and color
Vertex::Vertex(double newX, double newY, double newZ, unsigned int newColor){
    x = newX;
    y = newY;
    z = newZ;
    this->color = newColor;

    w = 1; // Set the common divisor to 1;
}

// 4 arg constructor: Sets x, y, z, w
Vertex::Vertex(double newX, double newY, double newZ, double newW){
    x = newX;
    y = newY;
    z = newZ;
    w = newW;
    color = DEFAULT_COLOR;

    if (w != 0) // Handle vertices initialized with w != 0
        divideByW();
}

// 5 arg constructor: Sets x, y, z, color and vertex number. For use by polygon object (only)
Vertex::Vertex(double newX, double newY, double newZ, unsigned int newColor, int newVertexNumber){
    x = newX;
    y = newY;
    z = newZ;
    color = newColor;
    vertexNumber = newVertexNumber;

    w = 1; // Set the common divisor to 1;
}

// Copy Constructor
Vertex::Vertex(const Vertex &currentVertex){
    this->x = currentVertex.x;
    this->y = currentVertex.y;
    this->z = currentVertex.z;
    this->color = currentVertex.color;
    this->vertexNumber = currentVertex.vertexNumber;
    this->w = currentVertex.w;

    // Vertex normals:
    this->normal = currentVertex.normal;
}

// Overloaded assignment operator
Vertex& Vertex::operator=(const Vertex& rhs){
    if (this == &rhs)
        return *this;

    this->x = rhs.x;
    this->y = rhs.y;
    this->z = rhs.z;
    this->color = rhs.color;
    this->vertexNumber = rhs.vertexNumber;
    this->w = rhs.w;

    // Vertex normals:
    this->normal = rhs.normal;

    return *this;
}

// Check vertices for spacial equality:
bool Vertex::operator==(Vertex &otherVertex){
    return (   this->x/this->w == otherVertex.x/otherVertex.w
            && this->y/this->w == otherVertex.y/otherVertex.w
            && this->z/this->w == otherVertex.z/otherVertex.w
            );
}

// Checks vertices for spacial non-equality
// Returns true if these vertices do NOT occupy the same relative position in 3D space, false otherwise
bool Vertex::operator!=(Vertex &otherVertex){
    return !(  this->x/this->w == otherVertex.x/otherVertex.w
            && this->y/this->w == otherVertex.y/otherVertex.w
            && this->z/this->w == otherVertex.z/otherVertex.w
            );
}

// Overloaded subtraction operator: Perform componenent-wise subtraction: LHS(x, y, z) - RHS(x, y, z). All other attributes maintained from LHS. LHS and RHS remain unchanged
Vertex Vertex::operator-(const Vertex& rhs) const {

    Vertex result(*this); // Copy the LHS
    result.x -= rhs.x; // Perform the component-wise subtraction
    result.y -= rhs.y;
    result.z -= rhs.z;

    return result;
}

// Overloaded compound addition operator: Perform componenent-wise addition of a vertex and a normal
Vertex& Vertex::operator+=(const NormalVector& rhs){
    this->x += rhs.xn;
    this->y += rhs.yn;
    this->z += rhs.zn;

    return *this;
}

// Overloaded addition operator: Perform componenent-wise addition of 2 vertices
Vertex Vertex::operator+(const Vertex& rhs) const{
    Vertex result(*this); // Copy the LHS
    result.x += rhs.x; // Perform the component-wise addition
    result.y += rhs.y;
    result.z += rhs.z;

    return result;
}

// Overloaded addition operator: Perform componenent-wise addition of 2 vertices
Vertex Vertex::operator+(const NormalVector& rhs) const{
    Vertex result = (*this);
    result.x += rhs.xn;
    result.y += rhs.yn;
    result.z += rhs.zn;

    return result;
}

// Multiply the components of this vertex by a scalar
Vertex Vertex::operator*(double scale){
    Vertex result(*this);

    result.x *= scale;
    result.y *= scale;
    result.z *= scale;

    return result;
}

// Perform a dot product: Vertex vector dot normal vector.
double Vertex::dot(NormalVector theNormal){
    double result = 0;
    result += this->x * theNormal.xn;
    result += this->y * theNormal.yn;
    result += this->z * theNormal.zn;

    return result;
}

// Get the vector length of this vertex (ie its distance from the origin)
double Vertex::length(){
    return sqrt( (x * x) + (y * y) + (z * z) );
}

// Transform the vertex by a transformation matrix
void Vertex::transform(TransformationMatrix* theMatrix){
    transform(theMatrix, false);
}

// Transform the vertex by a transformation matrix, and round the x/y components. Used for transforming to screenspace to prevent rounding errors
void Vertex::transform(TransformationMatrix* theMatrix, bool doRound){

    // Copy our coordinate into an array (4-vector), to simplify our calculations
    double coords[4];
    coords[0] = x;
    coords[1] = y;
    coords[2] = z;
    coords[3] = w;

    // Create an array (4-vector) to hold our results
    double result[4];
    for (int i = 0; i < 4; i++) // Can do this once each row loop below?
        result[i] = 0;

    // Multiply the transformation matrix and the coorinate 4-vector array
    for (int row = 0; row < theMatrix->size(); row++){
        for (int col = 0; col < theMatrix->size(); col++){
            result[row] += (theMatrix->arrayVal(row, col) * coords[col]); // Multiply: [xform]*[x, y, z]
        }
    }

    // Store the transformed values
    if (doRound){ // Round x & y coords -> Used when transforming to screen space
        x = round(result[0]);
        y = round(result[1]);
    }
    else{
        x = result[0];
        y = result[1];
    }
    z = result[2];
    w = result[3];

    // Divide by W, if neccessary
    if (w != 1)
        divideByW();

    // If we're NOT transforming to screen space, transform the normal:
    if (!doRound){
        // Transform the normal:
        normal.transform(theMatrix);
    }
}

// // Divide this vector by the W coordinate. Resets the common denomintor as 1. Used when adding perspective transformations
void Vertex::divideByW(){
    if (w != 1 && w != 0){ // Ensure W != 0.
        x = x/w;
        y = y/w; // We don't divide Z because we want to maintain its information for depth buffering
    }
    w = 1;
}

// Update the W component of this vertex
void Vertex::setW(double newW){
    w = newW;
    if (w != 1)
        divideByW();
}

// Debug this vertex:
void Vertex::debug(){
    cout << "Vertex: (" << x << ", " << y << ", " << z << ", " << w << ") color: " << std::hex << color << std::dec << " R: " << extractColorChannel(color, 1) << " G: " << extractColorChannel(color, 2) << " B: " << extractColorChannel(color, 3) << "\n";
    normal.debug();
}
