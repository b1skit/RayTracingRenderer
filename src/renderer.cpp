// Renderer object
// By Adam Badke

#include "renderer.h"
#include "renderutilities.h"

// STL includes:
#include <cmath>
#include <iostream>
#include "math.h"               // The STL math library

using std::round;
using std::cout;

// Constructor
Renderer::Renderer(Drawable* newDrawable, int newXRes, int newYRes, int borderWidth){
    this->drawable = newDrawable;

    border = borderWidth;
    xRes = newXRes;
    yRes = newYRes;

    // Initialize the ZBuffer:
    ZBuffer = new int*[yRes];
    for (int row = 0; row < yRes; row++){
        ZBuffer[row] = new int[xRes];
        for (int col = 0; col < xRes; col++){
            ZBuffer[row][col] = maxZVal;
        }
    }

    // Create a perspective transformation matrix:
    cameraToPerspective.arrayVal(3, 3) = 0; // Removes w component
    cameraToPerspective.arrayVal(3, 2) = 1; // Replaces w component with a copy of the z component
}

// Destructor
Renderer::~Renderer(){
    // Deallocate the Z-Buffer:
    for (int x = 0; x < xRes; x++){
        for (int y = 0; y < yRes; y++){
            delete ZBuffer[x];
        }
    }
    delete ZBuffer;
}

// Draw a rectangle. Used for setting panel background colors only. Ignores z-buffer.
// Pre-condition: topLeft_ and botRight_ coords are valid, and in UI window space (ie. (0,0) is in the top left of the screen!)
void Renderer::drawRectangle(int topLeftX, int topLeftY, int botRightX, int botRightY, unsigned int color){
    // Draw the rectangle
    for (int x = topLeftX; x <= botRightX; x++){
        for (int y = topLeftY; y <= botRightY; y++){
            drawable->setPixel(x, y, color);
        }
    }
    // Update the screen:
    drawable->updateScreen();
}

// Draw a line
// If theLine's p1.color != p2.color, the line color will be LERP'd. Updates the Z-Buffer.
void Renderer::drawLine(Line theLine, ShadingModel theShadingModel, bool doAmbient, double specularCoefficient, double specularExponent){

    // Handle vertical lines:
    if (theLine.p1.x == theLine.p2.x){
        int y, y_max;
        double z;
        double z_slope = (theLine.p2.z - theLine.p1.z)/(double)(theLine.p2.y - theLine.p1.y); // How much we're moving in the z-axis for every unit along the y-axis

        if (theLine.p1.y > theLine.p2.y){ // Always draw bottom to top
            Vertex temp = theLine.p1;
            theLine.p1 = theLine.p2;
            theLine.p2 = temp;
        }
        y = (int)theLine.p1.y;
        y_max = (int)theLine.p2.y;
        z = theLine.p1.z;

        // Draw the line:
        while (y < y_max){

            double ratio = (y - theLine.p1.y)/(double)(theLine.p2.y - theLine.p1.y); // Get our current position as a ratio

            double correctZ = getPerspCorrectLerpValue(theLine.p1.z, theLine.p1.z, theLine.p2.z, theLine.p2.z, ratio);

            // Only bother drawing if we're in front of the current z-buffer depth
            if ( isVisible( (int)round(theLine.p1.x), y, correctZ) ){

                if (theShadingModel == phong){
                    // Calculate the current pixel position, as a vertex in camera space:
                    Vertex currentPosition(theLine.p1.x, y, correctZ);        // Create a vertex representing the current point on the scanline

                    currentPosition.transform(&screenToPerspective); // Transform back to perspective space

                    // Correct the perspective transformation: Transform the point back to camera space
                    currentPosition.x *= correctZ;
                    currentPosition.y *= correctZ;

                    // Set the perspective correct normal:
                    currentPosition.normal.xn = getPerspCorrectLerpValue(theLine.p1.normal.xn, theLine.p1.z, theLine.p2.normal.xn, theLine.p2.z, ratio);
                    currentPosition.normal.yn = getPerspCorrectLerpValue(theLine.p1.normal.yn, theLine.p1.z, theLine.p2.normal.yn, theLine.p2.z, ratio);
                    currentPosition.normal.zn = getPerspCorrectLerpValue(theLine.p1.normal.zn, theLine.p1.z, theLine.p2.normal.zn, theLine.p2.z, ratio);
                    currentPosition.normal.normalize(); // Normalize

                    currentPosition.color = getPerspCorrectLerpColor(&theLine.p1, &theLine.p2, ratio); // Get the (perspective correct) base color

                    // Create a view vector: Points from the face towards the camera
                    NormalVector viewVector(-currentPosition.x, -currentPosition.y, -currentPosition.z);
                    viewVector.normalize();

                    // Calculate the lit pixel value, apply distance fog then attempt to set it:
                    currentPosition.color = lightPointInCameraSpace(&currentPosition, &viewVector, doAmbient, specularExponent, specularCoefficient);

                    if (currentScene->isDepthFogged)
                        setPixel((int)theLine.p1.x, y, correctZ, getDistanceFoggedColor( currentPosition.color, correctZ ) );
                    else
                        setPixel((int)theLine.p1.x, y, correctZ, currentPosition.color );
                }
                else{
                    if (currentScene->isDepthFogged)
                        setPixel((int)theLine.p1.x, (int)y, correctZ, getFogPixelValue(&theLine.p1, &theLine.p2, ratio, correctZ) );
                    else
                        setPixel((int)theLine.p1.x, (int)y, correctZ, getPerspCorrectLerpColor(&theLine.p1, &theLine.p2, ratio) );
                }

            } // End z-check

            y++;
            z += z_slope;
        }

    } // Handle non-vertical lines:
    else {
        // If endpoints are not left (p1) to right (p2), swap them:
        if (theLine.p1.x > theLine.p2.x){
            Vertex temp = theLine.p1;
            theLine.p1 = theLine.p2;
            theLine.p2 = temp;
        }

        // Calculate the slope:
        double slope = (theLine.p2.y - theLine.p1.y)/(double)(theLine.p2.x - theLine.p1.x);
        bool steep = false;
        if (slope < -1 || slope > 1){ // If the line is in Octant II, III, VI, VII, flag it and invert the slope
            steep = true;
            slope = 1/slope;
        }

        // Draw steep line:
        if (steep){ // Handle drawing in Octants II, III, VI, VII:
            int y, y_min, y_max;
            double x, z;
            double z_slope = (theLine.p2.z - theLine.p1.z)/(double)(theLine.p2.y - theLine.p1.y); // How much we're moving in the z-axis for every unit along the x-axis
            Vertex lowest, highest; // Since we don't know whether we're drawing from top to bottom, or bottom to top, we make a copy to use when we perform interpolation

            if (theLine.p1.y < theLine.p2.y){
                x = (double)theLine.p1.x;
                y = (int)theLine.p1.y;
                y_min = (int)theLine.p1.y; // We use ints here because the points have already been rounded as part of the transformation to screen space
                y_max = (int)theLine.p2.y;
                z = theLine.p1.z;
                lowest = theLine.p1;
                highest = theLine.p2;
            } else {
                x = theLine.p2.x;
                y = (int)theLine.p2.y;
                y_min = (int)theLine.p2.y;
                y_max = (int)theLine.p1.y;
                z = theLine.p2.z;
                lowest = theLine.p2;
                highest = theLine.p1;
            }

            while (y < y_max){
                int round_x = (int)round(x);

                double ratio = (y - y_min)/(double)(y_max - y_min); // Get our current position as a ratio

                double correctZ = getPerspCorrectLerpValue(lowest.z, lowest.z, highest.z, highest.z, ratio);

                // Only bother drawing if we're in front of the current z-depth
                if ( isVisible(round_x, y, correctZ) ){

                    if (theShadingModel == phong){

                        // Calculate the current pixel position, as a vertex in camera space:
                        Vertex currentPosition(x, y, correctZ);        // Create a vertex representing the current point on the scanline
                        currentPosition.transform(&screenToPerspective); // Transform back to perspective space

                        // Correct the perspective transformation: Transform the point back to camera space
                        currentPosition.x *= correctZ;
                        currentPosition.y *= correctZ;

                        // Set the perspective correct normal:
                        currentPosition.normal.xn = getPerspCorrectLerpValue(lowest.normal.xn, lowest.z, highest.normal.xn, highest.z, ratio);
                        currentPosition.normal.yn = getPerspCorrectLerpValue(lowest.normal.yn, lowest.z, highest.normal.yn, highest.z, ratio);
                        currentPosition.normal.zn = getPerspCorrectLerpValue(lowest.normal.zn, lowest.z, highest.normal.zn, highest.z, ratio);
                        currentPosition.normal.normalize(); // Normalize

                        currentPosition.color = getPerspCorrectLerpColor(&theLine.p1, &theLine.p2, ratio); // Get the (perspective correct) base color

                        // Create a view vector: Points from the face towards the camera
                        NormalVector viewVector(-currentPosition.x, -currentPosition.y, -currentPosition.z);
                        viewVector.normalize();

                        currentPosition.color = lightPointInCameraSpace(&currentPosition, &viewVector, doAmbient, specularExponent, specularCoefficient);

                        if (currentScene->isDepthFogged) // Calculate the lit pixel value, apply distance fog then attempt to set it:
                            setPixel(round_x, y, correctZ, getDistanceFoggedColor( currentPosition.color, correctZ ) );
                        else
                            setPixel(round_x, y, correctZ, currentPosition.color );
                    }
                    else{
                        if (currentScene->isDepthFogged)
                            setPixel(round_x, y, correctZ, getFogPixelValue(&theLine.p1, &theLine.p2, ratio, correctZ) );
                        else
                            setPixel(round_x, y, correctZ, getPerspCorrectLerpColor(&theLine.p1, &theLine.p2, ratio) );
                    }

                }

                y++;
                x += slope;
                z += z_slope;
            }

        } // End if steep

        else { // Handle Octants I, IV, V, VIII:
            double y = theLine.p1.y;
            double z = theLine.p1.z;
            double z_slope = (theLine.p2.z - theLine.p1.z)/(double)(theLine.p2.x - theLine.p1.x); // How much we're moving in the z-axis for every unit along the x-axis

            for (int x = (int)theLine.p1.x; x <= theLine.p2.x; x++){
                int round_y = (int)round(y);

                double ratio = (x - theLine.p1.x)/(double)(theLine.p2.x - theLine.p1.x); // Get our current position as a ratio
                double correctZ = getPerspCorrectLerpValue(theLine.p1.z, theLine.p1.z, theLine.p2.z, theLine.p2.z, ratio);

                // Only bother drawing if we're in front of the current z-depth
                if ( isVisible(x, round_y, correctZ) ){

                    if (theShadingModel == phong){

                        // Calculate the current pixel position, as a vertex in camera space:
                        Vertex currentPosition(x, y, correctZ);        // Create a vertex representing the current point on the scanline
                        currentPosition.transform(&screenToPerspective); // Transform back to perspective space

                        // Correct the perspective transformation: Transform the point back to camera space
                        currentPosition.x *= correctZ;
                        currentPosition.y *= correctZ;

                        // Set the perspective correct normal:
                        currentPosition.normal.xn = getPerspCorrectLerpValue(theLine.p1.normal.xn, theLine.p1.z, theLine.p2.normal.xn, theLine.p2.z, ratio);
                        currentPosition.normal.yn = getPerspCorrectLerpValue(theLine.p1.normal.yn, theLine.p1.z, theLine.p2.normal.yn, theLine.p2.z, ratio);
                        currentPosition.normal.zn = getPerspCorrectLerpValue(theLine.p1.normal.zn, theLine.p1.z, theLine.p2.normal.zn, theLine.p2.z, ratio);
                        currentPosition.normal.normalize(); // Normalize

                        currentPosition.color = getPerspCorrectLerpColor(&theLine.p1, &theLine.p2, ratio); // Get the (perspective correct) base color

                        // Create a view vector: Points from the face towards the camera
                        NormalVector viewVector(-currentPosition.x, -currentPosition.y, -currentPosition.z);
                        viewVector.normalize();

                        // Calculate the lit pixel value, apply distance fog then attempt to set it:
                        currentPosition.color = lightPointInCameraSpace(&currentPosition, &viewVector, doAmbient, specularExponent, specularCoefficient);

                        if (currentScene->isDepthFogged)
                            setPixel(x, round_y, correctZ, getDistanceFoggedColor( currentPosition.color, correctZ ) );
                        else
                            setPixel(x, round_y, correctZ, currentPosition.color );
                    }
                    else{
                        if (currentScene->isDepthFogged)
                            setPixel(x, round_y, correctZ, getFogPixelValue(&theLine.p1, &theLine.p2, ratio, correctZ) );
                        else
                            setPixel(x, round_y, correctZ, getPerspCorrectLerpColor(&theLine.p1, &theLine.p2, ratio) );
                    }
                }

                y += slope;
                z += z_slope;
            }
        }
    } // End non-vertical line else

    // Update the screen:
    drawable->updateScreen();
}

