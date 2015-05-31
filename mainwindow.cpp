//------------------------------------------------------------------------------------------
// mainwindow.cpp
//
// Created on: 1/17/2015
//     Author: Nghia Truong
//
//------------------------------------------------------------------------------------------

#include "mainwindow.h"

MainWindow::MainWindow(QWidget* parent) : QWidget(parent)
{
    setWindowTitle("Texture Billboard");

    setupGUI();


    // Update continuously
    QTimer* timer = new QTimer(this);
    connect(timer, SIGNAL(timeout()), renderer, SLOT(update()));
    timer->start(0);
}

//------------------------------------------------------------------------------------------
QSize MainWindow::sizeHint() const
{
    return QSize(1600, 1200);
}

//------------------------------------------------------------------------------------------
QSize MainWindow::minimumSizeHint() const
{
    return QSize(50, 50);
}

//------------------------------------------------------------------------------------------
void MainWindow::keyPressEvent(QKeyEvent* e)
{
    switch(e->key())
    {
    case Qt::Key_Escape:
        close();
        break;

    case Qt::Key_R:
        renderer->resetCameraPosition();
        break;

    case Qt::Key_A:
        chkTextureAnisotropicFiltering->toggle();
        break;

    default:
        renderer->keyPressEvent(e);
    }
}


//------------------------------------------------------------------------------------------
void MainWindow::setupGUI()
{
    renderer = new Renderer(this);

    ////////////////////////////////////////////////////////////////////////////////
    // texture filtering mode
    cbTextureFiltering = new QComboBox;
    QString str;

    str = QString("NEAREST");
    cbTextureFiltering->addItem(str);
    str2TextureFilteringMap[str] = QOpenGLTexture::Nearest;

    str = QString("LINEAR");
    cbTextureFiltering->addItem(str);
    str2TextureFilteringMap[str] = QOpenGLTexture::Linear;


    str = QString("NEAREST_MIPMAP_NEAREST");
    cbTextureFiltering->addItem(str);
    str2TextureFilteringMap[str] = QOpenGLTexture::NearestMipMapNearest;


    str = QString("NEAREST_MIPMAP_LINEAR");
    cbTextureFiltering->addItem(str);
    str2TextureFilteringMap[str] = QOpenGLTexture::NearestMipMapLinear;


    str = QString("LINEAR_MIPMAP_NEAREST");
    cbTextureFiltering->addItem(str);
    str2TextureFilteringMap[str] = QOpenGLTexture::LinearMipMapNearest;

    str = QString("LINEAR_MIPMAP_LINEAR");
    cbTextureFiltering->addItem(str);
    str2TextureFilteringMap[str] = QOpenGLTexture::LinearMipMapLinear;



    QVBoxLayout* textureFilteringLayout = new QVBoxLayout;
    textureFilteringLayout->addWidget(cbTextureFiltering);
    QGroupBox* textureFilteringGroup = new QGroupBox("Floor Texture MinMag Filtering Mode");
    textureFilteringGroup->setLayout(textureFilteringLayout);

    cbTextureFiltering->setCurrentIndex(5);
    connect(cbTextureFiltering, SIGNAL(currentIndexChanged(int)), this,
            SLOT(changeTextureFilteringMode()));

    chkTextureAnisotropicFiltering = new QCheckBox("Texture Anisotropic Filtering");
    chkTextureAnisotropicFiltering->setChecked(true);
    connect(chkTextureAnisotropicFiltering, &QCheckBox::toggled, renderer, &Renderer::enableTextureAnisotropicFiltering);

    ////////////////////////////////////////////////////////////////////////////////
    // plane size
    sldPlaneSize = new QSlider(Qt::Horizontal);
    sldPlaneSize->setMinimum(1);
    sldPlaneSize->setMaximum(200);
    sldPlaneSize->setValue(10);

    connect(sldPlaneSize, &QSlider::valueChanged, renderer,
            &Renderer::changePlaneSize);

    QVBoxLayout* planeSizeLayout = new QVBoxLayout;
    planeSizeLayout->addWidget(sldPlaneSize);
    QGroupBox* planeSizeGroup = new QGroupBox("Plane Size");
    planeSizeGroup->setLayout(planeSizeLayout);



    ////////////////////////////////////////////////////////////////////////////////
    // others
    chkEnableDepthTest = new QCheckBox("Enable Depth Test");
    chkEnableDepthTest->setChecked(true);
    connect(chkEnableDepthTest, &QCheckBox::toggled, renderer,
            &Renderer::enableDepthTest);

    chkEnableZAxisRotation = new QCheckBox("Enable Z Axis Rotation");
    chkEnableZAxisRotation->setChecked(false);
    connect(chkEnableZAxisRotation, &QCheckBox::toggled, renderer,
            &Renderer::enableZAxisRotation);

    QPushButton* btnResetCamera = new QPushButton("Reset Camera");
    connect(btnResetCamera, &QPushButton::clicked, renderer,
            &Renderer::resetCameraPosition);


    ////////////////////////////////////////////////////////////////////////////////
    // Add slider group to parameter group
    QVBoxLayout* parameterLayout = new QVBoxLayout;
    parameterLayout->addWidget(textureFilteringGroup);
    parameterLayout->addWidget(chkTextureAnisotropicFiltering);
    parameterLayout->addWidget(planeSizeGroup);
    parameterLayout->addWidget(chkEnableDepthTest);
    parameterLayout->addWidget(chkEnableZAxisRotation);

    parameterLayout->addWidget(btnResetCamera);



    parameterLayout->addStretch();

    QGroupBox* parameterGroup = new QGroupBox;
    parameterGroup->setMinimumWidth(350);
    parameterGroup->setMaximumWidth(350);
    parameterGroup->setLayout(parameterLayout);



    QHBoxLayout* hLayout = new QHBoxLayout;
    hLayout->addWidget(renderer);
    hLayout->addWidget(parameterGroup);

    setLayout(hLayout);

}

//------------------------------------------------------------------------------------------
void MainWindow::changeTextureFilteringMode()
{
    QOpenGLTexture::Filter filterMode =
        str2TextureFilteringMap[cbTextureFiltering->currentText()];
    renderer->changeFloorTextureFilteringMode(filterMode);
}
