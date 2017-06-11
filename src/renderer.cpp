// Renderer object
// By Adam Badke

#include "renderer.h"
#include "renderutilities.h"

#include <cmath>
#include <iostream>

// STL includes:
#define _USE_MATH_DEFINES       // Allow use of M_PI (3.14159265358979323846)
#include "math.h"               // The STL math library

using std::round;
using std::cout;

// Constructor
Renderer::Renderer(Drawable* newDrawable, int newXRes, int newYRes, int borderWidth){
    this->drawable = newDrawable;

    border = borderWidth;
    xRes = newXRes;
    yRes = newYRes;

    // Calculate pixel clipping values
    xDrawMin = border;
    xDrawMax = xRes - border;
    yDrawMin = border;
    yDrawMax = yRes - border;


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

    // Calculate highest resolution
    int highestResolution;
    if (xRes > yRes)
        highestResolution = xRes;
    else
        highestResolution = yRes;

    double highestXYDelta;
    if ((xHigh - xLow) > (yHigh - yLow))
        highestXYDelta = xHigh - xLow;
    else
        highestXYDelta = yHigh - yLow;

    // We build our matrix in reverse here, so each new xForm is on the right  [currentxForms] * [newXform]
    // Ie. Last xForm added is the first applied when we use this to transform a vector
    perspectiveToScreen.addTranslation(xRes/2, yRes/2, 0); // Shift local space origin to be centered at center of raster

    // Scale:
    perspectiveToScreen.addNonUniformScale( (highestResolution - (2 * border)) / (highestXYDelta), ( (highestResolution - (2 * border)) / (highestXYDelta) ), 1 ); // Scale

    // Center the camera within the xlow/ylow/xhigh/yhigh view window:
    perspectiveToScreen.addTranslation(-(xHigh + xLow)/2.0, -(yHigh + yLow)/2.0, 0);

    // Store the inverse, to return points back to perspective space:
    screenToPerspective = perspectiveToScreen.getInverse();
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
// Assumption: topLeft_ and botRight_ coords are valid, and in UI window space (ie. (0,0) is in the top left of the screen!)
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

                    // Calculate the lit pixel value, apply distance fog then attempt to set it:
                    lightPointInCameraSpace(&currentPosition, doAmbient, specularExponent, specularCoefficient);
                    setPixel((int)theLine.p1.x, y, correctZ, getDistanceFoggedColor( currentPosition.color, correctZ ) );

                }
                else
                    setPixel((int)theLine.p1.x, (int)y, correctZ, getFogPixelValue(&theLine.p1, &theLine.p2, ratio, correctZ) );
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

                        // Calculate the lit pixel value, apply distance fog then attempt to set it:
                        lightPointInCameraSpace(&currentPosition, doAmbient, specularExponent, specularCoefficient);
                        setPixel(round_x, y, correctZ, getDistanceFoggedColor( currentPosition.color, correctZ ) );
                    }
                    else
                        setPixel(round_x, y, correctZ, getFogPixelValue(&theLine.p1, &theLine.p2, ratio, correctZ) );
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

                        // Calculate the lit pixel value, apply distance fog then attempt to set it:
                        lightPointInCameraSpace(&currentPosition, doAmbient, specularExponent, specularCoefficient);
                        setPixel(x, round_y, correctZ, getDistanceFoggedColor( currentPosition.color, correctZ ) );
                    }
                    else
                        setPixel(x, round_y, correctZ, getFogPixelValue(&theLine.p1, &theLine.p2, ratio, correctZ) );
                }

                y += slope;
                z += z_slope;
            }
        }
    } // End non-vertical line else

    // Update the screen:
    drawable->updateScreen();
}




//// Draw a line (Bresenham's Algorithm)
//void Renderer::draw_lineBRES(Line theLine, unsigned int color, bool doLerp){

//    Vertex *p1 = &theLine.p1;
//    Vertex *p2 = &theLine.p2;

