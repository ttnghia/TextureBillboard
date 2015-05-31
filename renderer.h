//------------------------------------------------------------------------------------------
// renderer.h
//
// Created on: 1/17/2015
//     Author: Nghia Truong
//
//------------------------------------------------------------------------------------------

#ifndef GLRENDERER_H
#define GLRENDERER_H

#include <QtGui>
#include <QtWidgets>
#include <QOpenGLFunctions_4_0_Core>

#include "unitplane.h"

//------------------------------------------------------------------------------------------
#define PRINT_ERROR(_errStr) \
{ \
    qDebug()<< "Error occured at line:" << __LINE__ << ", file:" << __FILE__; \
    qDebug()<< "Error message:" << _errStr; \
}

#define PRINT_AND_DIE(_errStr) \
{ \
    qDebug()<< "Error occured at line:" << __LINE__ << ", file:" << __FILE__; \
    qDebug()<< "Error message:" << _errStr; \
    exit(EXIT_FAILURE); \
}

#define TRUE_OR_DIE(_condition, _errStr) \
{ \
    if(!(_condition)) \
    { \
        qDebug()<< "Fatal error occured at line:" << __LINE__ << ", file:" << __FILE__; \
        qDebug()<< "Error message:" << _errStr; \
        exit(EXIT_FAILURE); \
    } \
}

#define SIZE_OF_MAT4 (4 * 4 *sizeof(GLfloat))
#define SIZE_OF_VEC4 (4 * sizeof(GLfloat))
//------------------------------------------------------------------------------------------
#define MOVING_INERTIA 0.9f
#define CUBE_MAP_SIZE 512
#define DEFAULT_CAMERA_POSITION QVector3D(-4.0f,  5.0f, 15.0f)
#define DEFAULT_CAMERA_FOCUS QVector3D(-4.0f,  2.0f, 0.0f)
#define DEFAULT_LIGHT_POSITION QVector3D(0.0f, 100.0f, 100.0f)
#define DEFAULT_BILLBOARD_OBJECT_POSITION QVector3D(-1.0f, 1.001f, -3.0f)

struct Light
{
    Light():
        position(10.0f, 10.0f, 10.0f, 1.0f),
        color(1.0f, 1.0f, 1.0f, 1.0f),
        intensity(1.0f) {}

    int getStructSize()
    {
        return (2 * 4 + 1) * sizeof(GLfloat);
    }

    QVector4D position;
    QVector4D color;
    GLfloat intensity;
};

struct Material
{
    Material():
        diffuseColor(-10.0f, 1.0f, 0.0f, 1.0f),
        specularColor(1.0f, 1.0f, 1.0f, 1.0f),
        reflection(0.0f),
        shininess(10.0f) {}

    int getStructSize()
    {
        return (2 * 4 + 2) * sizeof(GLfloat);
    }

    void setDiffuse(QVector4D _diffuse)
    {
        diffuseColor = _diffuse;
    }

    void setSpecular(QVector4D _specular)
    {
        specularColor = _specular;
    }

    void setReflection(float _reflection)
    {
        reflection = _reflection;
    }

    QVector4D diffuseColor;
    QVector4D specularColor;
    GLfloat reflection;
    GLfloat shininess;
};

enum FloorTexture
{
    CHECKERBOARD = 0,
    NUM_FLOOR_TEXTURES
};

enum ShadingProgram
{
    PHONG_SHADING = 0,
    BILLBOARD_SHADING,
    NUM_SHADING_MODE
};


enum UBOBinding
{
    BINDING_MATRICES = 0,
    BINDING_LIGHT,
    BINDING_BACKGROUND_MATERIAL,
    BINDING_FLOOR_MATERIAL,
    BINDING_BILLBOARD_OBJECT_MATERIAL,
    NUM_BINDING_POINTS
};


//------------------------------------------------------------------------------------------
class Renderer : public QOpenGLWidget, QOpenGLFunctions_4_0_Core// QOpenGLFunctions
{
public:
    enum SpecialKey
    {
        NO_KEY,
        SHIFT_KEY,
        CTRL_KEY
    };

    enum MouseButton
    {
        NO_BUTTON,
        LEFT_BUTTON,
        RIGHT_BUTTON
    };

    Renderer(QWidget* parent = 0);
    ~Renderer();

