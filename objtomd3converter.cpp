#include "objtomd3converter.h"
#include <QCoreApplication>
#include <QDataStream>
#include <QDebug>
#include <QFile>
#include <QFileInfo>
#include <QMatrix4x4>
#include <QPainter>
#include <QPainterPath>
#include <QProcess>
#include <QTextStream>
#include <qmath.h>

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonValue>
#include <QRegularExpression>

ObjToMd3Converter::ObjToMd3Converter() {}

bool ObjToMd3Converter::loadMtl(const QString &filename) {
  QFile file(filename);
  if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
    qWarning() << "Could not open MTL file:" << filename;
    return false;
  }

  QTextStream in(&file);
  QString currentMatName;

  while (!in.atEnd()) {
    QString line = in.readLine().trimmed();
    if (line.isEmpty() || line.startsWith("#"))
      continue;

    QStringList parts =
        line.split(QRegularExpression("\\s+"), Qt::SkipEmptyParts);
    if (parts.isEmpty())
      continue;

    if (parts[0] == "newmtl") {
      currentMatName = parts.mid(1).join(" ");
      ObjMaterial mat;
      mat.name = currentMatName;
      mat.color = Qt::gray;
      m_materials[currentMatName] = mat;
      m_materialNames.append(currentMatName);
    } else if (parts[0] == "Kd" && !currentMatName.isEmpty()) {
      if (parts.size() >= 4) {
        int r = static_cast<int>(parts[1].toFloat() * 255);
        int g = static_cast<int>(parts[2].toFloat() * 255);
        int b = static_cast<int>(parts[3].toFloat() * 255);
        m_materials[currentMatName].color = QColor(r, g, b);
      }
    } else if (parts[0] == "map_Kd" && !currentMatName.isEmpty()) {
      QString rawPath = parts.mid(1).join(" ");
      rawPath.replace("\\", "/"); // Normalize slashes

      qDebug() << "MTL: Processing map_Kd for material" << currentMatName
               << "- raw path:" << rawPath;

      QFileInfo fiMtl(filename);
      QString mtlDir = fiMtl.absolutePath();
      bool found = false;

      // Search strategy
      QStringList searchPaths;
      searchPaths << mtlDir + "/" + rawPath; // 1. Path in MTL relative to MTL
      searchPaths
          << mtlDir + "/" +
                 QFileInfo(rawPath).fileName(); // 2. Filename only in MTL dir
      searchPaths
          << mtlDir + "/textures/" +
                 QFileInfo(rawPath).fileName(); // 3. Subfolder 'textures'
      searchPaths << mtlDir + "/images/" +
                         QFileInfo(rawPath).fileName(); // 4. Subfolder 'images'

      // Extraction of path from options like -o -s etc
      if (parts.size() > 2) {
        QString lastPart = parts.last();
        searchPaths << mtlDir + "/" + lastPart;
        searchPaths << mtlDir + "/textures/" + lastPart;
      }

      for (const QString &tryPath : searchPaths) {
        if (QFile::exists(tryPath)) {
          m_materials[currentMatName].texturePath = tryPath;
          found = true;
          qDebug() << "  ✓ Found texture for" << currentMatName
                   << "at:" << tryPath;
          break;
        }
      }

      if (!found) {
        m_materials[currentMatName].texturePath = rawPath;
        qDebug() << "  ✗ Texture NOT found, keeping raw path:" << rawPath;
      }
    }
  }

  return true;
}

bool ObjToMd3Converter::loadObj(const QString &filename) {
  setProgress(0, "Abriendo archivo OBJ...");
  QFile file(filename);
  if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
    return false;
  }

  // Speculative MTL loading removed - relying on mtllib directive inside OBJ

  m_rawVertices.clear();
  m_rawTexCoords.clear();
  m_finalVertices.clear();
  m_finalTexCoords.clear();
  m_triangles.clear();
  m_faceMaterialIndices.clear();
  m_materials.clear();
  m_materialNames.clear();
  m_vertexSkins.clear();
  m_glbNodes.clear();
  m_glbAnimations.clear();
  m_glbSkins.clear();
  m_animationFrames.clear();
  // Pre-add dummy UV if needed (though logic usually handles it)

  QTextStream in(&file);
  int currentMatIdx = -1;
  QMap<QString, int>
      vertexCache; // Key: "v/vt/vn", Value: index in m_finalVertices

  qint64 totalBytes = file.size();
  qint64 readBytes = 0;

  while (!in.atEnd()) {
    QString line = in.readLine().trimmed();
    readBytes += line.length() + 1;

    // Don't spam progress updates
    if (readBytes % 50000 == 0) {
      int pct = (int)((double)readBytes / totalBytes * 60.0); // Map to 0-60%
      setProgress(pct, QString("Analizando lineas... %1%").arg(pct));
    }

    if (line.isEmpty() || line.startsWith("#"))
      continue;

    QStringList parts =
        line.split(QRegularExpression("\\s+"), Qt::SkipEmptyParts);
    if (parts.isEmpty())
      continue;

    if (parts[0] == "v") {
      m_rawVertices.append(QVector3D(parts[1].toFloat(), parts[2].toFloat(),
                                     parts[3].toFloat()));
    } else if (parts[0] == "vt") {
      m_rawTexCoords.append(QVector2D(parts[1].toFloat(), parts[2].toFloat()));
    } else if (parts[0] == "mtllib") {
      QString mtlFileName = parts.mid(1).join(" ");
      mtlFileName.replace("\\", "/");
      QFileInfo fiObj(filename);
      QString mtlPath = fiObj.absolutePath() + "/" + mtlFileName;
      loadMtl(mtlPath);
    } else if (parts[0] == "usemtl") {
      QString matName = parts.mid(1).join(" ");
      // Find index
      currentMatIdx = m_materialNames.indexOf(matName);
      if (currentMatIdx == -1) {
        // Add if not exists (handling missing mtl definition)
        m_materialNames.append(matName);
        ObjMaterial mat;
        mat.name = matName;
        mat.color = Qt::gray;
        m_materials[matName] = mat;
        currentMatIdx = m_materialNames.size() - 1;
      }
    } else if (parts[0] == "f") {
      // Triangulate fan
      QVector<int> faceIndices;
      for (int i = 1; i < parts.size(); ++i) {
        QStringList idxParts = parts[i].split('/');
        int vIdx = idxParts[0].toInt() - 1;
        int vtIdx = (idxParts.size() > 1 && !idxParts[1].isEmpty())
                        ? idxParts[1].toInt() - 1
                        : -1;
        int vnIdx = (idxParts.size() > 2 && !idxParts[2].isEmpty())
                        ? idxParts[2].toInt() - 1
                        : -1;

        // Create unique vertex key
        QString key = QString("%1/%2/%3").arg(vIdx).arg(vtIdx).arg(vnIdx);

        if (!vertexCache.contains(key)) {
          // Add new vertex
          QVector3D pos = m_rawVertices[vIdx];
          QVector2D tex = (vtIdx >= 0 && vtIdx < m_rawTexCoords.size())
                              ? m_rawTexCoords[vtIdx]
                              : QVector2D(0.5f, 0.5f);

          vertexCache[key] = m_finalVertices.size();
          m_finalVertices.append(pos); // Keep original coords (y up)
          m_finalTexCoords.append(tex);
        }
        faceIndices.append(vertexCache[key]);
      }

      // Triangulate
      for (int i = 1; i < faceIndices.size() - 1; ++i) {
        Md3Triangle tri;
        tri.indices[0] = faceIndices[0];
        tri.indices[1] = faceIndices[i];
        tri.indices[2] = faceIndices[i + 1];
        m_triangles.append(tri);
        m_faceMaterialIndices.append(currentMatIdx);
      }
    }
  }

  // Load all textures into memory immediately after loading the mesh
  for (auto it = m_materials.begin(); it != m_materials.end(); ++it) {
    ObjMaterial &mat = it.value();
    if (!mat.texturePath.isEmpty() && !mat.hasTexture) {
      if (mat.textureImage.load(mat.texturePath)) {
        mat.hasTexture = true;
      }
    }
  }

  return true;
}