//    // TO DO:
//    // ******
//    // Z-Buffer checks
//    // Pass pointers instead of value
//    // Incremental ratio values (for correct z)
//    // Bresenham's incrementation of Z values...?

//    double z, ZDiff;

//    // Handle vertical lines:
//    if (p1->x == p2->x){
//        if (p1->y < p2->y){ // p1 is lower

//            z = p1->z;

//            for (int y = (int)p1->y; y < (int)p2->y; y++){

//               // ZDiff =

//                double ratio = (y - p1->y)/(double)(p2->y - p1->y); // Get our current position as a ratio
//                double correctZ = getPerspCorrectLerpValue(p1->z, p1->z, p2->z, p2->z, ratio);

//                setPixel((int)p1->x, y, correctZ, getPerspCorrectLerpColor(p1, p2, p1->x, y, p1->z) );

//            }
//        } else { // p2 is lower
//            for (int y = (int)p2->y; y < (int)p1->y; y++){

//                double ratio = (y - p2->y)/(double)(p1->y - p2->y); // Get our current position as a ratio
//                double correctZ = getPerspCorrectLerpValue(p2->z, p2->z, p1->z, p1->z, ratio); // IS THIS CORRECT?????????????????????????


//                drawable->setPixel(p1->x, y, correctZ, getPerspCorrectLerpColor(p1, p2, p1->x, y)); // ??????????????
//            }
//        }
//        return; // We're done! No need to continue.
//    }

//    // Handle non-vertical lines:
//    // Flip the points, so we're always drawing left to right
//    if (p1->x > p2->x){
//        int temp = p1->x;
//        p1->x = p2->x;
//        p2->x = temp;
//        temp = p1->y;
//        p1->y = p2->y;
//        p2->y = temp;

//        unsigned int colorTemp = p1->color;
//        p1->color = p2->color;
//        p2->color = colorTemp;
//    }

//    // Determine which octant we're in:
//    int diff_x = p2->x - p1->x;
//    int diff_y = p2->y - p1->y;
//    int abs_x = abs(diff_x);
//    int abs_y = abs(diff_y);

//    // Octants I, V, II, VI
//    if (diff_y >= 0){

//        // Handle Octants II, VI
//        bool isOctII_VI = false;
//        if (abs_x <= abs_y){
//            // Transpose from II or VI into Octant I:
//            int temp = p1->x;
//            p1->x = p1->y;
//            p1->y = temp;
//            temp = p2->x;
//            p2->x = p2->y;
//            p2->y = temp;

//            isOctII_VI = true;
//        }

//        // Draw the line:
//        int dx = p2->x - p1->x;
//        int two_dx = 2 * dx;
//        int two_dy = 2 * (p2->y - p1->y);
//        int two_dy_minus_two_dx = two_dy - two_dx;
//        int error = two_dy - dx;
//        int y = p1->y;
//        for (int x = p1->x; x <= p2->x; x++){ // DRAWING LOOP
//            if (isOctII_VI) // Flip x, y for Octant II and VI
//                drawable->setPixel(y, x, getLerpColor(p1, p2, x, y)); // y, x !

//            else{ // Don't flip x, y for Octant I, V
//                    drawable->setPixel(x, y, getLerpColor(p1, p2, x, y)); // y, x !
//            }
//            if (error >= 0){
//                error += two_dy_minus_two_dx;
//                y++;
//            }
//            else{
//                error += two_dy;
//            }
//        } // End for
//    } // End octants I, V, II, VI

//    else { // Octants III, VII, IV, VIII

//        // Detect Octants III, VII
//        bool isOctIII_VII = false;
//        if (abs_x < abs_y){
//            // Transpose to Octant VIII (maintaining left to right point order):
//            int temp = p1->x;
//            p1->x = p2->y;
//            p2->y = temp;
//            temp = p1->y;
//            p1->y = p2->x;
//            p2->x = temp;

//            unsigned int colorTemp = p1->color;
//            p1->color = p2->color;
//            p2->color = colorTemp;

//            isOctIII_VII = true;
//        }