    QSize sizeHint() const;
    QSize minimumSizeHint() const;
    void keyPressEvent(QKeyEvent* _event);
    void keyReleaseEvent(QKeyEvent* _event);
    void wheelEvent(QWheelEvent* _event);

    void changeShadingMode(ShadingProgram _shadingMode);
    void changeFloorTexture(FloorTexture _texture);
    void changeFloorTextureFilteringMode(QOpenGLTexture::Filter _textureFiltering);

public slots:
    void enableDepthTest(bool _status);
    void enableZAxisRotation(bool _status);
    void enableTextureAnisotropicFiltering(bool _state);
    void resetCameraPosition();
    void changePlaneSize(int _planeSize);

protected:
    void initializeGL();
    void resizeGL(int w, int h);
    void paintGL();

    void mousePressEvent(QMouseEvent* _event);
    void mouseMoveEvent(QMouseEvent* _event);
    void mouseReleaseEvent(QMouseEvent* _event);

private:
    void checkOpenGLVersion();
    bool initShaderPrograms();
    bool initProgram(ShadingProgram _shadingMode);
    void initRenderingData();
    void initSharedBlockUniform();
    void initTexture();
    void initSceneMemory();
    void initPlaneMemory();
    void initBillboardMemory();
    void initVertexArrayObjects();
    void initPlaneVAO(ShadingProgram _shadingMode);
    void initBillboardVAO(ShadingProgram _shadingMode);
    void initSceneMatrices();

    void updateCamera();
    void translateCamera();
    void rotateCamera();
    void zoomCamera();

    void renderScene();
    void renderFloor();
    void renderBillboardObject();

    QOpenGLTexture* floorTextures[NUM_FLOOR_TEXTURES];
    QOpenGLTexture* billboardTexture;
    UnitPlane* planeObject;


    QMap<ShadingProgram, QString> vertexShaderSourceMap;
    QMap<ShadingProgram, QString> fragmentShaderSourceMap;
    QOpenGLShaderProgram* glslPrograms[NUM_SHADING_MODE];
    QOpenGLShaderProgram* currentProgram;
    GLuint UBOBindingIndex[NUM_BINDING_POINTS];
    GLuint UBOMatrices;
    GLuint UBOLight;
    GLuint UBOPlaneMaterial;
    GLuint UBOBillboardObjectMaterial;
    GLint attrVertex[NUM_SHADING_MODE];
    GLint attrNormal[NUM_SHADING_MODE];
    GLint attrTexCoord[NUM_SHADING_MODE];

    GLint uniMatrices[NUM_SHADING_MODE];
    GLint uniCameraPosition[NUM_SHADING_MODE];
    GLint uniLight[NUM_SHADING_MODE];
    GLint uniMaterial[NUM_SHADING_MODE];
    GLint uniObjTexture[NUM_SHADING_MODE];
    GLint uniEnvTexture[NUM_SHADING_MODE];
    GLint uniHasObjTexture[NUM_SHADING_MODE];

    QOpenGLVertexArrayObject vaoPlane[NUM_SHADING_MODE];
    QOpenGLVertexArrayObject vaoBillboard[NUM_SHADING_MODE];
    QOpenGLBuffer vboPlane;
    QOpenGLBuffer vboBillboard;
    QOpenGLBuffer iboPlane;
    QOpenGLBuffer iboBillboard;

    Material planeMaterial;
    Material billboardObjectMaterial;
    Light light;


    QMatrix4x4 viewMatrix;
    QMatrix4x4 projectionMatrix;
    QMatrix4x4 viewProjectionMatrix;
    QMatrix4x4 planeModelMatrix;
    QMatrix4x4 planeNormalMatrix;
    QMatrix4x4 billboardObjectModelMatrix;

    qreal retinaScale;
    float zooming;
    QVector3D cameraPosition;
    QVector3D cameraFocus;
    QVector3D cameraUpDirection;

    QVector2D lastMousePos;
    QVector3D translation;
    QVector3D translationLag;
    QVector3D rotation;
    QVector3D rotationLag;
    QVector3D scalingLag;
    SpecialKey specialKeyPressed;
    MouseButton mouseButtonPressed;

    ShadingProgram shadingMode;
    FloorTexture floorTexture;
    bool enabledZAxisRotation;
    bool enabledTextureAnisotropicFiltering;
};

#endif // GLRENDERER_H