bool ObjToMd3Converter::loadGlb(const QString &filename) {
  setProgress(10, "Cargando GLB...");
  QFile file(filename);
  if (!file.open(QIODevice::ReadOnly))
    return false;

  QDataStream in(&file);
  in.setByteOrder(QDataStream::LittleEndian);

  quint32 magic, version, length;
  in >> magic >> version >> length;
  if (magic != 0x46546C67)
    return false;

  quint32 jsonChunkLen, jsonChunkType;
  in >> jsonChunkLen >> jsonChunkType;
  if (jsonChunkType != 0x4E4F534A)
    return false;

  QByteArray jsonData = file.read(jsonChunkLen);
  QByteArray binData;
  if (!file.atEnd()) {
    quint32 binChunkLen, binChunkType;
    in >> binChunkLen >> binChunkType;
    if (binChunkType == 0x004E4942)
      binData = file.read(binChunkLen);
  }

  QJsonDocument doc = QJsonDocument::fromJson(jsonData);
  if (doc.isNull())
    return false;
  QJsonObject root = doc.object();

  m_glbAccessors = root["accessors"].toArray();
  m_glbBufferViews = root["bufferViews"].toArray();
  m_glbBinData = binData;

  m_rawVertices.clear();
  m_rawTexCoords.clear();
  m_finalVertices.clear();
  m_finalTexCoords.clear();
  m_triangles.clear();
  m_faceMaterialIndices.clear();
  m_materials.clear();
  m_materialNames.clear();
  m_vertexSkins.clear();
  m_glbNodes.clear();
  m_glbAnimations.clear();
  m_glbSkins.clear();
  m_animationFrames.clear();

  QJsonArray images = root["images"].toArray();
  QJsonArray textures = root["textures"].toArray();
  QJsonArray materials = root["materials"].toArray();
  QJsonArray meshes = root["meshes"].toArray();
  QJsonArray nodes = root["nodes"].toArray();

  QVector<QImage> loadedImages;
  for (const QJsonValue &iv : images) {
    QJsonObject imgObj = iv.toObject();
    QImage img;
    if (imgObj.contains("bufferView")) {
      int bvIdx = imgObj["bufferView"].toInt(-1);
      if (bvIdx >= 0 && bvIdx < m_glbBufferViews.size()) {
        QJsonObject bv = m_glbBufferViews[bvIdx].toObject();
        int bvOffset = bv["byteOffset"].toInt(0);
        int bvLen = bv["byteLength"].toInt();
        img.loadFromData(binData.mid(bvOffset, bvLen));
      }
    }
    loadedImages.append(img);
  }

  for (int i = 0; i < materials.size(); ++i) {
    QJsonObject mat = materials[i].toObject();
    QString name = mat["name"].toString(QString("Material_%1").arg(i));
    ObjMaterial m;
    m.glbImageIdx = -1;
    QColor color = Qt::gray;
    bool hasTex = false;

    if (mat.contains("pbrMetallicRoughness")) {
      QJsonObject pbr = mat["pbrMetallicRoughness"].toObject();
      if (pbr.contains("baseColorFactor")) {
        QJsonArray f = pbr["baseColorFactor"].toArray();
        color = QColor(f[0].toDouble() * 255, f[1].toDouble() * 255,
                       f[2].toDouble() * 255);
      }
      if (pbr.contains("baseColorTexture")) {
        int tIdx = pbr["baseColorTexture"].toObject()["index"].toInt(-1);
        if (tIdx >= 0 && tIdx < textures.size()) {
          int sIdx = textures[tIdx].toObject()["source"].toInt(-1);
          if (sIdx >= 0 && sIdx < loadedImages.size()) {
            m.textureImage = loadedImages[sIdx];
            m.hasTexture = !m.textureImage.isNull();
            m.glbImageIdx = sIdx;
          }
        }
      }
    }
    m.name = name;
    m.color = color;
    m_materials[name] = m;
    m_materialNames.append(name);
  }

  for (const QJsonValue &nv : nodes) {
    QJsonObject n = nv.toObject();
    GlbNode node;
    node.name = n["name"].toString();
    if (n.contains("translation")) {
      QJsonArray t = n["translation"].toArray();
      node.translation =
          QVector3D(t[0].toDouble(), t[1].toDouble(), t[2].toDouble());
    }
    if (n.contains("rotation")) {
      QJsonArray r = n["rotation"].toArray();
      node.rotation = QQuaternion(r[3].toDouble(), r[0].toDouble(),
                                  r[1].toDouble(), r[2].toDouble());
    }
    if (n.contains("scale")) {
      QJsonArray s = n["scale"].toArray();
      node.scale = QVector3D(s[0].toDouble(), s[1].toDouble(), s[2].toDouble());
    }
    if (n.contains("matrix")) {
      QJsonArray m = n["matrix"].toArray();
      for (int row = 0; row < 4; ++row)
        for (int col = 0; col < 4; ++col)
          node.matrix(row, col) = m[col * 4 + row].toDouble();
    }
    if (n.contains("children")) {
      for (const QJsonValue &cv : n["children"].toArray())
        node.children.append(cv.toInt());
    }
    node.skin = n["skin"].toInt(-1);
    node.mesh = n["mesh"].toInt(-1);
    m_glbNodes.append(node);
  }

  for (int i = 0; i < m_glbNodes.size(); ++i) {
    for (int child : m_glbNodes[i].children) {
      if (child >= 0 && child < m_glbNodes.size())
        m_glbNodes[child].parent = i;
    }
  }

  QJsonArray skins = root["skins"].toArray();
  for (const QJsonValue &sv : skins) {
    QJsonObject s = sv.toObject();
    GlbSkin skin;
    skin.name = s["name"].toString();
    for (const QJsonValue &jv : s["joints"].toArray())
      skin.joints.append(jv.toInt());
    int ibmIdx = s["inverseBindMatrices"].toInt(-1);
    if (ibmIdx >= 0) {
      int count, compType, stride;
      const char *data = getAccessorData(ibmIdx, count, compType, stride);
      if (data && compType == 5126) {
        const float *f = reinterpret_cast<const float *>(data);
        for (int i = 0; i < count; ++i) {
          QMatrix4x4 m;
          for (int r = 0; r < 4; ++r)
            for (int c = 0; c < 4; ++c)
              m(r, c) = f[i * 16 + c * 4 + r];
          skin.inverseBindMatrices.append(m);
        }
      }
    }
    m_glbSkins.append(skin);
  }

  QJsonArray animations = root["animations"].toArray();
  for (const QJsonValue &av : animations) {
    QJsonObject a = av.toObject();
    GlbAnimation anim;
    anim.name = a["name"].toString();
    QJsonArray samplers = a["samplers"].toArray();
    for (const QJsonValue &sv : samplers) {
      QJsonObject s = sv.toObject();
      GlbAnimation::Sampler sampler;
      int inIdx = s["input"].toInt(), outIdx = s["output"].toInt();
      int count, compType, stride;
      const char *inData = getAccessorData(inIdx, count, compType, stride);
      if (inData && compType == 5126) {
        const float *f = reinterpret_cast<const float *>(inData);
        for (int i = 0; i < count; ++i)
          sampler.times.append(f[i]);
      }
      const char *outData = getAccessorData(outIdx, count, compType, stride);
      if (outData && compType == 5126) {
        const float *f = reinterpret_cast<const float *>(outData);
        QString typeStr = m_glbAccessors[outIdx].toObject()["type"].toString();
        int components =
            (typeStr == "VEC4")
                ? 4
                : (typeStr == "VEC3" ? 3 : (typeStr == "VEC2" ? 2 : 1));
        for (int i = 0; i < count * components; ++i)
          sampler.values.append(f[i]);
      }
      anim.samplers.append(sampler);
    }
    QJsonArray channels = a["channels"].toArray();
    for (const QJsonValue &cv : channels) {
      QJsonObject c = cv.toObject();
      GlbAnimation::Channel channel;
      channel.sampler = c["sampler"].toInt();
      QJsonObject target = c["target"].toObject();
      channel.node = target["node"].toInt();
      channel.path = target["path"].toString();
      anim.channels.append(channel);
    }
    m_glbAnimations.append(anim);
  }

  for (int nIdx = 0; nIdx < m_glbNodes.size(); ++nIdx) {
    const auto &node = m_glbNodes[nIdx];
    if (node.mesh == -1)
      continue;
    QJsonObject meshObj = meshes[node.mesh].toObject();
    QJsonArray primitives = meshObj["primitives"].toArray();
    for (const QJsonValue &pv : primitives) {
      QJsonObject prim = pv.toObject();
      QJsonObject attrs = prim["attributes"].toObject();
      int posIdx = attrs["POSITION"].toInt(-1),
          uvIdx = attrs["TEXCOORD_0"].toInt(-1);
      int jointIdx = attrs["JOINTS_0"].toInt(-1),
          weightIdx = attrs["WEIGHTS_0"].toInt(-1);
      int indicesIdx = prim["indices"].toInt(-1),
          matIdx = prim["material"].toInt(-1);
      if (posIdx == -1)
        continue;

      int vCount, vCompType, vStride;
      const char *vDataRaw =
          getAccessorData(posIdx, vCount, vCompType, vStride);
      if (!vDataRaw)
        continue;
      if (vStride == 0)
        vStride = 12;

      int uvCount, uvCompType, uvStride;
      const char *uvRaw =
          (uvIdx >= 0) ? getAccessorData(uvIdx, uvCount, uvCompType, uvStride)
                       : nullptr;
      if (uvRaw && uvStride == 0)
        uvStride = (uvCompType == 5126 ? 8 : (uvCompType == 5123 ? 4 : 2));

      int baseVertexIndex = m_finalVertices.size();
      for (int i = 0; i < vCount; i++) {
        const float *vPtr =
            reinterpret_cast<const float *>(vDataRaw + i * vStride);
        m_finalVertices.append(QVector3D(vPtr[0], vPtr[1], vPtr[2]));
        if (uvRaw && i < uvCount) {
          float u = 0, v = 0;
          const char *uvPtr = uvRaw + i * uvStride;
          if (uvCompType == 5126) {
            u = reinterpret_cast<const float *>(uvPtr)[0];
            v = reinterpret_cast<const float *>(uvPtr)[1];
          } else if (uvCompType == 5123) {
            u = reinterpret_cast<const quint16 *>(uvPtr)[0] / 65535.f;
            v = reinterpret_cast<const quint16 *>(uvPtr)[1] / 65535.f;
          }
          m_finalTexCoords.append(QVector2D(u, v));
        } else
          m_finalTexCoords.append(QVector2D(0.5f, 0.5f));

        SkinData sd;
        sd.parentNodeIdx = nIdx;
        if (jointIdx >= 0 && weightIdx >= 0) {
          int jCount, jCompType, jStride;
          const char *jData =
              getAccessorData(jointIdx, jCount, jCompType, jStride);
          if (jData) {
            if (jStride == 0)
              jStride = (jCompType == 5123 ? 8 : 4);
            const char *jPtr = jData + i * jStride;
            for (int j = 0; j < 4; ++j)
              sd.joints[j] =
                  (jCompType == 5123
                       ? (int)reinterpret_cast<const quint16 *>(jPtr)[j]
                       : (int)reinterpret_cast<const quint8 *>(jPtr)[j]);
          }
          int wCount, wCompType, wStride;
          const char *wData =
              getAccessorData(weightIdx, wCount, wCompType, wStride);
          if (wData) {
            if (wStride == 0)
              wStride = 16;
            const float *wPtr =
                reinterpret_cast<const float *>(wData + i * wStride);
            for (int j = 0; j < 4; ++j)
              sd.weights[j] = wPtr[j];
          }
        }
        m_vertexSkins.append(sd);
      }

      if (indicesIdx >= 0) {
        int iCount, iCompType, iStride;
        const char *iData =
            getAccessorData(indicesIdx, iCount, iCompType, iStride);
        if (iData) {
          for (int i = 0; i < iCount; i += 3) {
            Md3Triangle tri;
            if (iCompType == 5123) {
              const quint16 *p = reinterpret_cast<const quint16 *>(iData);
              tri.indices[0] = baseVertexIndex + p[i];
              tri.indices[1] = baseVertexIndex + p[i + 1];
              tri.indices[2] = baseVertexIndex + p[i + 2];
            } else {
              const quint32 *p = reinterpret_cast<const quint32 *>(iData);
              tri.indices[0] = baseVertexIndex + p[i];
              tri.indices[1] = baseVertexIndex + p[i + 1];
              tri.indices[2] = baseVertexIndex + p[i + 2];
            }
            m_triangles.append(tri);
            m_faceMaterialIndices.append(matIdx);
          }
        }
      }
    }
  }

  bakeAnimations();
  return true;
}

