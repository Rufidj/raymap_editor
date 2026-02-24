#ifndef MAPDATA_H
#define MAPDATA_H

#include <QByteArray>
#include <QPixmap>
#include <QPointF>
#include <QSet>
#include <QString>
#include <QVector>
#include <cstdint>

/* ============================================================================
   TEXTURE ENTRY
   ============================================================================
 */

struct TextureEntry {
  QString filename;
  uint32_t id;
  QPixmap pixmap;

  TextureEntry() : id(0) {}
  TextureEntry(const QString &fname, uint32_t tid) : filename(fname), id(tid) {}
};

/* ============================================================================
   TERRAIN (HEIGHTMAP) v28
   ============================================================================
 */

struct Terrain {
  int id;
  float x, y, z;
  int cols, rows;
  float cell_size;
  QVector<float> heights; // Size must be (cols+1)*(rows+1)

  // Multitexturing (Splatting)
  int texture_ids[4];
  float u_scales[4];
  float v_scales[4];

  // Blendmap
  int blendmap_width;
  int blendmap_height;
  QByteArray blendmap_data; // RGBA pixels (width * height * 4)

  Terrain()
      : id(0), x(0), y(0), z(0), cols(32), rows(32), cell_size(64.0f),
        blendmap_width(32), blendmap_height(32) {
    heights.fill(0.0f, (cols + 1) * (rows + 1));
    for (int i = 0; i < 4; i++) {
      texture_ids[i] = 0;
      u_scales[i] = 1.0f;
      v_scales[i] = 1.0f;
    }
    blendmap_data.fill(char(0), blendmap_width * blendmap_height * 4);
    // Default: Layer 0 fully opaque (Red channel = 255)
    for (int i = 0; i < blendmap_width * blendmap_height * 4; i += 4) {
      blendmap_data[i] = char(255);
    }
  }
};

/* ============================================================================
   WALL - Pared con texturas múltiples
   ============================================================================
 */

struct Wall {
  int wall_id;
  float x1, y1, x2, y2;

  /* Texturas múltiples por altura */
  int texture_id_lower;
  int texture_id_middle;
  int texture_id_upper;
  float texture_split_z_lower; /* Default: 64.0 */
  float texture_split_z_upper; /* Default: 192.0 */

  int texture_id_lower_normal;
  int texture_id_middle_normal;
  int texture_id_upper_normal;

  int portal_id; /* -1 = sólida, >=0 = portal */
  int flags;

  Wall()
      : wall_id(0), x1(0), y1(0), x2(0), y2(0), texture_id_lower(0),
        texture_id_middle(0), texture_id_upper(0), texture_split_z_lower(64.0f),
        texture_split_z_upper(192.0f), texture_id_lower_normal(0),
        texture_id_middle_normal(0), texture_id_upper_normal(0), portal_id(-1),
        flags(0) {}
};

/* ============================================================================
   SECTOR - Polígono convexo
   ============================================================================
 */

struct Sector {
  int sector_id;

  /* Geometría del polígono (máx 16 vértices) */
  QVector<QPointF> vertices;

  /* Paredes */
  QVector<Wall> walls;

  /* Alturas */
  float floor_z;
  float ceiling_z;

  /* Slopes - REMOVED (Replaced by MD3 Models) */
  // Pivot is implicitly wall[0]

  /* Texturas */
  int floor_texture_id;
  int ceiling_texture_id;

  /* Normal Maps */
  int floor_normal_id;
  int ceiling_normal_id;

  /* Iluminación */
  int light_level;

  /* Portales (IDs) */
  QVector<int> portal_ids;

  int group_id; // NEW: ID of the group this sector belongs to (-1 if ungrouped)

  /* Jerarquía de sectores anidados */
  int parent_sector_id;          // -1 = root, >=0 = parent sector ID
  QVector<int> child_sector_ids; // List of child sector IDs

  /* ========================================================================
     SECTORES ANIDADOS (Nested Sectors) - REMOVED (v9 is flat)
     ======================================================================== */

  /* Constructor */
  Sector()
      : sector_id(0), floor_z(0.0f), ceiling_z(256.0f), floor_texture_id(0),
        ceiling_texture_id(0), floor_normal_id(0), ceiling_normal_id(0),
        light_level(255), group_id(-1), parent_sector_id(-1) {}
};

