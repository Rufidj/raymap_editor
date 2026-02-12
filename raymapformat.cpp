/*
 * raymapformat.cpp - Map Loading/Saving for Editor (Format v9)
 * v9: Added nested sectors support
 * v8: Geometric sectors only
 */

#include "raymapformat.h"
#include "mapdata.h"
#include <QDataStream>
#include <QDebug>
#include <QFile>
#include <QFileInfo>
#include <QMap>
#include <cstring>

RayMapFormat::RayMapFormat() {}

/* ============================================================================
   MAP LOADING
   ============================================================================
 */

bool RayMapFormat::loadMap(
    const QString &filename, MapData &mapData,
    std::function<void(const QString &)> progressCallback) {
  QFile file(filename);
  if (!file.open(QIODevice::ReadOnly)) {
    qWarning() << "No se pudo abrir el archivo:" << filename;
    return false;
  }

  QDataStream in(&file);
  in.setByteOrder(QDataStream::LittleEndian);

  /* Read header */
  char magic[8];
  uint32_t version, num_sectors, num_portals, num_sprites, num_spawn_flags;
  float camera_x, camera_y, camera_z, camera_rot, camera_pitch;
  int32_t skyTextureID;

  in.readRawData(magic, 8);
  in.readRawData(reinterpret_cast<char *>(&version), sizeof(uint32_t));
  in.readRawData(reinterpret_cast<char *>(&num_sectors), sizeof(uint32_t));
  in.readRawData(reinterpret_cast<char *>(&num_portals), sizeof(uint32_t));
  in.readRawData(reinterpret_cast<char *>(&num_sprites), sizeof(uint32_t));
  in.readRawData(reinterpret_cast<char *>(&num_spawn_flags), sizeof(uint32_t));
  in.readRawData(reinterpret_cast<char *>(&camera_x), sizeof(float));
  in.readRawData(reinterpret_cast<char *>(&camera_y), sizeof(float));
  in.readRawData(reinterpret_cast<char *>(&camera_z), sizeof(float));
  in.readRawData(reinterpret_cast<char *>(&camera_rot), sizeof(float));
  in.readRawData(reinterpret_cast<char *>(&camera_pitch), sizeof(float));
  in.readRawData(reinterpret_cast<char *>(&skyTextureID), sizeof(int32_t));

  /* Verify magic number */
  if (memcmp(magic, "RAYMAP\x1a", 7) != 0) {
    qWarning() << "Formato de archivo inválido";
    file.close();
    return false;
  }

  /* Verify version */
  if (version < 8 || version > 15) {
    qWarning() << "Versión no soportada:" << version << "(solo v8-v15)";
    file.close();
    return false;
  }

  qDebug() << "Cargando mapa v" << version << ":" << num_sectors << "sectores,"
           << num_portals << "portales";

  if (progressCallback)
    progressCallback("Cargando sectores...");

  /* Set camera */
  mapData.camera.x = camera_x;
  mapData.camera.y = camera_y;
  mapData.camera.z = camera_z;
  mapData.camera.rotation = camera_rot;
  mapData.camera.pitch = camera_pitch;
  mapData.camera.enabled = true;
  mapData.skyTextureID = skyTextureID;

  /* Load sectors */
  mapData.sectors.clear();
  for (uint32_t i = 0; i < num_sectors; i++) {
    Sector sector;

    in.readRawData(reinterpret_cast<char *>(&sector.sector_id), sizeof(int));
    in.readRawData(reinterpret_cast<char *>(&sector.floor_z), sizeof(float));
    in.readRawData(reinterpret_cast<char *>(&sector.ceiling_z), sizeof(float));
    in.readRawData(reinterpret_cast<char *>(&sector.floor_texture_id),
                   sizeof(int));
    in.readRawData(reinterpret_cast<char *>(&sector.ceiling_texture_id),
                   sizeof(int));
    in.readRawData(reinterpret_cast<char *>(&sector.light_level), sizeof(int));

    /* Read vertices */
    uint32_t num_vertices;
    in.readRawData(reinterpret_cast<char *>(&num_vertices), sizeof(uint32_t));

    for (uint32_t v = 0; v < num_vertices; v++) {
      float x, y;
      in.readRawData(reinterpret_cast<char *>(&x), sizeof(float));
      in.readRawData(reinterpret_cast<char *>(&y), sizeof(float));
      sector.vertices.append(QPointF(x, y));
    }

    /* Read walls */
    uint32_t num_walls;
    in.readRawData(reinterpret_cast<char *>(&num_walls), sizeof(uint32_t));

    for (uint32_t w = 0; w < num_walls; w++) {
      Wall wall;

      in.readRawData(reinterpret_cast<char *>(&wall.wall_id), sizeof(int));
      in.readRawData(reinterpret_cast<char *>(&wall.x1), sizeof(float));
      in.readRawData(reinterpret_cast<char *>(&wall.y1), sizeof(float));
      in.readRawData(reinterpret_cast<char *>(&wall.x2), sizeof(float));
      in.readRawData(reinterpret_cast<char *>(&wall.y2), sizeof(float));
      in.readRawData(reinterpret_cast<char *>(&wall.texture_id_lower),
                     sizeof(int));
      in.readRawData(reinterpret_cast<char *>(&wall.texture_id_middle),
                     sizeof(int));
      in.readRawData(reinterpret_cast<char *>(&wall.texture_id_upper),
                     sizeof(int));
      in.readRawData(reinterpret_cast<char *>(&wall.texture_split_z_lower),
                     sizeof(float));
      in.readRawData(reinterpret_cast<char *>(&wall.texture_split_z_upper),
                     sizeof(float));
      in.readRawData(reinterpret_cast<char *>(&wall.portal_id), sizeof(int));
      in.readRawData(reinterpret_cast<char *>(&wall.flags), sizeof(int));

      sector.walls.append(wall);
    }

    // Load hierarchy fields (parent and children)
    in.readRawData(reinterpret_cast<char *>(&sector.parent_sector_id),
                   sizeof(int));
    int numChildren;
    in.readRawData(reinterpret_cast<char *>(&numChildren), sizeof(int));
    for (int c = 0; c < numChildren; c++) {
      int childId;
      in.readRawData(reinterpret_cast<char *>(&childId), sizeof(int));
      sector.child_sector_ids.append(childId);
    }

    mapData.sectors.append(sector);
  }

  if (progressCallback)
    progressCallback("Cargando portales...");

  /* Load portals */
  mapData.portals.clear();
  for (uint32_t i = 0; i < num_portals; i++) {
    Portal portal;

    in.readRawData(reinterpret_cast<char *>(&portal.portal_id), sizeof(int));
    in.readRawData(reinterpret_cast<char *>(&portal.sector_a), sizeof(int));
    in.readRawData(reinterpret_cast<char *>(&portal.sector_b), sizeof(int));
    in.readRawData(reinterpret_cast<char *>(&portal.wall_id_a), sizeof(int));
    in.readRawData(reinterpret_cast<char *>(&portal.wall_id_b), sizeof(int));
    in.readRawData(reinterpret_cast<char *>(&portal.x1), sizeof(float));
    in.readRawData(reinterpret_cast<char *>(&portal.y1), sizeof(float));
    in.readRawData(reinterpret_cast<char *>(&portal.x2), sizeof(float));
    in.readRawData(reinterpret_cast<char *>(&portal.y2), sizeof(float));

    /* Add portal IDs to sectors */
    for (Sector &sector : mapData.sectors) {
      if (sector.sector_id == portal.sector_a ||
          sector.sector_id == portal.sector_b) {
        if (!sector.portal_ids.contains(portal.portal_id)) {
          sector.portal_ids.append(portal.portal_id);
        }
      }
    }

    mapData.portals.append(portal);
  }

  if (progressCallback)
    progressCallback("Cargando sprites...");

  /* Load sprites */
  mapData.sprites.clear();
  for (uint32_t i = 0; i < num_sprites; i++) {
    SpriteData sprite;

    in.readRawData(reinterpret_cast<char *>(&sprite.texture_id), sizeof(int));
    in.readRawData(reinterpret_cast<char *>(&sprite.x), sizeof(float));
    in.readRawData(reinterpret_cast<char *>(&sprite.y), sizeof(float));
    in.readRawData(reinterpret_cast<char *>(&sprite.z), sizeof(float));
    in.readRawData(reinterpret_cast<char *>(&sprite.w), sizeof(int));
    in.readRawData(reinterpret_cast<char *>(&sprite.h), sizeof(int));
    in.readRawData(reinterpret_cast<char *>(&sprite.rot), sizeof(float));

    mapData.sprites.append(sprite);
  }

  if (progressCallback)
    progressCallback("Cargando spawn flags...");

  /* Load spawn flags */
  mapData.spawnFlags.clear();
  for (uint32_t i = 0; i < num_spawn_flags; i++) {
    SpawnFlag flag;

    in.readRawData(reinterpret_cast<char *>(&flag.flagId), sizeof(int));
    in.readRawData(reinterpret_cast<char *>(&flag.x), sizeof(float));
    in.readRawData(reinterpret_cast<char *>(&flag.y), sizeof(float));
    in.readRawData(reinterpret_cast<char *>(&flag.z), sizeof(float));

    // Reset extended fields for basic spawn flags
    flag.isIntro = false;
    flag.npcPathId = -1;
    flag.autoStartPath = false;

    mapData.spawnFlags.append(flag);
  }

  /* Load entities (appended at end for compatibility) */
  mapData.entities.clear();

  // Check if there is more data (entities appended)
  if (!in.atEnd()) {
    uint32_t num_entities;
    in.readRawData(reinterpret_cast<char *>(&num_entities), sizeof(uint32_t));

    for (uint32_t i = 0; i < num_entities; i++) {
      EntityInstance entity;

      in.readRawData(reinterpret_cast<char *>(&entity.spawn_id), sizeof(int));
      in.readRawData(reinterpret_cast<char *>(&entity.x), sizeof(float));
      in.readRawData(reinterpret_cast<char *>(&entity.y), sizeof(float));
      in.readRawData(reinterpret_cast<char *>(&entity.z), sizeof(float));

      // Asset path
      uint32_t pathLen;
      in.readRawData(reinterpret_cast<char *>(&pathLen), sizeof(uint32_t));
      QByteArray pathBytes;
      pathBytes.resize(pathLen);
      in.readRawData(pathBytes.data(), pathLen);
      entity.assetPath = QString::fromUtf8(pathBytes);

      // Type
      uint32_t typeLen;
      in.readRawData(reinterpret_cast<char *>(&typeLen), sizeof(uint32_t));
      QByteArray typeBytes;
      typeBytes.resize(typeLen);
      in.readRawData(typeBytes.data(), typeLen);

      // Cleanup type string (remove potential garbage)
      QString rawType = QString::fromUtf8(typeBytes).trimmed();
      QString cleanType;
      for (QChar c : rawType) {
        if (c.isLetterOrNumber() || c == '_' || c == '-')
          cleanType.append(c);
      }
      entity.type = cleanType;

      // Fallback logic for corrupted types
      if (entity.type != "model" && entity.type != "campath" &&
          entity.type != "info_player_start") {
        if (entity.assetPath.endsWith(".md3", Qt::CaseInsensitive))
          entity.type = "model";
        else if (entity.assetPath.endsWith(".campath", Qt::CaseInsensitive) ||
                 entity.assetPath.contains(".campath"))
          entity.type = "campath";
      }

      // Version 10 fields (Behaviors)
      if (version >= 10) {
        int32_t actType;
        in.readRawData(reinterpret_cast<char *>(&actType), sizeof(int32_t));
        entity.activationType =
            static_cast<EntityInstance::ActivationType>(actType);

        int32_t visible;
        in.readRawData(reinterpret_cast<char *>(&visible), sizeof(int32_t));
        entity.isVisible = (visible != 0);

        uint32_t collLen;
        in.readRawData(reinterpret_cast<char *>(&collLen), sizeof(uint32_t));
        QByteArray collBytes;
        collBytes.resize(collLen);
        in.readRawData(collBytes.data(), collLen);
        entity.collisionTarget = QString::fromUtf8(collBytes);

        uint32_t actionLen;
        in.readRawData(reinterpret_cast<char *>(&actionLen), sizeof(uint32_t));
        QByteArray actionBytes;
        actionBytes.resize(actionLen);
        in.readRawData(actionBytes.data(), actionLen);
        entity.customAction = QString::fromUtf8(actionBytes);

        uint32_t eventLen;
        in.readRawData(reinterpret_cast<char *>(&eventLen), sizeof(uint32_t));
        QByteArray eventBytes;
        eventBytes.resize(eventLen);
        in.readRawData(eventBytes.data(), eventLen);
        entity.eventName = QString::fromUtf8(eventBytes);
      }

      // Version 11 fields (Player & Controls)
      if (version >= 11) {
        int32_t isPlayer;
        in.readRawData(reinterpret_cast<char *>(&isPlayer), sizeof(int32_t));
        entity.isPlayer = (isPlayer != 0);

        int32_t ctrlType;
        in.readRawData(reinterpret_cast<char *>(&ctrlType), sizeof(int32_t));
        entity.controlType = static_cast<EntityInstance::ControlType>(ctrlType);

        int32_t camFollow;
        in.readRawData(reinterpret_cast<char *>(&camFollow), sizeof(int32_t));
        entity.cameraFollow = (camFollow != 0);

        in.readRawData(reinterpret_cast<char *>(&entity.cameraOffset_x),
                       sizeof(float));
        in.readRawData(reinterpret_cast<char *>(&entity.cameraOffset_y),
                       sizeof(float));
        in.readRawData(reinterpret_cast<char *>(&entity.cameraOffset_z),
                       sizeof(float));
        in.readRawData(reinterpret_cast<char *>(&entity.cameraRotation),
                       sizeof(float));
      }

      // Version 12 fields (Intro)
      if (version >= 12 && !in.atEnd()) {
        int32_t isIntroVal = 0;
        in.readRawData(reinterpret_cast<char *>(&isIntroVal), sizeof(int32_t));
        entity.isIntro = (isIntroVal != 0);
      }

      // Version 13 fields (NPC Path)
      if (version >= 13 && !in.atEnd()) {
        int32_t npcPathIdVal = -1;
        in.readRawData(reinterpret_cast<char *>(&npcPathIdVal),
                       sizeof(int32_t));
        entity.npcPathId = npcPathIdVal;
      }

      // Version 14 fields (Extended NPC Path properties)
      if (version >= 14 && !in.atEnd()) {
        int8_t autoStart = 0;
        in.readRawData(reinterpret_cast<char *>(&autoStart), sizeof(int8_t));
        entity.autoStartPath = (autoStart != 0);
      } else {
        entity.autoStartPath = false;
      }

      // Version 15 fields (Snap to ground)
      if (version >= 15 && !in.atEnd()) {
        int8_t snap = 0;
        in.readRawData(reinterpret_cast<char *>(&snap), sizeof(int8_t));
        entity.snapToFloor = (snap != 0);
      } else {
        entity.snapToFloor = false;
      }

      // Generate processName from asset path if not set
      // (for backward compatibility with old maps or if explicit name missing)
      // Note: We always regenerate for now to ensure consistency with filename
      // but ideally we should only do it if empty.
      QFileInfo assetInfo(entity.assetPath);
      QString rawName = assetInfo.completeBaseName();

      // SANITIZE processName for BennuGD (must be valid identifier)
      QString cleanProcName;
      for (QChar c : rawName) {
        if (c.isLetterOrNumber() || c == '_')
          cleanProcName.append(c);
        else
          cleanProcName.append('_');
      }

      // Ensure it doesnt start with number or is empty
      if (cleanProcName.isEmpty())
        cleanProcName = "entity_" + QString::number(entity.spawn_id);
      if (cleanProcName[0].isDigit())
        cleanProcName.prepend("proc_");

      entity.processName = cleanProcName;

      mapData.entities.append(entity);
    }
  }

  // ===== NPC PATHS (v13) =====
  if (version >= 13) {
    uint32_t num_npc_paths;
    in.readRawData(reinterpret_cast<char *>(&num_npc_paths), sizeof(uint32_t));

    for (uint32_t i = 0; i < num_npc_paths; ++i) {
      NPCPath path;

      // Path ID
      int32_t pathId;
      in.readRawData(reinterpret_cast<char *>(&pathId), sizeof(int32_t));
      path.path_id = pathId;

      // Path name
      uint32_t nameLen;
      in.readRawData(reinterpret_cast<char *>(&nameLen), sizeof(uint32_t));
      QByteArray nameBytes(nameLen, Qt::Uninitialized);
      in.readRawData(nameBytes.data(), nameLen);
      path.name = QString::fromUtf8(nameBytes);

      // Loop mode
      int32_t loopMode;
      in.readRawData(reinterpret_cast<char *>(&loopMode), sizeof(int32_t));
      path.loop_mode = static_cast<NPCPath::LoopMode>(loopMode);

      // Visibility
      int32_t visibleVal;
      in.readRawData(reinterpret_cast<char *>(&visibleVal), sizeof(int32_t));
      path.visible = (visibleVal != 0);

      // Waypoints
      uint32_t num_waypoints;
      in.readRawData(reinterpret_cast<char *>(&num_waypoints),
                     sizeof(uint32_t));

      for (uint32_t j = 0; j < num_waypoints; ++j) {
        Waypoint wp;
        in.readRawData(reinterpret_cast<char *>(&wp.x), sizeof(float));
        in.readRawData(reinterpret_cast<char *>(&wp.y), sizeof(float));
        in.readRawData(reinterpret_cast<char *>(&wp.z), sizeof(float));
        in.readRawData(reinterpret_cast<char *>(&wp.speed), sizeof(float));
        in.readRawData(reinterpret_cast<char *>(&wp.wait_time),
                       sizeof(int32_t));
        in.readRawData(reinterpret_cast<char *>(&wp.look_angle), sizeof(float));
        path.waypoints.append(wp);
      }

      mapData.npcPaths.append(path);
    }
  }

  file.close();

  qDebug() << "Mapa cargado:" << mapData.sectors.size() << "sectores,"
           << mapData.portals.size() << "portales," << mapData.sprites.size()
           << "sprites," << mapData.spawnFlags.size() << "spawn flags,"
           << mapData.entities.size() << "entidades";

  return true;
}

