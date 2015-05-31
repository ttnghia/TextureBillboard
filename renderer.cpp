//------------------------------------------------------------------------------------------
// renderer.cpp
//
// Created on: 1/17/2015
//     Author: Nghia Truong
//
//------------------------------------------------------------------------------------------

#include "renderer.h"

//------------------------------------------------------------------------------------------
Renderer::Renderer(QWidget* _parent):
    QOpenGLWidget(_parent),
    enabledZAxisRotation(false),
    enabledTextureAnisotropicFiltering(true),
    iboPlane(QOpenGLBuffer::IndexBuffer),
    iboBillboard(QOpenGLBuffer::IndexBuffer),
    specialKeyPressed(Renderer::NO_KEY),
    mouseButtonPressed(Renderer::NO_BUTTON),
    translation(0.0f, 0.0f, 0.0f),
    translationLag(0.0f, 0.0f, 0.0f),
    rotation(0.0f, 0.0f, 0.0f),
    rotationLag(0.0f, 0.0f, 0.0f),
    zooming(0.0f),
    planeObject(NULL),
    shadingMode(PHONG_SHADING),
    cameraPosition(DEFAULT_CAMERA_POSITION),
    cameraFocus(DEFAULT_CAMERA_FOCUS),
    cameraUpDirection(0.0f, 1.0f, 0.0f),
    floorTexture(CHECKERBOARD)
{
    retinaScale = devicePixelRatio();
    setFocusPolicy(Qt::StrongFocus);
}

//------------------------------------------------------------------------------------------
Renderer::~Renderer()
{

}

//------------------------------------------------------------------------------------------
void Renderer::checkOpenGLVersion()
{
    QString verStr = QString((const char*)glGetString(GL_VERSION));
    int major = verStr.left(verStr.indexOf(".")).toInt();
    int minor = verStr.mid(verStr.indexOf(".") + 1, 1).toInt();

    if(!(major >= 4 && minor >= 0))
    {
        QMessageBox::critical(this, "Error",
                              QString("Your OpenGL version is %1.%2, which does not satisfy this program requirement (OpenGL >= 4.0)").arg(
                                  major).arg(minor));
        close();
    }

//    qDebug() << major << minor;
//    qDebug() << verStr;
//    TRUE_OR_DIE(major >= 4 && minor >= 0, "OpenGL version must >= 4.0");
}
//------------------------------------------------------------------------------------------
bool Renderer::initProgram(ShadingProgram _shadingMode)
{
    QOpenGLShaderProgram* program;
    GLint location;

    /////////////////////////////////////////////////////////////////
    glslPrograms[_shadingMode] = new QOpenGLShaderProgram;
    program = glslPrograms[_shadingMode];
    bool success;

    success = program->addShaderFromSourceFile(QOpenGLShader::Vertex,
              vertexShaderSourceMap.value(_shadingMode));
    TRUE_OR_DIE(success, "Cannot compile shader from file.");

    success = program->addShaderFromSourceFile(QOpenGLShader::Fragment,
              fragmentShaderSourceMap.value(_shadingMode));
    TRUE_OR_DIE(success, "Cannot compile shader from file.");

    success = program->link();
    TRUE_OR_DIE(success, "Cannot link GLSL program.");

    location = program->attributeLocation("v_coord");
    TRUE_OR_DIE(location >= 0, "Cannot bind attribute vertex coordinate.");
    attrVertex[_shadingMode] = location;

    location = program->attributeLocation("v_normal");
    TRUE_OR_DIE(location >= 0, "Cannot bind attribute vertex normal.");
    attrNormal[_shadingMode] = location;

    location = program->attributeLocation("v_texcoord");
    TRUE_OR_DIE(location >= 0, "Cannot bind attribute texture coordinate.");
    attrTexCoord[_shadingMode] = location;


    location = glGetUniformBlockIndex(program->programId(), "Matrices");
    TRUE_OR_DIE(location >= 0, "Cannot bind block uniform.");
    uniMatrices[_shadingMode] = location;


    location = glGetUniformBlockIndex(program->programId(), "Light");
    TRUE_OR_DIE(location >= 0, "Cannot bind block uniform.");
    uniLight[_shadingMode] = location;


    location = glGetUniformBlockIndex(program->programId(), "Material");
    TRUE_OR_DIE(location >= 0, "Cannot bind block uniform.");
    uniMaterial[_shadingMode] = location;

    location = program->uniformLocation("cameraPosition");
    TRUE_OR_DIE(location >= 0, "Cannot bind uniform cameraPosition.");
    uniCameraPosition[_shadingMode] = location;

    location = program->uniformLocation("envTex");
    TRUE_OR_DIE(location >= 0, "Cannot bind uniform envTex.");
    uniEnvTexture[_shadingMode] = location;


    location = program->uniformLocation("objTex");
    TRUE_OR_DIE(location >= 0, "Cannot bind uniform objTex.");
    uniObjTexture[_shadingMode] = location;


    location = program->uniformLocation("hasObjTex");
    TRUE_OR_DIE(location >= 0, "Cannot bind uniform hasObjTex.");
    uniHasObjTexture[_shadingMode] = location;

    return true;
}

