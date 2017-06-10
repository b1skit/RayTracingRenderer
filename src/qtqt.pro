#-------------------------------------------------
#
# Project created by QtCreator 2016-09-21T01:38:20
#
#-------------------------------------------------

QT       += core gui

CONFIG+=c++11

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = qtqt
TEMPLATE = app


SOURCES += main.cpp\
    client.cpp \
    window361.cpp \
    renderarea361.cpp \
    renderer.cpp \
    polygon.cpp \
    line.cpp \
    vertex.cpp \
    fileinterpreter.cpp \
    mesh.cpp \
    transformationmatrix.cpp \
    renderutilities.cpp \
    normalvector.cpp \
    light.cpp

HEADERS  += \
    drawable.h \
    pageturner.h \
    client.h \
    window361.h \
    renderarea361.h \
    renderer.h \
    polygon.h \
    line.h \
    vertex.h \
    fileinterpreter.h \
    mesh.h \
    transformationmatrix.h \
    renderutilities.h \
    normalvector.h \
    light.h

