#ifndef DRAWABLE_H
#define DRAWABLE_H

class Drawable
{
public:
    virtual void setPixel(int x, int y, unsigned int color) = 0;
    virtual unsigned int getPixel(int x, int y) = 0;
    virtual void updateScreen() = 0;
};

#endif // DRAWABLE_H
