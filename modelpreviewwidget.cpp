#include "modelpreviewwidget.h"
#include <QFont>
#include <QMouseEvent>
#include <QPainter>
#include <QtGlobal>
#include <QtMath>

ModelPreviewWidget::ModelPreviewWidget(QWidget *parent)
    : QOpenGLWidget(parent), m_xRot(30.0f), m_yRot(-45.0f), m_zRot(0.0f),
      m_scale(1.0f), m_zoom(-300.0f), m_modelOrientX(0.0f),
      m_modelOrientY(0.0f), m_modelOrientZ(0.0f) {
  // Explicitly request Compatibility Profile for this widget
  QSurfaceFormat format;
  format.setVersion(2, 1);
  format.setProfile(QSurfaceFormat::CompatibilityProfile);
  format.setDepthBufferSize(24);
  setFormat(format);

  m_animTimer = new QTimer(this);
  connect(m_animTimer, &QTimer::timeout, this,
          &ModelPreviewWidget::updateAnimation);
}

ModelPreviewWidget::~ModelPreviewWidget() { clearSurfaces(); }

void ModelPreviewWidget::clearSurfaces() {
  makeCurrent();
  for (Surface *s : m_surfaces) {
    if (s->texture)
      delete s->texture;
    delete s;
  }
  m_surfaces.clear();
  doneCurrent();
  update();
}

void ModelPreviewWidget::addSurface(const MD3Generator::MeshData &mesh,
                                    const QImage &img) {
  Surface *s = new Surface();
  s->mesh = mesh;
  s->textureImage = img;
  s->hasTexture = !img.isNull();
  m_surfaces.append(s);

  // Check for animation support
  if (!mesh.animationFrames.isEmpty()) {
    m_isAnimated = true;
    m_maxFrames = qMax(m_maxFrames, mesh.animationFrames.size());
    if (!m_animTimer->isActive())
      m_animTimer->start(1000 / 24); // 24 FPS
  }

  update();
}

void ModelPreviewWidget::addSurface(const MD3Generator::MeshData &mesh,
                                    const QString &texturePath) {
  Surface *s = new Surface();
  s->mesh = mesh;
  s->texturePath = texturePath;
  s->hasTexture = !texturePath.isEmpty() && QFile::exists(texturePath);
  if (s->hasTexture)
    s->textureImage.load(texturePath);
  m_surfaces.append(s);

  // Check for animation support
  if (!mesh.animationFrames.isEmpty()) {
    m_isAnimated = true;
    m_maxFrames = qMax(m_maxFrames, mesh.animationFrames.size());
    if (!m_animTimer->isActive())
      m_animTimer->start(1000 / 24); // 24 FPS
  }

  update();
}

void ModelPreviewWidget::setRotation(float degrees) {
  m_zRot = degrees;
  update();
}

void ModelPreviewWidget::setScale(float scale) {
  m_scale = scale;
  update();
}

void ModelPreviewWidget::setModelOrientation(float xDeg, float yDeg,
                                             float zDeg) {
  m_modelOrientX = xDeg;
  m_modelOrientY = yDeg;
  m_modelOrientZ = zDeg;
  update();
}

void ModelPreviewWidget::initializeGL() {
  initializeOpenGLFunctions();
  glClearColor(0.2f, 0.2f, 0.2f, 1.0f);
  glEnable(GL_DEPTH_TEST);
  glDisable(
      GL_CULL_FACE); // Disable culling to see all faces (helps debug geometry)
}

void ModelPreviewWidget::resizeGL(int w, int h) {
  m_projection.setToIdentity();
  m_projection.perspective(45.0f, GLfloat(w) / h, 1.0f, 1000.0f);
}

// Methods removed: setTexture is now handled via addSurface