bool ObjToMd3Converter::saveMd3(const QString &filename, float scale,
                                float rotationDegrees, float orientXDeg,
                                float orientYDeg, float orientZDeg,
                                float cameraXRot, float cameraYRot) {
  QFile file(filename);
  if (!file.open(QIODevice::WriteOnly))
    return false;

  QDataStream out(&file);
  out.setByteOrder(QDataStream::LittleEndian);
  out.setFloatingPointPrecision(QDataStream::SinglePrecision);

  // Apply transformations
  QMatrix4x4 transform;
  transform.rotate(orientXDeg, 1.0f, 0.0f, 0.0f);
  transform.rotate(orientYDeg, 0.0f, 1.0f, 0.0f);
  transform.rotate(orientZDeg, 0.0f, 0.0f, 1.0f);
  transform.rotate(rotationDegrees, 0.0f, 0.0f, 1.0f);

  QVector<QVector<QVector3D>> allFrames;
  int frameCount = numFrames();

  for (int f = 0; f < frameCount; ++f) {
    QVector<QVector3D> frameVerts;
    const QVector<QVector3D> &sourceVerts = vertices(f);
    for (const QVector3D &v : sourceVerts) {
      QVector3D tv = transform.map(v);
      // Coordinate System Mapping: GLTF (Y-Up, Z-Forward) -> MD3 (Z-Up,
      // X-Forward, Y-Left) GLTF X(Right) -> MD3 -Y(Left) GLTF Y(Up)    -> MD3
      // Z(Up) GLTF Z(Fwd)   -> MD3 X(Fwd)
      frameVerts.append(QVector3D(tv.z(), -tv.x(), tv.y()));
    }
    allFrames.append(frameVerts);
  }

  // Group triangles by material
  QMap<int, QVector<Md3Triangle>> materialTriangles;
  for (int i = 0; i < m_triangles.size(); ++i) {
    int matIdx =
        (i < m_faceMaterialIndices.size()) ? m_faceMaterialIndices[i] : -1;
    materialTriangles[matIdx].append(m_triangles[i]);
  }

  // Count active surfaces
  int numSurfaces = materialTriangles.size();
  if (numSurfaces == 0)
    numSurfaces = 1;

  // Header
  out.writeRawData("IDP3", 4);
  out << (int)15; // Version

  char nameBuf[64] = {0};
  QByteArray surfName = QFileInfo(filename).baseName().toLatin1();
  surfName.truncate(63);
  strcpy(nameBuf, surfName.constData());
  out.writeRawData(nameBuf, 64);

  out << (int)0;           // Flags
  out << (int)frameCount;  // Num frames
  out << (int)0;           // Num tags
  out << (int)numSurfaces; // Multiple surfaces!
  out << (int)0;           // Num skins

  int ofs_frames = 108;
  int ofs_tags = ofs_frames + (56 * frameCount);
  int ofs_surfaces = ofs_tags;
  int ofs_eof = 0;

  out << ofs_frames << ofs_tags << ofs_surfaces << ofs_eof;

  // Frame block (MD3 needs one entry per frame)
  for (int f = 0; f < frameCount; ++f) {
    float minX = 99999, minY = 99999, minZ = 99999;
    float maxX = -99999, maxY = -99999, maxZ = -99999;
    const QVector<QVector3D> &frameVerts = allFrames[f];
    for (const QVector3D &v : frameVerts) {
      float x = v.x() * scale;
      float y = v.y() * scale;
      float z = v.z() * scale;
      minX = qMin(minX, x);
      maxX = qMax(maxX, x);
      minY = qMin(minY, y);
      maxY = qMax(maxY, y);
      minZ = qMin(minZ, z);
      maxZ = qMax(maxZ, z);
    }
    out << minX << minY << minZ << maxX << maxY << maxZ;
    out << 0.0f << 0.0f
        << 0.0f; // Origin MUST be fixed at (0,0,0) to prevent animation jumping
    out << qMax(qAbs(maxX),
                qMax(qAbs(maxY), qAbs(maxZ))); // Radius based on center
    char frameNameBuf[16] = {0};
    sprintf(frameNameBuf, "frame_%d", f);
    out.writeRawData(frameNameBuf, 16);
  }

  // Surfaces
  QList<int> matIndices = materialTriangles.keys();
  for (int sIdx = 0; sIdx < matIndices.size(); ++sIdx) {
    int matIdx = matIndices[sIdx];
    QVector<Md3Triangle> &tris = materialTriangles[matIdx];

    // Find unique vertices used by this material to keep surface clean
    QSet<int> usedGlobalVerts;
    for (const auto &t : tris) {
      usedGlobalVerts.insert(t.indices[0]);
      usedGlobalVerts.insert(t.indices[1]);
      usedGlobalVerts.insert(t.indices[2]);
    }

    QList<int> sortedVerts = usedGlobalVerts.values();
    std::sort(sortedVerts.begin(), sortedVerts.end());

    QMap<int, int> globalToLocal;
    for (int i = 0; i < sortedVerts.size(); ++i)
      globalToLocal[sortedVerts[i]] = i;

    qint64 surfStart = file.pos();
    out.writeRawData("IDP3", 4);

    QString matName = (matIdx >= 0 && matIdx < m_materialNames.size())
                          ? m_materialNames[matIdx]
                          : "default";
    QByteArray matNameUtf8 = matName.toLatin1();
    matNameUtf8.truncate(63);
    char surfNameBuf[64] = {0};
    strcpy(surfNameBuf, matNameUtf8.constData());
    out.writeRawData(surfNameBuf, 64);

    ObjMaterial &mat = m_materials[matName];

    out << (int)0;                  // Flags
    out << (int)frameCount;         // Frames
    out << (int)0;                  // Shaders
    out << (int)sortedVerts.size(); // Num verts
    out << (int)tris.size();        // Num triangles

    int ofs_s_tris = 108;
    int ofs_s_shaders = ofs_s_tris + tris.size() * 12;
    int ofs_s_st = ofs_s_shaders;
    int ofs_s_verts = ofs_s_st + sortedVerts.size() * 8;
    int ofs_s_end = ofs_s_verts + sortedVerts.size() * 8 * frameCount;

    out << ofs_s_tris << ofs_s_shaders << ofs_s_st << ofs_s_verts << ofs_s_end;

    // Triangles
    for (const auto &t : tris) {
      out << globalToLocal[t.indices[0]] << globalToLocal[t.indices[1]]
          << globalToLocal[t.indices[2]];
    }

    // UVs
    for (int gIdx : sortedVerts) {
      QVector2D uv = m_finalTexCoords[gIdx];

      // Apply material-specific UV transform (for atlas)
      uv.setX(uv.x() - floor(uv.x())); // Handle tiling
      uv.setY(uv.y() - floor(uv.y()));

      float newU = mat.uvOffset.x() + uv.x() * mat.uvScale.x();
      float newV = mat.uvOffset.y() + uv.y() * mat.uvScale.y();

      out << newU << newV; // V=0 is TOP for libmod_ray
    }

    // Vertices (multiple frames)
    for (int f = 0; f < frameCount; ++f) {
      for (int gIdx : sortedVerts) {
        QVector3D v = allFrames[f][gIdx];
        // Bake the user-provided scale back into the mesh
        // MD3 coordinates are int16 scaled by 1/64.0f by the engine
        out << (short)qBound(-32768, (int)(v.x() * scale * 64.0f), 32767);
        out << (short)qBound(-32768, (int)(v.y() * scale * 64.0f), 32767);
        out << (short)qBound(-32768, (int)(v.z() * scale * 64.0f), 32767);
        out << (short)0; // Normal placeholder
      }
    }

    // Update Surface Offsets
    qint64 currentPos = file.pos();
    file.seek(surfStart + 104);
    out << (int)(currentPos - surfStart); // offsetEnd
    file.seek(currentPos);
  }

  // Final EOF
  ofs_eof = file.pos();
  file.seek(104);
  out << ofs_eof;

  // Generate model animation config if we have animations from GLB
  if (!m_glbAnimations.isEmpty()) {
    QFileInfo md3Info(filename);
    QString cfgName = md3Info.baseName() + ".cfg";
    QString cfgPath = md3Info.absolutePath() + "/" + cfgName;
    QFile cfgFile(cfgPath);
    if (cfgFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
      QTextStream stream(&cfgFile);
      stream << "// MD3 Animation config generated by RayMap Editor\n";
      stream << "// first-frame, num-frames, looping-frames, "
                "frames-per-second\n\n";
      for (const auto &anim : m_glbAnimations) {
        int length = anim.endFrame - anim.startFrame + 1;
        stream << QString("%1\t%2\t%3\t%4\t// %5\n")
                      .arg(anim.startFrame)
                      .arg(length)
                      .arg(length) // looping frames (default to all)
                      .arg(24)     // fps (default)
                      .arg(anim.name.isEmpty() ? "anim" : anim.name);
      }
      cfgFile.close();
      qDebug() << "Generated animation config at" << cfgPath;
    }
  }

  return true;
}

