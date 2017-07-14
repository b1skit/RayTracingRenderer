// Renderer client object
// By Adam Badke

#ifndef CLIENT_H
#define CLIENT_H
#include "drawable.h"
#include "pageturner.h"

#include "renderer.h"
#include "fileinterpreter.h"

using std::string;

// Client object:
class Client : public PageTurner
{
public:
    // Default constructor
    Client();

    // Constructor
    Client(Drawable *drawable);

    // Command Line Constructor:
    Client(Drawable *drawable, string filename);

    // Turn the window's page
    void nextPage();

private:
    // Client variables and parameters:
    // ********************************

    Drawable *drawable;                     // Drawable object: Passed to the renderer
    Renderer* clientRenderer;               // The renderer
    FileInterpreter clientFileInterpreter;  // The file interpreter

    // Render window x, y resolution (in px):
    const int xRes = 1000; // Must be the same as the values in renderarea361.cpp
    const int yRes = 1000;

    const int PANEL_BORDER_WIDTH = 1;  // The width of the panel borders. Must be > 0 ????????????????

    // Command line arguments:
    bool commandLineMode;
    string filename;
};

#endif // CLIENT_H
