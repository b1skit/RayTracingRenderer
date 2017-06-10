// Transformation Matrix object
// By Adam Badke

#include "transformationmatrix.h"
#include <iostream>


#define _USE_MATH_DEFINES       // Allow use of M_PI (3.14159265358979323846)
#include "math.h"

using std::cout;

// Constructor
TransformationMatrix::TransformationMatrix(){
    CTM = new double*[DIMENSION];
    // Initialize the CTM to the identity matrix:
    for (int row = 0; row < DIMENSION; row++){
        CTM[row] = new double[DIMENSION];
        for (int col = 0; col < DIMENSION; col++){
            if (row == col)
                CTM[row][col] = 1;
            else
                CTM[row][col] = 0;
        }
    }
}

// Copy Constructor
TransformationMatrix::TransformationMatrix(const TransformationMatrix& existingMatrix){
    CTM = new double*[DIMENSION];
    // Initialize the CTM to the identity matrix:
    for (int row = 0; row < DIMENSION; row++){
        CTM[row] = new double[DIMENSION];
        for (int col = 0; col < DIMENSION; col++){
            this->CTM[row][col] = existingMatrix.CTM[row][col];
        }
    }
}

// Destructor
TransformationMatrix::~TransformationMatrix(){
    if (CTM != nullptr){
        for (int i = 0; i < DIMENSION; i++){
                delete [] CTM[i];
        }
    }
    delete [] CTM;
}

// Multiply this matrix by a scalar
void TransformationMatrix::addScaleUniform(double scalar){
    // Build a scale matrix
    TransformationMatrix scale; // Start with the identity matrix
    scale.arrayVal(0, 0) = scalar; // X
    scale.arrayVal(1, 1) = scalar; // Y
    scale.arrayVal(2, 2) = scalar; // Z

    // Multiply it in to the transformation matrix
    *this *= scale;
}

// Add non-uniform scale to this matrix
void TransformationMatrix::addNonUniformScale(double x, double y, double z){
    TransformationMatrix scale;
    scale.arrayVal(0, 0) = x;
    scale.arrayVal(1, 1) = y;
    scale.arrayVal(2, 2) = z;

    // Multiply it in to the transformation matrix
    *this *= scale;
}

// Translate this matrix in (x, y, z)
void TransformationMatrix::addTranslation(double x, double y, double z){
    TransformationMatrix translate;
    translate.arrayVal(0, 3) = x;
    translate.arrayVal(1, 3) = y;
    translate.arrayVal(2, 3) = z;

    *this *= translate;
}

// Add a rotation to this matrix
void TransformationMatrix::addRotation(Axis theAxis, double angle){
    // Convert angle to radians:
    angle = angle * M_PI/180;

    switch(theAxis){
    case 0:{ // X Axis
        TransformationMatrix rotateX;
        rotateX.arrayVal(1, 1) = cos(angle);
        rotateX.arrayVal(1, 2) = sin(angle);
        rotateX.arrayVal(2, 1) = -sin(angle);
        rotateX.arrayVal(2, 2) = cos(angle);

        *this *= rotateX; // Combine the transformations
    }
        break;

    case 1:{ // Y Axis
        TransformationMatrix rotateY;
        rotateY.arrayVal(0, 0) = cos(angle);
        rotateY.arrayVal(0, 2) = -sin(angle);
        rotateY.arrayVal(2, 0) = sin(angle);
        rotateY.arrayVal(2, 2) = cos(angle);

        *this *= rotateY; // Combine the transformations
    }
        break;

    case 2:{ // Z Axis
        TransformationMatrix rotateZ;
        rotateZ.arrayVal(0, 0) = cos(angle);
        rotateZ.arrayVal(0, 1) = sin(angle);
        rotateZ.arrayVal(1, 0) = -sin(angle);
        rotateZ.arrayVal(1, 1) = cos(angle);

        *this *= rotateZ; // Combine the transformations
    }
        break;

    default:
        cout << "ERROR! Invalid transformation axis!\n";
        break;
    }
}

// Get the size of this matrix
int TransformationMatrix::size(){
    return DIMENSION;
}

// Access the array values
double& TransformationMatrix::arrayVal(int x, int y) {
    return CTM[x][y];
}

// Overloaded *= operator: Multiplies  [this] * [rhs]
TransformationMatrix& TransformationMatrix::operator*=(const TransformationMatrix& rhs){
    // Initialize a result matrix
    TransformationMatrix result;
    for (int i = 0; i < DIMENSION; i++){ // 0 out the identity matrix
        result.CTM[i][i] = 0;
    }

    // Peform the multiplication
    for (int row = 0; row < DIMENSION; row++){
        for (int col = 0; col < DIMENSION; col++){
            for (int pos = 0; pos < DIMENSION; pos++){
                result.CTM[row][col] += this->CTM[row][pos] * rhs.CTM[pos][col]; //[lhs]*[rhs]
            }
        }
    }

    // Copy the results back to the calling object
    for (int row = 0; row < DIMENSION; row++){
        for (int col = 0; col < DIMENSION; col++){
            this->CTM[row][col] = result.CTM[row][col] ;
        }
    }
    return *this;
}

