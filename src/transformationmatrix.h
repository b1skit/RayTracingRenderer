// Transformation Matrix object
// By Adam Badke

#ifndef TRANSFORMATIONMATRIX_H
#define TRANSFORMATIONMATRIX_H

// Axis enumerator
enum Axis {
    X = 0,
    Y = 1,
    Z = 2
};

class TransformationMatrix
{
public:
    // Constructor
    TransformationMatrix();

    // Copy Constructor
    TransformationMatrix(const TransformationMatrix& existingMatrix);

    // Overloaded assignment "=" operator
    TransformationMatrix& operator=(const TransformationMatrix& rhs);

    // Destructor
    ~TransformationMatrix();

    // Multiply this matrix by a scalar
    void addScaleUniform(double scalar);

    // Add non-uniform scale to this matrix
    void addNonUniformScale(double x, double y, double z);

    // Add a translation to this matrix in (x, y, z)
    void addTranslation(double x, double y, double z);

    // Add a rotation to this matrix
    void addRotation(Axis theAxis, double angle);

    // Overloaded *= operator
    TransformationMatrix& operator*=(const TransformationMatrix& rhs);

    // Get the size of this matrix
    int size();

    // Access the array values
    double& arrayVal(int x, int y);

    // Get inverse: Calculate this Matrix's inverse, and return int
    TransformationMatrix getInverse();


    // Debug this matrix:
    void debug();

private:
    // Matrix properties
    static const int DIMENSION = 4;

    double** CTM; // The transformation matrix array


    // Calculate the determinant of this matrix: Invokes the recursive helper function
    double getDeterminant();

    // Recursive helper function: Get the determinant of a matrix
    double getDeterminantRecursive(double** theMatrix, int theDimension);

    // Make a minor matrix from the current matrix
    double** makeMinor(double** theMatrix, int currentDimension, int row, int col);
};

// Non-member functions:
//************************

// Overloaded, non-member matrix multiplication "*" operator
TransformationMatrix operator*(TransformationMatrix& lhs, TransformationMatrix& rhs);

#endif // TRANSFORMATIONMATRIX_H