bool ObjToMd3Converter::generateTextureAtlas(const QString &outputPath,
                                             int size) {
  if (m_finalTexCoords.isEmpty()) {
    qDebug() << "Atlas generation skipped: No UVs";
    return false;
  }

  qDebug() << "Generating texture atlas:" << outputPath << "size:" << size;
  qDebug() << "  Materials:" << m_materials.size()
           << "Triangles:" << m_triangles.size();

  // PRE-LOAD all external textures before the loop
  int texturesLoaded = 0;
  int texturesFailed = 0;
  QVector<QString> loadedTexturePaths;

  for (auto it = m_materials.begin(); it != m_materials.end(); ++it) {
    ObjMaterial &mat = it.value();

    // Skip if already has embedded texture
    if (mat.hasTexture) {
      qDebug() << "  Material" << mat.name
               << "has embedded texture, size:" << mat.textureImage.size();
      loadedTexturePaths.append(mat.name + "_embedded");
      continue;
    }

    // Try to load external texture
    if (!mat.texturePath.isEmpty() && QFile::exists(mat.texturePath)) {
      qDebug() << "  Loading texture for material" << mat.name
               << "from:" << mat.texturePath;

      if (mat.textureImage.load(mat.texturePath)) {
        mat.hasTexture = true;
        texturesLoaded++;
        loadedTexturePaths.append(mat.texturePath);
        qDebug() << "    ✓ Loaded successfully, size:"
                 << mat.textureImage.size();
      } else {
        texturesFailed++;
        qDebug() << "    ✗ Failed to load image file";
      }
    } else if (!mat.texturePath.isEmpty()) {
      qDebug() << "  Material" << mat.name
               << "texture path does not exist:" << mat.texturePath;
    }
  }

  qDebug() << "Pre-load complete. Textures loaded:" << texturesLoaded
           << "failed:" << texturesFailed;

  // Pre-load all external textures before the loop
  for (auto it = m_materials.begin(); it != m_materials.end(); ++it) {
    ObjMaterial &mat = it.value();
    if (!mat.hasTexture && !mat.texturePath.isEmpty()) {
      mat.textureImage.load(mat.texturePath);
      mat.hasTexture = !mat.textureImage.isNull();
    }
  }

  // Single texture case: optimization
  if (m_materials.size() == 1) {
    ObjMaterial &mat = m_materials.first();
    if (mat.hasTexture && !mat.textureImage.isNull()) {
      QImage atlas = mat.textureImage.scaled(size, size, Qt::IgnoreAspectRatio,
                                             Qt::SmoothTransformation);
      return atlas.save(outputPath);
    }
  }

  // Multiple materials: need proper UV baking
  qDebug() << "Multiple materials, using UV-based baking";

  QImage image(size, size, QImage::Format_ARGB32);
  image.fill(QColor(128, 128, 128));

  QPainter painter(&image);

  // Draw each triangle with its material color
  for (int i = 0; i < m_triangles.size(); ++i) {
    const Md3Triangle &tri = m_triangles[i];
    int matIdx =
        (i < m_faceMaterialIndices.size()) ? m_faceMaterialIndices[i] : -1;

    ObjMaterial *mat = nullptr;
    if (matIdx >= 0 && matIdx < m_materialNames.size()) {
      mat = &m_materials[m_materialNames[matIdx]];
    }

    QVector2D uv1 = m_finalTexCoords[tri.indices[0]];
    QVector2D uv2 = m_finalTexCoords[tri.indices[1]];
    QVector2D uv3 = m_finalTexCoords[tri.indices[2]];

    QPointF points[3] = {QPointF(uv1.x() * size, uv1.y() * size),
                         QPointF(uv2.x() * size, uv2.y() * size),
                         QPointF(uv3.x() * size, uv3.y() * size)};

    if (mat && mat->hasTexture && !mat->textureImage.isNull()) {
      // High quality bake: use UV transformation to draw the actual sub-texture
      // We use the center point as fallback but for the atlas we should ideally
      // draw more accurately.
      // For now, let's at least pick the color from the right spot.
      float cx = (uv1.x() + uv2.x() + uv3.x()) / 3.0f;
      float cy = (uv1.y() + uv2.y() + uv3.y()) / 3.0f;

      // Handle tiling in atlas too
      cx = cx - floor(cx);
      cy = cy - floor(cy);

      int tx = qBound(0, (int)(cx * mat->textureImage.width()),
                      mat->textureImage.width() - 1);
      int ty = qBound(0, (int)(cy * mat->textureImage.height()),
                      mat->textureImage.height() - 1);

      QColor c = mat->textureImage.pixelColor(tx, ty);
      painter.setBrush(c);
      painter.setPen(QPen(c, 1.0));
    } else {
      QColor c = mat ? mat->color : Qt::gray;
      painter.setBrush(c);
      painter.setPen(QPen(c, 1.0));
    }
    painter.drawPolygon(points, 3);
  }

  qDebug() << "Atlas baking complete, saving to:" << outputPath;
  return image.save(outputPath);
}

