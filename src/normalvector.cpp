// Vertex normal object
// By Adam Badke

#include "normalvector.h"

#include "renderutilities.h"
#include "math.h"               // The STL math library
#include <iostream>

using std::cout;

// Default constructor
NormalVector::NormalVector(){
    xn = 0;
    yn = 0;
    zn = 0;
}

// XYZ Constructor
NormalVector::NormalVector(double newX, double newY, double newZ){
    xn = newX;
    yn = newY;
    zn = newZ;
}

// Copy Constructor
NormalVector::NormalVector(const NormalVector& rhs){
    this->xn = rhs.xn;
    this->yn = rhs.yn;
    this->zn = rhs.zn;
}

// Interpolation constructor: Builds an interpolated normal vector based on current position between start and end positions
NormalVector::NormalVector(const NormalVector& lhs, double lhsZ, const NormalVector& rhs, double rhsZ, double current, double start, double end){

    double ratio;
    if (end - start == 0)
        ratio = 0;
    else
        ratio = (current - start) / (double)(end - start);

    this->xn = getPerspCorrectLerpValue(lhs.xn, lhsZ, rhs.xn, rhsZ, ratio);
    this->yn = getPerspCorrectLerpValue(lhs.yn, lhsZ, rhs.yn, rhsZ, ratio);
    this->zn = getPerspCorrectLerpValue(lhs.zn, lhsZ, rhs.zn, rhsZ, ratio);

    this->normalize();

}

// Overloaded assignment operator
NormalVector& NormalVector::operator=(const NormalVector& rhs){
    if (this == &rhs)
        return *this;

    this->xn = rhs.xn;
    this->yn = rhs.yn;
    this->zn = rhs.zn;

    return *this;
}

// Normalize the vector
void NormalVector::normalize(){
    // Calcualte the inverse of the length of the vector
    double inverseLength = 1 / length();

    // Normalize:
    xn *= inverseLength;
    yn *= inverseLength;
    zn *= inverseLength;
}

// Get the length of this normal vector
double NormalVector::length(){
    return sqrt( (xn * xn) + (yn * yn) + (zn * zn));
}

// Get the cross product of this and another vector
NormalVector NormalVector::crossProduct(const NormalVector& rhs){
    return NormalVector( // Note: We reverse the "standard" cross product order here, in order to reverse the resulting vector so that it works with our left handed coordinate system
                ( (this->zn * rhs.yn) - (this->yn * rhs.zn) ),
                ( (this->xn * rhs.zn) - (this->zn * rhs.xn) ),
                ( (this->yn * rhs.xn) - (this->xn * rhs.yn) )
                );
}

// Get the dot product of this and another vector
double NormalVector::dotProduct(const NormalVector& rhs){
    double result = 0;
    result += this->xn * rhs.xn;
    result += this->yn * rhs.yn;
    result += this->zn * rhs.zn;

    return result;
}

// Overloaded scalar multiplication operator
NormalVector& NormalVector::operator*=(const double rhs){

    this->xn *= rhs;
    this->yn *= rhs;
    this->zn *= rhs;

    return *this;
}

// Overloaded scalar multiplication operator
NormalVector NormalVector::operator*(double scalar){
    NormalVector result(*this);

    result.xn = this->xn * scalar;
    result.yn = this->yn * scalar;
    result.zn = this->zn * scalar;

    return result;
}

// Overloaded subtraction operator
NormalVector NormalVector::operator-(const NormalVector& rhs){
    NormalVector newNormal(*this);
    newNormal.xn -= rhs.xn;
    newNormal.yn -= rhs.yn;
    newNormal.zn -= rhs.zn;

    return newNormal;
}

// Overloaded -= operator
NormalVector& NormalVector::operator-=(const NormalVector& rhs){
    this->xn -= rhs.xn;
    this->yn -= rhs.yn;
    this->zn -= rhs.zn;

    return *this;
}

// Determine if this normal is (0, 0, 0)
bool NormalVector::isZero(){
    return (xn == 0 && yn == 0 && zn == 0);
}

// Transform the normal by a transformation matrix
void NormalVector::transform(TransformationMatrix* theMatrix){

    // Copy our coordinate into an array (4-vector), to simplify our calculations
    double coords[4];
    coords[0] = xn;
    coords[1] = yn;
    coords[2] = zn;

    // Create an array (4-vector) to hold our results
    double result[3];
    for (int i = 0; i < 3; i++) // Can do this once each row loop below?
        result[i] = 0;

    // Multiply the transformation matrix and the coorinate 4-vector array
    for (int row = 0; row < 3; row++){
        for (int col = 0; col < 3; col++){
            result[row] += (theMatrix->arrayVal(row, col) * coords[col]); // Multiply: [xform]*[x, y, z]
        }
    }

    xn = result[0];
    yn = result[1];
    zn = result[2];

    // Re-normalize
    normalize();
}

// Reverse the direction of this vector
void NormalVector::reverse(){
    xn *= -1;
    yn *= -1;
    zn *= -1;
}

// Debug this object
void NormalVector::debug(){
    cout << "normal: (" << xn << ", " << yn << ", " << zn << ")\n";
}