void ModelPreviewWidget::paintGL() {
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

  m_view.setToIdentity();
  m_view.translate(0.0f, 0.0f, m_zoom);
  m_view.rotate(m_xRot, 1.0f, 0.0f, 0.0f);
  m_view.rotate(m_yRot, 0.0f, 1.0f, 0.0f);
  m_view.rotate(-90.0f, 1.0f, 0.0f, 0.0f);

  // Apply model transformations
  m_view.rotate(m_modelOrientX, 1.0f, 0.0f, 0.0f); // Model base orientation X
  m_view.rotate(m_modelOrientY, 0.0f, 1.0f, 0.0f); // Model base orientation Y
  m_view.rotate(m_modelOrientZ, 0.0f, 0.0f, 1.0f); // Model base orientation Z
  m_view.rotate(m_zRot, 0.0f, 0.0f,
                1.0f);   // Apply model rotation (game rotation)
  m_view.scale(m_scale); // Apply model scale

  glMatrixMode(GL_PROJECTION);
  glLoadMatrixf(m_projection.constData());

  glMatrixMode(GL_MODELVIEW);
  glLoadMatrixf(m_view.constData());

  // Draw Axis (still using immediate mode for simplicity, or could convert too)
  glDisable(GL_TEXTURE_2D);
  glBegin(GL_LINES);
  glColor3f(1, 0, 0);
  glVertex3f(0, 0, 0);
  glVertex3f(50, 0, 0);
  glColor3f(0, 1, 0);
  glVertex3f(0, 0, 0);
  glVertex3f(0, 50, 0);
  glColor3f(0, 0, 1);
  glVertex3f(0, 0, 0);
  glVertex3f(0, 0, 50);
  glEnd();

  // Draw Mesh using Vertex Arrays
  for (Surface *s : m_surfaces) {
    if (s->mesh.vertices.isEmpty())
      continue;

    // Lazy-load texture
    if (s->hasTexture && !s->texture && !s->textureImage.isNull()) {
      s->texture = new QOpenGLTexture(s->textureImage.mirrored(false, true));
      s->texture->setMinificationFilter(QOpenGLTexture::LinearMipMapLinear);
      s->texture->setMagnificationFilter(QOpenGLTexture::Linear);
    }

    // Flatten data (could be optimized with VBOs, but this is simple for
    // preview)
    QVector<GLfloat> vertData;
    QVector<GLfloat> uvData;
    vertData.reserve(s->mesh.indices.size() * 3);
    if (s->texture)
      uvData.reserve(s->mesh.indices.size() * 2);

    for (int idx : s->mesh.indices) {
      if (idx < 0)
        continue;

      // Determine which vertex data to use
      bool useAnim = !s->mesh.animationFrames.isEmpty();
      if (useAnim && m_currentFrame >= s->mesh.animationFrames.size())
        useAnim =
            false; // Fallback to base if this surface lacks sufficient frames

      if (!useAnim && (idx >= s->mesh.vertices.size()))
        continue;
      if (useAnim && (idx >= s->mesh.animationFrames[m_currentFrame].size()))
        continue;

      const auto &v = useAnim ? s->mesh.animationFrames[m_currentFrame][idx]
                              : s->mesh.vertices[idx];

      vertData.append(v.pos.x());
      vertData.append(v.pos.y());
      vertData.append(v.pos.z());

      if (s->texture) {
        uvData.append(v.uv.x());
        uvData.append(v.uv.y());
      }
    }

    if (s->texture) {
      glEnable(GL_TEXTURE_2D);
      s->texture->bind();
      glColor3f(1, 1, 1);
      glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    } else {
      glDisable(GL_TEXTURE_2D);
      glColor3f(0.8f, 0.8f, 0.8f);
      glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    }

    glEnableClientState(GL_VERTEX_ARRAY);
    glVertexPointer(3, GL_FLOAT, 0, vertData.constData());

    if (s->texture && !uvData.isEmpty()) {
      glEnableClientState(GL_TEXTURE_COORD_ARRAY);
      glTexCoordPointer(2, GL_FLOAT, 0, uvData.constData());
    }

    glDrawArrays(GL_TRIANGLES, 0, vertData.size() / 3);

    if (s->texture) {
      glDisableClientState(GL_TEXTURE_COORD_ARRAY);
      s->texture->release();
      glDisable(GL_TEXTURE_2D);
    }
  }

  // Draw HUD (Frames)
  if (m_isAnimated) {
    QPainter painter(this);
    painter.setPen(Qt::green);
    painter.setFont(QFont("Arial", 12, QFont::Bold));
    painter.drawText(
        10, 20,
        QString("Frame: %1 / %2").arg(m_currentFrame + 1).arg(m_maxFrames));
    painter.end();
  }
}

void ModelPreviewWidget::mousePressEvent(QMouseEvent *event) {
  m_lastPos = event->pos();
}

void ModelPreviewWidget::mouseMoveEvent(QMouseEvent *event) {
  // Qt6 compatible way might be position().toPoint()
  QPoint currentPos =
      event->pos(); // pos() is still available in 6.0 but deprecated in newer?
  // The warning said x() is deprecated.

  int dx = currentPos.x() - m_lastPos.x();
  int dy = currentPos.y() - m_lastPos.y();

  if (event->buttons() & Qt::LeftButton) {
    m_xRot += dy;
    m_yRot += dx;
    update();
  } else if (event->buttons() & Qt::RightButton) {
    m_zoom += dy;
    update();
  }

  m_lastPos = event->pos();
}

void ModelPreviewWidget::updateAnimation() {
  if (!m_isAnimated)
    return;

  m_currentFrame++;
  if (m_currentFrame >= m_maxFrames)
    m_currentFrame = 0;

  update();
}