//        // Draw the line:
//        int dx = p2->x - p1->x;
//        int two_dx = 2 * dx;
//        int two_dy = 2 * (p1->y - p2->y); // Swapped order
//        int two_dy_minus_two_dx = two_dy - two_dx;
//        int error = two_dy - dx;
//        int y = p1->y;
//        for (int x = p1->x; x <= p2->x; x++){ // DRAWING LOOP
//            if (isOctIII_VII)
//                    drawable->setPixel(y, x, getLerpColor(p1, p2, x, y));
//            else {
//                    drawable->setPixel(x, y, getLerpColor(p1, p2, x, y));
//            }

//            if (error >= 0){
//                error += two_dy_minus_two_dx;
//                y--; // y--
//            }
//            else{
//                error += two_dy;
//            }
//        } // End for
//    } // End octants III, VII, IV, VIII

//    // Update the screen:
//    drawable->updateScreen();
//}






// Draw a polygon. Calls the rasterize Polygon helper function
// If thePolygon vertices are all not the same color, the color will be LERP'd
// Assumption: All polygons are in world space
void Renderer::drawPolygon(Polygon thePolygon){
    // Handle malformed polygons: Don't attempt to render them
    if (!thePolygon.isValid())
        return;

    // Transform to camera space
    thePolygon.transform(&worldToCamera);

    // Cull polygons outside of hither/yon
    if (!thePolygon.isInDepth(hither, yon)){
        return;
    }

    // Clip polygon to intersection with hither/yon
    thePolygon.clipHitherYon(hither, yon);

    // Handle polygons that have been rendered invalid by clipping
    if(!thePolygon.isValid())
        return;

    // Apply ambient lighting to Lines, if neccessary:
    if (thePolygon.isLine() && thePolygon.isAffectedByAmbientLight() ){
        thePolygon.lightAmbiently(ambientRedIntensity, ambientGreenIntensity, ambientBlueIntensity);
    }

    // Calculate flat/gouraud lighting (while we're still in camera space)
    // Handle flat:
    else if (thePolygon.getShadingModel() == flat && thePolygon.isFilled() && !thePolygon.isLine() ){ // Only light the polygon if it's not wireframe or a line
        flatShadePolygon( &thePolygon );
    }
    // Handle gouraud:
    else if (thePolygon.getShadingModel() == gouraud && thePolygon.isFilled() && !thePolygon.isLine()){ // Only light the polygon if it's not wireframe or a line
        gouraudShadePolygon( &thePolygon );
    }

    // Apply perspective xform:
    thePolygon.transform( &cameraToPerspective );

    // Backface cull: Don't attempt to render polygons not facing the camera
    if (!thePolygon.isFacingCamera() && thePolygon.isFilled() && !thePolygon.isLine()){ // Don't backface cull wireframe polygons or lines, as we still want to see them
      return;
    }

    // Polygon frustum culling: Cull polygons outside of xLow/xHigh/yLow/yHigh
    if (!thePolygon.isInFrustum(xLow, xHigh, yLow, yHigh)){
        return;
    }

    // Frustum clipping: Clip polygon to 4 sides of view window:
    thePolygon.clipToScreen(xLow, xHigh, yLow, yHigh);

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
        if ( theFaces->at(i).isFilled() ){
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
// Assumption: Received polygon is a triange, is in screen space, and all 3 vertices have been rounded to integer coordinates
void Renderer::rasterizePolygon(Polygon* thePolygon){
    // Get the vertices from the polygon:
    Vertex* topLeftVertex = thePolygon->getHighest();
    Vertex* topRightVertex = topLeftVertex; // Start edge traversal at the same point

    Vertex* botLeftVertex = thePolygon->getNext(topLeftVertex->vertexNumber); // Get the next/left vertex (using counter-clockwise ordering)
    Vertex* botRightVertex = thePolygon->getPrev(topRightVertex->vertexNumber); // Get the prev/right vertex

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
            lhs.normal = normalVector(topLeftVertex->normal, topLeftVertex->z, botLeftVertex->normal, botLeftVertex->z, y, topLeftVertex->y, botLeftVertex->y);

            Vertex rhs(xRight_rounded, (double)y, rightCorrectZ, getPerspCorrectLerpColor(topRightVertex, botRightVertex, rightRatio));
            rhs.normal = normalVector(topRightVertex->normal, topRightVertex->z, botRightVertex->normal, botRightVertex->z, y, topRightVertex->y, botRightVertex->y);

            drawPerPxLitScanlineIfVisible( &lhs, &rhs, thePolygon->isAffectedByAmbientLight(), thePolygon->getSpecularCoefficient(), thePolygon->getSpecularExponent());
        }
        else
            drawScanlineIfVisible( new Vertex(xLeft_rounded, (double)y, leftCorrectZ, getPerspCorrectLerpColor(topLeftVertex, botLeftVertex, leftRatio)),
                                   new Vertex(xRight_rounded, (double)y, rightCorrectZ, getPerspCorrectLerpColor(topRightVertex, botRightVertex, rightRatio))
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
// Assumption: All vertices have a valid normal
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
            ambientValues[i] = multiplyColorChannels(thePolygon->vertices[i].color, 1.0, ambientRedIntensity, ambientGreenIntensity, ambientBlueIntensity );
        }
    }

    // Get the center of the polygon face, as a point:
    Vertex faceCenter = thePolygon->getFaceCenter();

    // Calculate the average of the normals:
    normalVector faceNormal = thePolygon->getNormalAverage();

    // Loop through each light:
    for (unsigned int i = 0; i < theLights.size(); i++){

        // Get the (normalized) light direction vector: Points from the face towards the light
        normalVector lightDirection(theLights[i].position.x - faceCenter.x, theLights[i].position.y - faceCenter.y, theLights[i].position.z - faceCenter.z);
        lightDirection.normalize();

        // Get the cosine of the angle between the face normal and the light direction
        double faceNormalDotLightDirection = faceNormal.dotProduct(lightDirection);

        if (faceNormalDotLightDirection > 0){ // Only proceed if the angle < 90 degrees

            // Get the attenuation factor of the current light:
            double attenuationFactor = theLights[i].getAttenuationFactor(faceCenter);

            // Mutliply the light intensities by the attenuation:
            double redDiffuseIntensity = theLights[i].redIntensity * attenuationFactor;
            double greenDiffuseIntensity = theLights[i].greenIntensity * attenuationFactor;
            double blueDiffuseIntensity = theLights[i].blueIntensity * attenuationFactor;

            // Factor the cosine value into the light intensity values:
            redDiffuseIntensity *= faceNormalDotLightDirection;
            greenDiffuseIntensity *= faceNormalDotLightDirection;
            blueDiffuseIntensity *= faceNormalDotLightDirection;

            // Calculate the spec component:
            double redSpecIntensity = thePolygon->getSpecularCoefficient();
            double greenSpecIntensity = thePolygon->getSpecularCoefficient();
            double blueSpecIntensity = thePolygon->getSpecularCoefficient();

            // Create a view vector: Points from the face towards the camera
            normalVector viewVector(-faceCenter.x, -faceCenter.y, -faceCenter.z);
            viewVector.normalize();

            // Calculate the reflection vector:
            normalVector reflectionVector = faceNormal;
            reflectionVector *= 2 * faceNormalDotLightDirection;
            reflectionVector = reflectionVector - lightDirection;
            reflectionVector.normalize();

            // Calculate the cosine of the angle between the view vector and the reflection vector:
            double viewDotReflection = viewVector.dotProduct(reflectionVector);
            if (viewDotReflection < 0) // Clamp the value to be >=0
                viewDotReflection = 0;

            viewDotReflection = pow(viewDotReflection, thePolygon->getSpecularExponent() );

            redSpecIntensity *= (theLights[i].redIntensity * attenuationFactor * viewDotReflection);
            greenSpecIntensity *= (theLights[i].greenIntensity * attenuationFactor * viewDotReflection);
            blueSpecIntensity *= (theLights[i].blueIntensity * attenuationFactor * viewDotReflection);

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
        lightPointInCameraSpace(&thePolygon->vertices[i], thePolygon->isAffectedByAmbientLight(), thePolygon->getSpecularExponent(), thePolygon->getSpecularCoefficient() );
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
void Renderer::drawMesh(Mesh theMesh){
    vector<Polygon> theFaces = theMesh.getFaces();
    for (unsigned int i = 0; i < theFaces.size(); i++)
        drawPolygon(theFaces[i]);
}

// Draw a scanline, with consideration to the Z-Buffer
// Assumption: start and end vertices are in left to right order, and have been pre-rounded to integer coordinates
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
            setPixel(x, y_rounded, correctZ, getFogPixelValue(start, end, ratio, correctZ) );
        }

        z += z_slope;
        ratio += ratioDiff;
    }
}