/* ============================================================================
   PORTAL - Conexión entre sectores
   ============================================================================
 */

struct Portal {
  int portal_id;
  int sector_a, sector_b;
  int wall_id_a, wall_id_b;
  float x1, y1, x2, y2;

  Portal()
      : portal_id(0), sector_a(-1), sector_b(-1), wall_id_a(-1), wall_id_b(-1),
        x1(0), y1(0), x2(0), y2(0) {}
};

/* ============================================================================
   SPRITE
   ============================================================================
 */

struct SpriteData {
  float x, y, z;
  int texture_id;
  int w, h;
  float rot;

  SpriteData() : x(0), y(0), z(0), texture_id(0), w(128), h(128), rot(0) {}
};

/* ============================================================================
   SPAWN FLAG
   ============================================================================
 */

struct SpawnFlag {
  int flagId;
  float x, y, z;
  bool isIntro;
  int npcPathId;
  bool autoStartPath;

  SpawnFlag()
      : flagId(1), x(384.0f), y(384.0f), z(0.0f), isIntro(false), npcPathId(-1),
        autoStartPath(false) {}
  SpawnFlag(int id, float px, float py, float pz)
      : flagId(id), x(px), y(py), z(pz), isIntro(false), npcPathId(-1),
        autoStartPath(false) {}
};

/* ============================================================================
   BEHAVIOR NODE SYSTEM
   ============================================================================
 */

struct NodePinData {
  int pinId;
  QString name;
  bool isInput;
  bool isExecution;
  QString value;
  int linkedPinId;

  NodePinData()
      : pinId(-1), name(""), isInput(true), isExecution(false), value(""),
        linkedPinId(-1) {}
};

struct NodeData {
  int nodeId;
  QString type;
  float x, y;
  QVector<NodePinData> pins;

  NodeData() : nodeId(-1), type(""), x(0), y(0) {}
};

struct BehaviorGraph {
  QVector<NodeData> nodes;
  int nextNodeId;
  int nextPinId;

  BehaviorGraph() : nextNodeId(1), nextPinId(1) {}
};

/* ============================================================================
   ENTITY INSTANCE - For process generation
   ============================================================================
 */

struct EntityInstance {
  QString processName;
  QString assetPath;
  QString type;
  int spawn_id;
  float x, y, z;
  float angle; // Rotation in degrees (0-360)

  // ===== BEHAVIOR SYSTEM =====
  enum ActivationType {
    ACTIVATION_ON_START = 0,
    ACTIVATION_ON_COLLISION = 1,
    ACTIVATION_ON_TRIGGER = 2,
    ACTIVATION_MANUAL = 3,
    ACTIVATION_ON_EVENT = 4
  };
  ActivationType activationType;
  QString collisionTarget;
  bool isVisible;
  QString customAction;
  QString eventName;

  // Visual Node Behavior
  BehaviorGraph behaviorGraph;

  // ===== PLAYER & CONTROL SYSTEM =====
  bool isPlayer;
  enum ControlType {
    CONTROL_NONE = 0,
    CONTROL_FIRST_PERSON = 1,
    CONTROL_THIRD_PERSON = 2,
    CONTROL_CAR = 3
  };
  ControlType controlType;
  bool cameraFollow;
  float cameraOffset_x, cameraOffset_y, cameraOffset_z;
  float cameraRotation;
  float initialRotation; // Initial model rotation in degrees

  // Cutscene properties
  bool isIntro;

  // NPC Path assignment
  int npcPathId;      // -1 = no path, >= 0 = path ID to follow
  bool autoStartPath; // Auto-start following path on spawn
  bool snapToFloor;   // Snap to floor height automatically

  // Billboard & Model Rendering properties
  int graphId;
  int startGraph;
  int endGraph;
  float animSpeed;
  int billboard_directions;

  // Physics / Collision Box (3D)
  int width;
  int depth;
  int height;

  bool collisionEnabled;

  EntityInstance()
      : spawn_id(0), x(0), y(0), z(0), angle(0),
        activationType(ACTIVATION_ON_START), collisionTarget("TYPE_PLAYER"),
        isVisible(true), customAction(""), eventName(""), isPlayer(false),
        controlType(CONTROL_NONE), cameraFollow(false), cameraOffset_x(0),
        cameraOffset_y(0), cameraOffset_z(0), cameraRotation(0),
        initialRotation(0), isIntro(false), npcPathId(-1), autoStartPath(false),
        snapToFloor(false), graphId(0), startGraph(0), endGraph(0),
        animSpeed(0), billboard_directions(1), width(64), depth(64),
        height(128), collisionEnabled(true) {}