// Draw a polygon. Calls the rasterize Polygon helper function
// If thePolygon vertices are all not the same color, the color will be LERP'd
// Pre-condition: All polygons are in camera space
void Renderer::drawPolygon(Polygon thePolygon, bool isWireframe){

    // Cull polygons outside of hither/yon
    if (!thePolygon.isInDepth(currentScene->camHither, currentScene->camYon)){
        return;
    }

    // Clip polygon to intersection with hither/yon
    thePolygon.clipHitherYon(currentScene->camHither, currentScene->camYon);

    // Handle polygons that have been rendered invalid by clipping
    if(!thePolygon.isValid())
        return;

    // Apply ambient lighting to Lines, if neccessary:
    if (thePolygon.isLine() && thePolygon.isAffectedByAmbientLight() ){
        thePolygon.lightAmbiently( currentScene->ambientRedIntensity, currentScene->ambientGreenIntensity, currentScene->ambientBlueIntensity);
    }

    // Calculate flat/gouraud lighting (while we're still in camera space)
    // Handle flat shading:
    else if (thePolygon.getShadingModel() == flat && !isWireframe && !thePolygon.isLine() ){ // Only light the polygon if it's not wireframe or a line
        flatShadePolygon( &thePolygon );
    }
    // Handle gouraud shading:
    else if (thePolygon.getShadingModel() == gouraud && !isWireframe && !thePolygon.isLine()){ // Only light the polygon if it's not wireframe or a line
        gouraudShadePolygon( &thePolygon );
    }

    // Apply perspective xform:
    thePolygon.transform( &cameraToPerspective );

    // Backface cull: Don't attempt to render polygons not facing the camera
    if (!thePolygon.isFacingCamera() && !isWireframe && !thePolygon.isLine()){ // Don't backface cull wireframe polygons or lines, as we still want to see them
      return;
    }

    // Polygon frustum culling: Cull polygons outside of xLow/xHigh/yLow/yHigh
    if (!thePolygon.isInFrustum(currentScene->xLow, currentScene->xHigh, currentScene->yLow, currentScene->yHigh)){
        return;
    }

    // Frustum clipping: Clip polygon to 4 sides of view window:
    thePolygon.clipToScreen(currentScene->xLow, currentScene->xHigh, currentScene->yLow, currentScene->yHigh);

    // Handle polygons that have been rendered invalid by clipping
    if(!thePolygon.isValid())
        return;

    // Tranform perspective to screen space
    thePolygon.transform(&perspectiveToScreen, true);

    // Draw lines (Polygons with 2 points)
    if(thePolygon.isLine()){
        drawLine(Line(*(thePolygon.getLast()), *(thePolygon.getPrev(thePolygon.getLast()->vertexNumber) )), ambientOnly, true, 0, 0);
        return;
    }

    // Trianglulate:
    vector<Polygon>* theFaces = thePolygon.getTriangulatedFaces();  

    // Render each resulting triangle:
    for (unsigned int i = 0; i < theFaces->size(); i++){

        // Draw regular polygons:
        if (!isWireframe) {
            rasterizePolygon( &theFaces->at(i) );
        }
        else{ // Draw wireframe polygons
            drawPolygonWireframe( &theFaces->at(i) );
        }
    }

    // Cleanup:
    delete theFaces;
}

