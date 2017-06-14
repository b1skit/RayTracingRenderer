#include "scene.h"

// Constructor
Scene::Scene()
{
    // Do nothing
}

// Copy constructor
Scene::Scene(const Scene &rhs){
    this->theMeshes = rhs.theMeshes;
    this->theLights = rhs.theLights;

    this->ambientRedIntensity = rhs.ambientRedIntensity;
    this->ambientGreenIntensity = rhs.ambientGreenIntensity;
    this->ambientBlueIntensity = rhs.ambientBlueIntensity;

    this->xLow = rhs.xLow;
    this->xHigh = rhs.xHigh;
    this->yLow = rhs.yLow;
    this->yHigh = rhs.yHigh;

    this->camHither = rhs.camHither;
    this->camYon = rhs.camYon;

    this->cameraMovement = rhs.cameraMovement;

    this->fogHither = rhs.fogHither;
    this->fogYon = rhs.fogYon;

    this->fogRedIntensity = rhs.fogRedIntensity;
    this->fogGreenIntensity = rhs.fogGreenIntensity;
    this->fogBlueIntensity = rhs.fogBlueIntensity;
    this->fogColor = rhs.fogColor;
}

// Overloaded assignment operator
Scene& Scene::operator=(const Scene& rhs){
    this->theMeshes = rhs.theMeshes;
    this->theLights = rhs.theLights;

    this->ambientRedIntensity = rhs.ambientRedIntensity;
    this->ambientGreenIntensity = rhs.ambientGreenIntensity;
    this->ambientBlueIntensity = rhs.ambientBlueIntensity;

    this->xLow = rhs.xLow;
    this->xHigh = rhs.xHigh;
    this->yLow = rhs.yLow;
    this->yHigh = rhs.yHigh;

    this->camHither = rhs.camHither;
    this->camYon = rhs.camYon;

    this->cameraMovement = rhs.cameraMovement;

    this->fogHither = rhs.fogHither;
    this->fogYon = rhs.fogYon;

    this->fogRedIntensity = rhs.fogRedIntensity;
    this->fogGreenIntensity = rhs.fogGreenIntensity;
    this->fogBlueIntensity = rhs.fogBlueIntensity;
    this->fogColor = rhs.fogColor;

    return *this;
}