QString ObjToMd3Converter::debugInfo() const {
  return QString("Vertices: %1, Triangles: %2, Materials: %3")
      .arg(m_finalVertices.size())
      .arg(m_triangles.size())
      .arg(m_materials.size());
}

bool ObjToMd3Converter::convertViaPython(const QString &input,
                                         const QString &output, double scale,
                                         QString &outputLog) {
  // ... code ...
  return false; // Deprecated or Fallback
}

void ObjToMd3Converter::decimate(int targetTriangles) {
  if (m_triangles.size() <= targetTriangles)
    return;

  double ratio = (double)targetTriangles / m_triangles.size();
  int step = qMax(1, (int)(1.0 / ratio));

  QVector<Md3Triangle> newTris;
  QVector<int> newMatIndices;

  for (int i = 0; i < m_triangles.size(); i += step) {
    newTris.append(m_triangles[i]);
    if (i < m_faceMaterialIndices.size())
      newMatIndices.append(m_faceMaterialIndices[i]);
  }

  QVector<bool> used(m_finalVertices.size(), false);
  for (const auto &t : newTris) {
    used[t.indices[0]] = true;
    used[t.indices[1]] = true;
    used[t.indices[2]] = true;
  }

  QVector<QVector3D> packedV;
  QVector<QVector2D> packedUV;
  QVector<int> map(m_finalVertices.size(), -1);

  for (int i = 0; i < m_finalVertices.size(); ++i) {
    if (used[i]) {
      map[i] = packedV.size();
      packedV.append(m_finalVertices[i]);
      packedUV.append(m_finalTexCoords[i]);
    }
  }

  for (auto &t : newTris) {
    t.indices[0] = map[t.indices[0]];
    t.indices[1] = map[t.indices[1]];
    t.indices[2] = map[t.indices[2]];
  }

  m_triangles = newTris;
  m_faceMaterialIndices = newMatIndices;
  m_finalVertices = packedV;
  m_finalTexCoords = packedUV;

  qDebug() << "Decimated to" << m_triangles.size() << "triangles";
}