// Rasterize a polygon
// Pre-condition: Received polygon is a triange, is in screen space, and all 3 vertices have been rounded to integer coordinates
void Renderer::rasterizePolygon(Polygon* thePolygon){

    // Get the vertices from the polygon:
    Vertex* topLeftVertex = thePolygon->getHighest();
    Vertex* topRightVertex = topLeftVertex; // Start edge traversal at the same point

    Vertex* botLeftVertex = thePolygon->getNext(topLeftVertex->vertexNumber); // Get the next/left vertex (using counter-clockwise ordering)
    Vertex* botRightVertex = thePolygon->getPrev(topRightVertex->vertexNumber); // Get the prev/right vertex

    // Set the starting vertices: Handle multiple "top" vertices with the same y coordinate - Slide down the edge
    int count = thePolygon->getVertexCount(); // Track how many times we've moved, to prevent infinite loops when all vertices have the same y-coord.
    while (topLeftVertex->y == botLeftVertex->y && count > 0){
        count--;
        topLeftVertex = botLeftVertex;
        botLeftVertex = thePolygon->getNext(botLeftVertex->vertexNumber);
    }
    count = thePolygon->getVertexCount();
    while (topRightVertex->y == botRightVertex->y && count > 0){
        count--;
        topRightVertex = botRightVertex;
        botRightVertex = thePolygon->getPrev(botRightVertex->vertexNumber);
    }

    // Get the starting coords: No need to round, as recieved polygon verts should already have been rounded
    int y = (int)(topLeftVertex->y);
    int yMin = (int)( thePolygon->getLowest()->y );

    // X endpoints:
    double xLeft = topLeftVertex->x;
    double xRight = topRightVertex->x;

    // Calculate slopes of the polygon edges:
    double DYLeft = topLeftVertex->y - botLeftVertex->y;
    double DYRight = topRightVertex->y - botRightVertex->y;
    double zLeft = topLeftVertex->z;
    double zRight = topRightVertex->z;

    // Edge slopes:
    double xLeftSlope, xRightSlope, zLeftSlope, zRightSlope, leftRatioDiff, rightRatioDiff;
    if (DYLeft == 0){
        xLeftSlope = 0;
        zLeftSlope = 0;
        leftRatioDiff = 0;
    }
    else{
        xLeftSlope = (topLeftVertex->x - botLeftVertex->x)/(double)DYLeft;
        zLeftSlope = (topLeftVertex->z - botLeftVertex->z)/(double)DYLeft;
        leftRatioDiff = 1.0 /(double) DYLeft;
    }
    if (DYRight == 0){
        xRightSlope = 0;
        zRightSlope = 0;
        rightRatioDiff = 0;
    }
    else {
        xRightSlope = (topRightVertex->x - botRightVertex->x)/(double)DYRight;
        zRightSlope = (topRightVertex->z - botRightVertex->z)/(double)DYRight;
        rightRatioDiff = 1.0 /(double) DYRight;
    }

    // Left/Right edge starting ratio (for perspective correct Z calculations)
    double leftRatio = 0;
    double rightRatio = 0;

    // Main drawing loop:
    while (y >= yMin){

        // Assemble 2 points, and draw a scanline between them.
        double leftCorrectZ = getPerspCorrectLerpValue(topLeftVertex->z, topLeftVertex->z, botLeftVertex->z, botLeftVertex->z, leftRatio );
        double rightCorrectZ = getPerspCorrectLerpValue(topRightVertex->z, topRightVertex->z, botRightVertex->z, botRightVertex->z, rightRatio );

        double xLeft_rounded = round(xLeft); // Pre-round our coordinates for the scanline functions
        double xRight_rounded = round(xRight);

        if (thePolygon->getShadingModel() == phong){

            Vertex lhs(xLeft_rounded, (double)y, leftCorrectZ, getPerspCorrectLerpColor(topLeftVertex, botLeftVertex, leftRatio));
            lhs.normal = NormalVector(topLeftVertex->normal, topLeftVertex->z, botLeftVertex->normal, botLeftVertex->z, y, topLeftVertex->y, botLeftVertex->y);

            Vertex rhs(xRight_rounded, (double)y, rightCorrectZ, getPerspCorrectLerpColor(topRightVertex, botRightVertex, rightRatio));
            rhs.normal = NormalVector(topRightVertex->normal, topRightVertex->z, botRightVertex->normal, botRightVertex->z, y, topRightVertex->y, botRightVertex->y);

            drawPerPxLitScanlineIfVisible( &lhs, &rhs, thePolygon->isAffectedByAmbientLight(), thePolygon->getSpecularCoefficient(), thePolygon->getSpecularExponent());
        }
        else
            drawScanlineIfVisible( &Vertex(xLeft_rounded, (double)y, leftCorrectZ, getPerspCorrectLerpColor(topLeftVertex, botLeftVertex, leftRatio)),
                                   &Vertex(xRight_rounded, (double)y, rightCorrectZ, getPerspCorrectLerpColor(topRightVertex, botRightVertex, rightRatio))
                                   );

        y--; // Move to the next line, and handle transitions between vertices if neccessary:

        // Handle left edge
        if (y < botLeftVertex->y){
            topLeftVertex = botLeftVertex;
            botLeftVertex = thePolygon->getNext(botLeftVertex->vertexNumber);
            DYLeft = topLeftVertex->y - botLeftVertex->y;

            if (DYLeft == 0){
                xLeftSlope = 0;
                zLeftSlope = 0;
                leftRatioDiff = 0;
            }
            else{
                xLeftSlope = (topLeftVertex->x - botLeftVertex->x) / DYLeft;
                zLeftSlope = (topLeftVertex->z - botLeftVertex->z) / DYLeft;
                leftRatioDiff = 1.0 /(double) DYLeft;
            }

            zLeft = (double)topLeftVertex->z - zLeftSlope;   // Subtract the diffs, as the first scanline of the new poly segment has already been drawn
            xLeft = topLeftVertex->x - xLeftSlope;
            leftRatio = leftRatioDiff;                      // 0 + an increment of the diff
        }
        else { // Otherwise, increment the current positions
            xLeft -= xLeftSlope;
            zLeft -= zLeftSlope;
            leftRatio += leftRatioDiff;
        }

        // Handle right edge
        if (y < botRightVertex->y){
            topRightVertex = botRightVertex;
            botRightVertex = thePolygon->getPrev(botRightVertex->vertexNumber);
            DYRight = topRightVertex->y - botRightVertex->y;

            if (DYRight == 0){
                xRightSlope = 0;
                zRightSlope = 0;
                rightRatioDiff = 0;
            }
            else {
                xRightSlope = (topRightVertex->x - botRightVertex->x) / DYRight;
                zRightSlope = (topRightVertex->z - botRightVertex->z) / DYRight;
                rightRatioDiff = 1.0 /(double) DYRight;
            }

            zRight = (double)topRightVertex->z - zRightSlope;    // Subtract the diffs, as the first scanline of the new poly segment has already been drawn
            xRight = topRightVertex->x - xRightSlope;
            rightRatio = rightRatioDiff;                        // 0 + an increment of the diff
        }
        else { // Otherwise, increment the current positions
            xRight -= xRightSlope;
            zRight -= zRightSlope;
            rightRatio += rightRatioDiff;
        }

    } // End main drawing loop

    // Update the screen:
    drawable->updateScreen();
}