// Draw a scanline using per-pixel lighting (ie Phong shading)
void Renderer::drawPerPxLitScanlineIfVisible(Vertex* start, Vertex* end, bool doAmbient, double specularCoefficient, double specularExponent){

    // Calculate the starting parameters:
    double zCameraSpace = start->z; // Z is in camera space
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
        if ( isVisible(x, y_rounded,  correctZ) ){

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

            // Calculate the lit pixel value, apply distance fog then set it:
            lightPointInCameraSpace(&currentPosition, doAmbient, specularExponent, specularCoefficient);
            setPixel(x, y_rounded, correctZ, getDistanceFoggedColor( currentPosition.color, correctZ ) );
        }

        ratio += ratioDiff;

        zCameraSpace += z_slope;
    }
}

// Light a given point in camera space
void Renderer::lightPointInCameraSpace(Vertex* currentPosition, bool doAmbient, double specularExponent, double specularCoefficient) {

    unsigned int ambientValue = 0;

    double redTotalDiffuseIntensity, greenTotalDiffuseIntensity, blueTotalDiffuseIntensity, redTotalSpecIntensity, greenTotalSpecIntensity, blueTotalSpecIntensity;
    redTotalDiffuseIntensity = greenTotalDiffuseIntensity = blueTotalDiffuseIntensity = redTotalSpecIntensity = greenTotalSpecIntensity = blueTotalSpecIntensity = 0;

    // Calculate the ambient component:
    if (doAmbient ){
        ambientValue = multiplyColorChannels(currentPosition->color, 1.0, ambientRedIntensity, ambientGreenIntensity, ambientBlueIntensity );
    }

    // Loop through each light the renderer is holding:
    for (unsigned int i = 0; i < theLights.size(); i++){

        // Get the (normalized) light direction vector: Points from the face towards the light
        normalVector lightDirection(theLights[i].position.x - currentPosition->x, theLights[i].position.y - currentPosition->y, theLights[i].position.z - currentPosition->z);
        lightDirection.normalize();

        // Get the cosine of the angle between the face normal and the light direction
        double currentNormalDotLightDirection = currentPosition->normal.dotProduct(lightDirection);

        if (currentNormalDotLightDirection > 0){

            // Get the attenuation factor of the current light:
            double attenuationFactor = theLights[i].getAttenuationFactor(*currentPosition);

            // Mutliply the light intensities by the attenuation:
            double redDiffuseIntensity = theLights[i].redIntensity * attenuationFactor;
            double greenDiffuseIntensity = theLights[i].greenIntensity * attenuationFactor;
            double blueDiffuseIntensity = theLights[i].blueIntensity * attenuationFactor;

            // Factor the cosine value into the light intensity values:
            redDiffuseIntensity *= currentNormalDotLightDirection;
            greenDiffuseIntensity *= currentNormalDotLightDirection;
            blueDiffuseIntensity *= currentNormalDotLightDirection;

            // Add the diffuse intensity for the current light to the running totals
            redTotalDiffuseIntensity += redDiffuseIntensity;
            greenTotalDiffuseIntensity += greenDiffuseIntensity;
            blueTotalDiffuseIntensity += blueDiffuseIntensity;

            // Calculate the spec component:
            double redSpecIntensity = specularCoefficient;
            double greenSpecIntensity = specularCoefficient;
            double blueSpecIntensity = specularCoefficient;

            // Create a view vector: Points from the face towards the camera
            normalVector viewVector(-currentPosition->x, -currentPosition->y, -currentPosition->z);
            viewVector.normalize();

            // Calculate the reflection vector:
            normalVector reflectionVector = currentPosition->normal;
            reflectionVector *= 2 * currentNormalDotLightDirection;
            reflectionVector = reflectionVector - lightDirection;
            reflectionVector.normalize();

            // Calculate the cosine of the angle between the view vector and the reflection vector:
            double viewDotReflection = viewVector.dotProduct(reflectionVector);
            if (viewDotReflection < 0) // Clamp the value to be >=0
                viewDotReflection = 0;

            viewDotReflection = pow(viewDotReflection, specularExponent );

            redSpecIntensity *= (theLights[i].redIntensity * attenuationFactor * viewDotReflection);
            greenSpecIntensity *= (theLights[i].greenIntensity * attenuationFactor * viewDotReflection);
            blueSpecIntensity *= (theLights[i].blueIntensity * attenuationFactor * viewDotReflection);


            // Add the final diffuse/spec values to the running totals:

            redTotalSpecIntensity += redSpecIntensity;
            greenTotalSpecIntensity += greenSpecIntensity;
            blueTotalSpecIntensity += blueSpecIntensity;

        } // end if
    } // End lights loop

    // Combine the color values, and assign their sum as the color of the currentPosition:
    currentPosition->color = addColors(      ambientValue,
                                            addColors(
                                                multiplyColorChannels( currentPosition->color, 1.0, redTotalDiffuseIntensity, greenTotalDiffuseIntensity, blueTotalDiffuseIntensity ),
                                                combineColorChannels( redTotalSpecIntensity, greenTotalSpecIntensity, blueTotalSpecIntensity ) )
                                              );
}

