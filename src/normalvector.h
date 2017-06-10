// Vertex normal object
// By Adam Badke

#ifndef NORMALVECTOR_H
#define NORMALVECTOR_H

#include "transformationmatrix.h"

class normalVector
{
public:
    // Default constructor
    normalVector();

    // XYZ Constructor
    normalVector(double newX, double newY, double newZ);

    // Copy Constructor
    normalVector(const normalVector& rhs);

    // Interpolation constructor: Builds an interpolated normal vector based on current between start and end
    normalVector(const normalVector& lhs, double lhsZ, const normalVector& rhs, double rhsZ, double current, double start, double end);

    // Normalize the vector
    void normalize();

    // Get the length of this normal vector
    double length();

    // Get the cross product of this and another vector
    normalVector& crossProduct(const normalVector& rhs);

    // Get the dot product of this and another vector
    double dotProduct(const normalVector& rhs);

    // Overloaded scalar multiplication operator
    normalVector& operator*=(const double rhs);

    // Overloaded assignment operator
    normalVector& operator=(const normalVector& rhs);

    // Overloaded (friend) subtraction operator
    friend normalVector& operator-(const normalVector& lhs, const normalVector& rhs);

    // Determine if this normal is (0, 0, 0)
    bool isZero();

    // Transform the normal by a transformation matrix
    void transform(TransformationMatrix* theMatrix);


    // Normal vector attributes:
    //**************************

    double xn, yn, zn;


    // Debug this object
    void debug();

private:

};

#endif // NORMALVECTOR_H