// Light a Polygon using flat shading
// Pre-condition: All vertices have a valid normal
void Renderer::flatShadePolygon(Polygon* thePolygon){

    unsigned int* ambientValues = new unsigned int[ thePolygon->getVertexCount() ];

    double* redDiffuseTotals = new double[ thePolygon->getVertexCount() ];
    double* greenDiffuseTotals = new double[ thePolygon->getVertexCount() ];
    double* blueDiffuseTotals = new double[ thePolygon->getVertexCount() ];

    double* redSpecTotals = new double[ thePolygon->getVertexCount() ];
    double* greenSpecTotals = new double[ thePolygon->getVertexCount() ];
    double* blueSpecTotals = new double[ thePolygon->getVertexCount() ];

    // Calculate ambient lighting:
    for (int i = 0; i < thePolygon->getVertexCount(); i++){
        // Initialize the array values to zero:
        ambientValues[i] = 0;

        redDiffuseTotals[i] = 0;
        greenDiffuseTotals[i] = 0;
        blueDiffuseTotals[i] = 0;

        redSpecTotals[i] = 0;
        greenSpecTotals[i] = 0;
        blueSpecTotals[i] = 0;

        // Calculate the ambient component:
        if (thePolygon->isAffectedByAmbientLight() ){
            ambientValues[i] = multiplyColorChannels(thePolygon->vertices[i].color, 1.0, currentScene->ambientRedIntensity, currentScene->ambientGreenIntensity, currentScene->ambientBlueIntensity );
        }
    }

    // Get the center of the polygon face, as a point:
    Vertex faceCenter = thePolygon->getFaceCenter();

    // Calculate the average of the normals:
    NormalVector faceNormal = thePolygon->getNormalAverage();

    // Loop through each light:
    for (unsigned int i = 0; i < currentScene->theLights.size(); i++){

        // Get the (normalized) light direction vector: Points from the face towards the light
        NormalVector lightDirection(currentScene->theLights[i].position.x - faceCenter.x, currentScene->theLights[i].position.y - faceCenter.y, currentScene->theLights[i].position.z - faceCenter.z);
        lightDirection.normalize();

        // Get the cosine of the angle between the face normal and the light direction
        double faceNormalDotLightDirection = faceNormal.dotProduct(lightDirection);

        if (faceNormalDotLightDirection > 0){ // Only proceed if the angle < 90 degrees

            // Get the attenuation factor of the current light:
            double attenuationFactor = currentScene->theLights[i].getAttenuationFactor(faceCenter);

            // Mutliply the light intensities by the attenuation:
            double redDiffuseIntensity = currentScene->theLights[i].redIntensity * attenuationFactor;
            double greenDiffuseIntensity = currentScene->theLights[i].greenIntensity * attenuationFactor;
            double blueDiffuseIntensity = currentScene->theLights[i].blueIntensity * attenuationFactor;

            // Factor the cosine value into the light intensity values:
            redDiffuseIntensity *= faceNormalDotLightDirection;
            greenDiffuseIntensity *= faceNormalDotLightDirection;
            blueDiffuseIntensity *= faceNormalDotLightDirection;

            // Calculate the spec component:
            double redSpecIntensity = thePolygon->getSpecularCoefficient();
            double greenSpecIntensity = thePolygon->getSpecularCoefficient();
            double blueSpecIntensity = thePolygon->getSpecularCoefficient();

            // Create a view vector: Points from the face towards the camera
            NormalVector viewVector(-faceCenter.x, -faceCenter.y, -faceCenter.z);
            viewVector.normalize();

            // Calculate the reflection vector:
            NormalVector reflectionVector = reflectOutVector(&faceNormal, &lightDirection);

            // Calculate the cosine of the angle between the view vector and the reflection vector:
            double viewDotReflection = viewVector.dotProduct(reflectionVector);
            if (viewDotReflection < 0) // Clamp the value to be >=0
                viewDotReflection = 0;

            viewDotReflection = pow(viewDotReflection, thePolygon->getSpecularExponent() );

            redSpecIntensity *= (currentScene->theLights[i].redIntensity * attenuationFactor * viewDotReflection);
            greenSpecIntensity *= (currentScene->theLights[i].greenIntensity * attenuationFactor * viewDotReflection);
            blueSpecIntensity *= (currentScene->theLights[i].blueIntensity * attenuationFactor * viewDotReflection);

            // Loop through each vertex, adding the sum of the light values to the vertex's total light
            for (int i = 0; i < thePolygon->getVertexCount(); i++){

                redDiffuseTotals[i] += redDiffuseIntensity;
                greenDiffuseTotals[i] += greenDiffuseIntensity;
                blueDiffuseTotals[i] += blueDiffuseIntensity;

                redSpecTotals[i] += redSpecIntensity;
                greenSpecTotals[i] += greenSpecIntensity;
                blueSpecTotals[i] += blueSpecIntensity;

            } // End polygon vertices loop

        } // end if
    } // End light loop

    // Combine the color values, and assign their sum as the new vertex color:
    for (int i = 0; i < thePolygon->getVertexCount(); i++){
        thePolygon->vertices[i].color = addColors( ambientValues[i],
                                                  addColors(
                                                       multiplyColorChannels(thePolygon->vertices[i].color, 1.0, redDiffuseTotals[i], greenDiffuseTotals[i], blueDiffuseTotals[i]),
                                                       combineColorChannels( redSpecTotals[i], greenSpecTotals[i], blueSpecTotals[i] )
                                                       )
                                                  );
    }

    // Cleanup:
    delete [] ambientValues;

    delete [] redDiffuseTotals;
    delete [] greenDiffuseTotals;
    delete [] blueDiffuseTotals;

    delete [] redSpecTotals;
    delete [] greenSpecTotals;
    delete [] blueSpecTotals;
}

// Light a Polygon using gouraud shading
void Renderer::gouraudShadePolygon(Polygon* thePolygon){

    // Loop through each vertex and calculate lighting for it
    for (int i = 0; i < thePolygon->getVertexCount(); i++){

        // Create a view vector: Points from the face towards the camera
        NormalVector viewVector(-thePolygon->vertices[i].x, -thePolygon->vertices[i].y, -thePolygon->vertices[i].z);
        viewVector.normalize();

        thePolygon->vertices[i].color = lightPointInCameraSpace(&thePolygon->vertices[i], &viewVector, thePolygon->isAffectedByAmbientLight(), thePolygon->getSpecularExponent(), thePolygon->getSpecularCoefficient());
    }
}

// Draw a polygon in wireframe only
void Renderer::drawPolygonWireframe(Polygon* thePolygon){
    if (thePolygon->isValid() ){ // Only draw if our polygon has at least 3 vertices

        Vertex* theTop = thePolygon->getHighest();
        Vertex* v1 = theTop;
        Vertex* v2 = thePolygon->getNext(v1->vertexNumber);

        do {
            drawLine(Line(*v1, *v2), ambientOnly, thePolygon->isAffectedByAmbientLight(), 0, 0); // Uses DDA Drawer for now: Might change this in future
            v1 = v2;
            v2 = thePolygon->getNext(v2->vertexNumber);
        } while (v1 != theTop);
    }
}

// Draw a mesh object
void Renderer::drawMesh(Mesh* theMesh){
    for (unsigned int i = 0; i < theMesh->faces.size(); i++){
        currentPolygon = &theMesh->faces[i];    // Track the current polygon, so we can identify it after we've made a copy to pass down the rendering pipeline
        drawPolygon(theMesh->faces[i], theMesh->isWireframe);
    }

    // Remove the reference to the currentPolygon, for safety
    currentPolygon = nullptr;
}

// Render a scene
void Renderer::renderScene(Scene theScene){
    // Store a pointer to the current scene (for accessing various render settings)
    currentScene = &theScene;

    // Fill the canvas with the depth fog color:
    drawRectangle(0, 0, xRes - 1, yRes - 1, currentScene->fogColor);

    // Transform the render camera (also resets depth buffer):
    transformCamera(theScene.cameraMovement);

    // Transform lights from world space to camera space:
    for (auto &currentLight : theScene.theLights){
        currentLight.position.transform(&worldToCamera);

    }

    // Transform meshes into camera space:
    for(auto &processingMesh : theScene.theMeshes){
        processingMesh.transform(&worldToCamera);
    }

    // Process and draw each mesh in the scene:
    for (auto &renderMesh : theScene.theMeshes){
        currentMesh = &renderMesh; // Update the currentMesh pointer to the current mesh being drawn
        drawMesh(&renderMesh);

//        // UNCOMMENT TO VISIBLY DEBUG BOUNDING BOXES:
//        for (int i = 0; i < renderMesh.boundingBoxFaces.size(); i++){
//            drawPolygon(renderMesh.boundingBoxFaces[i], true);
//        }
    }

    // Remove the pointers to the current scene objects
    currentScene = nullptr;
    currentMesh = nullptr;
}

// Draw a scanline, with consideration to the Z-Buffer
// Pre-condition: start and end vertices are in left to right order, and have been pre-rounded to integer coordinates
// Note: LERP's if start->color != end->color. Does NOT update the screen!
void Renderer::drawScanlineIfVisible(Vertex* start, Vertex* end){

    double z = start->z;
    double z_slope; // Make sure we're setting a valid z-slope value
    if (end->x - start->x == 0)
        z_slope = 0;
    else
        z_slope = (end->z - start->z)/(double)(end->x - start->x);

    int x_start = (int)start->x;
    int x_end = (int)end->x;
    int y_rounded = (int)start->y;

    double ratio = 0;
    double ratioDiff;
    if (x_end - x_start == 0)
        ratioDiff = 0;
    else
        ratioDiff = 1/(double)(x_end - x_start);

    // Draw:
    for (int x = x_start; x <= x_end; x++){

        double correctZ = getPerspCorrectLerpValue(start->z, start->z, end->z, end->z, ratio);

        if (isVisible(x, y_rounded, correctZ) ){
            if (currentScene->isDepthFogged)
                setPixel(x, y_rounded, correctZ, getFogPixelValue(start, end, ratio, correctZ) );
            else
                setPixel(x, y_rounded, correctZ, getPerspCorrectLerpColor(start, end, ratio) );
        }

        z += z_slope;
        ratio += ratioDiff;
    }
}