// Calculate value of blending an existing pixel with a color, based on an opacity ratio
// Written color = opacity * color + (1 - opacity) * color at (x, y)
unsigned int Renderer::blendPixelValues(int x, int y, unsigned int color, float opacity){
    unsigned int currentColor = drawable->getPixel(x, y); // Sample the existing color

    // Return the blended sum
    return addColors (multiplyColorChannels(color, opacity), multiplyColorChannels(currentColor, (1 - opacity) ) ); // Calculate the blended colors
}

// Override: Lerp between the color values of 2 points (Perspective correct: Takes Z-Depth into account)
// Assumption: Recieved points are ordered left to right
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
// Assumption: Ambient lighting has already been applied to the vertex color values. All points/coords are in screen space
unsigned int Renderer::getFogPixelValue(Vertex* p1, Vertex* p2, double ratio, double correctZ) {

    // Apply distance fog to the base lerped color, and return the final value:
    return getDistanceFoggedColor( getPerspCorrectLerpColor(p1, p2, ratio) , correctZ );
}

// Calculate interpolated pixel and depth fog value
// Assumption: Z is in camera space
unsigned int Renderer::getDistanceFoggedColor(unsigned int pixelColor, double correctZ){

    // Handle objects too close for fog:
    if (correctZ <= fogHither)
        return pixelColor;

    // Handle objects past the max fog distance:
    if (correctZ >= fogYon)
        return fogColor;

    // Lerp, based on the fog distance:
    double ratio = (correctZ - fogHither) / (fogYon - fogHither);

    return addColors( multiplyColorChannels(pixelColor, (1 - ratio) ), multiplyColorChannels(fogColor, ratio) );
}

