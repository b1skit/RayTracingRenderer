// Line object
// By Adam Badke

#ifndef LINE_H
#define LINE_H

#include "vertex.h"

class Line
{
public:
    // Constructor
    Line(Vertex p1, Vertex p2);

    // Determine if this line has the same vertex colors
    bool hasSameVertexColors();

    // Line properties:
    Vertex p1;
    Vertex p2;

private:
    bool sameColors;
};

#endif // LINE_H