  EntityInstance(const QString &pname, const QString &asset, const QString &t,
                 int id, float px, float py, float pz)
      : processName(pname), assetPath(asset), type(t), spawn_id(id), x(px),
        y(py), z(pz), angle(0), activationType(ACTIVATION_ON_START),
        collisionTarget("TYPE_PLAYER"), isVisible(true), customAction(""),
        eventName(""), isPlayer(false), controlType(CONTROL_NONE),
        cameraFollow(false), cameraOffset_x(0), cameraOffset_y(0),
        cameraOffset_z(0), cameraRotation(0), initialRotation(0),
        isIntro(false), npcPathId(-1), autoStartPath(false), snapToFloor(false),
        graphId(0), startGraph(0), endGraph(0), animSpeed(0),
        billboard_directions(1), width(64), depth(64), height(128),
        collisionEnabled(true) {}
};

/* ============================================================================
   DECAL - Overlay texture for floors/ceilings
   ============================================================================
 */

struct Decal {
  int id;        // Unique decal ID
  int sector_id; // Sector this decal belongs to
  bool is_floor; // true = floor, false = ceiling

  // Position and size (world coordinates)
  float x, y;          // Center position
  float width, height; // Dimensions
  float rotation;      // Rotation in radians

  // Texture
  int texture_id; // Texture ID from FPG

  // Rendering
  float alpha;      // Transparency (0.0 = invisible, 1.0 = opaque)
  int render_order; // Render order (higher = on top)

  Decal()
      : id(0), sector_id(-1), is_floor(true), x(0), y(0), width(100),
        height(100), rotation(0), texture_id(0), alpha(1.0f), render_order(0) {}
};

/* ============================================================================
   CAMERA
   ============================================================================
 */

struct CameraData {
  float x, y, z;
  float rotation;
  float pitch;
  bool enabled;

  CameraData()
      : x(384.0f), y(384.0f), z(0.0f), rotation(0.0f), pitch(0.0f),
        enabled(false) {}
};

/* ============================================================================
   SECTOR GROUP - Agrupación de sectores relacionados
   ============================================================================
 */

struct SectorGroup {
  int group_id;
  QString name;
  QVector<int> sector_ids; // IDs de sectores en este grupo

  SectorGroup() : group_id(-1), name("Grupo") {}
};

/* ============================================================================
   LIGHT - Focal or omni light
   ============================================================================
 */

struct Light {
  int id;
  float x, y, z;
  float radius;
  int color_r, color_g, color_b;
  float intensity;
  float falloff; // 1=linear, 2=quadratic
  bool active;

  Light()
      : id(0), x(0), y(0), z(64), radius(256.0f), color_r(255), color_g(255),
        color_b(255), intensity(1.0f), falloff(1.0f), active(true) {}
};

/* ============================================================================
   NPC PATH SYSTEM - Waypoint-based movement for NPCs
   ============================================================================
 */

struct Waypoint {
  float x, y, z;    // Position in world coordinates
  int wait_time;    // Frames to wait at this waypoint (0 = no wait)
  float speed;      // Movement speed to reach this waypoint
  float look_angle; // Direction to face while at this waypoint (-1 = auto)

  Waypoint() : x(0), y(0), z(0), wait_time(0), speed(5.0f), look_angle(-1.0f) {}

  Waypoint(float px, float py, float pz)
      : x(px), y(py), z(pz), wait_time(0), speed(5.0f), look_angle(-1.0f) {}
};

struct NPCPath {
  int path_id;  // Unique path ID
  QString name; // Path name (e.g., "guard_patrol")
  QVector<Waypoint> waypoints;

  enum LoopMode {
    LOOP_NONE = 0,     // One-shot, stop at end
    LOOP_REPEAT = 1,   // Loop back to start
    LOOP_PINGPONG = 2, // Reverse direction at ends
    LOOP_RANDOM = 3    // Pick random waypoints
  };
  LoopMode loop_mode;

  bool visible; // Show path in editor