// Get the renderer's clip depth
double Renderer::getClipDepth(){
    return yon;
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
// Assumption: Point is a valid coordinate on the raster canvas and has been previously checked against the z-buffer
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
    return (int)( (correctZ - hither)/(double)(yon - hither) * std::numeric_limits<int>::max() );
}

// Change the frustum shape
void Renderer::transformCamera(double newXLow, double newYLow, double newXHigh, double newYHigh, double newHither, double newYon, TransformationMatrix cameraMovement){

    // Store the recieved values
    xLow = newXLow;
    yLow = newYLow;
    xHigh = newXHigh;
    yHigh = newYHigh;
    hither = newHither;
    yon = newYon;

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
    if ((xHigh - xLow) > (yHigh - yLow))
        highestXYDelta = xHigh - xLow;
    else
        highestXYDelta = yHigh - yLow;

    // We build our matrix in reverse here, so each new xForm is on the right  [currentxForms] * [newXform]
    // Ie. Last xForm added is the first applied when we use this to transform a vector
    perspectiveToScreen.addTranslation(xRes/2, yRes/2, 0); // Shift local space origin to be centered at center of raster

    // Scale:
    perspectiveToScreen.addNonUniformScale( (highestResolution - (2 * border)) / (highestXYDelta), ( (highestResolution - (2 * border)) / (highestXYDelta) ), 1 ); // Scale

    // Center the camera within the xlow/ylow/xhigh/yhigh view window:
    perspectiveToScreen.addTranslation(-(xHigh + xLow)/2.0, -(yHigh + yLow)/2.0, 0);

    // Rebuild the matrix that transforms points from screen space back to perspective space:
    screenToPerspective = TransformationMatrix();        // Reset the matrix back to the identity
    screenToPerspective *= perspectiveToScreen;
    screenToPerspective = screenToPerspective.getInverse();

}