//------------------------------------------------------------------------------------------
bool Renderer::initShaderPrograms()
{
    vertexShaderSourceMap.insert(PHONG_SHADING, ":/shaders/phong-shading.vs.glsl");

    fragmentShaderSourceMap.insert(PHONG_SHADING, ":/shaders/phong-shading.fs.glsl");


    return initProgram(PHONG_SHADING);
}

//------------------------------------------------------------------------------------------
void Renderer::initRenderingData()
{
    initTexture();
    initSceneMemory();
    initVertexArrayObjects();
}

//------------------------------------------------------------------------------------------
void Renderer::initSharedBlockUniform()
{
    /////////////////////////////////////////////////////////////////
    // setup the light and material
    cameraPosition = DEFAULT_CAMERA_POSITION;

    light.position = DEFAULT_LIGHT_POSITION;
    light.intensity = 1.0f;

    planeMaterial.shininess = 50.0f;
    planeMaterial.setSpecular(QVector4D(0.5f, 0.5f, 0.5f, 1.0f));

    billboardObjectMaterial.setSpecular(QVector4D(0.5f, 0.5f, 0.5f, 0.0f));

    /////////////////////////////////////////////////////////////////
    // setup binding points for block uniform
    for(int i = 0; i < NUM_BINDING_POINTS; ++i)
    {
        UBOBindingIndex[i] = i + 1;
    }

    /////////////////////////////////////////////////////////////////
    // setup data for block uniform
    glGenBuffers(1, &UBOMatrices);
    glBindBuffer(GL_UNIFORM_BUFFER, UBOMatrices);
    glBufferData(GL_UNIFORM_BUFFER, 3 * SIZE_OF_MAT4, NULL,
                 GL_STREAM_DRAW);
    glBindBuffer(GL_UNIFORM_BUFFER, 0);

    glGenBuffers(1, &UBOLight);
    glBindBuffer(GL_UNIFORM_BUFFER, UBOLight);
    glBufferData(GL_UNIFORM_BUFFER, light.getStructSize(),
                 NULL, GL_STREAM_DRAW);
    glBufferSubData(GL_UNIFORM_BUFFER, 0, light.getStructSize(),
                    &light);
    glBindBuffer(GL_UNIFORM_BUFFER, 0);

    glGenBuffers(1, &UBOPlaneMaterial);
    glBindBuffer(GL_UNIFORM_BUFFER, UBOPlaneMaterial);
    glBufferData(GL_UNIFORM_BUFFER, planeMaterial.getStructSize(),
                 NULL, GL_STREAM_DRAW);
    glBufferSubData(GL_UNIFORM_BUFFER, 0, planeMaterial.getStructSize(),
                    &planeMaterial);

    glGenBuffers(1, &UBOBillboardObjectMaterial);
    glBindBuffer(GL_UNIFORM_BUFFER, UBOBillboardObjectMaterial);
    glBufferData(GL_UNIFORM_BUFFER, billboardObjectMaterial.getStructSize(),
                 NULL, GL_STREAM_DRAW);
    glBufferSubData(GL_UNIFORM_BUFFER, 0, billboardObjectMaterial.getStructSize(),
                    &billboardObjectMaterial);
    glBindBuffer(GL_UNIFORM_BUFFER, 0);
}