  NPCPath()
      : path_id(0), name("npc_path"), loop_mode(LOOP_REPEAT), visible(true) {}
};

/* ============================================================================
   MAP DATA - Estructura principal del mapa
   ============================================================================
 */

struct MapData {
  /* Sectores y portales */
  QVector<Sector> sectors;
  QVector<Portal> portals;

  /* Sprites */
  QVector<SpriteData> sprites;

  /* Spawn Flags */
  QVector<SpawnFlag> spawnFlags;

  /* Decals */
  QVector<Decal> decals;

  /* Terrains (Heightmaps) */
  QVector<Terrain> terrains;

  /* Entities (MD3 Models) */
  QVector<EntityInstance> entities;

  /* Lights */
  QVector<Light> lights;

  /* Sector Groups */
  QVector<SectorGroup> sectorGroups;

  /* NPC Paths */
  QVector<NPCPath> npcPaths;

  /* Cámara */
  CameraData camera;

  /* Skybox */
  int skyTextureID = 0;

  /* Texturas cargadas */
  QVector<TextureEntry> textures;

  MapData() {}

  /* Helper: Get next sector ID */
  int getNextSectorId() const {
    int maxId = -1;
    for (const Sector &s : sectors) {
      if (s.sector_id > maxId)
        maxId = s.sector_id;
    }
    return maxId + 1;
  }

  /* Helper: Get next group ID */
  int getNextGroupId() const {
    int maxId = -1;
    for (const SectorGroup &g : sectorGroups) {
      if (g.group_id > maxId)
        maxId = g.group_id;
    }
    return maxId + 1;
  }

  /* Helper: Find sector by ID */
  Sector *findSector(int sectorId) {
    for (Sector &s : sectors) {
      if (s.sector_id == sectorId)
        return &s;
    }
    return nullptr;
  }

  const Sector *findSector(int sectorId) const {
    for (const Sector &s : sectors) {
      if (s.sector_id == sectorId)
        return &s;
    }
    return nullptr;
  }

  /* Helper: Find group by ID */
  SectorGroup *findGroup(int groupId) {
    for (SectorGroup &g : sectorGroups) {
      if (g.group_id == groupId)
        return &g;
    }
    return nullptr;
  }

  const SectorGroup *findGroup(int groupId) const {
    for (const SectorGroup &g : sectorGroups) {
      if (g.group_id == groupId)
        return &g;
    }
    return nullptr;
  }

  /* Helper: Find which group contains a sector */
  int findGroupForSector(int sectorId) const {
    for (const SectorGroup &g : sectorGroups) {
      if (g.sector_ids.contains(sectorId)) {
        return g.group_id;
      }
    }
    return -1;
  }

  /* Helper: Get next wall ID */
  int getNextWallId() const {
    int maxId = -1;
    for (const Sector &s : sectors) {
      for (const Wall &w : s.walls) {
        if (w.wall_id > maxId)
          maxId = w.wall_id;
      }
    }
    return maxId + 1;
  }

  /* Helper: Get next portal ID */
  int getNextPortalId() const {
    int maxId = -1;
    for (const Portal &p : portals) {
      if (p.portal_id > maxId)
        maxId = p.portal_id;
    }
    return maxId + 1;
  }

  /* Helper: Get next decal ID */
  int getNextDecalId() const {
    int maxId = -1;
    for (const Decal &d : decals) {
      if (d.id > maxId)
        maxId = d.id;
    }
    return maxId + 1;
  }

  /* Helper: Get next unified ID for SpawnFlags and Entities */
  int getNextSpawnEntityId() const {
    int maxId = 0;
    for (const SpawnFlag &f : spawnFlags) {
      if (f.flagId > maxId)
        maxId = f.flagId;
    }
    for (const EntityInstance &e : entities) {
      if (e.spawn_id > maxId)
        maxId = e.spawn_id;
    }
    return maxId + 1;
  }

  /* Helper: Find portal by ID */
  Portal *findPortal(int portal_id) {
    for (Portal &p : portals) {
      if (p.portal_id == portal_id)
        return &p;
    }
    return nullptr;
  }

  /* Helper: Find decal by ID */
  Decal *findDecal(int decal_id) {
    for (Decal &d : decals) {
      if (d.id == decal_id)
        return &d;
    }
    return nullptr;
  }
};

#endif // MAPDATA_H