// Overloaded, non-member multiplication "*" operator: Multiplies [lhs]*[rhs]
TransformationMatrix operator*(TransformationMatrix& lhs, TransformationMatrix& rhs){
    const int DIMENSION = 4; // Size of a 4x4 matrix

    TransformationMatrix result; // Create a result matrix
    for (int row = 0; row < DIMENSION; row++){
        for (int col = 0; col < DIMENSION; col++){
            result.arrayVal(row, col) = 0; // Initialize our array value to 0

            // Calculate the final value:
            for (int pos = 0; pos < DIMENSION; pos++){
                result.arrayVal(row, col) += (lhs.arrayVal(row, pos) * rhs.arrayVal(pos, col));
            }
        }
    }
    return result; // Return the result
}

// Overloaded assignment operator
TransformationMatrix& TransformationMatrix::operator=(const TransformationMatrix& rhs){
    for (int row = 0; row < DIMENSION; row++){
        for (int col = 0; col < DIMENSION; col++){
            this->CTM[row][col] = rhs.CTM[row][col];
        }
    }
    return *this;
}

// Get inverse: Calculate this Matrix's inverse, and return int
// Assumption: The matrix is non-singular
TransformationMatrix TransformationMatrix::getInverse(){

    // Create and initialize a cofactor matrix
    double** theCofactors = new double*[DIMENSION];
    for (int i = 0; i < DIMENSION; i++){
        theCofactors[i] = new double[DIMENSION];
        for (int j = 0; j < DIMENSION; j++){

            // Get a minor:
            double** currentMinor = makeMinor(CTM, DIMENSION, i, j);
            int minorDimension = DIMENSION - 1;

            // Calculate the determinant of the minor:
            theCofactors[i][j] = getDeterminantRecursive(currentMinor, minorDimension);

            // If i + j is odd, negate the value:
            if ((i + j) % 2 != 0){
                theCofactors[i][j] *= -1;
            }

            // Cleanup:
            for (int i = 0; i < minorDimension; i++){
                delete [] currentMinor[i];
            }
            delete [] currentMinor;
        }
    }

    // Calculate the inverse determinant of this matrix:
    double inverseDeterminant = 1 / this->getDeterminant();

    // Create transformation matrix to use as our result:
    TransformationMatrix result;

    // Multiply each element by the inverse determinant, and copy it into the result and transpose
    // A^-1 = 1/det(A) * adj(A)
    for (int i = 0; i < DIMENSION; i++){
        for (int j = 0; j < DIMENSION; j++){
            result.CTM[i][j] = inverseDeterminant * theCofactors[j][i]; // Simultaneously transpose
        }
    }

    // Cleanup:
    for (int i = 0; i < DIMENSION; i++){
        delete [] theCofactors[i];
    }
    delete theCofactors;

    return result;
}

// Calculate the determinant of this matrix: Invokes the recursive helper function
double TransformationMatrix::getDeterminant(){
    return getDeterminantRecursive(CTM, DIMENSION);
}

// Get the determinant of a matrix (4x4 to 2x2)
double TransformationMatrix::getDeterminantRecursive(double** theMatrix, int theDimension){
    // Base case: 2x2 matrix
    if (theDimension == 2){
        return ((theMatrix[0][0] * theMatrix[1][1]) - (theMatrix[0][1] * theMatrix[1][0]));
    }

    // Recursive case: 4x4 or 3x3 matrix
    double determinant = 0;
    double** theMinor;
    for (int coefficient = 0; coefficient < theDimension; coefficient++){ // Walk the top row, building cofactor matrices and recursively computing their determanents
        theMinor = makeMinor(theMatrix, theDimension, 0, coefficient);;
        if (coefficient % 2 == 0){ // Even: positive factor
            determinant += theMatrix[0][coefficient] * getDeterminantRecursive( theMinor, theDimension - 1);
        }
        else { // Odd: Negate factor
            determinant -= theMatrix[0][coefficient] * getDeterminantRecursive( theMinor, theDimension - 1);
        }

        // Cleanup minors:
        for (int i = 0; i < theDimension - 1; i++){
            delete theMinor[i];
        }
        delete theMinor;
    }
    return determinant;
}

// Make a minor from a matrix
double** TransformationMatrix::makeMinor(double** theMajor, int currentDimension, int row, int col){
    // Initialize a minor matrix:
    int minorDimension = currentDimension - 1;
    double** theMinor = new double*[minorDimension];
    for (int i = 0; i < minorDimension; i++){
        theMinor[i] = new double[minorDimension];
        for (int j = 0; j < minorDimension; j++){
            theMinor[i][j] = 0;
        }
    }

    // Copy the values into our minor:
    for (int minorRow = 0, majorRow = 0; minorRow < minorDimension; minorRow++, majorRow++){
        for (int minorCol = 0, majorCol = 0; minorCol < minorDimension; minorCol++, majorCol++){
            // Advanced past the rows/cols we don't want
            if (majorRow == row)
                majorRow++;
            if (majorCol == col)
                majorCol++;
            // Copy the value
            theMinor[minorRow][minorCol] = theMajor[majorRow][majorCol];
        }
    }
    return theMinor;
}

// Debug this matrix:
void TransformationMatrix::debug(){
    for (int i = 0; i < 4; i++){
        for (int j = 0; j < 4; j++)
            cout << CTM[i][j] << " ";
        cout << "\n";
    }
    cout << "\n";
}