bool ObjToMd3Converter::mergeTextures(const QString &atlasPath, int atlasSize) {
  QSet<int> usedMatSet;
  for (int idx : m_faceMaterialIndices)
    usedMatSet.insert(idx);

  QVector<int> activeMats = usedMatSet.values().toVector();
  std::sort(activeMats.begin(), activeMats.end()); // Predictable grid

  if (activeMats.size() <= 1)
    return false;

  int numMats = activeMats.size();
  int cols = qCeil(qSqrt(numMats));
  int rows = qCeil((double)numMats / cols);
  int cellW = atlasSize / cols;
  int cellH = atlasSize / rows;

  QImage atlas(atlasSize, atlasSize, QImage::Format_ARGB32);
  atlas.fill(Qt::gray);
  QPainter painter(&atlas);

  struct UVTransform {
    double u_off, v_off, u_scale, v_scale;
  };
  QMap<int, UVTransform> transforms;

  for (int i = 0; i < numMats; ++i) {
    int matIdx = activeMats[i];
    QString matName = m_materialNames[matIdx];
    ObjMaterial &mat = m_materials[matName];

    int r = i / cols;
    int c = i % cols;
    int x = c * cellW;
    int y = r * cellH; // Y grows down

    // Draw texture
    if (mat.hasTexture) {
      painter.drawImage(QRect(x, y, cellW, cellH), mat.textureImage);
    } else if (!mat.texturePath.isEmpty()) {
      if (QFile::exists(mat.texturePath)) {
        QImage img(mat.texturePath);
        painter.drawImage(QRect(x, y, cellW, cellH), img);
      } else {
        painter.fillRect(x, y, cellW, cellH, mat.color);
      }
    } else {
      painter.fillRect(x, y, cellW, cellH, mat.color);
    }

    // V offset for the TOP of the cell
    mat.uvScale = QVector2D(1.0f / cols, 1.0f / rows);
    mat.uvOffset = QVector2D((float)c / cols, (float)r / rows);
  }

  atlas.save(atlasPath);
  qDebug() << "Atlas saved and material UV transforms updated.";

  // Remap Vertices
  QVector<QVector2D> newUVs = m_finalTexCoords;

  // Fix 'Shifted' texture issue: if all materials reference the same texture
  // source (file or GLB image index), don't separate them into quadrants.
  QSet<QString> uniqueTexturePaths;
  QSet<int> uniqueGlbImageIndices;

  for (int matIdx : activeMats) {
    const QString &mName = m_materialNames[matIdx];
    const ObjMaterial &mat = m_materials[mName];
    if (mat.hasTexture) {
      if (!mat.texturePath.isEmpty())
        uniqueTexturePaths.insert(mat.texturePath);
      if (mat.glbImageIdx >= 0)
        uniqueGlbImageIndices.insert(mat.glbImageIdx);
    }
  }

  bool allHaveSameTexture =
      (uniqueTexturePaths.size() + uniqueGlbImageIndices.size()) == 1;
  bool noneMissingTexture = true;
  for (int matIdx : activeMats) {
    if (!m_materials[m_materialNames[matIdx]].hasTexture) {
      noneMissingTexture = false;
      break;
    }
  }

  bool sameTextureFamily = allHaveSameTexture && noneMissingTexture;
  if (sameTextureFamily && numMats > 1) {
    qDebug()
        << "All materials share same texture source, skipping grid-atlasing.";

    QImage unifiedTex;
    if (!uniqueTexturePaths.isEmpty()) {
      unifiedTex.load(*uniqueTexturePaths.begin());
    } else if (!uniqueGlbImageIndices.isEmpty()) {
      // Find the first material that has this GLB texture
      for (int matIdx : activeMats) {
        if (m_materials[m_materialNames[matIdx]].glbImageIdx ==
            *uniqueGlbImageIndices.begin()) {
          unifiedTex = m_materials[m_materialNames[matIdx]].textureImage;
          break;
        }
      }
    }

    if (!unifiedTex.isNull()) {
      painter.drawImage(atlas.rect(), unifiedTex.scaled(atlasSize, atlasSize));
      atlas.save(atlasPath);
    }

    // Reset transforms to 1:1 since it's a unified texture
    for (int matIdx : activeMats) {
      ObjMaterial &m = m_materials[m_materialNames[matIdx]];
      m.uvScale = QVector2D(1.0f, 1.0f);
      m.uvOffset = QVector2D(0.0f, 0.0f);
    }

    return true;
  }

  atlas.save(atlasPath);
  qDebug() << "Atlas saved and material UV transforms updated.";
  return true;
}