//------------------------------------------------------------------------------------------
void Renderer::initTexture()
{
    if(QOpenGLContext::currentContext()->hasExtension("GL_EXT_texture_filter_anisotropic"))
    {
        qDebug() << "GL_EXT_texture_filter_anisotropic: enabled";
        glEnable(GL_EXT_texture_filter_anisotropic);
    }
    else
    {
        qDebug() << "GL_EXT_texture_filter_anisotropic: disabled";
        glDisable(GL_EXT_texture_filter_anisotropic);
    }

    ////////////////////////////////////////////////////////////////////////////////
    // floor texture
    QMap<FloorTexture, QString> floorTexture2StrMap;
    floorTexture2StrMap[CHECKERBOARD] = "checkerboard.jpg";

    TRUE_OR_DIE(floorTexture2StrMap.size() == NUM_FLOOR_TEXTURES,
                "Ohh, you forget to initialize some floor texture...");

    for(int i = 0; i < NUM_FLOOR_TEXTURES; ++i)
    {
        FloorTexture tex = static_cast<FloorTexture>(i);

        QString texFile = QString(":/textures/%1").arg(floorTexture2StrMap[tex]);
        TRUE_OR_DIE(QFile::exists(texFile), "Cannot load texture from file.");
        floorTextures[tex] = new QOpenGLTexture(QImage(texFile).mirrored());
        floorTextures[tex]->setMinificationFilter(QOpenGLTexture::LinearMipMapLinear);
        floorTextures[tex]->setMagnificationFilter(QOpenGLTexture::LinearMipMapLinear);
        floorTextures[tex]->setWrapMode(QOpenGLTexture::Repeat);
    }

    ////////////////////////////////////////////////////////////////////////////////
    // billboard texture
    billboardTexture = new QOpenGLTexture(
        QImage(":/textures/billboardblueflowers.png").convertToFormat(QImage::Format_RGBA8888));
    billboardTexture->setMinificationFilter(QOpenGLTexture::LinearMipMapLinear);
    billboardTexture->setMagnificationFilter(QOpenGLTexture::LinearMipMapLinear);
    billboardTexture->setWrapMode(QOpenGLTexture::DirectionS,
                                  QOpenGLTexture::ClampToEdge);
    billboardTexture->setWrapMode(QOpenGLTexture::DirectionT,
                                  QOpenGLTexture::ClampToEdge);

}

//------------------------------------------------------------------------------------------
void Renderer::initSceneMemory()
{
    initPlaneMemory();
    initBillboardMemory();
}

//------------------------------------------------------------------------------------------
void Renderer::initPlaneMemory()
{
    if(!planeObject)
    {
        planeObject = new UnitPlane;
    }

    if(vboPlane.isCreated())
    {
        vboPlane.destroy();
    }

    if(iboPlane.isCreated())
    {
        iboPlane.destroy();
    }

    ////////////////////////////////////////////////////////////////////////////////
    // init memory for plane object
    vboPlane.create();
    vboPlane.bind();
    vboPlane.allocate(2 * planeObject->getVertexOffset() + planeObject->getTexCoordOffset());
    vboPlane.write(0, planeObject->getVertices(), planeObject->getVertexOffset());
    vboPlane.write(planeObject->getVertexOffset(), planeObject->getNormals(),
                   planeObject->getVertexOffset());
    vboPlane.write(2 * planeObject->getVertexOffset(),
                   planeObject->getTexureCoordinates(1.0f),
                   planeObject->getTexCoordOffset());
    vboPlane.release();
    // indices
    iboPlane.create();
    iboPlane.bind();
    iboPlane.allocate(planeObject->getIndices(), planeObject->getIndexOffset());
    iboPlane.release();

}

