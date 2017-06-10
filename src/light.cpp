// Light object
// By Adam Badke

#include "light.h"
#include <iostream>

using std::cout;

// Default constructor
Light::Light()
{
    // Set some default values:
    redIntensity = 0;
    greenIntensity = 0;
    blueIntensity = 0;
    attenuationA = 0.5;
    attenuationB = 0.5;
}


// 5-arg constructor
Light::Light(double newRedIntensity, double newGreenIntensity, double newBlueIntensity, double newAttA, double newAttB){
    redIntensity = newRedIntensity;
    greenIntensity = newGreenIntensity;
    blueIntensity = newBlueIntensity;
    attenuationA = newAttA;
    attenuationB = newAttB;
}

// Overloaded assignment operator
Light& Light::operator=(const Light& rhs){
    this->redIntensity = rhs.redIntensity;
    this->greenIntensity = rhs.greenIntensity;
    this->blueIntensity = rhs.blueIntensity;

    this->attenuationA = rhs.attenuationA;
    this->attenuationB = rhs.attenuationB;

    this->position = rhs.position;

    return *this;
}

// Calculate the attenuation of this light to a point, as a ratio
double Light::getAttenuationFactor(Vertex thePoint){
    normalVector distance(position.x - thePoint.x, position.y - thePoint.y, position.z - thePoint.z);

    return (1.0 /((double) (attenuationA + (attenuationB * distance.length() ) ) ));
}

// Debug this light:
void Light::debug(){
    cout << "\nLight: ";
    cout << "R: " << redIntensity << " G: " << greenIntensity << " B: " << blueIntensity << " att_A: " << attenuationA << " att_B " << attenuationB << "\n";
    cout << "Position: x: " << position.x << " y: " << position.y << " z: " << position.z << "\n\n";

}