void ObjToMd3Converter::updateNodeTransforms(
    QVector<GlbNode> &nodes, int nodeIdx, const QMatrix4x4 &parentTransform) {
  if (nodeIdx < 0 || nodeIdx >= nodes.size())
    return;

  GlbNode &node = nodes[nodeIdx];
  QMatrix4x4 local;
  if (!node.matrix.isIdentity()) {
    local = node.matrix;
  } else {
    local.translate(node.translation);
    local.rotate(node.rotation);
    local.scale(node.scale);
  }
  node.globalTransform = parentTransform * local;

  for (int child : node.children) {
    updateNodeTransforms(nodes, child, node.globalTransform);
  }
}

const char *ObjToMd3Converter::getAccessorData(int accessorIdx, int &count,
                                               int &compType, int &stride) {
  if (accessorIdx < 0 || accessorIdx >= m_glbAccessors.size())
    return nullptr;
  QJsonObject acc = m_glbAccessors[accessorIdx].toObject();
  count = acc["count"].toInt();
  compType = acc["componentType"].toInt();
  int bvIdx = acc["bufferView"].toInt(-1);
  int accOffset = acc["byteOffset"].toInt(0);

  if (bvIdx < 0 || bvIdx >= m_glbBufferViews.size())
    return nullptr;
  QJsonObject bv = m_glbBufferViews[bvIdx].toObject();
  int bvOffset = bv["byteOffset"].toInt(0);
  stride = bv["byteStride"].toInt(0);

  return m_glbBinData.constData() + bvOffset + accOffset;
}