//------------------------------------------------------------------------------------------
void Renderer::initBillboardMemory()
{
    if(!planeObject)
    {
        planeObject = new UnitPlane;
    }

    if(vboBillboard.isCreated())
    {
        vboBillboard.destroy();
    }

    if(iboBillboard.isCreated())
    {
        iboBillboard.destroy();
    }

    ////////////////////////////////////////////////////////////////////////////////
    // init memory for billboard object
    vboBillboard.create();
    vboBillboard.bind();
    vboBillboard.allocate(2 * planeObject->getVertexOffset() +
                          planeObject->getTexCoordOffset());
    vboBillboard.write(0, planeObject->getVertices(), planeObject->getVertexOffset());
    vboBillboard.write(planeObject->getVertexOffset(), planeObject->getNormals(),
                       planeObject->getVertexOffset());
    vboBillboard.write(2 * planeObject->getVertexOffset(),
                       planeObject->getTexureCoordinates(1.0f),
                       planeObject->getTexCoordOffset());
    vboBillboard.release();
    // indices
    iboBillboard.create();
    iboBillboard.bind();
    iboBillboard.allocate(planeObject->getIndices(), planeObject->getIndexOffset());
    iboBillboard.release();
}

//------------------------------------------------------------------------------------------
// record the buffer state by vertex array object
//------------------------------------------------------------------------------------------
void Renderer::initVertexArrayObjects()
{
    initPlaneVAO(PHONG_SHADING);

    initBillboardVAO(PHONG_SHADING);
}

//------------------------------------------------------------------------------------------
void Renderer::initPlaneVAO(ShadingProgram _shadingMode)
{
    if(vaoPlane[_shadingMode].isCreated())
    {
        vaoPlane[_shadingMode].destroy();
    }

    QOpenGLShaderProgram* program = glslPrograms[_shadingMode];

    vaoPlane[_shadingMode].create();
    vaoPlane[_shadingMode].bind();

    vboPlane.bind();
    program->enableAttributeArray(attrVertex[_shadingMode]);
    program->setAttributeBuffer(attrVertex[_shadingMode], GL_FLOAT, 0, 3);

    program->enableAttributeArray(attrNormal[_shadingMode]);
    program->setAttributeBuffer(attrNormal[_shadingMode], GL_FLOAT,
                                planeObject->getVertexOffset(), 3);

    program->enableAttributeArray(attrTexCoord[_shadingMode]);
    program->setAttributeBuffer(attrTexCoord[_shadingMode], GL_FLOAT,
                                2 * planeObject->getVertexOffset(), 2);

    iboPlane.bind();

    // release vao before vbo and ibo
    vaoPlane[_shadingMode].release();
    vboPlane.release();
    iboPlane.release();

}

//------------------------------------------------------------------------------------------
void Renderer::initBillboardVAO(ShadingProgram _shadingMode)
{
    if(vaoBillboard[_shadingMode].isCreated())
    {
        vaoBillboard[_shadingMode].destroy();
    }

    QOpenGLShaderProgram* program = glslPrograms[_shadingMode];

    vaoBillboard[_shadingMode].create();
    vaoBillboard[_shadingMode].bind();

    vboBillboard.bind();
    program->enableAttributeArray(attrVertex[_shadingMode]);
    program->setAttributeBuffer(attrVertex[_shadingMode], GL_FLOAT, 0, 3);

    program->enableAttributeArray(attrNormal[_shadingMode]);
    program->setAttributeBuffer(attrNormal[_shadingMode], GL_FLOAT,
                                planeObject->getVertexOffset(), 3);

    program->enableAttributeArray(attrTexCoord[_shadingMode]);
    program->setAttributeBuffer(attrTexCoord[_shadingMode], GL_FLOAT,
                                2 * planeObject->getVertexOffset(), 2);

    iboBillboard.bind();

    // release vao before vbo and ibo
    vaoBillboard[_shadingMode].release();
    vboBillboard.release();
    iboBillboard.release();
}

