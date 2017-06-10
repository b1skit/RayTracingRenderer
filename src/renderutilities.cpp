// Non-member utility functions
// By Adam Badke

#include "renderutilities.h"
#include "math.h"
#include <iostream>

using std::cout;

// Adjust a color by multiplying by some ratio
// Return: A 32 bit ARGB value, each channel modified by the given ratio
unsigned int multiplyColorChannels(unsigned int color, double ratio){
    // Clamp the ratio value:
    if (ratio > 1)
        ratio = 1;

    unsigned int mask = 0x000000ff; // Initialize the mask, starting with the blue channel
    unsigned int channel;
    unsigned int result = 0; // Initialize the result
    for (int i = 0; i < 4; i++){ // Loop for each of the 4 channels
        // Isolate the channel bits
        channel = color & mask;
        channel = channel >> (8 * i);
        channel = (unsigned int)round( channel * ratio ); // Adjust the channel value: Round to maintain integer accuracy
        channel = channel << (8 * i); // Shift the channel back into the correct position
        result += channel; // Add the channel to the result
        mask = mask << 8; // Shift the mask for the next iteration
    }
    return result;
}

// Adjust a color per channel by multiplying by a set of channel ratios
unsigned int multiplyColorChannels(unsigned int color, double alphaRatio, double redRatio, double greenRatio, double blueRatio){   
    // Prevent overflows:
    if (alphaRatio > 1)
        alphaRatio = 1;
    if (redRatio > 1)
        redRatio = 1;
    if (greenRatio > 1)
        greenRatio = 1;
    if (blueRatio > 1)
        blueRatio = 1;

    unsigned int mask = 0x000000ff; // Initialize the mask, starting with the blue channel
    unsigned int channel;
    unsigned int result = 0; // Initialize the result
    for (int i = 0; i < 4; i++){ // Loop for each of the 4 channels
        // Isolate the channel bits
        channel = color & mask;
        channel = channel >> (8 * i);

        switch (i){ // Adjust the channel values: Round to maintain integer accuracy
        case 0: // Blue
            channel = (unsigned int)round( channel * blueRatio); // Adjust the channel value: Round to maintain integer accuracy
            break;

        case 1: // Green
            channel = (unsigned int)round( channel * greenRatio); // Adjust the channel value: Round to maintain integer accuracy
            break;

        case 2: // Red
            channel = (unsigned int)round( channel * redRatio); // Adjust the channel value: Round to maintain integer accuracy
            break;

        case 3: // Alpha
            channel = (unsigned int)round( channel * alphaRatio); // Adjust the channel value: Round to maintain integer accuracy
            break;
        }
        channel = channel << (8 * i); // Shift the channel back into the correct position
        result += channel; // Add the channel to the result
        mask = mask << 8; // Shift the mask for the next iteration
    }
    return result;
}

// Multiply a color by a set of packed color channels
unsigned int multiplyColorChannels(unsigned int color, unsigned int intensities){
    return multiplyColorChannels(color, 1, extractColorChannel(intensities, 1), extractColorChannel(intensities, 2), extractColorChannel(intensities, 3));
}

// Add 2 colors together (without overflowing)
// Return: An unsigned int of 2 colors added together, channel by channel
unsigned int addColors(unsigned int color1, unsigned int color2){
    unsigned int mask = 0x000000ff; // Initialize the mask, starting with the blue channel
    unsigned int channel1;
    unsigned int channel2;
    unsigned int finalChannel;
    unsigned int result = 0; // Initialize the result value to 0
    for (int i = 0; i < 4; i++){ // Loop for each of the 4 channels
        // Isolate each channel's individual bits
        channel1 = color1 & mask;
        channel2 = color2 & mask;
        channel1 = channel1 >> (8 * i);
        channel2 = channel2 >> (8 * i);

        finalChannel = channel1 + channel2; // Add the channels together
        if (finalChannel > 0x000000ff) // Catch overflows
            finalChannel = 0x000000ff;

        finalChannel = finalChannel << (8 * i); // Shift the channel back into the correct position
        result += finalChannel; // Add the channel to the result
        mask = mask << 8; // Shift the mask for the next iteration
    }
    return result;
}

// Get a random ARGB color
// RETURN: An unsigned int containing an ARGB color, with A = FF/100%
unsigned int getRandomColor()
{
    unsigned int color = 0;
    int bit_shift = 0; // Number of bits to shift
    for (int i = 0; i < 3; i++){ // Loop 3 times, 1 for each color channel
        int channel = rand() % 0x000000ff; // Generate a random channel value
        channel = channel << bit_shift; // Shift the value to the correct channel position
        color += channel; // Store the channel
        bit_shift += 8; // Adjust the shift amount for the next iteration
    }
    color += 0xff000000;

    return color;
}

// Combine color channels into a single unsigned int
unsigned int combineColorChannels(double red, double green, double blue){

    unsigned int intRed = 0x00ff0000;
    unsigned int intGreen = 0x0000ff00;
    unsigned int intBlue = 0x000000ff;

    // No need to check if red/green/blue are > 1.0, as it is checked in multiplyColorChannels()
    intRed = multiplyColorChannels(intRed, red);
    intGreen = multiplyColorChannels(intGreen, green);
    intBlue = multiplyColorChannels(intBlue, blue);

    return addColors(0xff000000, addColors( addColors(intRed, intGreen), intBlue) );
}

// Extract a color channel as a double [0, 1]
// Channel flags: 0 = alpha, 1 = red, 2 = green, 3 = blue
double extractColorChannel(unsigned int color, int channel){
    double result = 0;
    switch (channel){// No case required 0 for alpha
    case 1:
        color = color << 8;
        break;
    case 2:
        color = color << 16;
        break;
    case 3:
        color = color << 24;
        break;
    default:
        break;
    }
    result = (color >> 24) /(double) 255;
    return result;
}

// Calculate a perspective correct linear interpolation of some value
// Assumption: Ratio is [0, 1]
double getPerspCorrectLerpValue(double startVal, double startZ, double endVal, double endZ, double ratio){

    double oneMinusRatio = 1 - ratio;

    return ( (ratio  * startZ * endVal) + (oneMinusRatio * endZ * startVal) )
            / (double)( (ratio * startZ) + (oneMinusRatio * endZ));
}


