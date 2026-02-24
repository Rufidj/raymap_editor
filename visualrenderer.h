#ifndef VISUALRENDERER_H
#define VISUALRENDERER_H

#include "mapdata.h"
#include "md3loader.h"
#include <QImage>
#include <QMap>
#include <QMatrix4x4>
#include <QOpenGLBuffer>
#include <QOpenGLFunctions>
#include <QOpenGLShaderProgram>
#include <QOpenGLTexture>
#include <QOpenGLVertexArrayObject>

/**
 * VisualRenderer - OpenGL renderer for the Visual Mode
 *
 * Renders the map geometry (sectors, walls, floors, ceilings) using OpenGL.
 * Handles texture loading, shader management, and camera transformations.
 */
class VisualRenderer : protected QOpenGLFunctions {
public:
  VisualRenderer();
  ~VisualRenderer();

  // Initialization
  bool initialize();
  void cleanup();

  // Map data
  void setMapData(const MapData &mapData);
  void loadTexture(int id, const QImage &image);

  // Camera
  void setCamera(float x, float y, float z, float yaw, float pitch);
  void setProjection(float fov, float aspect, float nearPlane, float farPlane);

  // Rendering
  void render(int width, int height);
  void updateAnimation(float deltaTime);

private:
  // Shader management
  bool createShaders();
  void destroyShaders();

  // Geometry generation
  void generateGeometry(const MapData &mapData);
  void generateSectorGeometry(const Sector &sector);
  void clearGeometry();

  // Rendering helpers
  void renderSectors();
  void renderWalls();
  void renderFloors();
  void renderCeilings();
  void drawSkybox(const QVector3D &cameraPos); // NEW

  // Shader programs
  QOpenGLShaderProgram *m_shaderProgram;

  // Shader uniform locations
  int m_uniformMVP;
  int m_uniformTexture;
  int m_uniformLightLevel;
  int m_uniformTime;
  int m_uniformSectorFlags;
  int m_uniformLiquidIntensity;
  int m_uniformLiquidSpeed;
  float m_time;

  // Geometry buffers
  struct GeometryBuffer {
    QOpenGLBuffer *vbo;
    QOpenGLVertexArrayObject *vao;
    int vertexCount;
    int textureId;
    float lightLevel;
    int flags;
    float liquidIntensity;
    float liquidSpeed;
  };

  QVector<GeometryBuffer> m_wallBuffers;
  QVector<GeometryBuffer> m_floorBuffers;
  QVector<GeometryBuffer> m_ceilingBuffers;
  QVector<GeometryBuffer> m_entityBuffers; // Billboard sprites for entities

  // Sky rendering
  GeometryBuffer m_skyBuffer;
  int m_skyTextureId;

  // Textures
  QMap<int, QOpenGLTexture *> m_textures;
  QOpenGLTexture *m_defaultTexture;

  // Models
  QMap<QString, MD3Loader *> m_models;

  // Camera matrices
  QMatrix4x4 m_viewMatrix;
  QMatrix4x4 m_projectionMatrix;

  // Camera state
  float m_cameraX, m_cameraY, m_cameraZ;
  float m_cameraYaw, m_cameraPitch;

  // Map data (needed for portal lookups)
  MapData m_mapData;

  // Initialization state
  bool m_initialized;
};

#endif // VISUALRENDERER_H