//------------------------------------------------------------------------------------------
void Renderer::initSceneMatrices()
{
    /////////////////////////////////////////////////////////////////
    // floor
    changePlaneSize(30);
    planeNormalMatrix = QMatrix4x4(planeModelMatrix.normalMatrix());

    /////////////////////////////////////////////////////////////////
    // billboard object
    billboardObjectModelMatrix.setToIdentity();
    billboardObjectModelMatrix.scale(4.0f);
    billboardObjectModelMatrix.translate(DEFAULT_BILLBOARD_OBJECT_POSITION);
}


//------------------------------------------------------------------------------------------
void Renderer::changePlaneSize(int _planeSize)
{
    planeModelMatrix.setToIdentity();
    planeModelMatrix.scale((float)_planeSize * 2.0f);

    vboPlane.bind();
    vboPlane.write(2 * planeObject->getVertexOffset(),
                   planeObject->getTexureCoordinates((float)_planeSize),
                   planeObject->getTexCoordOffset());
    vboPlane.release();
}

//------------------------------------------------------------------------------------------
void Renderer::changeFloorTexture(FloorTexture _texture)
{
    floorTexture = _texture;
}

//------------------------------------------------------------------------------------------
void Renderer::changeFloorTextureFilteringMode(QOpenGLTexture::Filter
                                               _textureFiltering)
{
    for(int i = 0; i < NUM_FLOOR_TEXTURES; ++i)
    {
        floorTextures[i]->setMinMagFilters(_textureFiltering, _textureFiltering);
    }
}
//------------------------------------------------------------------------------------------
void Renderer::updateCamera()
{
    zoomCamera();

    /////////////////////////////////////////////////////////////////
    // flush camera data to uniform buffer
    viewMatrix.setToIdentity();
    viewMatrix.lookAt(cameraPosition, cameraFocus, cameraUpDirection);

    viewProjectionMatrix = projectionMatrix * viewMatrix;

    glBindBuffer(GL_UNIFORM_BUFFER, UBOMatrices);
    glBufferSubData(GL_UNIFORM_BUFFER, 2 * SIZE_OF_MAT4, SIZE_OF_MAT4,
                    viewProjectionMatrix.constData());
    glBindBuffer(GL_UNIFORM_BUFFER, 0);
}

//------------------------------------------------------------------------------------------
QSize Renderer::sizeHint() const
{
    return QSize(1600, 1200);
}

//------------------------------------------------------------------------------------------
QSize Renderer::minimumSizeHint() const
{
    return QSize(50, 50);
}

//------------------------------------------------------------------------------------------
void Renderer::initializeGL()
{
    initializeOpenGLFunctions();

    checkOpenGLVersion();

    if(!initShaderPrograms())
    {
        PRINT_ERROR("Cannot initialize shaders. Exit...");
        exit(EXIT_FAILURE);

    }

    initRenderingData();
    initSharedBlockUniform();
    initSceneMatrices();

    glEnable(GL_DEPTH_TEST);

    changeShadingMode(PHONG_SHADING);
}

//------------------------------------------------------------------------------------------
void Renderer::resizeGL(int w, int h)
{
    projectionMatrix.setToIdentity();
    projectionMatrix.perspective(45, (float)w / (float)h, 0.1f, 10000.0f);
}

//------------------------------------------------------------------------------------------
void Renderer::paintGL()
{
    translateCamera();
    rotateCamera();


    updateCamera();

    // render scene
    glViewport(0, 0, width() * retinaScale, height() * retinaScale);
    renderScene();
}

