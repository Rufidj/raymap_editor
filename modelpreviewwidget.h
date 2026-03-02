#ifndef MODELPREVIEWWIDGET_H
#define MODELPREVIEWWIDGET_H

#include "md3generator.h"
#include <QMatrix4x4>
#include <QOpenGLFunctions>
#include <QOpenGLTexture>
#include <QOpenGLWidget>
#include <QTimer>
#include <QVector3D>

class ModelPreviewWidget : public QOpenGLWidget, protected QOpenGLFunctions {
  Q_OBJECT

public:
  explicit ModelPreviewWidget(QWidget *parent = nullptr);
  ~ModelPreviewWidget();

  struct Surface {
    MD3Generator::MeshData mesh;
    QOpenGLTexture *texture = nullptr;
    QString texturePath;
    QImage textureImage;
    bool hasTexture = false;
  };

  void clearSurfaces();
  void addSurface(const MD3Generator::MeshData &mesh,
                  const QImage &img = QImage());
  void addSurface(const MD3Generator::MeshData &mesh,
                  const QString &texturePath);

  void setRotation(float degrees); // Set model rotation in degrees
  void setScale(float scale);      // Set model scale
  void setModelOrientation(float xDeg, float yDeg,
                           float zDeg); // Set model base orientation

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
  QVector<Surface *> m_surfaces;

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

  // Animation support
  int m_currentFrame = 0;
  int m_maxFrames = 1;
  bool m_isAnimated = false;
  QTimer *m_animTimer = nullptr;

private slots:
  void updateAnimation();
};

#endif // MODELPREVIEWWIDGET_H