// Draw a scanline using per-pixel lighting (ie Phong shading)
void Renderer::drawPerPxLitScanlineIfVisible(Vertex* start, Vertex* end, bool doAmbient, double specularCoefficient, double specularExponent){

    // Calculate the starting parameters:
    double zCameraSpace = start->z; // Recieved Z is the correct, camera space Z
    double z_slope; // Make sure we're setting a valid z-slope value
    if (end->x - start->x == 0)
        z_slope = 0;
    else
        z_slope = (end->z - start->z)/(double)(end->x - start->x);

    int x_start = (int)start->x;
    int x_end = (int)end->x;

    int y_rounded = (int)start->y;

    double ratio = 0;
    double ratioDiff;
    if (x_end - x_start == 0)
        ratioDiff = 0;
    else
        ratioDiff = 1/(double)(x_end - x_start);

    // Draw:
    for (int x = x_start; x <= x_end; x++){

        double correctZ = getPerspCorrectLerpValue(start->z, start->z, end->z, end->z, ratio); // Calculate the perspective correct Z for the current pixel

        // Only bother drawing if we know we're in front of the current z-buffer value:
        if ( isVisible(x, y_rounded, correctZ) ){

            // Calculate the current pixel position, as a vertex in camera space:
            Vertex currentPosition(x, y_rounded, correctZ);        // Create a vertex representing the current point on the scanline

            currentPosition.transform(&screenToPerspective); // Transform back to perspective space

            // Correct the perspective transformation: Transform the point back to camera space
            currentPosition.x *= correctZ;
            currentPosition.y *= correctZ;

            // Set the perspective correct normal:
            currentPosition.normal.xn = getPerspCorrectLerpValue(start->normal.xn, start->z, end->normal.xn, end->z, ratio);
            currentPosition.normal.yn = getPerspCorrectLerpValue(start->normal.yn, start->z, end->normal.yn, end->z, ratio);
            currentPosition.normal.zn = getPerspCorrectLerpValue(start->normal.zn, start->z, end->normal.zn, end->z, ratio);
            currentPosition.normal.normalize(); // Normalize

            currentPosition.color = getPerspCorrectLerpColor(start, end, ratio); // Get the (perspective correct) base color

            // Create a view vector: Points from the face towards the camera
            NormalVector viewVector(-currentPosition.x, -currentPosition.y, -currentPosition.z);
            viewVector.normalize();

            // Calculate the lit pixel value, apply distance fog then set it:
            currentPosition.color = recursivelyLightPointInCS(&currentPosition, &viewVector, doAmbient, specularExponent, specularCoefficient, currentScene->numRayBounces, x == x_start || x == x_end);

            // Set the pixel value
            setPixel(x, y_rounded, correctZ, currentPosition.color);
        }

        ratio += ratioDiff;

        zCameraSpace += z_slope;
    }
}

// Recursively ray trace a point's lighting. Calls the recursive helper function
unsigned int Renderer::recursivelyLightPointInCS(Vertex* currentPosition, NormalVector* viewVector, bool doAmbient, double specularExponent, double specularCoefficient, int bounceRays, bool isEndPoint){
    // Light the initial point:
    unsigned int initialColor = lightPointInCameraSpace(currentPosition, viewVector, doAmbient, specularExponent, specularCoefficient);

    // Handle ray tracing:
    if (bounceRays > 0){
        // Calculate the bounce direction: Points from initial point towards the (potential) intersection
        NormalVector bounceDirection = reflectOutVector(&(currentPosition->normal), viewVector);

        // Add the intial points' color and its reflective component:
        return addColors(   initialColor,
                            multiplyColorChannels(
                                    recursiveLightHelper(currentPosition, &bounceDirection, doAmbient, specularExponent, specularCoefficient, bounceRays - 1, isEndPoint),
                                    1.0, currentPolygon->getReflectivity(), currentPolygon->getReflectivity(), currentPolygon->getReflectivity()
                            )
                         );
    }
    else
        return initialColor;
}

// Recursive helper function for ray tracing. Finds a new bounce intersection point, and returns its lighting value
// Note: inBounceDirection is a normalized vector that points from a face towards a potential point of intersection
unsigned int Renderer::recursiveLightHelper(Vertex* currentPosition, NormalVector* inBounceDirection, bool doAmbient, double specularExponent, double specularCoefficient, int bounceRays, bool isEndPoint){

    // Find an intersection point, if it exists:
    Vertex* intersectionResult;
    Polygon* hitPoly;
    double hitDistance;

    intersectionResult = new Vertex(); // Allocate a new vertex

    hitPoly = nullptr; // Track which polygon, if any, we've hit
    hitDistance = std::numeric_limits<double>::max();         // Track how for the current nearest hit we've found is from the starting position
    Vertex closestIntersection;

    // Loop through all Meshes in the current scene
    for (auto &currentVisibleMesh : currentScene->theMeshes){

        // Loop through each face of the current mesh's bounding box
        for (int i = 0; i < currentVisibleMesh.boundingBoxFaces.size(); i++){

            // Find an intersection point with the bounding box, if it exists:
            if ( getPolyPlaneIntersectionPoint(currentPosition, inBounceDirection, &currentVisibleMesh.boundingBoxFaces[i].vertices[0], &currentVisibleMesh.boundingBoxFaces[i].faceNormal, intersectionResult ) ){

                // Ensure the intersection point hit the bounding box
                if ( pointIsInsidePoly( &currentVisibleMesh.boundingBoxFaces[i], intersectionResult ) || currentMesh == &currentVisibleMesh ){

                    // We have a bounding box intersection hit! Loop through each visible face in the current mesh and find an actual intersection point:
                    for (int j = 0; j < currentVisibleMesh.faces.size(); j++){

                        // Skip the current polygon (as it always has an intersection)
                        if ( &currentVisibleMesh.faces[j] == currentPolygon )
                            continue;

                        // Find an actual intersection point, if it exists:
                        if ( getPolyPlaneFrontFaceIntersectionPoint(currentPosition, inBounceDirection, &currentVisibleMesh.faces[j].vertices[0], &currentVisibleMesh.faces[j].faceNormal, intersectionResult ) ){

                            // Check if the intersection point is inside of the polygon
                            if( pointIsInsidePoly( &currentVisibleMesh.faces[j], intersectionResult ) // We've found an intersection!
                                    // Ensure the intersection is not a self intersection, or intersecting a shared edge: Prevents ray bounces striking shared convex edges at sides of polygons
                                    && (currentMesh !=  &currentVisibleMesh   || !isEndPoint || !haveSharedEdge(currentPolygon, &currentVisibleMesh.faces[j]) || !isFaceReflexAngle(currentPolygon, &currentVisibleMesh.faces[j]) )
                              )
                            {
                                // Make sure the intersection is nearest, and keep it if it is
                                double currentHitDistance = (*intersectionResult - *currentPosition).length();

                                if (currentHitDistance < hitDistance){ // Store the new closest hit
                                    hitDistance = currentHitDistance;
                                    hitPoly = &currentVisibleMesh.faces[j];
                                    closestIntersection = *intersectionResult;
                                }
                            }
                        }
                    } // End of visible face intersection check

                    break;  // If we've gotten this far, we've already checked all visible faces of the current mesh. Move to the next mesh.

                } // End inside bounding box check
            }
        }
    } // End looping through all meshes

    // Cleanup:
    delete intersectionResult;

    // If we've found bounced light intersection points, calculate their contribution and add it to the final color:
    if (hitPoly != nullptr){

        // Update the closestIntersection with the interpolated normal and color:
        setInterpolatedIntersectionValues(&closestIntersection, hitPoly);

        // Reverse the recieved bounce direction to make it a view vector from the previous point
        inBounceDirection->reverse();

        // Light the intersection point:
        closestIntersection.color = lightPointInCameraSpace(&closestIntersection, inBounceDirection, hitPoly->isAffectedByAmbientLight(), hitPoly->getSpecularExponent(), hitPoly->getSpecularCoefficient());

        // Make a recursive call
        if (bounceRays > 0 && hitPoly->getReflectivity() > 0){

            // Calculate new bounce direction:
            NormalVector nextBounceDirection = reflectOutVector(&closestIntersection.normal, inBounceDirection);

            return addColors(  closestIntersection.color,
                                   multiplyColorChannels(   recursiveLightHelper(&closestIntersection, &nextBounceDirection, doAmbient, specularExponent, specularCoefficient, bounceRays - 1, false),
                                                            1.0, hitPoly->getReflectivity(), hitPoly->getReflectivity(), hitPoly->getReflectivity() )
                             );

        }
        // No more recursive calls to make: Return the intersection color
        else
            return closestIntersection.color;

    } // End hitPoly check

    // We failed to hit anything: Return the scene's background color
    return currentScene->environmentColor;
}

