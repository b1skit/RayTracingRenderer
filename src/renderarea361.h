#ifndef RENDERAREA_H
#define RENDERAREA_H

#include <QWidget>
#include "drawable.h"

class RenderArea361 : public QWidget, public Drawable
{
    Q_OBJECT
public:
    explicit RenderArea361(QWidget *parent = 0);
    QSize sizeHint() const;
    void setPixel(int x, int y, uint color);
    uint getPixel(int x, int y);
    void updateScreen();

protected:
    void paintEvent(QPaintEvent *event);

signals:

public slots:

private:
    QImage image;

};

#endif // RENDERAREA_H