//-----------------------------------------------------------------------------------------
void Renderer::mousePressEvent(QMouseEvent* _event)
{
    lastMousePos = QVector2D(_event->localPos());

    if(_event->button() == Qt::RightButton)
    {
        mouseButtonPressed = RIGHT_BUTTON;
    }
    else
    {
        mouseButtonPressed = LEFT_BUTTON;
    }
}

//-----------------------------------------------------------------------------------------
void Renderer::mouseMoveEvent(QMouseEvent* _event)
{
    QVector2D mouseMoved = QVector2D(_event->localPos()) - lastMousePos;

    switch(specialKeyPressed)
    {
    case Renderer::NO_KEY:
    {

        if(mouseButtonPressed == RIGHT_BUTTON)
        {
            translation.setX(translation.x() + mouseMoved.x() / 50.0f);
            translation.setY(translation.y() - mouseMoved.y() / 50.0f);
        }
        else
        {
            rotation.setX(rotation.x() - mouseMoved.x() / 5.0f);
            rotation.setY(rotation.y() - mouseMoved.y() / 5.0f);
        }

    }
    break;

    case Renderer::SHIFT_KEY:
    {
        if(mouseButtonPressed == RIGHT_BUTTON)
        {
            QVector2D dir = mouseMoved.normalized();
            zooming += mouseMoved.length() * dir.x() / 500.0f;
        }
        else
        {
            rotation.setX(rotation.x() + mouseMoved.x() / 5.0f);
            rotation.setZ(rotation.z() + mouseMoved.y() / 5.0f);
        }
    }
    break;

    case Renderer::CTRL_KEY:
        break;
    }

    lastMousePos = QVector2D(_event->localPos());
    update();
}

//------------------------------------------------------------------------------------------
void Renderer::mouseReleaseEvent(QMouseEvent* _event)
{
    mouseButtonPressed = NO_BUTTON;
}

//------------------------------------------------------------------------------------------
void Renderer::wheelEvent(QWheelEvent* _event)
{
    if(!_event->angleDelta().isNull())
    {
        zooming +=  (_event->angleDelta().x() + _event->angleDelta().y()) / 500.0f;
    }


    update();
}

//------------------------------------------------------------------------------------------
void Renderer::changeShadingMode(ShadingProgram _shadingMode)
{
    shadingMode = _shadingMode;
    currentProgram = glslPrograms[shadingMode];

    update();
}

//------------------------------------------------------------------------------------------
void Renderer::resetCameraPosition()
{
    cameraPosition = DEFAULT_CAMERA_POSITION;
    cameraFocus = DEFAULT_CAMERA_FOCUS;
    cameraUpDirection = QVector3D(0.0f, 1.0f, 0.0f);
}

//------------------------------------------------------------------------------------------
void Renderer::enableDepthTest(bool _status)
{
    makeCurrent();

    if(_status)
    {
        glEnable(GL_DEPTH_TEST);
    }
    else
    {
        glDisable(GL_DEPTH_TEST);
    }

    doneCurrent();
}

//------------------------------------------------------------------------------------------
void Renderer::enableZAxisRotation(bool _status)
{
    enabledZAxisRotation = _status;

    if(!enabledZAxisRotation)
    {
        cameraUpDirection = QVector3D(0.0f, 1.0f, 0.0f);
    }
}


//------------------------------------------------------------------------------------------
void Renderer::enableTextureAnisotropicFiltering(bool _state)
{
    enabledTextureAnisotropicFiltering = _state;
}

//------------------------------------------------------------------------------------------
void Renderer::keyPressEvent(QKeyEvent* _event)
{
    switch(_event->key())
    {
    case Qt::Key_Shift:
        specialKeyPressed = Renderer::SHIFT_KEY;
        break;

    case Qt::Key_Plus:
        zooming -= 0.1f;
        break;

    case Qt::Key_Minus:
        zooming += 0.1f;
        break;

    default:
        QOpenGLWidget::keyPressEvent(_event);
    }
}