void ObjToMd3Converter::bakeAnimations() {
  m_animationFrames.clear();
  float fps = 24.0f;

  if (m_glbAnimations.isEmpty()) {
    // Generate static frame (Pose 0)
    QVector<GlbNode> nodes = m_glbNodes;
    for (int i = 0; i < nodes.size(); ++i) {
      if (nodes[i].parent == -1)
        updateNodeTransforms(nodes, i, QMatrix4x4());
    }
    QVector<QVector3D> frameVerts;
    frameVerts.reserve(m_finalVertices.size());
    for (int i = 0; i < m_finalVertices.size(); ++i) {
      const SkinData &sd = m_vertexSkins[i];
      if (!m_glbSkins.isEmpty() && (sd.weights[0] > 0 || sd.weights[1] > 0 ||
                                    sd.weights[2] > 0 || sd.weights[3] > 0)) {
        const GlbSkin &skin = m_glbSkins[0];
        QVector3D pos(0, 0, 0);
        float tw = 0;
        for (int k = 0; k < 4; ++k) {
          if (sd.weights[k] <= 0)
            continue;
          int jointNodeIdx = skin.joints[sd.joints[k]];
          if (jointNodeIdx >= 0 && jointNodeIdx < nodes.size() &&
              sd.joints[k] < skin.inverseBindMatrices.size()) {
            QMatrix4x4 jointMat = nodes[jointNodeIdx].globalTransform *
                                  skin.inverseBindMatrices[sd.joints[k]];
            pos += (jointMat.map(m_finalVertices[i])) * sd.weights[k];
            tw += sd.weights[k];
          }
        }
        frameVerts.append(tw > 0.0001f ? pos / tw : m_finalVertices[i]);
      } else if (sd.parentNodeIdx >= 0 && sd.parentNodeIdx < nodes.size()) {
        frameVerts.append(
            nodes[sd.parentNodeIdx].globalTransform.map(m_finalVertices[i]));
      } else {
        frameVerts.append(m_finalVertices[i]);
      }
    }
    m_animationFrames.append(frameVerts);
    setProgress(100, "Baking static pose complete.");
    return;
  }

  int totalFramesGlobal = 0;
  // First pass: Calculate frame ranges for all animations
  for (int a = 0; a < m_glbAnimations.size(); ++a) {
    float maxTime = 0;
    for (const auto &sampler : m_glbAnimations[a].samplers) {
      if (!sampler.times.isEmpty())
        maxTime = qMax(maxTime, sampler.times.last());
    }
    int frameCount = qMax(1, (int)(maxTime * fps));
    m_glbAnimations[a].startFrame = totalFramesGlobal;
    m_glbAnimations[a].endFrame = totalFramesGlobal + frameCount - 1;
    totalFramesGlobal += frameCount;
  }

  m_animationFrames.reserve(totalFramesGlobal);

  // Second pass: Bake each animation
  int currentFrameTotal = 0;
  for (int a = 0; a < m_glbAnimations.size(); ++a) {
    const auto &anim = m_glbAnimations[a];
    int animFrameCount = anim.endFrame - anim.startFrame + 1;

    setProgress(
        (a * 100) / m_glbAnimations.size(),
        QString("Baking animation %1/%2: %3 (%4 frames)")
            .arg(a + 1)
            .arg(m_glbAnimations.size())
            .arg(anim.name.isEmpty() ? QString("Anim %1").arg(a) : anim.name)
            .arg(animFrameCount));

    for (int f = 0; f < animFrameCount; ++f) {
      float curTime = f / fps;
      QVector<GlbNode> nodes = m_glbNodes;

      for (const auto &channel : anim.channels) {
        if (channel.node < 0 || channel.node >= nodes.size())
          continue;
        const auto &sampler = anim.samplers[channel.sampler];
        if (sampler.times.isEmpty())
          continue;

        int k1 = 0;
        while (k1 < sampler.times.size() - 1 && sampler.times[k1 + 1] < curTime)
          k1++;
        int k2 = qMin(k1 + 1, sampler.times.size() - 1);
        float t = (k1 == k2) ? 0.0f
                             : (curTime - sampler.times[k1]) /
                                   (sampler.times[k2] - sampler.times[k1]);

        // Determine number of components for this sampler based on path
        int comps = (channel.path == "rotation") ? 4 : 3;

        if (channel.path == "translation" &&
            sampler.values.size() >= k2 * comps + 3) {
          QVector3D p1(sampler.values[k1 * comps],
                       sampler.values[k1 * comps + 1],
                       sampler.values[k1 * comps + 2]);
          QVector3D p2(sampler.values[k2 * comps],
                       sampler.values[k2 * comps + 1],
                       sampler.values[k2 * comps + 2]);
          nodes[channel.node].translation = p1 * (1.0f - t) + p2 * t;
        } else if (channel.path == "rotation" &&
                   sampler.values.size() >= k2 * comps + 4) {
          QQuaternion q1(
              sampler.values[k1 * comps + 3], sampler.values[k1 * comps],
              sampler.values[k1 * comps + 1], sampler.values[k1 * comps + 2]);
          QQuaternion q2(
              sampler.values[k2 * comps + 3], sampler.values[k2 * comps],
              sampler.values[k2 * comps + 1], sampler.values[k2 * comps + 2]);
          nodes[channel.node].rotation = QQuaternion::slerp(q1, q2, t);
        } else if (channel.path == "scale" &&
                   sampler.values.size() >= k2 * comps + 3) {
          QVector3D s1(sampler.values[k1 * comps],
                       sampler.values[k1 * comps + 1],
                       sampler.values[k1 * comps + 2]);
          QVector3D s2(sampler.values[k2 * comps],
                       sampler.values[k2 * comps + 1],
                       sampler.values[k2 * comps + 2]);
          nodes[channel.node].scale = s1 * (1.0f - t) + s2 * t;
        }
      }

      for (int i = 0; i < nodes.size(); ++i) {
        if (nodes[i].parent == -1)
          updateNodeTransforms(nodes, i, QMatrix4x4());
      }

      QVector<QVector3D> frameVerts;
      frameVerts.reserve(m_finalVertices.size());

      for (int i = 0; i < m_finalVertices.size(); ++i) {
        const SkinData &sd = m_vertexSkins[i];
        if (!m_glbSkins.isEmpty() && (sd.weights[0] > 0 || sd.weights[1] > 0 ||
                                      sd.weights[2] > 0 || sd.weights[3] > 0)) {
          const GlbSkin &skin = m_glbSkins[0]; // Assume first skin
          QVector3D pos(0, 0, 0);
          float tw = 0;
          for (int k = 0; k < 4; ++k) {
            if (sd.weights[k] <= 0)
              continue;
            int jointNodeIdx = skin.joints[sd.joints[k]];
            if (jointNodeIdx >= 0 && jointNodeIdx < nodes.size() &&
                sd.joints[k] < skin.inverseBindMatrices.size()) {
              QMatrix4x4 jointMat = nodes[jointNodeIdx].globalTransform *
                                    skin.inverseBindMatrices[sd.joints[k]];
              pos += (jointMat.map(m_finalVertices[i])) * sd.weights[k];
              tw += sd.weights[k];
            }
          }
          frameVerts.append(tw > 0.0001f ? pos / tw : m_finalVertices[i]);
        } else if (sd.parentNodeIdx >= 0 && sd.parentNodeIdx < nodes.size()) {
          // Non-skinned vertex, apply its parent node transform
          frameVerts.append(
              nodes[sd.parentNodeIdx].globalTransform.map(m_finalVertices[i]));
        } else {
          frameVerts.append(m_finalVertices[i]);
        }
      }
      m_animationFrames.append(frameVerts);
      currentFrameTotal++;
    }
  }
  setProgress(100, "Baking complete.");
}
