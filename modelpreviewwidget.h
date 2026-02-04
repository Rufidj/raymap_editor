#ifndef MODELPREVIEWWIDGET_H
#define MODELPREVIEWWIDGET_H

#include <QOpenGLWidget>
#include <QOpenGLFunctions>
#include <QOpenGLTexture>
#include <QVector3D>
#include <QMatrix4x4>
#include "md3generator.h"

class ModelPreviewWidget : public QOpenGLWidget, protected QOpenGLFunctions
{
    Q_OBJECT

public:
    explicit ModelPreviewWidget(QWidget *parent = nullptr);
    ~ModelPreviewWidget();
    
    void setMesh(const MD3Generator::MeshData &mesh);
    void setTexture(const QString &path);
    void setRotation(float degrees);  // Set model rotation in degrees
    void setScale(float scale);  // Set model scale
    void setModelOrientation(float xDeg, float yDeg, float zDeg);  // Set model base orientation
    
    // Get current camera rotation (for baking into model)
    float getCameraXRotation() const { return m_xRot; }
    float getCameraYRotation() const { return m_yRot; }

protected:
    void initializeGL() override;
    void resizeGL(int w, int h) override;
    void paintGL() override;
    
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    
private:
    MD3Generator::MeshData m_mesh;
    QOpenGLTexture *m_texture = nullptr;
    QString m_texturePath;
    
    QMatrix4x4 m_projection;
    QMatrix4x4 m_view;
    QMatrix4x4 m_model;
    
    QPoint m_lastPos;
    float m_xRot;
    float m_yRot;
    float m_zRot;  // Model rotation (Z axis)
    float m_scale; // Model scale
    float m_zoom;
    
    // Model base orientation (for fixing vertical models, etc.)
    float m_modelOrientX;
    float m_modelOrientY;
    float m_modelOrientZ;
};

#endif // MODELPREVIEWWIDGET_H