//------------------------------------------------------------------------------------------
void Renderer::keyReleaseEvent(QKeyEvent* _event)
{
    specialKeyPressed = Renderer::NO_KEY;
}

//------------------------------------------------------------------------------------------
void Renderer::translateCamera()
{
    translation *= MOVING_INERTIA;

    if(translation.lengthSquared() < 1e-4)
    {
        return;
    }

    QVector3D eyeVector = cameraFocus - cameraPosition;
    float scale = sqrt(eyeVector.length()) * 0.01f;

    QVector3D u(0.0f, 1.0f, 0.0f);
    QVector3D v = QVector3D::crossProduct(eyeVector, u);
    u = QVector3D::crossProduct(v, eyeVector);
    u.normalize();
    v.normalize();

    cameraPosition -= scale * (translation.x() * v + translation.y() * u);
    cameraFocus -= scale * (translation.x() * v + translation.y() * u);

}

//------------------------------------------------------------------------------------------
void Renderer::rotateCamera()
{
    rotation *= MOVING_INERTIA;

    if(rotation.lengthSquared() < 1e-4)
    {
        return;
    }

    QVector3D nEyeVector = cameraPosition - cameraFocus ;

    float scale = sqrt(nEyeVector.length()) * 0.02f;
    QQuaternion qRotation = QQuaternion::fromAxisAndAngle(QVector3D(1, 0, 0),
                            rotation.y() * scale) *
                            QQuaternion::fromAxisAndAngle(QVector3D(0, 1, 0), rotation.x() * scale) *
                            QQuaternion::fromAxisAndAngle(QVector3D(0, 0, 1), rotation.z() * scale);
    nEyeVector = qRotation.rotatedVector(nEyeVector);

    cameraPosition = cameraFocus + nEyeVector;

    if(enabledZAxisRotation)
    {
        cameraUpDirection = qRotation.rotatedVector(cameraUpDirection);
    }
}

//------------------------------------------------------------------------------------------
void Renderer::zoomCamera()
{
    zooming *= MOVING_INERTIA;

    if(fabs(zooming) < 1e-4)
    {
        return;
    }

    QVector3D nEyeVector = cameraPosition - cameraFocus ;
    float len = nEyeVector.length();
    nEyeVector.normalize();

    len += sqrt(len) * zooming * 0.3f;

    if(len < 0.5f)
    {
        len = 0.5f;
    }

    cameraPosition = len * nEyeVector + cameraFocus;

}

//------------------------------------------------------------------------------------------
void Renderer::renderScene()
{
    glClearColor(0.8f, 0.8f, 0.8f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);


    // set the data for rendering
    currentProgram->bind();
    currentProgram->setUniformValue(uniCameraPosition[shadingMode], cameraPosition);
    currentProgram->setUniformValue(uniObjTexture[shadingMode], 0);
    currentProgram->setUniformValue(uniEnvTexture[shadingMode], 1);

    glUniformBlockBinding(currentProgram->programId(), uniMatrices[shadingMode],
                          UBOBindingIndex[BINDING_MATRICES]);
    glBindBufferBase(GL_UNIFORM_BUFFER, UBOBindingIndex[BINDING_MATRICES],
                     UBOMatrices);

    glUniformBlockBinding(currentProgram->programId(), uniLight[shadingMode],
                          UBOBindingIndex[BINDING_LIGHT]);
    glBindBufferBase(GL_UNIFORM_BUFFER, UBOBindingIndex[BINDING_LIGHT],
                     UBOLight);

    renderFloor();


    renderBillboardObject();
    currentProgram->release();
}