// Light a given point in camera space
// Precondition: viewVector is normalized
unsigned int Renderer::lightPointInCameraSpace(Vertex* currentPosition, NormalVector* viewVector, bool doAmbient, double specularExponent, double specularCoefficient) {

    // Running light totals:
    unsigned int ambientValue = 0;
    double redTotalDiffuseIntensity, greenTotalDiffuseIntensity, blueTotalDiffuseIntensity, redTotalSpecIntensity, greenTotalSpecIntensity, blueTotalSpecIntensity;
    redTotalDiffuseIntensity = greenTotalDiffuseIntensity = blueTotalDiffuseIntensity = redTotalSpecIntensity = greenTotalSpecIntensity = blueTotalSpecIntensity = 0;

    // Calculate the ambient component:
    if ( doAmbient ){
        ambientValue = multiplyColorChannels(currentPosition->color, 1.0, currentScene->ambientRedIntensity, currentScene->ambientGreenIntensity, currentScene->ambientBlueIntensity );
    }

    // Loop through each light in the scene:
    for (unsigned int i = 0; i < currentScene->theLights.size(); i++){

        // Get the (normalized) light direction vector: Points from the face towards the light
        NormalVector lightDirection(currentScene->theLights[i].position.x - currentPosition->x, currentScene->theLights[i].position.y - currentPosition->y, currentScene->theLights[i].position.z - currentPosition->z);
        lightDirection.normalize();

        // Get the cosine of the angle between the face normal and the light direction
        double currentNormalDotLightDirection = currentPosition->normal.dotProduct(lightDirection);

        // Ensure the light is within 90 degrees about the surface normal, and is not shaded by any other polygons in the scene:
        if (currentNormalDotLightDirection > 0) {

            double lightDistance = NormalVector(currentScene->theLights[i].position.x - currentPosition->x, currentScene->theLights[i].position.y - currentPosition->y, currentScene->theLights[i].position.z - currentPosition->z).length();

            // Calculate light value if scene or current point is unshadowed
            if (currentScene->noRayShadows || !isShadowed(*currentPosition, &lightDirection, lightDistance) ){

                double attenuationFactor = currentScene->theLights[i].getAttenuationFactor(lightDistance);

                // Mutliply the light intensities by the attenuation:
                double redDiffuseIntensity = currentScene->theLights[i].redIntensity * attenuationFactor;
                double greenDiffuseIntensity = currentScene->theLights[i].greenIntensity * attenuationFactor;
                double blueDiffuseIntensity = currentScene->theLights[i].blueIntensity * attenuationFactor;

                // Factor the cosine value into the light intensity values:
                redDiffuseIntensity *= currentNormalDotLightDirection;
                greenDiffuseIntensity *= currentNormalDotLightDirection;
                blueDiffuseIntensity *= currentNormalDotLightDirection;

                // Add the diffuse intensity for the current light to the running totals
                redTotalDiffuseIntensity += redDiffuseIntensity;
                greenTotalDiffuseIntensity += greenDiffuseIntensity;
                blueTotalDiffuseIntensity += blueDiffuseIntensity;

                // Calculate the reflection vector:
                NormalVector reflectionVector = reflectOutVector(&(currentPosition->normal), &lightDirection);

                // Calculate the cosine of the angle between the view vector and the reflection vector:
                double viewDotReflection = viewVector->dotProduct(reflectionVector);

                if (viewDotReflection > 0){

                    // Calculate the spec component:
                    double redSpecIntensity = specularCoefficient;
                    double greenSpecIntensity = specularCoefficient;
                    double blueSpecIntensity = specularCoefficient;

                    viewDotReflection = pow(viewDotReflection, specularExponent );

                    redSpecIntensity *= (currentScene->theLights[i].redIntensity * attenuationFactor * viewDotReflection);
                    greenSpecIntensity *= (currentScene->theLights[i].greenIntensity * attenuationFactor * viewDotReflection);
                    blueSpecIntensity *= (currentScene->theLights[i].blueIntensity * attenuationFactor * viewDotReflection);

                    // Add the final diffuse/spec values to the running totals:
                    redTotalSpecIntensity += redSpecIntensity;
                    greenTotalSpecIntensity += greenSpecIntensity;
                    blueTotalSpecIntensity += blueSpecIntensity;
                }

            } // End ifShadowed check
        } // end if surface normal check
    } // End lights loop

    if (currentScene->isDepthFogged && currentPolygon->getShadingModel() == phong){
        return getDistanceFoggedColor( addColors(     ambientValue,
                                                  addColors(
                                                      multiplyColorChannels( currentPosition->color, 1.0, redTotalDiffuseIntensity, greenTotalDiffuseIntensity, blueTotalDiffuseIntensity ),
                                                      combineColorChannels( redTotalSpecIntensity, greenTotalSpecIntensity, blueTotalSpecIntensity ) )
                                                  ),
                                   currentPosition->z);
    }
    else
        return addColors(     ambientValue,
                              addColors(
                                  multiplyColorChannels( currentPosition->color, 1.0, redTotalDiffuseIntensity, greenTotalDiffuseIntensity, blueTotalDiffuseIntensity ),
                                  combineColorChannels( redTotalSpecIntensity, greenTotalSpecIntensity, blueTotalSpecIntensity ) )
                        );
}

// Determine whether a current position is shadowed by some polygon in the scene that lies between it and a light
// Precondition: All Polygons in the scene must have at least 3 vertices, and all meshes must have pre-calcualted bounding boxes
bool Renderer::isShadowed(Vertex currentPosition, NormalVector* lightDirection, double lightDistance){

    // Shift the current position slightly along its normal, to avoid self-intersections
    currentPosition += (currentPosition.normal * 0.1);

    // Allocate a vertex to hold any intersection results we find:
    Vertex* intersectionResult = new Vertex(); // Modified if getPolyPlaneIntersectionPoint() finds a point of intersection

    // Loop through all Meshes in the current scene
    for (auto &currentVisibleMesh : currentScene->theMeshes){

        // Loop through each face of the current mesh's bounding box
        for (int i = 0; i < currentVisibleMesh.boundingBoxFaces.size(); i++){

                    // Find an intersection point with the bounding box, if it exists:
            if (    ( getPolyPlaneIntersectionPoint(&currentPosition, lightDirection, &currentVisibleMesh.boundingBoxFaces[i].vertices[0], &currentVisibleMesh.boundingBoxFaces[i].faceNormal, intersectionResult ) )

                    // Ensure the intersection is between the currentPosition and the light:
                 && ( (*intersectionResult - currentPosition).length() < lightDistance )

                    // Ensure the intersection point hit the bounding box
                 && ( pointIsInsidePoly( &currentVisibleMesh.boundingBoxFaces[i], intersectionResult ) )
                ) {
                        // We have a bounding box intersection hit! Loop through each visible face in the current mesh and find an actual intersection point:
                        for (int j = 0; j < currentVisibleMesh.faces.size(); j++){

                            // Skip the current polygon (as it always has an intersection)
                            if ( &currentVisibleMesh.faces[j] == currentPolygon )
                                continue;

                                // Find an actual intersection point, if it exists:
                            if ( ( getPolyPlaneBackFaceIntersectionPoint(&currentPosition, lightDirection, &currentVisibleMesh.faces[j].vertices[0], &currentVisibleMesh.faces[j].faceNormal, intersectionResult ) )

                                // Ensure the intersection is between the currentPosition and the light:
                                && ( (*intersectionResult - currentPosition).length() < lightDistance )

                                   // Check if the intersection point is inside of the polygon
                                && ( pointIsInsidePoly( &currentVisibleMesh.faces[j], intersectionResult ) )

                                 ){ // We've found an intersection!

                                    // Cleanup:
                                    delete intersectionResult;

                                    return true;

                            } // End current face checking ifs
                        } // End visible face checking loop

                        break; // Exit the bounding box loop: Don't bother checking any more bounding box faces, as we've already checked all of the visible faces once.

                    } // End if

        } // End bounding box face loop
    } // End mesh loop

    // Cleanup:
    delete intersectionResult;

    return false;
}

// Find the intersection point of a ray and the plane of a polygon. Used for finding intersections between shadow rays and faces shadowing a point.
// Return: True if the ray intersects, false otherwise. Modifies result Vertex to be the point of intersection, leaves it unchanged otherwise. Does not bother updating intersection point normal.
bool Renderer::getPolyPlaneBackFaceIntersectionPoint(Vertex* currentPosition, NormalVector* currentDirection, Vertex* planePoint, NormalVector* planeNormal, Vertex* intersectionResult){

    double currentDirectionDotPlaneNormal = currentDirection->dotProduct(*planeNormal);

    // Early out: Check if direction and poly plane are parallel (ie. == 0), or if we're hitting the FRONT face of the polygon (ie. < 0) (Light can pass through the back face, but not the front face
    if (currentDirectionDotPlaneNormal <= 0)
        return false;

    double distance = (*planePoint - *currentPosition).dot(*planeNormal)/(double)currentDirectionDotPlaneNormal;
    // Ensure the intersection is in front of the ray
    if (distance > 0.06){ // Offset by a small amount to avoid self/neighbour intersections
        *intersectionResult = (*currentPosition + (*currentDirection * distance));
        return true;
    }

    return false; // The intersection was behind the ray
}


// Find the intersection point of a ray and the plane of a polygon. Used to find intersections of bounced reflection rays with other polys
// Return: True if the ray intersects, false otherwise. Modifies result Vertex to be the point of intersection, leaves it unchanged otherwise. Note: The intersection point still needs an interpolated normal and color
bool Renderer::getPolyPlaneFrontFaceIntersectionPoint(Vertex* currentPosition, NormalVector* currentDirection, Vertex* planePoint, NormalVector* planeNormal, Vertex* intersectionResult){

    double currentDirectionDotPlaneNormal = currentDirection->dotProduct(*planeNormal);

    // Early out: Check if direction and poly plane are parallel (ie. == 0), or if we're hitting the BACK face of the polygon (ie. > 0) (Light can pass through the back face, but not the front face)
    if (currentDirectionDotPlaneNormal >= 0)
        return false;

    double distance = (*planePoint - *currentPosition).dot(*planeNormal)/(double)currentDirectionDotPlaneNormal;

    // Ensure the intersection is in front of the ray ( >0.1 to avoid intersections with neighbouring polys in the same mesh)
    if (distance > 0){
        *intersectionResult = (*currentPosition + (*currentDirection * distance));

        return true;
    }

    return false; // The intersection was behind the ray
}

