#include <iostream>
#include <QGridLayout>
#include "window361.h"

Window361::Window361()
{
    renderArea = new RenderArea361((QWidget *)0);
    nextPageButton = new QPushButton("next page");
    connect(nextPageButton, SIGNAL(clicked()), this, SLOT(nextPageClicked()));

    QGridLayout *layout = new QGridLayout;
    layout->addWidget(renderArea, 0, 0);
    layout->addWidget(nextPageButton, 0, 1);
    setLayout(layout);

    setWindowTitle(tr("361 Window"));
}
Drawable *Window361::getDrawable() {
    return renderArea;
}
Window361::~Window361() {
    delete renderArea;
    delete nextPageButton;
}

void Window361::setPageTurner(PageTurner *pageTurner) {
    this->pageTurner = pageTurner;
    pageTurner->nextPage();     // give them the change for the first page.
}

void Window361::nextPageClicked() {
    if(pageTurner != 0) {
        pageTurner->nextPage();
    }
}


