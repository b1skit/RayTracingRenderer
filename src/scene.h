// Scene object: Contains the meshes and lights that make up a renderable scene
// By Adam Badke

#ifndef SCENE_H
#define SCENE_H

#include <vector>
#include "mesh.h"
#include "light.h"

class Scene
{
public:
    // Constructor:
    Scene();

    // Copy constructor
    Scene(const Scene &rhs);

    // Overloaded assignment operator
    Scene& operator=(const Scene& rhs);

    vector<Mesh> theMeshes;     // Contains our meshes
    vector<Light> theLights;    // Contains our lights

    // Ambient lighting values:
    double ambientRedIntensity, ambientGreenIntensity, ambientBlueIntensity;

    // Camera settings:
    // View window settings: Default to 90 degree view angle
    double xLow = -1;
    double xHigh = 1;
    double yLow = -1;
    double yHigh = 1;

    double camHither = 1; // Default hither/yon distances
    double camYon = 200;

    TransformationMatrix cameraMovement;

    // Distance fog settings:
    double fogHither = camYon; // Set fogHither to current yon by default (ie. Don't apply depth fog)
    double fogYon = camYon;

    double fogRedIntensity = 0;
    double fogGreenIntensity = 0;
    double fogBlueIntensity = 0;
    unsigned int fogColor = 0xff000000; // Combined color, assembled from the recieved fog intensities. Default to black.

    // Scene ray trace settings:
    int numRayBounces = 0;          // Default number of bounces when ray tracing. Default = 0 (ie. No ray tracing)
    bool noRayShadows = false;      // Whether or not to use shadow rays. Default = false (ie. Calculate shadows). Shadows can be disabled with "noshadows" command in the .simp file
    double attenuationA = 1;        // Scene bounce light attenuation constants
    double attenuationB = 0.5;

    unsigned int backgroundColor = 0xffff0000;  // Default background color

private:

};

#endif // SCENE_H
