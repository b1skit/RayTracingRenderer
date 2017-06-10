// Light object
// By Adam Badke


#ifndef LIGHT_H
#define LIGHT_H

#include "vertex.h"

class Light
{
public:
    // Default constructor
    Light();

    // 5-arg constructor
    Light(double newRedIntensity, double newGreenIntensity, double newBlueIntensity, double newAttA, double newAttB);

    // Overloaded assignment operator
    Light& operator=(const Light& rhs);

    // Light attributes:
    Vertex position; // This lights position, as a point in space

    // Calculate the attenuation of this light to a point, as a ratio
    double getAttenuationFactor(Vertex thePoint);

    // The light's color:
    double redIntensity, greenIntensity, blueIntensity;     // [0, 1]

    double attenuationA, attenuationB;  // Attenuation constants

    // Debug this light:
    void debug();

private:

};

#endif // LIGHT_H
