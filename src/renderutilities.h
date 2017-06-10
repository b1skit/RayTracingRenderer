// Non-member utility functions
// By Adam Badke

#ifndef RENDERUTILITIES_H
#define RENDERUTILITIES_H

// Adjust a color by multiplying by some ratio
// Return: A 32 bit ARGB value, each channel modified by the given ratio
unsigned int multiplyColorChannels(unsigned int color, double ratio);

// Adjust a color per channel by multiplying by a set of channel ratios
unsigned int multiplyColorChannels(unsigned int color, double alphaRatio, double redRatio, double greenRatio, double blueRatio);

// Multiply a color by a set of packed color channels
unsigned int multiplyColorChannels(unsigned int color, unsigned int intensities);

// Add 2 colors together (without overflowing)
// Return: An unsigned int of 2 colors added together, channel by channel
unsigned int addColors(unsigned int color1, unsigned int color2);

// Get a random ARGB color
// RETURN: An unsigned int containing an ARGB color, with A = FF/100%
unsigned int getRandomColor();

// Combine color channels into a single unsigned int
unsigned int combineColorChannels(double red, double green, double blue);

// Extract a color channel as a double [0, 1]
// Channel flags: 0 = alpha, 1 = red, 2 = green, 3 = blue
double extractColorChannel(unsigned int color, int channel);

// Calculate a perspective correct linear interpolation of some value
double getPerspCorrectLerpValue(double startVal, double startZ, double endVal, double endZ, double ratio);

#endif // RENDERUTILITIES_H
