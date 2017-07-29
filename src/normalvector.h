// Vertex normal object
// By Adam Badke

#ifndef NORMALVECTOR_H
#define NORMALVECTOR_H

#include "transformationmatrix.h"

class NormalVector
{
public:
    // Default constructor
    NormalVector();

    // XYZ Constructor
    NormalVector(double newX, double newY, double newZ);

    // Copy Constructor
    NormalVector(const NormalVector& rhs);

    // Interpolation constructor: Builds an interpolated normal vector based on current between start and end
    NormalVector(const NormalVector& lhs, double lhsZ, const NormalVector& rhs, double rhsZ, double current, double start, double end);

    // Overloaded assignment operator
    NormalVector& operator=(const NormalVector& rhs);

    // Normalize the vector
    void normalize();

    // Get the length of this normal vector
    double length();

    // Get the cross product of this and another vector
    NormalVector crossProduct(const NormalVector& rhs);

    // Get the dot product of this and another vector
    double dotProduct(const NormalVector& rhs);

    // Overloaded scalar multiplication operator
    NormalVector& operator*=(const double rhs);

    // Overloaded scalar multiplication operator
    NormalVector operator*(double scalar);

    // Overloaded subtraction operator
    NormalVector operator-(const NormalVector& rhs);

    // Overloaded -= operator
    NormalVector& operator-=(const NormalVector& rhs);

    // Determine if this normal is (0, 0, 0)
    bool isZero();

    // Transform the normal by a transformation matrix
    void transform(TransformationMatrix* theMatrix);

    // Reverse the direction of this vector
    void reverse();


    // Normal vector attributes:
    //**************************

    double xn = 0; // Initialize to zero, to prevent nan issues
    double yn = 0;
    double zn = 0;


    // Debug this object
    void debug();
};

#endif // NORMALVECTOR_H