// Find the intersection point of a ray and the plane of a polygon. Does not consider whether front or back face is being hit. Used for bounding box checks.
// Return: True if the ray intersects, false otherwise. Modifies result Vertex to be the point of intersection, leaves it unchanged otherwise
bool Renderer::getPolyPlaneIntersectionPoint(Vertex* currentPosition, NormalVector* currentDirection, Vertex* planePoint, NormalVector* planeNormal, Vertex* intersectionResult){

    double currentDirectionDotPlaneNormal = currentDirection->dotProduct(*planeNormal);

    // Early out: Check if direction and poly plane are parallel (ie. == 0)
    if (currentDirectionDotPlaneNormal == 0)
        return false;

    double distance = (*planePoint - *currentPosition).dot(*planeNormal)/(double)currentDirectionDotPlaneNormal;

    // Ensure the intersection is in front of the ray
    if (distance > 0){
        *intersectionResult = (*currentPosition + (*currentDirection * distance));
        return true;
    }

    return false; // The intersection was behind the ray
}

// Determine whether a point on a polygon's plane lies within the polygon
bool Renderer::pointIsInsidePoly(Polygon* thePolygon, Vertex* intersectionPoint){

    // Loop through each pair of vertices (ie. Edges), checking the point is inside the positive halfspace of the poly's plane
    bool isInside = true;
    for (int i = 0; i < thePolygon->getVertexCount() - 1; i++){

        NormalVector innerNormal(thePolygon->vertices[i].x - thePolygon->vertices[i+1].x, thePolygon->vertices[i].y - thePolygon->vertices[i+1].y, thePolygon->vertices[i].z - thePolygon->vertices[i+1].z);

        innerNormal = innerNormal.crossProduct(thePolygon->faceNormal);

        if ((*intersectionPoint - thePolygon->vertices[i]).dot(innerNormal) < 0){
            isInside = false;
            break;  // Don't bother continuing
        }
    }

    if (isInside){ // Only bother checking the last edge if there's a chance the point is inside the polygon:
        NormalVector innerNormal(thePolygon->vertices[thePolygon->getVertexCount() - 1].x - thePolygon->vertices[0].x, thePolygon->vertices[thePolygon->getVertexCount() - 1].y - thePolygon->vertices[0].y, thePolygon->vertices[thePolygon->getVertexCount() - 1].z - thePolygon->vertices[0].z);

        innerNormal = innerNormal.crossProduct(thePolygon->faceNormal);

        if ((*intersectionPoint - thePolygon->vertices[thePolygon->getVertexCount() - 1]).dot(innerNormal) < 0){
            isInside = false;
        }
    }

    return isInside;
}

// Calculate value of blending an existing pixel with a color, based on an opacity ratio
// Written color = opacity * color + (1 - opacity) * color at (x, y)
unsigned int Renderer::blendPixelValues(int x, int y, unsigned int color, float opacity){
    unsigned int currentColor = drawable->getPixel(x, y); // Sample the existing color

    // Return the blended sum
    return addColors (multiplyColorChannels(color, opacity), multiplyColorChannels(currentColor, (1 - opacity) ) ); // Calculate the blended colors
}

// Override: Lerp between the color values of 2 points (Perspective correct: Takes Z-Depth into account)
// Pre-condition: Recieved points are ordered left to right
// Return: An unsigned int color value, calculated based on a LERP of the current position between the 2 points
unsigned int Renderer::getPerspCorrectLerpColor(Vertex* p1, Vertex* p2, double ratio) const {
    // Handle solidly colored objects:
    if (p1->color == p2->color || ratio <= 0)
        return p1->color;

    if (ratio >= 1)
        return p2->color;

    double red = getPerspCorrectLerpValue(extractColorChannel(p1->color, 1), p1->z, extractColorChannel(p2->color, 1), p2->z, ratio);
    double green = getPerspCorrectLerpValue(extractColorChannel(p1->color, 2), p1->z, extractColorChannel(p2->color, 2), p2->z, ratio);
    double blue = getPerspCorrectLerpValue(extractColorChannel(p1->color, 3), p1->z, extractColorChannel(p2->color, 3), p2->z, ratio);

    return combineColorChannels(red, green, blue);
}

// Calculate fogged pixel value for a given pixel on a line between 2 points
// Pre-condition: Ambient lighting has already been applied to the vertex color values. All points/coords are in screen space
unsigned int Renderer::getFogPixelValue(Vertex* p1, Vertex* p2, double ratio, double correctZ) {

    // Apply distance fog to the base lerped color, and return the final value:
    return getDistanceFoggedColor( getPerspCorrectLerpColor(p1, p2, ratio) , correctZ );
}

// Calculate interpolated pixel and depth fog value
// Pre-condition: Z is in camera space
unsigned int Renderer::getDistanceFoggedColor(unsigned int pixelColor, double correctZ){

    // Handle objects too close for fog:
    if (correctZ <= currentScene->fogHither)
        return pixelColor;

    // Handle objects past the max fog distance:
    if (correctZ >= currentScene->fogYon)
        return currentScene->fogColor;

    // Lerp, based on the fog distance:
    double ratio = (correctZ - currentScene->fogHither) / (currentScene->fogYon - currentScene->fogHither);

    return addColors( multiplyColorChannels(pixelColor, (1 - ratio) ), multiplyColorChannels(currentScene->fogColor, ratio) );
}

// Reset the depth buffer
void Renderer::resetDepthBuffer(){
    for (int x = 0; x < xRes; x++){
        for (int y = 0; y < yRes; y++){
            ZBuffer[x][y] = maxZVal;
        }
    }
}

// Set a pixel on the raster
// Pre-condition: Point is a valid coordinate on the raster canvas and has been previously checked against the z-buffer
void Renderer::setPixel(int x, int y, double z, unsigned int color){

    // Flip the Y coordinate:
    y = yRes - y;

    // Update the frame buffer:
    drawable->setPixel(x, y, color);

    // Update the z buffer:
    ZBuffer[x][y] = getScaledZVal( z );
}

// Check if a pixel coordinate is in front of the current z-buffer depth
bool Renderer::isVisible(int x, int y, double z){
    return ( getScaledZVal( z ) < ZBuffer[x][yRes - y]);
}

// Get a scaled z-buffer value for a given Z
int Renderer::getScaledZVal(double correctZ){
    return (int)( (correctZ - currentScene->camHither)/(double)(currentScene->camYon - currentScene->camHither) * std::numeric_limits<int>::max() );
}

// Change the frustum shape
// Precondition: currentScene != nullptr
void Renderer::transformCamera(TransformationMatrix cameraMovement){

    // Reset the depth buffer
    resetDepthBuffer();

    worldToCamera = cameraMovement.getInverse(); // Store the inverse of the camera movements as the world->camera xform

    // Rebuild the toScreen matrix:
    perspectiveToScreen = TransformationMatrix(); // Reset to the identity matrix

    // Calculate highest resolution
    int highestResolution;
    if (xRes > yRes)
        highestResolution = xRes;
    else
        highestResolution = yRes;

    double highestXYDelta;
    if ((currentScene->xHigh - currentScene->xLow) > (currentScene->yHigh - currentScene->yLow))
        highestXYDelta = currentScene->xHigh - currentScene->xLow;
    else
        highestXYDelta = currentScene->yHigh - currentScene->yLow;

    // We build our matrix in reverse here, so each new xForm is on the right  [currentxForms] * [newXform]
    // Ie. Last xForm added is the first applied when we use this to transform a vector
    perspectiveToScreen.addTranslation(xRes/2, yRes/2, 0); // Shift local space origin to be centered at center of raster

    // Scale:
    perspectiveToScreen.addNonUniformScale( (highestResolution - (2 * border)) / (highestXYDelta), ( (highestResolution - (2 * border)) / (highestXYDelta) ), 1 ); // Scale

    // Center the camera within the xlow/ylow/xhigh/yhigh view window:
    perspectiveToScreen.addTranslation(-(currentScene->xHigh + currentScene->xLow)/2.0, -(currentScene->yHigh + currentScene->yLow)/2.0, 0);

    // Rebuild the matrix that transforms points from screen space back to perspective space:
    screenToPerspective = TransformationMatrix();        // Reset the matrix back to the identity
    screenToPerspective *= perspectiveToScreen;
    screenToPerspective = screenToPerspective.getInverse();
}