/* ============================================================================
   MAP SAVING
   ============================================================================
 */

bool RayMapFormat::saveMap(
    const QString &filename, const MapData &mapData,
    std::function<void(const QString &)> progressCallback) {
  QFile file(filename);
  if (!file.open(QIODevice::WriteOnly)) {
    qWarning() << "No se pudo crear el archivo:" << filename;
    return false;
  }

  QDataStream out(&file);
  out.setByteOrder(QDataStream::LittleEndian);

  /* Prepare header */
  char magic[8];
  memcpy(magic, "RAYMAP\x1a", 7);
  magic[7] = 0;

  // --- PORTAL RENUMBERING (Defragmentation) ---
  // Engine expects portals to be contiguous 0..N-1 matching array index.
  // Editor IDs might have gaps due to deletions. We must map them.
  QMap<int, int> portalIdMap;
  int nextPortalId = 0;
  for (const Portal &p : mapData.portals) {
    portalIdMap[p.portal_id] = nextPortalId++;
  }
  // --------------------------------------------

  uint32_t version = 15; // Updated to v15 for Snap to Floor field
  uint32_t num_sectors = mapData.sectors.size();
  uint32_t num_portals = mapData.portals.size();
  uint32_t num_sprites = mapData.sprites.size();
  uint32_t num_spawn_flags =
      mapData.spawnFlags.size() + mapData.entities.size(); // Include entities!

  float camera_x = mapData.camera.x;
  float camera_y = mapData.camera.y;
  float camera_z = mapData.camera.z;
  float camera_rot = mapData.camera.rotation;
  float camera_pitch = mapData.camera.pitch;
  int32_t skyTextureID = mapData.skyTextureID;

  qDebug() << "DEBUG SAVE: Writing camera position:" << camera_x << ","
           << camera_y << "," << camera_z;

  /* Write header */
  out.writeRawData(magic, 8);
  out.writeRawData(reinterpret_cast<const char *>(&version), sizeof(uint32_t));
  out.writeRawData(reinterpret_cast<const char *>(&num_sectors),
                   sizeof(uint32_t));
  out.writeRawData(reinterpret_cast<const char *>(&num_portals),
                   sizeof(uint32_t));
  out.writeRawData(reinterpret_cast<const char *>(&num_sprites),
                   sizeof(uint32_t));
  out.writeRawData(reinterpret_cast<const char *>(&num_spawn_flags),
                   sizeof(uint32_t));
  out.writeRawData(reinterpret_cast<const char *>(&camera_x), sizeof(float));
  out.writeRawData(reinterpret_cast<const char *>(&camera_y), sizeof(float));
  out.writeRawData(reinterpret_cast<const char *>(&camera_z), sizeof(float));
  out.writeRawData(reinterpret_cast<const char *>(&camera_rot), sizeof(float));
  out.writeRawData(reinterpret_cast<const char *>(&camera_pitch),
                   sizeof(float));
  out.writeRawData(reinterpret_cast<const char *>(&skyTextureID),
                   sizeof(int32_t));

  if (progressCallback)
    progressCallback("Guardando sectores...");

  /* Create Sector ID Map (Editor ID -> Sequential Export ID) */
  QMap<int, int> sectorIdMap;
  for (int i = 0; i < mapData.sectors.size(); i++) {
    sectorIdMap[mapData.sectors[i].sector_id] = i;
  }

  /* Write sectors */
  for (const Sector &sector : mapData.sectors) {
    // Write ID (we write the sequential one to match engine expectation, or
    // original? Engine v9 loads sectors into array index. It doesn't use the
    // stored ID for indexing usually, but assumes order. Let's write the
    // original ID just in case, but hierarchy must use INDEX.)
    out.writeRawData(reinterpret_cast<const char *>(&sector.sector_id),
                     sizeof(int));
    out.writeRawData(reinterpret_cast<const char *>(&sector.floor_z),
                     sizeof(float));
    out.writeRawData(reinterpret_cast<const char *>(&sector.ceiling_z),
                     sizeof(float));
    out.writeRawData(reinterpret_cast<const char *>(&sector.floor_texture_id),
                     sizeof(int));
    out.writeRawData(reinterpret_cast<const char *>(&sector.ceiling_texture_id),
                     sizeof(int));
    out.writeRawData(reinterpret_cast<const char *>(&sector.light_level),
                     sizeof(int));

    /* Write vertices */
    uint32_t num_vertices = sector.vertices.size();
    out.writeRawData(reinterpret_cast<const char *>(&num_vertices),
                     sizeof(uint32_t));

    for (const QPointF &vertex : sector.vertices) {
      float x = vertex.x();
      float y = vertex.y();
      out.writeRawData(reinterpret_cast<const char *>(&x), sizeof(float));
      out.writeRawData(reinterpret_cast<const char *>(&y), sizeof(float));
    }

    /* Write walls */
    uint32_t num_walls = sector.walls.size();
    out.writeRawData(reinterpret_cast<const char *>(&num_walls),
                     sizeof(uint32_t));

    for (const Wall &wall : sector.walls) {
      out.writeRawData(reinterpret_cast<const char *>(&wall.wall_id),
                       sizeof(int));
      out.writeRawData(reinterpret_cast<const char *>(&wall.x1), sizeof(float));
      out.writeRawData(reinterpret_cast<const char *>(&wall.y1), sizeof(float));
      out.writeRawData(reinterpret_cast<const char *>(&wall.x2), sizeof(float));
      out.writeRawData(reinterpret_cast<const char *>(&wall.y2), sizeof(float));
      out.writeRawData(reinterpret_cast<const char *>(&wall.texture_id_lower),
                       sizeof(int));
      out.writeRawData(reinterpret_cast<const char *>(&wall.texture_id_middle),
                       sizeof(int));
      out.writeRawData(reinterpret_cast<const char *>(&wall.texture_id_upper),
                       sizeof(int));
      out.writeRawData(
          reinterpret_cast<const char *>(&wall.texture_split_z_lower),
          sizeof(float));
      out.writeRawData(
          reinterpret_cast<const char *>(&wall.texture_split_z_upper),
          sizeof(float));

      // USE MAPPED ID for Portals
      int savedPortalId = -1;
      if (wall.portal_id >= 0 && portalIdMap.contains(wall.portal_id)) {
        savedPortalId = portalIdMap[wall.portal_id];
      } else if (wall.portal_id >= 0) {
        // qWarning() << "Warning: Wall points to non-existent portal ID:" <<
        // wall.portal_id;
      }
      out.writeRawData(reinterpret_cast<const char *>(&savedPortalId),
                       sizeof(int));

      out.writeRawData(reinterpret_cast<const char *>(&wall.flags),
                       sizeof(int));
    }

    // Save hierarchy (parent and children) - WITH REMAPPING
    int savedParentId = -1;
    if (sector.parent_sector_id >= 0 &&
        sectorIdMap.contains(sector.parent_sector_id)) {
      savedParentId = sectorIdMap[sector.parent_sector_id];
    }
    out.writeRawData(reinterpret_cast<const char *>(&savedParentId),
                     sizeof(int));

    // Remap children
    QVector<int> remappedChildren;
    for (int childId : sector.child_sector_ids) {
      if (sectorIdMap.contains(childId)) {
        remappedChildren.append(sectorIdMap[childId]);
      }
    }

    int numChildren = remappedChildren.size();
    out.writeRawData(reinterpret_cast<const char *>(&numChildren), sizeof(int));
    for (int childId : remappedChildren) {
      out.writeRawData(reinterpret_cast<const char *>(&childId), sizeof(int));
    }
  }

  if (progressCallback)
    progressCallback("Guardando portales...");

  /* Write portals */
  for (const Portal &portal : mapData.portals) {
    // USE MAPPED ID (which is sequential 0..N-1)
    int savedPortalId = portalIdMap[portal.portal_id];

    out.writeRawData(reinterpret_cast<const char *>(&savedPortalId),
                     sizeof(int));
    out.writeRawData(reinterpret_cast<const char *>(&portal.sector_a),
                     sizeof(int));
    out.writeRawData(reinterpret_cast<const char *>(&portal.sector_b),
                     sizeof(int));
    out.writeRawData(reinterpret_cast<const char *>(&portal.wall_id_a),
                     sizeof(int));
    out.writeRawData(reinterpret_cast<const char *>(&portal.wall_id_b),
                     sizeof(int));
    out.writeRawData(reinterpret_cast<const char *>(&portal.x1), sizeof(float));
    out.writeRawData(reinterpret_cast<const char *>(&portal.y1), sizeof(float));
    out.writeRawData(reinterpret_cast<const char *>(&portal.x2), sizeof(float));
    out.writeRawData(reinterpret_cast<const char *>(&portal.y2), sizeof(float));
  }

  if (progressCallback)
    progressCallback("Guardando sprites...");

  /* Write sprites */
  for (const SpriteData &sprite : mapData.sprites) {
    out.writeRawData(reinterpret_cast<const char *>(&sprite.texture_id),
                     sizeof(int));
    out.writeRawData(reinterpret_cast<const char *>(&sprite.x), sizeof(float));
    out.writeRawData(reinterpret_cast<const char *>(&sprite.y), sizeof(float));
    out.writeRawData(reinterpret_cast<const char *>(&sprite.z), sizeof(float));
    out.writeRawData(reinterpret_cast<const char *>(&sprite.w), sizeof(int));
    out.writeRawData(reinterpret_cast<const char *>(&sprite.h), sizeof(int));
    out.writeRawData(reinterpret_cast<const char *>(&sprite.rot),
                     sizeof(float));
  }

  if (progressCallback)
    progressCallback("Guardando spawn flags...");

  /* Write spawn flags (including entities) */
  // First write regular spawn flags (Legacy 16-byte format for engine
  // compatibility)
  for (const SpawnFlag &flag : mapData.spawnFlags) {
    out.writeRawData(reinterpret_cast<const char *>(&flag.flagId), sizeof(int));
    out.writeRawData(reinterpret_cast<const char *>(&flag.x), sizeof(float));
    out.writeRawData(reinterpret_cast<const char *>(&flag.y), sizeof(float));
    out.writeRawData(reinterpret_cast<const char *>(&flag.z), sizeof(float));
  }

  // Then write entities as spawn flags (Legacy 16-byte format for motor)
  for (const EntityInstance &entity : mapData.entities) {
    out.writeRawData(reinterpret_cast<const char *>(&entity.spawn_id),
                     sizeof(int));
    out.writeRawData(reinterpret_cast<const char *>(&entity.x), sizeof(float));
    out.writeRawData(reinterpret_cast<const char *>(&entity.y), sizeof(float));
    out.writeRawData(reinterpret_cast<const char *>(&entity.z), sizeof(float));
  }

  if (progressCallback)
    progressCallback("Guardando entidades...");

  /* Write entities (appended at end for compatibility) */
  uint32_t num_entities = mapData.entities.size();
  out.writeRawData(reinterpret_cast<const char *>(&num_entities),
                   sizeof(uint32_t));

  for (const EntityInstance &entity : mapData.entities) {
    out.writeRawData(reinterpret_cast<const char *>(&entity.spawn_id),
                     sizeof(int));
    out.writeRawData(reinterpret_cast<const char *>(&entity.x), sizeof(float));
    out.writeRawData(reinterpret_cast<const char *>(&entity.y), sizeof(float));
    out.writeRawData(reinterpret_cast<const char *>(&entity.z), sizeof(float));

    // Asset path
    QByteArray pathBytes = entity.assetPath.toUtf8();
    uint32_t pathLen = pathBytes.size();
    out.writeRawData(reinterpret_cast<const char *>(&pathLen),
                     sizeof(uint32_t));
    out.writeRawData(pathBytes.data(), pathLen);

    // Type
    QByteArray typeBytes = entity.type.toUtf8();
    uint32_t typeLen = typeBytes.size();
    out.writeRawData(reinterpret_cast<const char *>(&typeLen),
                     sizeof(uint32_t));
    out.writeRawData(typeBytes.data(), typeLen);

    // Behavior (v10)
    int32_t actType = static_cast<int32_t>(entity.activationType);
    out.writeRawData(reinterpret_cast<const char *>(&actType), sizeof(int32_t));

    int32_t visible = entity.isVisible ? 1 : 0;
    out.writeRawData(reinterpret_cast<const char *>(&visible), sizeof(int32_t));

    QByteArray collBytes = entity.collisionTarget.toUtf8();
    uint32_t collLen = collBytes.size();
    out.writeRawData(reinterpret_cast<const char *>(&collLen),
                     sizeof(uint32_t));
    out.writeRawData(collBytes.data(), collLen);

    QByteArray actionBytes = entity.customAction.toUtf8();
    uint32_t actionLen = actionBytes.size();
    out.writeRawData(reinterpret_cast<const char *>(&actionLen),
                     sizeof(uint32_t));
    out.writeRawData(actionBytes.data(), actionLen);

    QByteArray eventBytes = entity.eventName.toUtf8();
    uint32_t eventLen = eventBytes.size();
    out.writeRawData(reinterpret_cast<const char *>(&eventLen),
                     sizeof(uint32_t));
    out.writeRawData(eventBytes.data(), eventLen);

    // Player & Controls (v11)
    int32_t isPlayer = entity.isPlayer ? 1 : 0;
    out.writeRawData(reinterpret_cast<const char *>(&isPlayer),
                     sizeof(int32_t));

    int32_t ctrlType = static_cast<int32_t>(entity.controlType);
    out.writeRawData(reinterpret_cast<const char *>(&ctrlType),
                     sizeof(int32_t));

    int32_t camFollow = entity.cameraFollow ? 1 : 0;
    out.writeRawData(reinterpret_cast<const char *>(&camFollow),
                     sizeof(int32_t));

    out.writeRawData(reinterpret_cast<const char *>(&entity.cameraOffset_x),
                     sizeof(float));
    out.writeRawData(reinterpret_cast<const char *>(&entity.cameraOffset_y),
                     sizeof(float));
    out.writeRawData(reinterpret_cast<const char *>(&entity.cameraOffset_z),
                     sizeof(float));
    out.writeRawData(reinterpret_cast<const char *>(&entity.cameraRotation),
                     sizeof(float));

    // Intro (v12)
    int32_t isIntroVal = entity.isIntro ? 1 : 0;
    out.writeRawData(reinterpret_cast<const char *>(&isIntroVal),
                     sizeof(int32_t));

    // NPC Path (v13)
    int32_t npcPathIdVal = entity.npcPathId;
    out.writeRawData(reinterpret_cast<const char *>(&npcPathIdVal),
                     sizeof(int32_t));

    // Auto-start (v14)
    int8_t autoStartVal = entity.autoStartPath ? 1 : 0;
    out.writeRawData(reinterpret_cast<const char *>(&autoStartVal),
                     sizeof(int8_t));

    // Snap to floor (v15)
    int8_t snapVal = entity.snapToFloor ? 1 : 0;
    out.writeRawData(reinterpret_cast<const char *>(&snapVal), sizeof(int8_t));
  }

  // ===== NPC PATHS (v13) =====
  uint32_t num_npc_paths = mapData.npcPaths.size();
  out.writeRawData(reinterpret_cast<const char *>(&num_npc_paths),
                   sizeof(uint32_t));

  for (const NPCPath &path : mapData.npcPaths) {
    // Path ID
    int32_t pathId = path.path_id;
    out.writeRawData(reinterpret_cast<const char *>(&pathId), sizeof(int32_t));

    // Path name
    QByteArray nameBytes = path.name.toUtf8();
    uint32_t nameLen = nameBytes.size();
    out.writeRawData(reinterpret_cast<const char *>(&nameLen),
                     sizeof(uint32_t));
    out.writeRawData(nameBytes.constData(), nameLen);

    // Loop mode
    int32_t loopMode = static_cast<int32_t>(path.loop_mode);
    out.writeRawData(reinterpret_cast<const char *>(&loopMode),
                     sizeof(int32_t));

    // Visibility
    int32_t visibleVal = path.visible ? 1 : 0;
    out.writeRawData(reinterpret_cast<const char *>(&visibleVal),
                     sizeof(int32_t));

    // Waypoints
    uint32_t num_waypoints = path.waypoints.size();
    out.writeRawData(reinterpret_cast<const char *>(&num_waypoints),
                     sizeof(uint32_t));

    for (const Waypoint &wp : path.waypoints) {
      out.writeRawData(reinterpret_cast<const char *>(&wp.x), sizeof(float));
      out.writeRawData(reinterpret_cast<const char *>(&wp.y), sizeof(float));
      out.writeRawData(reinterpret_cast<const char *>(&wp.z), sizeof(float));
      out.writeRawData(reinterpret_cast<const char *>(&wp.speed),
                       sizeof(float));
      out.writeRawData(reinterpret_cast<const char *>(&wp.wait_time),
                       sizeof(int32_t));
      out.writeRawData(reinterpret_cast<const char *>(&wp.look_angle),
                       sizeof(float));
    }
  }

  /* Flush */
  out.device()->waitForBytesWritten(-1);
  file.flush();
  file.close();

  qDebug() << "Mapa guardado:" << mapData.sectors.size() << "sectores,"
           << mapData.portals.size() << "portales," << mapData.entities.size()
           << "entidades";

  return true;
}
