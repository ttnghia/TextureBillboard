#-------------------------------------------------
#
# Project created by QtCreator 2015-01-17T09:10:42
#
#-------------------------------------------------

QT       += core gui
QT += opengl
greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = TextureBillboard
TEMPLATE = app

#QMAKE_CXXFLAGS_WARN_ON += -Wno-reorder

SOURCES += main.cpp\
        mainwindow.cpp \
    unitplane.cpp \
    renderer.cpp

HEADERS  += mainwindow.h \
    unitplane.h \
    renderer.h

RESOURCES += \
    shaders.qrc \
    textures.qrc