//------------------------------------------------------------------------------------------
void Renderer::renderFloor()
{
    /////////////////////////////////////////////////////////////////
    // flush the model and normal matrices
    glBindBuffer(GL_UNIFORM_BUFFER, UBOMatrices);
    glBufferSubData(GL_UNIFORM_BUFFER, 0, SIZE_OF_MAT4,
                    planeModelMatrix.constData());
    glBufferSubData(GL_UNIFORM_BUFFER, SIZE_OF_MAT4, SIZE_OF_MAT4,
                    planeNormalMatrix.constData());
    glBindBuffer(GL_UNIFORM_BUFFER, 0);

    /////////////////////////////////////////////////////////////////
    // set the uniform
    currentProgram->setUniformValue(uniHasObjTexture[shadingMode], GL_TRUE);

    glUniformBlockBinding(currentProgram->programId(), uniMaterial[shadingMode],
                          UBOBindingIndex[BINDING_FLOOR_MATERIAL]);
    glBindBufferBase(GL_UNIFORM_BUFFER, UBOBindingIndex[BINDING_FLOOR_MATERIAL],
                     UBOPlaneMaterial);

    /////////////////////////////////////////////////////////////////
    // render the floor
    vaoPlane[shadingMode].bind();
    floorTextures[floorTexture]->bind(0);

    if(enabledTextureAnisotropicFiltering)
    {
        GLfloat fLargest;
        glGetFloatv(GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, &fLargest);
        glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, fLargest);
    }
    else
    {
        glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, 1.0f);
    }

    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_SHORT, 0);
    floorTextures[floorTexture]->release();
    vaoPlane[shadingMode].release();
}

//------------------------------------------------------------------------------------------
void Renderer::renderBillboardObject()
{
    /////////////////////////////////////////////////////////////////
    // rotate the billboard to face the camera
    QVector3D billboardPos = DEFAULT_BILLBOARD_OBJECT_POSITION;
    QVector3D cameraDir = cameraPosition - cameraFocus;
    float angle = atan2(billboardPos.x() - cameraDir.x(), billboardPos.z() - cameraDir.z()) ;

    QMatrix4x4 rotationMatrix = billboardObjectModelMatrix;
    rotationMatrix.rotate(angle * 180 / M_PI, QVector3D(0.0f, 1.0f, 0.0f));
    rotationMatrix.rotate(90, QVector3D(1.0f, 0.0f, 0.0f));
    QMatrix4x4 normalMatrix = -QMatrix4x4(rotationMatrix.normalMatrix());

    /////////////////////////////////////////////////////////////////
    // flush the model and normal matrices
    glBindBuffer(GL_UNIFORM_BUFFER, UBOMatrices);
    glBufferSubData(GL_UNIFORM_BUFFER, 0, SIZE_OF_MAT4,
                    rotationMatrix.constData());
    glBufferSubData(GL_UNIFORM_BUFFER, SIZE_OF_MAT4, SIZE_OF_MAT4,
                    normalMatrix.constData());
    glBindBuffer(GL_UNIFORM_BUFFER, 0);

    /////////////////////////////////////////////////////////////////
    // set the uniform
    currentProgram->setUniformValue(uniHasObjTexture[shadingMode], GL_TRUE);

    glUniformBlockBinding(currentProgram->programId(), uniMaterial[shadingMode],
                          UBOBindingIndex[BINDING_BILLBOARD_OBJECT_MATERIAL]);
    glBindBufferBase(GL_UNIFORM_BUFFER, UBOBindingIndex[BINDING_BILLBOARD_OBJECT_MATERIAL],
                     UBOBillboardObjectMaterial);

    /////////////////////////////////////////////////////////////////
    // render the billboard
    vaoBillboard[shadingMode].bind();
    billboardTexture->bind(0);
    glEnable(GL_BLEND);
    glBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDrawElements(GL_TRIANGLES, planeObject->getNumIndices(), GL_UNSIGNED_SHORT, 0);
    glDisable(GL_BLEND);
    billboardTexture->release();
    vaoBillboard[shadingMode].release();


}
