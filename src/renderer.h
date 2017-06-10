// Renderer object
// By Adam Badke

#ifndef MYRENDERER_H
#define MYRENDERER_H

// Assignment includes:
#include "drawable.h"

// My includes:
#include "polygon.h"
#include "vertex.h"
#include "line.h"
#include "mesh.h"
#include "transformationmatrix.h"
#include "light.h"

// Custom renderer class
class Renderer{
public:
    // Constructor
    Renderer(Drawable* newDrawable, int newXRes, int newYRes, int borderWidth);

    // Destructor
    ~Renderer();

    // Draw a rectangle. Used for setting panel background colors only. Ignores z-buffer.
    // Assumption: topLeft_ and botRight_ coords are valid, and in UI window space (ie. (0,0) is in the top left of the screen!)
    void drawRectangle(int topLeftX, int topLeftY, int botRightX, int botRightY, unsigned int color);

    // Draw a line
    // If theLine's p1.color != p2.color, the line color will be LERP'd. Updates the Z-Buffer.
    void drawLine(Line theLine, ShadingModel theShadingModel, bool doAmbient, double specularCoefficient, double specularExponent);

    // Draw a polygon. Calls the rasterize Polygon helper function
    // If thePolygon vertices are all not the same color, the color will be LERP'd
    // Assumption: All polygons are in world space
    void drawPolygon(Polygon thePolygon);

    // Draw a polygon in wireframe only
    void drawPolygonWireframe(Polygon* thePolygon);

    // Draw a mesh object
    void drawMesh(Mesh theMesh);

    // Get the renderer's clip depth
    double getClipDepth();

    // Reset the depth buffer
    void resetDepthBuffer();

    // Change the frustum shape
    void transformCamera(double newXLow, double newYLow, double newXHigh, double newYHigh, double newHither, double newYon, TransformationMatrix cameraMovement);

    // Set the ambient color
    void setAmbientIntensities(double newRed, double newGreen, double newBlue);

    // Set the distance fog parameters
    void setDistanceFog(double newHither, double newYon, double redIntensity, double greenIntensity, double blueIntensity);

    // Flush the vector of light object that the renderer is currently holding
    void flushLights();

    // Add a new light to the renderer
    // Assumption: The light is in world space
    void addLight(Light newLight);

    // Visually debug the renderer's collection of lights
    void debugLights();

private:
    Drawable* drawable; // A drawable object, used to interface with QT  

    // Raster settings:
    int border;         // Screen border width
    int xRes;           // Calculated horizontal raster resolution
    int yRes;           // Calculated vertical raster resolution
    int xDrawMin, xDrawMax, yDrawMin, yDrawMax; // Calculated draw boundaries

    // Render view settings:
    // Default near/far clip planes, in terms of Z:
    double hither = 1;
    double yon = 200;

    int** ZBuffer;               // Z Depth buffer
    int maxZVal = std::numeric_limits<int>::max();    // Max possible z-depth value

    // View window settings: Default to 90 degree view angle
    double xLow = -1;
    double xHigh = 1;
    double yLow = -1;
    double yHigh = 1;

    // Lighting settings
    double ambientRedIntensity = 0; // Default ambient light intensity values
    double ambientGreenIntensity = 0;
    double ambientBlueIntensity = 0;

    // Default values used for the depth command
    double fogHither = yon;   // Set fogHither to current yon by default (ie. Don't apply depth fog)
    double fogYon = yon;

    double fogRedIntensity = 0;
    double fogGreenIntensity = 0;
    double fogBlueIntensity = 0;
    unsigned int fogColor = 0xff000000; // Combined color, assembled from the recieved fog intensities. Default to black.

    // Lights
    vector<Light> theLights;

    // A transformation matrix from world to camera space
    TransformationMatrix worldToCamera;

    // Perspective transformation: Takes an object in camera space, and adds perspective
    TransformationMatrix cameraToPerspective;   // Assembled in the constructor

    // A world to screen transformation matrix
    TransformationMatrix perspectiveToScreen;   // Assembled in the constructor

    // A transformation matrix from screen space back to perspective space
    TransformationMatrix screenToPerspective;

    // Draw a scanline, with consideration to the Z-Buffer.
    // Assumption: start and end vertices are in left to right order
    // Note: LERP's if start.color != end.color. Does NOT update the screen!
    void drawScanlineIfVisible(Vertex* start, Vertex* end);

    // Draw a scanline with per-pixel phong lighting, with consideration to the Z-Buffer
    void drawPerPxLitScanlineIfVisible(Vertex* start, Vertex* end, bool doAmbient, double specularCoefficient, double specularExponent);

    // Calculate result of overlaying a pixel
    // Written color = opacity * color + (1 - opacity) * color at (x, y)
    // Return: An unsigned int, representing a blended translucent pixel value
    unsigned int blendPixelValues(int x, int y, unsigned int color, float opacity);

    // Lerp between the color values of 2 points (Perspective correct: Takes Z-Depth into account)
    // Return: An unsigned int color value, calculated based on a LERP of the current position between the 2 provided points
    unsigned int getPerspCorrectLerpColor(Vertex* p1, Vertex* p2, double x, double y, double z) const;

    // Calculate a lighting value for a given pixel on a line between 2 points
    // Assumption: Ambient lighting has already been applied to the vertex color values
    unsigned int getFogPixelValue(Vertex* p1, Vertex* p2, double x, double y, double z);

    // Calculate interpolated pixel and depth fog value
    unsigned int getDistanceFoggedColor(unsigned int pixelColor, double z);

    // Set a pixel on the raster
    // Assumption: Point is a valid coordinate on the raster canvas and has been previously checked against the z-buffer
    void setPixel(int x, int y, double z, unsigned int color);

    // Draw a polygon using opacity
    // If thePolygon vertices are all not the same color, the color will be LERP'd
    void rasterizePolygon(Polygon* thePolygon);

    // Light a Polygon using flat shading
    // Assumption: All vertices have a valid normal
    void flatShadePolygon(Polygon* thePolygon);

    // Light a Polygon using gouraud shading
    // Assumption: All vertices have a valid normal
    void gouraudShadePolygon(Polygon* thePolygon);

    // Light a given point in camera space
    void lightPointInCameraSpace(Vertex* currentPosition, bool doAmbient, double specularExponent, double specularCoefficient);

    // Check if a pixel coordinate is in front of the current z-buffer depth
    bool isVisible(int x, int y, double z);

    // Get a scaled z-buffer value for a given Z
    int getScaledZVal(double correctZ);
};

#endif // MYRENDERER_H