// Set the ambient color
void Renderer::setAmbientIntensities(double newRed, double newGreen, double newBlue){
    ambientRedIntensity = newRed;
    ambientGreenIntensity = newGreen;
    ambientBlueIntensity = newBlue;
}

// Set the distance fog parameters
void Renderer::setDistanceFog(double newHither, double newYon, double redIntensity, double greenIntensity, double blueIntensity){
    fogHither = newHither;
    fogYon = newYon;

    fogRedIntensity = redIntensity;
    fogGreenIntensity = greenIntensity;
    fogBlueIntensity = blueIntensity;

    // Compute and store the fog color, based on the recieved color channel intensities:
    fogColor = combineColorChannels(redIntensity, greenIntensity, blueIntensity );
}

// Flush the vector of light object that the renderer is currently holding
void Renderer::flushLights(){
    theLights.clear();

    // Reset the default near/far clip planes, in terms of Z:
    hither = 1;
    yon = 200;

    // Default values used for the depth command
    fogHither = yon;   // Set fogHither to current yon by default (ie. Don't apply depth fog)
    fogYon = yon;
}

// Add a new light to the renderer
// Assumption: The light is in world space
void Renderer::addLight(Light newLight){

    // Transform the light from world space to camera space:
    newLight.position.transform(&worldToCamera);

    theLights.emplace_back(newLight);
}

// Visually debug lights:
void Renderer::debugLights(){

    for (unsigned int i = 0; i < theLights.size(); i++){
        Light debug = theLights[i];
        debug.position.transform(&cameraToPerspective);
        debug.position.transform(&perspectiveToScreen);
        drawLine( Line(Vertex(debug.position.x - 15, debug.position.y, debug.position.z, 0xffff0000), Vertex(debug.position.x + 15, debug.position.y, debug.position.z, 0xffff0000) ), ambientOnly, true, 0, 0);
        drawLine( Line(Vertex(debug.position.x, debug.position.y - 15, debug.position.z, 0xff00ff00), Vertex(debug.position.x, debug.position.y + 15, debug.position.z, 0xff00ff00) ), ambientOnly, true, 0, 0);
        drawLine( Line(Vertex(debug.position.x, debug.position.y, debug.position.z - 15, 0xffff00ff), Vertex(debug.position.x, debug.position.y, debug.position.z + 15, 0xffff00ff) ), ambientOnly, true, 0, 0);

        debug.debug();
    }
}
