// Line object
// By Adam Badke

#include "line.h"
#include "vertex.h"

Line::Line(Vertex p1, Vertex p2)
{
    // Store the points in left to right order
    if (p1.x <= p1.y){
        this->p1 = p1;
        this->p2 = p2;
    }
    else{
        this->p1 = p2;
        this->p2 = p1;
    }

    // Check if the colors match or not
    if (this->p1.color == this->p2.color)
        sameColors = true;
    else
        sameColors = false;
}

// Determine if this line has the same vertex colors
bool Line::hasSameVertexColors(){
    return sameColors;
}
