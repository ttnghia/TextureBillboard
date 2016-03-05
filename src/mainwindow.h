//------------------------------------------------------------------------------------------
// mainwindow.h
//
// Created on: 1/17/2015
//     Author: Nghia Truong
//
//------------------------------------------------------------------------------------------

#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QSPinbox>
#include <QtGui>
#include <QtWidgets>

#include "renderer.h"

class MainWindow : public QWidget
{
    Q_OBJECT

public:
    MainWindow(QWidget* parent = 0);

    QSize sizeHint() const;
    QSize minimumSizeHint() const;

protected:
    void keyPressEvent(QKeyEvent*);

    void setupGUI();

public slots:
    void changeTextureFilteringMode();

private:

    Renderer* renderer;


    QMap<QString, QOpenGLTexture::Filter> str2TextureFilteringMap;
    QComboBox* cbTextureFiltering;


    QCheckBox* chkTextureAnisotropicFiltering;
    QCheckBox* chkEnableDepthTest;
    QCheckBox* chkEnableZAxisRotation;
    QSlider* sldPlaneSize;

};



#endif // MAINWINDOW_H
