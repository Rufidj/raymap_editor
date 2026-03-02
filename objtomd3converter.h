#ifndef OBJTOMD3CONVERTER_H
#define OBJTOMD3CONVERTER_H

#include <QByteArray>
#include <QColor>
#include <QImage>
#include <QJsonArray>
#include <QJsonObject>
#include <QMap>
#include <QMatrix4x4>
#include <QQuaternion>
#include <QString>
#include <QVector2D>
#include <QVector3D>
#include <QVector>
#include <functional>

struct Md3Triangle {
  int indices[3];
};

struct ObjMaterial {
  QString name;
  QColor color;
  QString texturePath;
  QImage textureImage; // For embedded textures (GLB)
  bool hasTexture = false;

  // UV remapping for atlas
  QVector2D uvScale = QVector2D(1.0f, 1.0f);
  QVector2D uvOffset = QVector2D(0.0f, 0.0f);
  bool useAtlasPadding = false;
  int glbImageIdx = -1; // New field to track shared texture source in GLB
};

class ObjToMd3Converter {
public:
  ObjToMd3Converter();

  bool loadObj(const QString &filename);
  bool loadGlb(const QString &filename); // GLB Support
  bool loadMtl(const QString &filename);
  bool saveMd3(const QString &filename, float scale = 1.0f,
               float rotationDegrees = 0.0f, float orientXDeg = 0.0f,
               float orientYDeg = 0.0f, float orientZDeg = 0.0f,
               float cameraXRot = 0.0f, float cameraYRot = 0.0f);

  // Texture handling
  bool generateTextureAtlas(const QString &outputPath, int size = 512);
  QString debugInfo() const;

  // Processing
  void decimate(int targetTriangles);
  bool mergeTextures(const QString &atlasPath, int atlasSize = 2048);
  int triangleCount() const { return m_triangles.size(); }
  int vertexCount() const { return m_finalVertices.size(); }
  int numFrames() const {
    return m_animationFrames.isEmpty() ? 1 : m_animationFrames.size();
  }

  // Getters for preview
  const QVector<QVector3D> &vertices(int frame = 0) const {
    if (frame >= 0 && frame < m_animationFrames.size())
      return m_animationFrames[frame];
    return m_finalVertices;
  }
  const QVector<QVector<QVector3D>> &animationFrames() const {
    return m_animationFrames;
  }
  const QVector<QVector2D> &texCoords() const { return m_finalTexCoords; }
  const QVector<Md3Triangle> &triangles() const { return m_triangles; }
  const QVector<int> &faceMaterialIndices() const {
    return m_faceMaterialIndices;
  }
  const QVector<QString> &materialNames() const { return m_materialNames; }
  const QMap<QString, ObjMaterial> &materials() const { return m_materials; }

  // Python Integration (Deprecated)
  static bool convertViaPython(const QString &input, const QString &output,
                               double scale, QString &outputLog);

private:
  struct VertexData {
    QVector3D position;
    QVector2D texCoord;
    int originalIndex;
    // Key for map: position indices + uv indices
  };

  QVector<QVector3D> m_rawVertices;
  QVector<QVector2D> m_rawTexCoords;
  QVector<QVector3D> m_finalVertices;
  QVector<QVector2D> m_finalTexCoords;
  QVector<Md3Triangle> m_triangles;
  QVector<int> m_faceMaterialIndices;

  QMap<QString, ObjMaterial> m_materials;
  QVector<QString>
      m_materialNames; // Indexed list matching m_faceMaterialIndices
  QString m_baseTexturePath;

  // Animation Data
  QVector<QVector<QVector3D>> m_animationFrames;

  struct GlbNode {
    QString name;
    int parent = -1;
    QVector<int> children;
    QVector3D translation = QVector3D(0, 0, 0);
    QQuaternion rotation = QQuaternion(1, 0, 0, 0);
    QVector3D scale = QVector3D(1, 1, 1);
    QMatrix4x4 matrix;
    int skin = -1;
    int mesh = -1;
    QMatrix4x4 globalTransform;
  };

  struct GlbAnimation {
    struct Channel {
      int node;
      QString path; // "translation", "rotation", "scale"
      int sampler;
    };
    struct Sampler {
      QVector<float> times;
      QVector<float> values; // Packed floats (3 for pos/scale, 4 for rot)
    };
    QString name;
    QVector<Channel> channels;
    QVector<Sampler> samplers;
    int startFrame = 0;
    int endFrame = 0;
  };

  struct GlbSkin {
    QString name;
    QVector<int> joints;
    QVector<QMatrix4x4> inverseBindMatrices;
    int skeletonRoot = -1;
  };

  QVector<GlbNode> m_glbNodes;
  QVector<GlbAnimation> m_glbAnimations;
  QVector<GlbSkin> m_glbSkins;

  // Vertex skinning data
  struct SkinData {
    int joints[4] = {0, 0, 0, 0};
    float weights[4] = {0, 0, 0, 0};
    int parentNodeIdx = -1;
  };
  QVector<SkinData> m_vertexSkins;

  QJsonArray m_glbAccessors;
  QJsonArray m_glbBufferViews;
  QByteArray m_glbBinData;

  const char *getAccessorData(int accessorIdx, int &count, int &compType,
                              int &stride);

  void bakeAnimations();
  void updateNodeTransforms(QVector<GlbNode> &nodes, int nodeIdx,
                            const QMatrix4x4 &parentTransform);

  // Helper to unify vertices
  void processFaceVertex(int vIdx, int vtIdx, int vnIdx, int matIdx,
                         QMap<QString, int> &vertexMap,
                         QVector<QVector3D> &tempVerts,
                         QVector<QVector2D> &tempUVs);

public:
  // Progress callback: (percentage 0-100, status text)
  std::function<void(int, QString)> onProgress;
  void setProgress(int p, const QString &s) {
    if (onProgress)
      onProgress(p, s);
  }
};

#endif // OBJTOMD3CONVERTER_H