// Calculate a reflection of vector pointing away from a surface
NormalVector Renderer::reflectOutVector(NormalVector* faceNormal, NormalVector* outVector){

    NormalVector bounceDirection( *faceNormal );
    bounceDirection *= 2 * (faceNormal->dotProduct( *outVector ) );
    bounceDirection -= *outVector;
    bounceDirection.normalize();

    return bounceDirection;
}

// Update a raytracing intersection point with interpolated normals and color values
void Renderer::setInterpolatedIntersectionValues(Vertex* intersectionPoint, Polygon* hitPoly){

    // Find our position between the edges of the polygon:
    Vertex* topLeftVertex = hitPoly->getHighest();
    Vertex* topRightVertex = topLeftVertex; // Start edge traversal at the same point

    Vertex* botLeftVertex = hitPoly->getNext(topLeftVertex->vertexNumber); // Get the next/left vertex (using counter-clockwise ordering)
    Vertex* botRightVertex = hitPoly->getPrev(topRightVertex->vertexNumber); // Get the prev/right vertex

    // Handle multiple "top" vertices with the same y coordinate - Slide down the edge
    int count = hitPoly->getVertexCount(); // Track how many times we've moved, to prevent infinite loops when all vertices have the same y-coord.
    while (topLeftVertex->y == botLeftVertex->y && count > 0){
        count--;
        topLeftVertex = botLeftVertex;
        botLeftVertex = hitPoly->getNext(botLeftVertex->vertexNumber);
    }
    count = hitPoly->getVertexCount();
    while (topRightVertex->y == botRightVertex->y && count > 0){
        count--;
        topRightVertex = botRightVertex;
        botRightVertex = hitPoly->getPrev(botRightVertex->vertexNumber);
    }

    // Move the pointers to encompass the correct edge:
    count = hitPoly->getVertexCount();
    while (!(topLeftVertex->y >= intersectionPoint->y && botLeftVertex->y <= intersectionPoint->y) && count > 0){
        count--;
        topLeftVertex = botLeftVertex;
        botLeftVertex = hitPoly->getNext(botLeftVertex->vertexNumber);
    }
    count = hitPoly->getVertexCount();
    while (!(topRightVertex->y >= intersectionPoint->y && botRightVertex->y <= intersectionPoint->y) && count > 0){
        count--;
        topRightVertex = botRightVertex;
        botRightVertex = hitPoly->getPrev(botRightVertex->vertexNumber);
    }

    // Find how far along each edge we are, as a ratio:
    double leftRatio = (intersectionPoint->y - topLeftVertex->y)/(double)(botLeftVertex->y - topLeftVertex->y); // How far from top to bot we are
    double rightRatio = (intersectionPoint->y - topRightVertex->y)/(double)(botRightVertex->y - topRightVertex->y);

    // Calculate normals on the edges:
    NormalVector leftNml;
    leftNml.xn = ((1 - leftRatio) * topLeftVertex->normal.xn) + (leftRatio * botLeftVertex->normal.xn);
    leftNml.yn = ((1 - leftRatio) * topLeftVertex->normal.yn) + (leftRatio * botLeftVertex->normal.yn);
    leftNml.zn = ((1 - leftRatio) * topLeftVertex->normal.zn) + (leftRatio * botLeftVertex->normal.zn);
    leftNml.normalize();

    NormalVector rightNml;
    rightNml.xn = ((1 - rightRatio) * topRightVertex->normal.xn) + (rightRatio * botRightVertex->normal.xn);
    rightNml.yn = ((1 - rightRatio) * topRightVertex->normal.yn) + (rightRatio * botRightVertex->normal.yn);
    rightNml.zn = ((1 - rightRatio) * topRightVertex->normal.zn) + (rightRatio * botRightVertex->normal.zn);
    rightNml.normalize();

    // Calculate edge colors:
    unsigned int leftColor = addColors(
                                        multiplyColorChannels(topLeftVertex->color, 1 - leftRatio ),
                                        multiplyColorChannels(botLeftVertex->color, leftRatio)
                );

    unsigned int rightColor = addColors(
                                        multiplyColorChannels(topRightVertex->color, 1 - rightRatio ),
                                        multiplyColorChannels(botRightVertex->color, rightRatio)
                );

    // Calculate the x positions along the polygon edges:

    double leftX, rightX;
    double leftYDiff = botLeftVertex->y - topLeftVertex->y;
    double rightYDiff = botRightVertex->y - topRightVertex->y;

    if (leftYDiff != 0){
        leftX = ((intersectionPoint->y - topLeftVertex->y) * ((botLeftVertex->x - topLeftVertex->x)/leftYDiff) + topLeftVertex->x);
    }
    else
        leftX = botLeftVertex->x;

    if (rightYDiff != 0){
        rightX = ((intersectionPoint->y - topRightVertex->y) * ((botRightVertex->x - topRightVertex->x)/rightYDiff) + topRightVertex->x);
    }
    else
        rightX = botRightVertex->x;

    // Calculate how far along the face the intersection point is, as a ratio:
    double pointRatio;
    double xDiff = rightX - leftX;
    if (xDiff != 0)
        pointRatio = (intersectionPoint->x - leftX)/(double)(xDiff);
    else
        pointRatio = 0;


    // Set the normal:
    intersectionPoint->normal.xn = ((1 - pointRatio) * leftNml.xn) + (pointRatio * rightNml.xn);
    intersectionPoint->normal.yn = ((1 - pointRatio) * leftNml.yn) + (pointRatio * rightNml.yn);
    intersectionPoint->normal.zn = ((1 - pointRatio) * leftNml.zn) + (pointRatio * rightNml.zn);
    intersectionPoint->normal.normalize();

    // Set the color:
    intersectionPoint->color = addColors(
                                        multiplyColorChannels(leftColor, 1 - pointRatio ),
                                        multiplyColorChannels(rightColor, pointRatio )
    );

}

// Check if 2 polygons share an edge
bool Renderer::haveSharedEdge(Polygon* poly1, Polygon* poly2){
    int count = 0;
    for (int i = 0; i < poly1->getVertexCount(); i++){
        for (int j = 0; j < poly2->getVertexCount(); j++){
            if (poly1->vertices[i] == poly2->vertices[j]){
                count++;
                if (count >= 2)
                    return true;

                break;      // No need to keep checking
            }
        }
    }
    return false;
}

// Check if the angle between 2 polygon faces that share an edge is greater than 180 degrees
bool Renderer::isFaceReflexAngle(Polygon* currentPoly, Polygon* hitPoly){
    // Find a common edge
    Vertex* notCommon = nullptr;
    for (int i = 0; i < hitPoly->getVertexCount(); i++){

        bool foundCommon = false;

        for (int j = 0; j < currentPoly->getVertexCount(); j++){

            if (hitPoly->vertices[i] == currentPoly->vertices[j]){
                foundCommon = true;

                break;  // No need to keep checking currentPoly verts for the current hitPoly vert
            }
        }

        // If we've checked every vertex in currentPoly without finding a match, the current hitPoly vert is not part of a shared edge:
        if (!foundCommon){
            notCommon = &hitPoly->vertices[i];
            break;      // No need to keep checking hitPoly verts: We've found one that isn't part of a common edge
        }
    }

    // Ensure that we've found an uncommon vertex:
    if (notCommon == nullptr)
        return false;

    // Build a tangent vector along the face of the hitPoly from uncommon point towards common edge:
    NormalVector faceTangent(hitPoly->getNext(notCommon->vertexNumber)->x - notCommon->x, hitPoly->getNext(notCommon->vertexNumber)->y - notCommon->y, hitPoly->getNext(notCommon->vertexNumber)->z - notCommon->z);
    faceTangent.normalize();

    // Check the angle:
    if (currentPoly->faceNormal.dotProduct(faceTangent) > 0)
        return true;
    else
        return false;
}

// Visually debug lights:
void Renderer::debugLights(){

    for (unsigned int i = 0; i < currentScene->theLights.size(); i++){
        Light debug = currentScene->theLights[i];
        debug.position.transform(&cameraToPerspective);
        debug.position.transform(&perspectiveToScreen);
        drawLine( Line(Vertex(debug.position.x - 15, debug.position.y, debug.position.z, 0xffff0000), Vertex(debug.position.x + 15, debug.position.y, debug.position.z, 0xffff0000) ), ambientOnly, true, 0, 0);
        drawLine( Line(Vertex(debug.position.x, debug.position.y - 15, debug.position.z, 0xff00ff00), Vertex(debug.position.x, debug.position.y + 15, debug.position.z, 0xff00ff00) ), ambientOnly, true, 0, 0);
        drawLine( Line(Vertex(debug.position.x, debug.position.y, debug.position.z - 15, 0xffff00ff), Vertex(debug.position.x, debug.position.y, debug.position.z + 15, 0xffff00ff) ), ambientOnly, true, 0, 0);

        debug.debug();
    }
}
