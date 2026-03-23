#include "processgenerator.h"
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QMap>
#include <QSet>
#include <QString>
#include <QStringList>
#include <QTextStream>
#include <QVector>
#include <QtGlobal>
#include <cmath>
#include <functional>

QString ProcessGenerator::generateProcessCode(const QString &processName,
                                              const QString &assetPath,
                                              const QString &type,
                                              const QString &wrapperOpen,
                                              const QString &wrapperClose) {
  QString code;
  QTextStream out(&code);

  out << "// Auto-generated process for " << processName << "\n";
  out << "// Asset: " << assetPath << "\n";
  out << "// Type: " << type << "\n\n";

  if (type == "model") {
    // Removed GLOBAL block to prevent compiler errors with multiple globals in
    // includes. Caching is disabled for file stability.

    out << "process " << processName << "(int spawn_id)\n";
    out << "private\n";
    out << "    int model_id;\n";
    out << "    int texture_id;\n";
    out << "    int sprite_id;\n";
    out << "    double world_x, world_y, world_z;\n";
    out << "    double rotation;\n";
    out << "    double scale;\n";
    out << "    // AI and Combat Variables\n";
    out << "    int s_id;\n";
    out << "    int s_idx;\n";
    out << "    double d_dist;\n";
    out << "    int behavior_timer;\n";
    out << "    double last_health;\n";
    out << "    int _npc_target;\n"; // Cached target process ID
    out << "    int colliding;\n"; // Collision target for graphs
    out << "    int recovery_timer;\n";
    out << "    int current_anim_start, current_anim_end, "
           "current_anim_speed;\n";
    out << "    int anim_current_frame, anim_next_frame;\n";
    out << "    float anim_interpolation;\n";
    out << "    int npc_path_active;\n";
    out << "begin\n";
    out << "    model_id = 0;\n";
    out << "    texture_id = 0;\n";
    out << "    rotation = 0.0;\n";
    out << "    scale = 1.0;\n";
    out << "    s_id = 0;\n";
    out << "    d_dist = 0.0;\n";
    out << "    behavior_timer = 0;\n";
    out << "    current_anim_start = -1; current_anim_end = -1;\n";
    out << "    current_anim_speed = 0;\n";
    out << "    anim_current_frame = 0; anim_next_frame = 0;\n";
    out << "    anim_interpolation = 0.0;\n";
    out << "    npc_path_active = 1;\n    recovery_timer = 0;\n";
    out << "    \n";
    out << "    // Get spawn position from flag\n";
    out << "    world_x = RAY_GET_FLAG_X(spawn_id);\n";
    out << "    world_y = RAY_GET_FLAG_Y(spawn_id);\n";
    out << "    world_z = RAY_GET_FLAG_Z(spawn_id);\n";
    out << "    \n";

    // Clean path logic
    QString cleanPath = assetPath;
    QFileInfo fi(assetPath);
    if (fi.isAbsolute()) {
      cleanPath = "assets/models/" + fi.fileName();
    }

    // Create texture path (assume .png for now, matching editor save logic)
    QString texturePath = cleanPath;
    if (texturePath.endsWith(".md3", Qt::CaseInsensitive)) {
      texturePath.replace(".md3", ".png", Qt::CaseInsensitive);
    } else {
      texturePath += ".png";
    }

    out << "    // Load Model and Texture\n";
    out << "    model_id = RAY_LOAD_MD3(" << wrapperOpen << "\"" << cleanPath
        << "\"" << wrapperClose << ");\n";
    out << "    texture_id = map_load(" << wrapperOpen << "\"" << texturePath
        << "\"" << wrapperClose << ");\n";
    out << "    \n";
    out << "    if (texture_id == 0)\n";
    out << "        say(\"[" << processName
        << "] WARNING: Failed to load texture: \" + \"" << texturePath
        << "\");\n";
    out << "    end\n";
    out << "    if (model_id == 0)\n";
    out << "        say(\"[" << processName
        << "] ERROR: Failed to load model: \" + \"" << cleanPath << "\");\n";
    out << "        RAY_CLEAR_FLAG();\n";
    out << "        return;\n";
    out << "    end\n";
    out << "    \n";

    out << "    // Create sprite with model\n";
    out << "    sprite_id = RAY_ADD_SPRITE(world_x, world_y, world_z, 0, 0, "
           "64, 64, 0);\n";
    out << "    if (sprite_id < 0)\n";
    out << "        say(\"[" << processName
        << "] ERROR: Failed to create sprite\");\n";
    out << "        RAY_CLEAR_FLAG();\n";
    out << "        return;\n";
    out << "    end\n";
    out << "    \n";
    out << "    RAY_SET_SPRITE_MD3(sprite_id, model_id, texture_id);\n";
    out << "    RAY_SET_SPRITE_SCALE(sprite_id, scale);\n";
    out << "    RAY_SET_SPRITE_ANGLE(sprite_id, rotation);\n";
    out << "    \n";
    out << "    loop\n";
    out << "        // Entity logic here\n";
    out << "        // Update position if needed:\n";
    out << "        // RAY_UPDATE_SPRITE_POSITION(sprite_id, world_x, world_y, "
           "world_z);\n";
    out << "        frame;\n";
    out << "    end\n";
    out << "    \n";
    out << "    // Cleanup\n";
    out << "    RAY_CLEAR_FLAG();\n";
    out << "    RAY_REMOVE_SPRITE(sprite_id);\n";
    out << "end\n";
  } else if (type == "campath" ||
             assetPath.endsWith(".campath", Qt::CaseInsensitive)) {
    // Camera Path Process wrapper using Native Engine Functions

    QFileInfo fi(assetPath);
    QString cleanPath = "assets/paths/" + fi.fileName();

    out << "process " << processName << "(int spawn_id)\n";
    out << "private\n";
    out << "    string path_file = \"" << cleanPath << "\";\n";
    out << "    int path_id = -1;\n";
    out << "    int p_id;\n";
    out << "    int dist;\n";
    out << "    double pos_x, pos_y, pos_z;\n";
    out << "begin\n";
    out << "    pos_x = RAY_GET_FLAG_X(spawn_id);\n";
    out << "    pos_y = RAY_GET_FLAG_Y(spawn_id);\n";
    out << "    pos_z = RAY_GET_FLAG_Z(spawn_id);\n";
    out << "    \n";
    out << "    // Preload path\n";
    out << "    path_id = RAY_CAMERA_LOAD(" << wrapperOpen << "path_file"
        << wrapperClose << ");\n";
    out << "    if (path_id < 0) say(\"Error loading path: \" + path_file); "
           "return; end\n";
    out << "    \n";
    out << "    loop\n";
    out << "        p_id = get_id(type player);\n";
    out << "        if (p_id)\n";
    out << "            dist = abs(p_id.x - pos_x) + abs(p_id.y - pos_y);\n";
    out << "            if (dist < 64)\n";
    out << "                // Trigger Cutscene\n";
    out << "                RAY_CAMERA_PLAY(path_id);\n";
    out << "                break;\n";
    out << "            end\n";
    out << "        end\n";
    out << "        frame;\n";
    out << "    end\n";
    out << "    RAY_CLEAR_FLAG();\n";
    out << "end\n";
  } else if (type == "sprite") {
    out << "process " << processName
        << "(float world_x, float world_y, float world_z)\n";
    out << "private\n";
    out << "    int sprite_id;\n";
    out << "    int texture_id = 1;  // TODO: Get from FPG\n";
    out << "begin\n";
    out << "    // Create sprite\n";
    out << "    sprite_id = RAY_ADD_SPRITE(world_x, world_y, world_z, 0, "
           "texture_id, 64, 64, 0);\n";
    out << "    \n";
    out << "    loop\n";
    out << "        // Sprite logic here\n";
    out << "        // Update position if needed:\n";
    out << "        // RAY_UPDATE_SPRITE_POSITION(sprite_id, world_x, world_y, "
           "world_z);\n";
    out << "        frame;\n";
    out << "    end\n";
    out << "    \n";
    out << "    // Cleanup\n";
    out << "    RAY_REMOVE_SPRITE(sprite_id);\n";
    out << "end\n";
  }

  return code;
}

QString ProcessGenerator::generateIncludesSection(
    const QVector<EntityInstance> &entities) {
  QString includes;
  QTextStream out(&includes);

  // Get unique process names
  QStringList uniqueProcesses = getUniqueProcessNames(entities);

  if (!uniqueProcesses.isEmpty()) {
    out << "// Entity includes\n";
    for (const QString &processName : uniqueProcesses) {
      out << "include \"includes/" << processName << ".h\";\n";
    }
    out << "\n";
  }

  return includes;
}

QString
ProcessGenerator::generateSpawnCalls(const QVector<EntityInstance> &entities) {
  QString spawns;
  QTextStream out(&spawns);

  if (!entities.isEmpty()) {
    out << "    // Initialize NPC paths\n";
    out << "    npc_paths_init();\n\n";
    out << "    // Spawn entities using data from flags\n";
    for (const EntityInstance &entity : entities) {
      QString uniqueName =
          entity.processName + "_" + QString::number(entity.spawn_id);
      out << "    // Entity " << uniqueName << " at flag " << entity.spawn_id
          << "\n";
      out << "    " << uniqueName << "(";
      out << "RAY_GET_FLAG_X(" << entity.spawn_id << "), ";
      out << "RAY_GET_FLAG_Y(" << entity.spawn_id << "), ";
      out << "RAY_GET_FLAG_Z(" << entity.spawn_id << "), ";
      out << "(float)" << entity.cameraRotation << ");\n";
    }
    out << "\n";
  }

  return spawns;
}

QString ProcessGenerator::generateAllProcessesCode(
    const QVector<EntityInstance> &entities, const QString &wrapperOpen,
    const QString &wrapperClose) {
  QString code;
  QTextStream out(&code);

  // Find player process name
  QString playerProcessName = ""; // Default empty
  for (const EntityInstance &entity : entities) {
    if (entity.isPlayer) {
      playerProcessName =
          entity.processName + "_" + QString::number(entity.spawn_id);
      out << "// DEBUG INFO: Found Player Entity -> Process Name: '"
          << playerProcessName << "' (Type: " << entity.type << ")\n";
      break;
    }
  }

  out << "// Combat and identity variables accessible across processes\n";
  out << "local\n";
  out << "  float health;\n";
  out << "  int is_dead;\n";
  out << "  int collision_detected;\n";
  out << "  int _npc_target;\n";
  out << "  int attack_hit_timer;\n";
  out << "  double last_health;\n";
  out << "  string ent_name;\n";
  out << "end\n\n";

  // Generate one process per entity INSTANCE (not per unique name)
  // Each instance gets a unique name: processName_spawnId
  QSet<QString> generatedNames;
  for (const EntityInstance &entity : entities) {
    // Build unique process name for this instance
    QString uniqueName =
        entity.processName + "_" + QString::number(entity.spawn_id);

    // Skip if already generated (e.g. from hybrid map scan duplicates)
    if (generatedNames.contains(uniqueName.toLower()))
      continue;
    generatedNames.insert(uniqueName.toLower());

    // Create a copy with the unique process name
    EntityInstance instanceCopy = entity;
    instanceCopy.processName = uniqueName;

    QString procCode = generateProcessCodeWithBehavior(
        instanceCopy, wrapperOpen, wrapperClose, playerProcessName);

    out << procCode << "\n";
  }

  return code;
}

bool ProcessGenerator::saveProcessFile(const QString &projectPath,
                                       const QString &processName,
                                       const QString &code) {
  // Create includes directory if it doesn't exist
  QString includesPath = projectPath + "/src/includes";
  QDir dir;
  if (!dir.exists(includesPath)) {
    dir.mkpath(includesPath);
  }

  // Save file
  QString filePath = includesPath + "/" + processName + ".h";
  QFile file(filePath);

  // Safety check: Do not overwrite existing files to preserve manual user edits
  if (file.exists()) {
    return true;
  }

  if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
    return false;
  }

  QTextStream out(&file);
  out << code << "\n";
  out.flush(); // Ensure written
  file.close();

  return true;
}

QStringList ProcessGenerator::getUniqueProcessNames(
    const QVector<EntityInstance> &entities) {
  QSet<QString> uniqueNames;

  for (const EntityInstance &entity : entities) {
    uniqueNames.insert(entity.processName.toLower());
  }

  return uniqueNames.values();
}

QString ProcessGenerator::generateDeclarationsSection(
    const QVector<EntityInstance> &entities) {
  QString declarations;
  QTextStream out(&declarations);

  // Global player position for NPC AI and Delta Time
  out << "// Global variables shared across all auto-generated processes\n";
  out << "GLOBAL\n";
  out << "    double g_player_x;\n";
  out << "    double g_player_y;\n";
  out << "    double g_player_z;\n";
  out << "    int    g_player_health;\n";
  out << "    double g_delta_time;   // seconds since last frame (FPS-independent)\n";
  out << "    int    g_last_frame_ms; // last get_timer() value\n";
  out << "END\n\n";

  // Generate one declaration per entity instance
  QSet<QString> declaredNames;
  for (const EntityInstance &entity : entities) {
    QString uniqueName =
        entity.processName + "_" + QString::number(entity.spawn_id);
    if (!declaredNames.contains(uniqueName.toLower())) {
      declaredNames.insert(uniqueName.toLower());
      out << "declare process " << uniqueName
          << "(int param_x, int param_y, int param_z, int param_angle);\n";
    }
  }

  return declarations;
}

QString ProcessGenerator::generateProcessCodeWithBehavior(
    const EntityInstance &entity, const QString &wrapperOpen,
    const QString &wrapperClose, const QString &playerTypeName) {
  QString code;
  QTextStream out(&code);

  // Derive hook base name from asset file (e.g. "Car.md3" -> "car")
  // so all instances of the same model share hook functions
  QString hookBaseName = QFileInfo(entity.assetPath).baseName().toLower();
  hookBaseName =
      hookBaseName.replace(" ", "_").replace("-", "_").replace(".", "_");

  out << "// ========================================\n";
  out << "// Entity: " << entity.processName << "\n";
  out << "// Type: " << entity.type << "\n";
  out << "// Asset: " << entity.assetPath << "\n";
  out << "// ========================================\n\n";

  // Pre-generate graph code to check for event presence
  QString actionCode = entity.customAction;
  if (!entity.behaviorGraph.nodes.isEmpty()) {
    actionCode = ProcessGenerator::generateGraphCode(
        entity, entity.behaviorGraph, "event_start", playerTypeName);
  }

  QString updateCode;
  if (!entity.behaviorGraph.nodes.isEmpty()) {
    updateCode = ProcessGenerator::generateGraphCode(
        entity, entity.behaviorGraph, "event_update", playerTypeName);
  }

  QString collisionCode;
  if (!entity.behaviorGraph.nodes.isEmpty()) {
    collisionCode = ProcessGenerator::generateGraphCode(
        entity, entity.behaviorGraph, "event_collision", playerTypeName);
  }

  QString deathCode;
  if (!entity.behaviorGraph.nodes.isEmpty()) {
    deathCode = ProcessGenerator::generateGraphCode(
        entity, entity.behaviorGraph, "event_death", playerTypeName);
  }

  QString playerDeathCode;
  if (!entity.behaviorGraph.nodes.isEmpty()) {
    playerDeathCode = ProcessGenerator::generateGraphCode(
        entity, entity.behaviorGraph, "event_player_death", playerTypeName);
  }

  out << "process " << entity.processName
      << "(int param_x, int param_y, int param_z, int param_angle)\n";
  out << "private\n";

  // Hitbox graphic handle
  out << "    int hitbox_id;\n";
  // Common variables - double for radians/subpixel precision
  out << "    double world_x; double world_y; double world_z; double world_angle;\n";
  out << "    double dx, dy, dz, d_dist;\n";
  out << "    int player_death_triggered = 0;\n";
  out << "    int s_idx; int recovery_timer; int colliding;\n";
  out << "    int car_engine_id;\n";
  
  out << "    public\n";
  out << "        int npc_path_active;\n";
  
  if (entity.type == "model" || entity.type == "gltf") {
    out << "        int model_id;\n";
    out << "        int texture_id;\n";
    out << "        int sprite_id;\n";
    out << "        double rotation;\n";
    out << "        double scale;\n";
    out << "        double anim_interpolation;\n";
    out << "        int anim_current_frame;\n";
    out << "        int anim_next_frame;\n";
    out << "        int current_anim_start;\n";
    out << "        int current_anim_end;\n";
    out << "        double current_anim_speed;\n";
  } else if (entity.type == "campath") {
    out << "        int campath_id;\n";
  }

  if (entity.activationType == EntityInstance::ACTIVATION_ON_EVENT) {
    out << "        int event_triggered;\n";
  }

  if (entity.isPlayer || entity.controlType == EntityInstance::CONTROL_CAR) {
    out << "        double move_speed;\n";
    out << "        double rot_speed;\n";
    out << "        double player_angle;\n";
    out << "        double turn_offset;\n";
    out << "        double angle_milis_car;\n";
    out << "        double speed;\n";
  }

  if (entity.isPlayer) {
    out << "        double pitch_speed;\n";
    out << "        double player_pitch;\n";
    out << "        double cam_angle_off;\n";
    out << "        double move_angle;\n";
    out << "        double cam_safe_x, cam_safe_y, cam_dx, cam_dy;\n";
    out << "        double cam_test_x, cam_test_y;\n";
    out << "        int cam_steps, cam_i;\n";
    out << "        double angle_milis;\n";
  }

  if (entity.npcPathId >= 0) {
    out << "        int npc_current_waypoint;\n";
    out << "        int npc_wait_counter;\n";
    out << "        int npc_direction;\n";
  }

  out << "        string texture_path_base;\n";
  out << "        string alt_path;\n";
  out << "        int s_id;\n";
  out << "        int cur_speed;\n";
  out << "        int speed_vol;\n";
  out << "        int collision_target;\n";
  out << "        double angle_to_target;\n";
  out << "        int behavior_timer;\n";
  out << "        int _npc_ang;\n";
  out << "        int is_attacking;\n";
  out << "        int pending_dmg;\n";
  out << "        double cx, cy, cz, s, c, angle_deg;\n";
  out << "begin\n";
  out << "    car_engine_id = 0;\n";
  out << "    health = 100.0; // Init local health\n";
  out << "    last_health = 100.0;\n";
  out << "    is_dead = 0;\n";
  out << "    is_attacking = 0;\n";
  out << "    attack_hit_timer = 0;\n";
  out << "    pending_dmg = 0;\n";
  out << "    recovery_timer = 0;\n";
  out << "    colliding = -1;\n";
  out << "    s_idx = -1;\n";
  out << "    npc_path_active = 0;\n";
  out << "    \n";
  out << "    // Create collision hitbox (Generic for 3D Actors)\n";
  out << "    hitbox_id = map_new(10, 10, 32);\n";
  out << "    map_clear(0, hitbox_id, rgb(255, 255, 255));\n";
  out << "    graph = hitbox_id;\n";
  out << "    size = " << qRound((entity.width / 10.0f) * 100.0f) << ";\n";
  out << "    flags = 4; // Center graph and make it invisible-ish\n";
  out << "    \n";
  out << "    ent_name = \"" << entity.processName << "\";\n";
  if (entity.type == "model" || entity.type == "gltf") {
    out << "    model_id = 0;\n";
    out << "    texture_id = 0;\n";
    out << "    sprite_id = -1;\n";
    out << "    rotation = 0.0;\n";
    out << "    scale = " << entity.scale << ";\n";
    out << "    anim_interpolation = 0.0;\n";
    out << "    current_anim_start = " << entity.startGraph << ";\n";
    out << "    current_anim_end = " << entity.endGraph << ";\n";
    out << "    current_anim_speed = " << entity.animSpeed << ";\n";
    out << "    anim_current_frame = current_anim_start;\n";
    out << "    anim_next_frame = current_anim_start + 1;\n";
    out << "    if (anim_next_frame > current_anim_end) anim_next_frame = "
           "current_anim_start; end\n";
  } else if (entity.type == "campath") {
    out << "    campath_id = 0;\n";
  }

  // Keep graph = 0 for 3D entities (no 2D rendering)
  // Collision is handled via d_dist proximity check, not 2D collision()
  out << "    graph = 0; // Disable 2D rendering for 3D entities\n";

  // Store collision proximity threshold (80 world units = touching distance)
  bool hasCollisionEvent = !collisionCode.isEmpty();
  if ((entity.activationType == EntityInstance::ACTIVATION_ON_COLLISION ||
       hasCollisionEvent) && !entity.isPlayer) {
    out << "    collision_detected = 0;\n";
  }

  if (entity.isPlayer) {
    out << "    move_speed = 8.0;\n";
    out << "    rot_speed = 0.05;\n";
    out << "    pitch_speed = 0.05;\n";
    out << "    player_angle = 0.0;\n";
    out << "    player_pitch = 0.0;\n";
    out << "    turn_offset = 0.0;\n";
    out << "    dx = 0.0; dy = 0.0;\n";
    out << "    cam_angle_off = " << (entity.cameraRotation * 0.0174f) << ";\n";
  }
  if (entity.npcPathId >= 0) {
    out << "    npc_current_waypoint = 0;\n";
    out << "    npc_wait_counter = 0;\n";
    out << "    npc_direction = 1;\n";
    out << "    npc_path_active = 1;\n";
  }

  out << "    world_x = param_x; world_y = param_y;\n";
  if (entity.type == "campath") {
    out << "    world_z = param_z;\n";
  } else {
    // Auto-adjust height to floor + offset for regular entities
    out << "    world_z = RAY_GET_FLOOR_HEIGHT(world_x, world_y) + param_z;\n";
  }
  out << "    x = world_x; y = world_y; z = world_z;\n";
  out << "    world_angle = param_angle * 0.017453;\n";
  if (entity.isPlayer) {
    out << "    player_angle = world_angle;\n";
  }

  out << "    // USER HOOK: Initialization\n";
  out << "    hook_" << hookBaseName << "_init(id);\n\n";

  out << "    say(\"Spawned Entity: " << entity.processName
      << " at \" + world_x + \",\" + world_y);\n";

  // Load assets based on type
  if (entity.type == "model") {
    QString cleanPath = entity.assetPath;
    // Logic to ensure path is relative (distributable)
    if (cleanPath.contains("/assets/")) {
      cleanPath = cleanPath.mid(cleanPath.indexOf("assets/"));
    } else if (cleanPath.contains("\\assets\\")) {
      cleanPath = cleanPath.mid(cleanPath.indexOf("assets\\"));
    }

    QString texturePath = cleanPath;
    if (texturePath.endsWith(".md3", Qt::CaseInsensitive)) {
      texturePath.replace(".md3", ".png", Qt::CaseInsensitive);
    } else {
      texturePath += ".png";
    }

    out << "    // Load Model and Texture (Localized path for MD3)\n";
    out << "    texture_path_base = \"" << texturePath.section('.', 0, -2)
        << "\";\n";
    out << "    model_id = RAY_LOAD_MD3(" << wrapperOpen << "\"" << cleanPath
        << "\"" << wrapperClose << ");\n";

    out << "    // Try PNG then JPG\n";
    out << "    texture_id = map_load(" << wrapperOpen
        << "texture_path_base + \".png\"" << wrapperClose << ");\n";
    out << "    if (texture_id <= 0) texture_id = map_load(" << wrapperOpen
        << "texture_path_base + \".jpg\"" << wrapperClose << "); end\n";

    out << "    if (texture_id <= 0)\n";
    out << "       // Try same directory as model\n";
    out << "       alt_path = \"assets/md3/\" + \""
        << QFileInfo(entity.assetPath).baseName() << "\";\n";
    out << "       texture_id = map_load(" << wrapperOpen
        << "alt_path + \".png\"" << wrapperClose << ");\n";
    out << "       if (texture_id <= 0) texture_id = map_load(" << wrapperOpen
        << "alt_path + \".jpg\"" << wrapperClose << "); end\n";
    out << "    end\n";

    out << "    if (model_id == 0) say(\"[" << entity.processName
        << "] ERROR: Failed to load model: \" + " << wrapperOpen << "\""
        << cleanPath << "\"" << wrapperClose << "); end\n";
    out << "    if (texture_id == 0) say(\"[" << entity.processName
        << "] WARNING: Failed to load texture: \" + " << wrapperOpen
        << "texture_path_base" << wrapperClose << "); end\n";
    out << "    if (model_id == 0)\n";
    out << "        // RAY_CLEAR_FLAG();\n";
    out << "        return;\n";
    out << "    end\n";
    out << "    \n";
    out << "    // Create sprite\n";
    out << "    sprite_id = RAY_ADD_SPRITE(world_x, world_y, world_z, 0, 0, "
           "64, 64, 0);\n";
    out << "    if (sprite_id < 0)\n";
    out << "        say(\"[" << entity.processName
        << "] ERROR: Failed to create sprite\");\n";
    out << "        // RAY_CLEAR_FLAG();\n";
    out << "        return;\n";
    out << "    end\n";
    out << "    \n";
    out << "    RAY_SET_SPRITE_MD3(sprite_id, model_id, texture_id);\n";
    out << "    RAY_SET_SPRITE_SCALE(sprite_id, scale);\n";
    out << "    RAY_SET_SPRITE_ANGLE(sprite_id, param_angle);\n";
    out << "    \n";

    // Animation support
    if (entity.startGraph != 0 || entity.endGraph != 0 ||
        entity.animSpeed != 0) {
      out << "    RAY_SET_SPRITE_ANIM(sprite_id, " << entity.startGraph << ", "
          << entity.endGraph << ", 0.0);\n";
    }
  } else if (entity.type == "gltf") {
    QString cleanPath = entity.assetPath;
    if (cleanPath.contains("/assets/")) {
      cleanPath = cleanPath.mid(cleanPath.indexOf("assets/"));
    } else if (cleanPath.contains("\\assets\\")) {
      cleanPath = cleanPath.mid(cleanPath.indexOf("assets\\"));
    }

    out << "    // Load GLTF/GLB Model\n";
    out << "    model_id = RAY_LOAD_GLTF(" << wrapperOpen << "\"" << cleanPath
        << "\"" << wrapperClose << ");\n";
    out << "    if (model_id == 0) say(\"[" << entity.processName
        << "] ERROR: Failed to load GLTF: " << cleanPath << "\"); end\n";

    out << "    // Create sprite\n";
    out << "    sprite_id = RAY_ADD_SPRITE(world_x, world_y, world_z, 0, 0, "
           "64, 64, 0);\n";
    out << "    if (sprite_id >= 0)\n";
    out << "        RAY_SET_SPRITE_GLTF(sprite_id, model_id);\n";
    out << "        RAY_SET_SPRITE_SCALE(sprite_id, scale);\n";
    out << "        RAY_SET_SPRITE_ANGLE(sprite_id, param_angle);\n";
    out << "    end\n";
  }

  // Common properties for all models (MD3 and GLTF)
  if (entity.type == "model" || entity.type == "gltf") {
    // Visibility
    if (!entity.isVisible) {
      out << "    // Entity is invisible\n";
      out << "    RAY_SET_SPRITE_FLAGS(sprite_id, SPRITE_INVISIBLE);\n";
      out << "    \n";
    }

    // Physics Engine
    if (entity.physicsEnabled) {
      out << "    // Physics Engine Configuration\n";
      out << "    RAY_PHYSICS_ENABLE(sprite_id, " << entity.physicsMass << ", "
          << (entity.width / 2.0f) << ", " << entity.height << ");\n";

      if (entity.physicsFriction != 0.5f) {
        out << "    RAY_PHYSICS_SET_FRICTION(sprite_id, "
            << entity.physicsFriction << ");\n";
      }
      if (entity.physicsRestitution != 0.3f) {
        out << "    RAY_PHYSICS_SET_RESTITUTION(sprite_id, "
            << entity.physicsRestitution << ");\n";
      }
      if (entity.physicsGravityScale != 1.0f) {
        out << "    RAY_PHYSICS_SET_GRAVITY_SCALE(sprite_id, "
            << entity.physicsGravityScale << ");\n";
      }
      if (entity.physicsLinearDamping != 0.05f ||
          entity.physicsAngularDamping != 0.1f) {
        out << "    RAY_PHYSICS_SET_DAMPING(sprite_id, "
            << entity.physicsLinearDamping << ", "
            << entity.physicsAngularDamping << ");\n";
      }
      if (entity.physicsIsStatic) {
        out << "    RAY_PHYSICS_SET_STATIC(sprite_id, 1);\n";
      }
      if (entity.physicsIsKinematic) {
        out << "    RAY_PHYSICS_SET_KINEMATIC(sprite_id, 1);\n";
      }
      if (entity.physicsIsTrigger) {
        out << "    RAY_PHYSICS_SET_TRIGGER(sprite_id, 1);\n";
      }
      if (entity.physicsLockRotX || entity.physicsLockRotY ||
          entity.physicsLockRotZ) {
        out << "    RAY_PHYSICS_LOCK_ROTATION(sprite_id, "
            << (entity.physicsLockRotX ? 1 : 0) << ", "
            << (entity.physicsLockRotY ? 1 : 0) << ", "
            << (entity.physicsLockRotZ ? 1 : 0) << ");\n";
      }
      if (entity.physicsCollisionLayer != 1 ||
          entity.physicsCollisionMask != 0xFFFF) {
        out << "    RAY_PHYSICS_SET_LAYER(sprite_id, "
            << entity.physicsCollisionLayer << ", "
            << entity.physicsCollisionMask << ");\n";
      }
      out << "    \n";
    }
  } else if (entity.type == "campath") {
    QString cleanPath = entity.assetPath;
    // Logic to ensure path is relative (distributable)
    if (cleanPath.contains("/assets/")) {
      cleanPath = cleanPath.mid(cleanPath.indexOf("assets/"));
    } else if (cleanPath.contains("\\assets\\")) {
      cleanPath = cleanPath.mid(cleanPath.indexOf("assets\\"));
    } else {
      // Fallback: if not in assets folder, assume assets/paths/ for legacy
      // compatibility or just filename
      QFileInfo fi(entity.assetPath);
      cleanPath = "assets/paths/" + fi.fileName();
    }

    out << "    // Load Camera Path\n";
    out << "    campath_id = RAY_CAMERA_LOAD(" << wrapperOpen << "\""
        << cleanPath << "\"" << wrapperClose << ");\n";
    out << "    if (campath_id < 0)\n";
    out << "        say(\"[" << entity.processName
        << "] ERROR: Failed to load campath\");\n";
    out << "        RAY_CLEAR_FLAG();\n";
    out << "        return;\n";
    out << "    end\n";
    out << "    \n";
    out << "    // Start playing automatically\n";
    out << "    RAY_CAMERA_PLAY(campath_id);\n";
    out << "    say(\"DEBUG: [\" + campath_id + \"] Started Playback. "
           "Playing=\" + RAY_CAMERA_IS_PLAYING());\n";

    // Intro Logic
    if (entity.isIntro) {
      out << "    say(\"DEBUG: Entering Intro Loop (Player: " << playerTypeName
          << ")\");\n";
      out << "    \n";
      out << "    // Intro Sequence: Block Player\n";
      if (!playerTypeName.isEmpty()) {
        out << "    signal(type " << playerTypeName << ", s_sleep);\n";
      } else {
        out << "    // WARNING: No player process found to block input\n";
      }
      out << "    \n";
      out << "    while(RAY_CAMERA_IS_PLAYING())\n";
      out << "        RAY_CAMERA_PATH_UPDATE(0.0166);\n";
      out << "        frame;\n";
      out << "    end\n";
      out << "    \n";
      if (!playerTypeName.isEmpty()) {
        out << "    signal(type " << playerTypeName << ", s_wakeup);\n";
      }
    }
  }

  // --- SCAN BEHAVIOR GRAPH FOR UNIFIED PARAMS (CHASE, DAMAGE, ETC) ---
  bool hasChase = false;
  QString chaseSpeed = "1";
  int chaseRange = 400;
  bool hasDamage = false;
  QString damageAmount = "10";
  QString hitFrame = ""; 

  for (const auto &node : entity.behaviorGraph.nodes) {
    if (node.type == "action_npc_chase") {
      hasChase = true;
      if (node.pins.size() > 3 && !node.pins[3].value.isEmpty())
        chaseSpeed = node.pins[3].value;
    }
    if (node.type == "logic_compare") {
      if (node.pins.size() > 2 && !node.pins[2].value.isEmpty()) {
        bool ok;
        int val = node.pins[2].value.toInt(&ok);
        if (ok && val > 0) chaseRange = val;
      }
    }
    if (node.type == "action_damage") {
      hasDamage = true;
      if (node.pins.size() > 2 && !node.pins[2].value.isEmpty())
        damageAmount = node.pins[2].value;
      if (node.pins.size() > 4 && !node.pins[4].value.isEmpty() && node.pins[4].value != "0")
        hitFrame = node.pins[4].value;
    }
  }

  // Behavior implementation
  out << "    // ===== BEHAVIOR =====\n";

  switch (entity.activationType) {
  case EntityInstance::ACTIVATION_ON_START: {
    out << "    // Activate on start\n";
    if (!actionCode.isEmpty()) {
      QString customCode = actionCode;
      out << "    " << customCode.replace("\n", "\n    ") << "\n";
    }
    out << "    g_last_frame_ms = get_timer();\n";
    out << "    loop\n";
        out << "        // --- GLOBAL UPDATE SYNC ---\n";
        out << "        x = world_x; y = world_y; z = world_z;\n";
        out << "        if (is_dead == 0)\n";
        if (entity.isPlayer) {
            out << "            g_player_x = world_x; g_player_y = world_y; g_player_z = world_z; g_player_health = health;\n";
        } else {
            out << "            dx = world_x - g_player_x; dy = world_y - g_player_y; d_dist = sqrt(dx*dx + dy*dy);\n";
        }
        out << "        end\n\n";
        out << "        // Nodes found in graph: " << entity.behaviorGraph.nodes.size() << "\n";

        // --- 1. COLLISION SCAN (Always active) ---\n";
        out << "        s_idx = RAY_CHECK_SPRITE_COLLISION(sprite_id, world_x, world_y, " << (entity.width / 1.5f) << ");\n";
        out << "        if (s_idx >= 0)\n";
        out << "            _npc_target = RAY_GET_SPRITE_ID(s_idx);\n";
        out << "            if (_npc_target > 0)\n";
        out << "                _npc_target.collision_detected = 1; // Bidirectional alert\n";
        out << "                _npc_target._npc_target = sprite_id;\n";
        out << "                if (colliding == 0)\n";
        out << "                    collision_detected = 1;\n";
        if (!collisionCode.isEmpty()) {
            out << "                    " << collisionCode.replace("\n", "\n                    ") << "\n";
        }
        out << "                end\n";
        out << "            end\n";
        out << "            colliding = 1;\n";
        out << "        else\n";
        out << "            collision_detected = 0;\n";
        out << "            colliding = 0;\n";
        out << "        end\n\n";

        // --- 2. BEHAVIOR GRAPH LOGIC ---\n";
        // (Nodes like ACTION_DAMAGE or ACTION_NPC_CHASE are generated here)\n";

        // --- 3. INPUTS / AI / PATHS ---\n";
        out << "        if (is_dead == 0)\n";
        if (entity.isPlayer) {
            switch (entity.controlType) {
                case EntityInstance::CONTROL_FIRST_PERSON:
                    out << "        if (key(_w)) RAY_MOVE_FORWARD(move_speed); end\n";
                    out << "        if (key(_s)) RAY_MOVE_BACKWARD(move_speed); end\n";
                    out << "        if (key(_a)) RAY_STRAFE_LEFT(move_speed); end\n";
                    out << "        if (key(_d)) RAY_STRAFE_RIGHT(move_speed); end\n";
                    out << "        if (key(_left)) RAY_ROTATE(-rot_speed); end\n";
                    out << "        if (key(_right)) RAY_ROTATE(rot_speed); end\n";
                    out << "        if (key(_up)) RAY_LOOK_UP_DOWN(pitch_speed); end\n";
                    out << "        if (key(_down)) RAY_LOOK_UP_DOWN(-pitch_speed); end\n";
                    out << "        world_x = RAY_GET_CAMERA_X(); world_y = RAY_GET_CAMERA_Y(); world_z = RAY_GET_CAMERA_Z();\n";
                    out << "        player_angle = RAY_GET_CAMERA_ROT(); player_pitch = RAY_GET_CAMERA_PITCH();\n";
                    break;
                case EntityInstance::CONTROL_CAR:
                    out << "        angle_milis_car = player_angle * 57295.8; dx = 0; dy = 0; turn_offset *= 0.8;\n";
                    out << "        if (key(_left) || key(_a)) player_angle -= rot_speed; turn_offset -= 5.0; end\n";
                    out << "        if (key(_right) || key(_d)) player_angle += rot_speed; turn_offset += 5.0; end\n";
                    if (entity.physicsEnabled) {
                        out << "        if (key(_w) || key(_up)) RAY_PHYSICS_APPLY_FORCE(sprite_id, cos(angle_milis_car) * move_speed * " << (entity.physicsMass * 200.0f) << ", sin(angle_milis_car) * move_speed * " << (entity.physicsMass * 200.0f) << ", 0); end\n";
                        out << "        if (key(_s) || key(_down)) RAY_PHYSICS_APPLY_FORCE(sprite_id, -cos(angle_milis_car) * move_speed * " << (entity.physicsMass * 120.0f) << ", -sin(angle_milis_car) * move_speed * " << (entity.physicsMass * 120.0f) << ", 0); end\n";
                        out << "        world_x = RAY_GET_SPRITE_X(sprite_id); world_y = RAY_GET_SPRITE_Y(sprite_id); world_z = RAY_GET_SPRITE_Z(sprite_id);\n";
                    } else {
                        out << "        if (key(_w) || key(_up)) dx += cos(player_angle * 57295.8) * move_speed * 60 * g_delta_time; dy += sin(player_angle * 57295.8) * move_speed * 60 * g_delta_time; end\n";
                        out << "        if (key(_s) || key(_down)) dx -= cos(player_angle * 57295.8) * move_speed * 60 * g_delta_time; dy -= sin(player_angle * 57295.8) * move_speed * 60 * g_delta_time; end\n";
                        out << "        RAY_SET_STEP_HEIGHT(5.0);\n";
                    }
                    break;
            }
        } else {
            // NPC AI / PATHS / OTHERS
            out << "        dx = world_x - g_player_x; dy = world_y - g_player_y; d_dist = sqrt(dx*dx + dy*dy);\n";
            if (entity.npcPathId >= 0) {
                out << "        if (npc_path_active == 1)\n";
                out << "            npc_follow_path(" << entity.npcPathId << ", &npc_current_waypoint, &npc_wait_counter, &npc_direction, &world_x, &world_y, &world_z, &world_angle, " << (entity.snapToFloor ? "1" : "0") << ");\n";
                out << "        end\n";
            }
        }
        out << "        end\n";
        
            if (entity.type == "campath") {
            out << "        if (RAY_CAMERA_IS_PLAYING()) RAY_CAMERA_PATH_UPDATE(0.0166); world_x = RAY_GET_CAMERA_X(); world_y = RAY_GET_CAMERA_Y(); world_z = RAY_GET_CAMERA_Z(); end\n";
        }

        if (entity.snapToFloor && (entity.npcPathId < 0 || !entity.autoStartPath)) {
            out << "        world_z = RAY_GET_FLOOR_HEIGHT(world_x, world_y);\n";
        }

        // --- 4. GENERIC UPDATE LOGIC ---\n";
        if (!updateCode.isEmpty()) {
            out << "        if (is_dead == 0)\n";
            out << "            " << updateCode.replace("\n", "\n            ") << "\n";
            out << "        end\n";
        }

        // --- 5. CORE STATE: DEATH & DAMAGE (UNIFIED) ---\n";
        out << "        if (is_dead == 0 and health <= 0.0)\n";
        out << "            is_dead = 1;\n";
        out << "            npc_path_active = 0;\n";
        if (!deathCode.isEmpty()) {
            out << "            " << deathCode.replace("\n", "\n            ") << "\n";
        } else {
            out << "            fx_hit(world_x, world_y, world_z + 32);\n";
        }
        if (entity.isPlayer && !playerDeathCode.isEmpty()) {
            out << "            " << playerDeathCode.replace("\n", "\n            ") << "\n";
        }
        out << "        end\n\n";

        out << "        if (health < last_health)\n";
        out << "            recovery_timer = 12;\n";
        out << "            if (health <= 0.0)\n";
        {
            QString onDeathGenInner = ProcessGenerator::generateGraphCode(entity, entity.behaviorGraph, "event_death", playerTypeName);
            if (!onDeathGenInner.isEmpty()) {
                out << "                " << onDeathGenInner.replace("\n", "\n                ") << "\n";
            }
        }
        out << "                is_dead = 1; npc_path_active = 0; collision_detected = 0;\n";
        out << "                // Force death animation if none set\n";
        out << "                if (current_anim_start == 0) current_anim_start = 78; current_anim_end = 148; end\n";
        out << "            end\n";
        QString onDamageGen = ProcessGenerator::generateGraphCode(entity, entity.behaviorGraph, "event_damage", playerTypeName);
        if (!onDamageGen.isEmpty()) {
            out << "            " << onDamageGen.replace("\n", "\n            ") << "\n";
        }
        out << "        end\n";
        out << "        last_health = health;\n";
        out << "        if (recovery_timer > 0) recovery_timer--; end\n\n";

        // --- 6. VISUAL SYNC & CAMERA ---\n";
        if (entity.type == "model" || entity.type == "gltf") {
            if (entity.isPlayer || entity.controlType == EntityInstance::CONTROL_CAR) {
                out << "        world_angle = player_angle + (turn_offset * 0.005);\n";
            }
            out << "        RAY_UPDATE_SPRITE_POSITION(sprite_id, world_x, world_y, world_z);\n";
            out << "        RAY_SET_SPRITE_ANGLE(sprite_id, (world_angle * 57.2957));\n";
            
            if (entity.cameraFollow && (entity.controlType == EntityInstance::CONTROL_THIRD_PERSON || entity.controlType == EntityInstance::CONTROL_CAR)) {
                 out << "        cx = world_x - cos((player_angle + cam_angle_off) * 57295.8) * 450; cy = world_y - sin((player_angle + cam_angle_off) * 57295.8) * 450;\n";
                 out << "        RAY_SET_CAMERA(cx, cy, world_z + 120, (player_angle + cam_angle_off), player_pitch);\n";
            }
            
            out << "        if (current_anim_speed != 0)\n";
            out << "            anim_interpolation += abs(current_anim_speed) * g_delta_time;\n";
            out << "            if (anim_interpolation >= 1.0)\n";
            out << "                anim_interpolation = 0.0;\n";
            out << "                if (is_dead == 0 or anim_current_frame < current_anim_end)\n";
            out << "                    anim_current_frame = anim_next_frame;\n";
            out << "                    anim_next_frame++;\n";
            out << "                    if (anim_next_frame > current_anim_end)\n";
            out << "                        if (is_dead == 1)\n";
            out << "                            anim_current_frame = current_anim_end;\n";
            out << "                            anim_next_frame = current_anim_end;\n";
            out << "                            anim_interpolation = 1.0;\n";
            out << "                        else\n";
            out << "                            anim_next_frame = current_anim_start;\n";
            out << "                        end\n";
            out << "                    end\n";
            out << "                else\n";
            out << "                    anim_current_frame = current_anim_end;\n";
            out << "                    anim_next_frame = current_anim_end;\n";
            out << "                    anim_interpolation = 1.0;\n";
            out << "                end\n";
            out << "            end\n";
            out << "        end\n";
            if (entity.type == "model") {
                out << "        RAY_SET_SPRITE_ANIM(sprite_id, anim_current_frame, anim_next_frame, anim_interpolation);\n";
            } else {
                out << "        RAY_SET_SPRITE_GLB_ANIM(sprite_id, current_anim_start, anim_interpolation);\n";
            }
        }

        out << "        g_delta_time = (get_timer() - g_last_frame_ms) / 1000.0;\n";
        out << "        if (g_delta_time <= 0 or g_delta_time > 0.1) g_delta_time = 0.016; end\n";
        out << "        g_last_frame_ms = get_timer();\n";
        out << "        hook_" << hookBaseName << "_update(id);\n";
        out << "        frame;\n";
    out << "    end\n";
    break;
  }

  case EntityInstance::ACTIVATION_ON_COLLISION:
    out << "    // Activate on collision\n";
    if (!collisionCode.isEmpty()) {
      actionCode = collisionCode;
    }
    out << "    loop\n";
    out << "        if (collision(collision_target) and collision_detected "
           "== "
           "0)\n";
    out << "            collision_detected = 1;\n";
    if (!actionCode.isEmpty()) {
      out << "            " << actionCode.replace("\n", "\n            ")
          << "\n";
    } else {
      out << "            say(\"[" << entity.processName
          << "] Collision detected!\");\n";
    }
    out << "        end\n";
    out << "        // USER HOOK: Update\n";
    out << "        hook_" << hookBaseName << "_update(id);\n";
    if (entity.isPlayer) {
      out << "        g_player_health = health;\n";
    }
    out << "        frame;\n";
    out << "    end\n";
    break;

  case EntityInstance::ACTIVATION_ON_TRIGGER:
    out << "    // Activate on trigger (area detection)\n";
    out << "    loop\n";
    out << "        // TODO: Implement area trigger detection\n";
    out << "        // Check if player is within range\n";
    out << "        // USER HOOK: Update\n";
    out << "        hook_" << hookBaseName << "_update(id);\n";
    out << "        frame;\n";
    out << "    end\n";
    break;

  case EntityInstance::ACTIVATION_ON_EVENT:
    out << "    // Activate on event: " << entity.eventName << "\n";
    out << "    loop\n";
    out << "        if (event_triggered)\n";
    if (!actionCode.isEmpty()) {
      out << "            " << actionCode.replace("\n", "\n            ")
          << "\n";
    }
    out << "            break;\n";
    out << "        end\n";
    out << "        frame;\n";
    out << "    end\n";
    break;

  case EntityInstance::ACTIVATION_MANUAL:
    out << "    // Manual activation\n";
    if (!actionCode.isEmpty()) {
      out << "    " << actionCode.replace("\n", "\n    ") << "\n";
    }
    out << "    loop\n";
    out << "        // Custom logic here\n";
    out << "        frame;\n";
    out << "    end\n";
    break;
  }

  // Cleanup
  out << "    \n";
  out << "    // Cleanup\n";
  out << "    RAY_CLEAR_FLAG();\n";
  if (entity.type == "model") {
    out << "    RAY_REMOVE_SPRITE(sprite_id);\n";
  }
  out << "end\n\n";

  return code;
}

// ===== NPC PATH CODE GENERATION =====

QString
ProcessGenerator::generateNPCPathsCode(const QVector<NPCPath> &npcPaths) {
  QString code;
  QTextStream out(&code);

  out << "// ===== NPC PATH SYSTEM =====\n";
  out << "// Auto-generated NPC path data and helper functions\n\n";

  // Generate path data structures
  for (const NPCPath &path : npcPaths) {
    if (path.waypoints.isEmpty())
      continue;

    out << "// Path: " << path.name << " (ID: " << path.path_id << ")\n";
    out << "global\n";
    out << "  int npc_path_" << path.path_id << "_waypoints["
        << path.waypoints.size() << "][6];\n";
    out << "  int npc_path_" << path.path_id << "_count;\n";
    out << "  int npc_path_" << path.path_id << "_loop_mode;\n";
    out << "end\n\n";
  }

  // Initialize path data helper (function, not process, so it runs inline)
  out << "function npc_paths_init()\n";
  out << "begin\n";
  for (const NPCPath &path : npcPaths) {
    if (path.waypoints.isEmpty())
      continue;
    out << "  npc_path_" << path.path_id << "_count = " << path.waypoints.size()
        << ";\n";
    out << "  npc_path_" << path.path_id
        << "_loop_mode = " << static_cast<int>(path.loop_mode) << ";\n";

    int wpIndex = 0;
    for (const Waypoint &wp : path.waypoints) {
      out << "  npc_path_" << path.path_id << "_waypoints[" << wpIndex
          << "][0] = " << static_cast<int>(wp.x * 1000) << ";\n";
      out << "  npc_path_" << path.path_id << "_waypoints[" << wpIndex
          << "][1] = " << static_cast<int>(wp.y * 1000) << ";\n";
      out << "  npc_path_" << path.path_id << "_waypoints[" << wpIndex
          << "][2] = " << static_cast<int>(wp.z * 1000) << ";\n";
      out << "  npc_path_" << path.path_id << "_waypoints[" << wpIndex
          << "][3] = " << static_cast<int>(wp.speed * 1000) << ";\n";
      out << "  npc_path_" << path.path_id << "_waypoints[" << wpIndex
          << "][4] = " << wp.wait_time << ";\n";
      out << "  npc_path_" << path.path_id << "_waypoints[" << wpIndex
          << "][5] = " << static_cast<int>(wp.look_angle * 1000) << ";\n";
      wpIndex++;
    }
  }
  out << "end\n\n";

  // Helper functions for NPC behavior (Safe from "Process 0 not active"
  // errors)
  out << "// Safe property access helpers\n";
  out << "function float get_val_x(int _id) begin if (exists(_id)) return "
         "_id.x; end return 0.0; end\n";
  out << "function float get_val_y(int _id) begin if (exists(_id)) return "
         "_id.y; end return 0.0; end\n";
  out << "function float get_val_z(int _id) begin if (exists(_id)) return "
         "_id.z; end return 0.0; end\n\n";

  out << "// Safe 3D Distance calculation (returns distance in editor "
         "units)\n";
  out << "function float get_dist_3d(int id1, int id2)\n";
  out << "begin\n";
  out << "    if (not exists(id1) or not exists(id2)) return 999999.0; "
         "end\n";
  out << "    return sqrt(pow(get_val_x(id1)-get_val_x(id2),2) + "
         "pow(get_val_y(id1)-get_val_y(id2),2) + "
         "pow((get_val_z(id1)-get_val_z(id2))/16.0,2)) / 100.0;\n";
  out << "end\n\n";

  // Helper function to follow a path - ALWAYS present to avoid undefined
  // procedure errors
  out << "// NPC Path Following Helper\n";
  out << "function npc_follow_path(int path_id, int pointer current_wp, "
         "int "
         "pointer wait_counter, int pointer direction, double pointer "
         "cur_x, "
         "double pointer cur_y, double pointer cur_z, double pointer "
         "cur_angle, int snap_to_floor)\n";
  out << "private\n";
  out << "  int waypoint_count;\n";
  out << "  int loop_mode;\n";
  out << "  int wp_idx;\n";
  out << "  double target_x, target_y, target_z;\n";
  out << "  double speed;\n";
  out << "  int wait_time;\n";
  out << "  double look_angle;\n";
  out << "  double dx, dy, dz, d_dist;\n";
  out << "begin\n";
  out << "  waypoint_count = 0;\n";
  out << "  loop_mode = 0;\n";
  out << "  speed = 0.0;\n";
  out << "  wait_time = 0;\n";
  out << "  look_angle = -1000000.0;\n";

  if (!npcPaths.isEmpty()) {
    out << "  switch (path_id)\n";
    for (const NPCPath &path : npcPaths) {
      if (path.waypoints.isEmpty())
        continue;
      out << "    case " << path.path_id << ":\n";
      out << "      waypoint_count = npc_path_" << path.path_id << "_count;\n";
      out << "      loop_mode = npc_path_" << path.path_id << "_loop_mode;\n";
      out << "      wp_idx = *current_wp;\n";
      out << "      if (wp_idx >= 0 and wp_idx < waypoint_count)\n";
      out << "        target_x = npc_path_" << path.path_id
          << "_waypoints[wp_idx][0] / 1000.0;\n";
      out << "        target_y = npc_path_" << path.path_id
          << "_waypoints[wp_idx][1] / 1000.0;\n";
      out << "        target_z = npc_path_" << path.path_id
          << "_waypoints[wp_idx][2] / 1000.0;\n";
      out << "        speed = npc_path_" << path.path_id
          << "_waypoints[wp_idx][3] / 1000.0;\n";
      out << "        wait_time = npc_path_" << path.path_id
          << "_waypoints[wp_idx][4];\n";
      out << "        look_angle = npc_path_" << path.path_id
          << "_waypoints[wp_idx][5] / 1000.0;\n";
      out << "      end\n";
      out << "    end\n";
    }
    out << "  end\n\n";
  } else {
    out << "  return;\n";
  }

  out << "  if (*wait_counter > 0)\n";
  out << "      *wait_counter = *wait_counter - 1;\n";
  out << "      return;\n";
  out << "  end\n\n";

  out << "  dx = target_x - *cur_x;\n";
  out << "  dy = target_y - *cur_y;\n";
  out << "  d_dist = sqrt(dx*dx + dy*dy);\n\n";

  out << "  if (d_dist < speed + 1.0)\n";
  out << "    *cur_x = target_x; *cur_y = target_y;\n";
  out << "    if (wait_time > 0) *wait_counter = wait_time; end\n";
  out << "    switch (loop_mode)\n";
  out << "      case 0: if (*current_wp < waypoint_count - 1) *current_wp "
         "= "
         "*current_wp + 1; end; end\n";
  out << "      case 1: *current_wp = (*current_wp + 1) % waypoint_count; "
         "end\n";
  out << "      case 2: *current_wp = *current_wp + *direction;\n";
  out << "              if (*current_wp >= waypoint_count - 1) *direction "
         "= "
         "-1; end\n";
  out << "              if (*current_wp <= 0) *direction = 1; end; end\n";
  out << "      case 3: *current_wp = rand(0, waypoint_count - 1); end\n";
  out << "    end\n";
  out << "  elseif (d_dist > 0.0)\n";
  out << "    *cur_x = *cur_x + (dx * speed / d_dist);\n";
  out << "    *cur_y = *cur_y + (dy * speed / d_dist);\n";
  out << "    // Smooth Z Z-climb vs floor sensing\n";
  out << "    dz = target_z - *cur_z;\n";
  out << "    if (snap_to_floor != 0 or target_z <= 1.0)\n";
  out << "        *cur_z = RAY_GET_FLOOR_HEIGHT(*cur_x, *cur_y);\n";
  out << "    elseif (dz > 1.0 or dz < -1.0)\n";
  out << "        *cur_z = *cur_z + (dz * 0.1);\n";
  out << "    else\n";
  out << "        *cur_z = target_z;\n";
  out << "    end\n\n";

  out << "    // Rotation: face direction of movement\n";
  out << "    if (look_angle > -0.9) // If look_angle is set (not the -1.0 "
         "magic value)\n";
  out << "        target_x = look_angle * 0.01745329; // Degrees to Radians\n";
  out << "    elseif (d_dist > 5.0)\n";
  out << "        // fget_angle returns millidegrees; negate Y for 3D coord "
         "system, convert to radians\n";
  out << "        target_x = -fget_angle(0, 0, dx * 1000.0, dy * 1000.0) / "
         "57295.78;\n";
  out << "    else\n";
  out << "        target_x = *cur_angle;\n";
  out << "    end\n";
  out << "    \n";
  out << "    // Correct angular difference for smooth Lerp (handles 0-2PI "
         "wrap)\n";
  out << "    target_y = target_x - *cur_angle;\n";
  out << "    while (target_y > 3.14159) target_y = target_y - 6.28318; end\n";
  out << "    while (target_y < -3.14159) target_y = target_y + 6.28318; end\n";
  out << "    *cur_angle = *cur_angle + (target_y * 0.15); // Smooth turn (15% "
         "per frame)\n";
  out << "  end\n";
  out << "end\n";

  return code;
}

QString ProcessGenerator::generateGraphCode(const EntityInstance &entity,
                                            const BehaviorGraph &graph,
                                            const QString &eventType,
                                            const QString &playerTypeName) {
  if (graph.nodes.isEmpty())
    return "";

  QMap<int, const NodePinData *> pinMap;
  QMap<int, const NodeData *> pinToNodeMap;

  for (int i = 0; i < graph.nodes.size(); ++i) {
    const NodeData &node = graph.nodes[i];
    for (int j = 0; j < node.pins.size(); ++j) {
      const NodePinData &pin = node.pins[j];
      pinMap[pin.pinId] = &pin;
      pinToNodeMap[pin.pinId] = &node;
    }
  }

  struct Resolver {
    const QMap<int, const NodePinData *> &pm;
    const QMap<int, const NodeData *> &ptnm;
    const QString &playerProcessName;

    QString resolve(int pinId) const {
      const NodePinData *pin = pm.value(pinId, nullptr);
      if (!pin)
        return "0";

      if (!pin->linkedPinIds.isEmpty()) {
        const NodePinData *linkedPin = nullptr;
        for (int lid : pin->linkedPinIds) {
            linkedPin = pm.value(lid, nullptr);
            if (linkedPin) break;
        }
        if (linkedPin) {
          const NodeData *srcNode = ptnm.value(linkedPin->pinId, nullptr);
          if (srcNode) {
            if (srcNode->type == "math_dist" ||
                srcNode->type == "math_camera_dist") {
              QString target = (srcNode->type == "math_dist")
                                   ? resolve(srcNode->pins[1].pinId)
                                   : "TYPE_PLAYER";

              bool isPlayer = (target == "TYPE_PLAYER" ||
                               (!playerProcessName.isEmpty() &&
                                target.contains(playerProcessName)));

              if (isPlayer) {
                // Use pre-computed d_dist scaled for editor units
                // (divide by 4 so user values like 60/500 map to real
                // 240/2000)
                return "(d_dist / 4.0)";
              }

              if (target == "TYPE_PLAYER" && !playerProcessName.isEmpty())
                target = "get_id(type " + playerProcessName + ")";
              return QString("get_dist_3d(id, %1)").arg(target);
            } else if (srcNode->type == "logic_compare") {
              QString a = resolve(srcNode->pins[0].pinId);
              QString b = resolve(srcNode->pins[1].pinId);
              QString op = srcNode->pins[2].value;
              if (op == "Greater") return QString("(%1 > %2)").arg(a, b);
              if (op == "Greater Equal") return QString("(%1 >= %2)").arg(a, b);
              if (op == "Less") return QString("(%1 < %2)").arg(a, b);
              if (op == "Less Equal") return QString("(%1 <= %2)").arg(a, b);
              if (op == "Equal") return QString("(%1 == %2)").arg(a, b);
              if (op == "Not Equal") return QString("(%1 != %2)").arg(a, b);
              return QString("(%1 < %2)").arg(a, b); // Default
            } else if (srcNode->type == "logic_and") {
              return QString("(%1 AND %2)").arg(resolve(srcNode->pins[0].pinId), resolve(srcNode->pins[1].pinId));
            } else if (srcNode->type == "logic_or") {
              return QString("(%1 OR %2)").arg(resolve(srcNode->pins[0].pinId), resolve(srcNode->pins[1].pinId));
            } else if (srcNode->type == "logic_not") {
              return QString("(NOT %1)").arg(resolve(srcNode->pins[0].pinId));
            } else if (srcNode->type == "math_add") {
              return QString("(%1 + %2)").arg(resolve(srcNode->pins[0].pinId), resolve(srcNode->pins[1].pinId));
            } else if (srcNode->type == "math_sub") {
              return QString("(%1 - %2)").arg(resolve(srcNode->pins[0].pinId), resolve(srcNode->pins[1].pinId));
            } else if (srcNode->type == "math_mul") {
              return QString("(%1 * %2)").arg(resolve(srcNode->pins[0].pinId), resolve(srcNode->pins[1].pinId));
            } else if (srcNode->type == "math_div") {
              return QString("(%1 / %2)").arg(resolve(srcNode->pins[0].pinId), resolve(srcNode->pins[1].pinId));
            } else if (srcNode->type == "math_random") {
                QString min = resolve(srcNode->pins[0].pinId);
                QString max = resolve(srcNode->pins[1].pinId);
                return QString("rand(%1, %2)").arg(min, max);
            } else if (srcNode->type == "math_point_dist") {
              return QString("RAY_GET_POINT_DIST(%1, %2, %3, %4, %5, %6)")
                  .arg(resolve(srcNode->pins[0].pinId))
                  .arg(resolve(srcNode->pins[1].pinId))
                  .arg(resolve(srcNode->pins[2].pinId))
                  .arg(resolve(srcNode->pins[3].pinId))
                  .arg(resolve(srcNode->pins[4].pinId))
                  .arg(resolve(srcNode->pins[5].pinId));
            } else if (srcNode->type == "math_angle") {
              return QString("RAY_GET_ANGLE(%1, %2)")
                  .arg(resolve(srcNode->pins[0].pinId))
                  .arg(resolve(srcNode->pins[1].pinId));
            } else if (srcNode->type == "math_camera_angle") {
              return QString("RAY_GET_CAMERA_ANGLE(%1)")
                  .arg(resolve(srcNode->pins[0].pinId));
            } else if (srcNode->type == "logic_key") {
                QString keyName = resolve(srcNode->pins[0].pinId);
                keyName.remove("\"");
                if (keyName.startsWith("K_") || keyName.startsWith("_"))
                    return "key(" + keyName + ")";
                return "key(_" + keyName + ")";
            } else if (srcNode->type == "math_op" ||
                       srcNode->type == "logic_compare") {
              return QString("(%1 %2 %3)")
                  .arg(resolve(srcNode->pins[0].pinId))
                  .arg(srcNode->pins[1].value)
                  .arg(resolve(srcNode->pins[2].pinId));
            }
          }
        }
      }

      QString val = pin->value;
      if (val.toLower() == "health")
        return "health";
      if (val.toLower() == "last_health")
        return "last_health";
      if (val.toLower() == "colliding")
        return "colliding"; // BennuGD global variable for collision target
      if (val.toLower() == "nearby_npc")
        return "get_id(type campath)"; // Usually NPCs are on paths. We should probably use a better way later.
      if (!playerProcessName.isEmpty()) {
        QString lowerPlayerProc = playerProcessName.toLower();
        // Only replace if it doesn't already look like it's been resolved
        if (!val.toLower().contains(lowerPlayerProc)) {
          val.replace("TYPE_PLAYER", "get_id(type " + playerProcessName + ")",
                      Qt::CaseInsensitive);
          val.replace("type player", "type " + playerProcessName,
                      Qt::CaseInsensitive);
          if (val.toLower() == "player" || val.toLower() == "target")
            val = "get_id(type " + playerProcessName + ")";
        }
      }
      return val.isEmpty() ? "0" : val;
    }
  } res = {pinMap, pinToNodeMap, playerTypeName};

  QString code;
  QTextStream out(&code);

  std::function<void(const NodeData *, int, QSet<int> &)> generateFlow;
  generateFlow = [&](const NodeData *current, int indent, QSet<int> &visited) {
    if (!current) {
        QString ind = QString(indent, ' ');
        out << ind << "// generateFlow null pointer hit!\n";
        return;
    }
    QString ind = QString(indent, ' ');
    out << ind << "// generateFlow visiting: " << current->type << "\n";
    if (visited.contains(current->nodeId))
      return;
    visited.insert(current->nodeId);


    if (current->type == "action_say") {
      QString msg = res.resolve(current->pins[2].pinId);
      if (!msg.startsWith("\""))
        msg = "\"" + msg + "\"";
      out << ind << "say(" << msg << ");\n";
    } else if (current->type == "action_kill") {
      out << ind << "signal(" << res.resolve(current->pins[1].pinId)
          << ", s_kill);\n";
    } else if (current->type == "action_moveto") {
      QString tx = res.resolve(current->pins[2].pinId);
      QString ty = res.resolve(current->pins[3].pinId);
      out << ind << " world_x = " << tx << "; world_y = " << ty << ";\n";
      out << ind
          << " my_x = world_x; my_y = world_y; // Update UI pos if text\n";
      out << ind
          << " RAY_UPDATE_SPRITE_POSITION(sprite_id, world_x, world_y, "
             "world_z);\n";
    } else if (current->type == "action_stop_music") {
      out << ind << " music_stop();\n";
    } else if (current->type == "action_stop_sound") {
      out << ind << " sound_stop(0); // Stop all sounds\n";
    } else if (current->type == "action_damage") {
      QString dmgNodeVal = res.resolve(current->pins[2].pinId);
      QString targetVal = res.resolve(current->pins[3].pinId);

      if (targetVal.isEmpty() || targetVal == "0" || targetVal == "TYPE_PLAYER") {
          targetVal = "_npc_target";
      }

      if (dmgNodeVal.isEmpty()) dmgNodeVal = "10.0";
      
      // If it's a numeric constant, keep it as is. If it's a string, we wrap it?
      // Actually, BennuGD accepts both if the string matches a variable name like 'move_speed'.
      
      // Pin 4 = Hit Frame
      QString hitFrame = "0"; // Default to instant for graphs
      if (current->pins.size() > 4) {
        QString hf = res.resolve(current->pins[4].pinId);
        // If empty, auto-calculate. If "0", it's instant.
        if (hf.isEmpty()) {
            hitFrame = "current_anim_end";
        } else {
            hitFrame = hf;
        }
      }

      out << ind << " // Damage impact logic\n";
      out << ind << " if (anim_current_frame >= " << hitFrame << " and attack_hit_timer == 0)\n";
      out << ind << "    if (" << targetVal << " > 0)\n";
      out << ind << "        " << targetVal << ".health = " << targetVal << ".health - (" << dmgNodeVal << ");\n";
      out << ind << "        attack_hit_timer = 30;\n";
      out << ind << "    end\n";
      out << ind << " end\n";
      out << ind << " // Reset hit timer when animation loops or is reset\n";
      out << ind << " if (anim_current_frame <= current_anim_start + 1)\n";
      out << ind << "     attack_hit_timer = 0;\n";
      out << ind << " end\n";
    } else if (current->type == "action_die") {
      QString startF = res.resolve(current->pins[2].pinId);
      QString endF = res.resolve(current->pins[3].pinId);
      QString billboard = res.resolve(current->pins[4].pinId);
      out << ind << " current_anim_start = " << startF
          << "; current_anim_end = " << endF << "; current_anim_speed = 10;\n";
      out << ind << " anim_current_frame = " << startF
          << "; anim_next_frame = " << startF << " + 1;\n";
      out << ind << " fx_hit(world_x, world_y, world_z + 32);\n";
    } else if (current->type == "action_set_health") {
      QString hVal = res.resolve(current->pins[2].pinId);
      QString target = res.resolve(current->pins[3].pinId);
      if (target.toLower() == "self" || target == "0") {
        out << ind << " health = " << hVal << ";\n";
      } else {
        out << ind << " _npc_target = " << target << ";\n";
        out << ind << " _npc_target.health = " << hVal << ";\n";
      }
    } else if (current->type == "action_spawn_billboard") {
      QString file = res.resolve(current->pins[2].pinId);
      file.remove("\"");
      QString gStart = res.resolve(current->pins[3].pinId);
      QString gEnd = res.resolve(current->pins[4].pinId);
      QString speed = res.resolve(current->pins[5].pinId);
      QString scale = res.resolve(current->pins[6].pinId);
      QString repeatMode = "0";
      if (current->pins.size() > 7) {
          repeatMode = res.resolve(current->pins[7].pinId);
      }
      
      // Load the FPG file and spawn the billboard
      out << ind << " s_id = fpg_load(get_asset_path(\"" << file << "\"));\n";
      out << ind << " if (s_id > 0) Billboard_Effect_Process(world_x, world_y, world_z, s_id, " 
          << gStart << ", " << gEnd << ", " << speed << ", " << scale << ", " << repeatMode << "); end\n";
    } else if (current->type == "action_npc_chase") {
      QString speed = res.resolve(current->pins[3].pinId);
      out << ind << " // Chase: move toward player while player is alive\n";
      out << ind << " npc_path_active = 0;\n";
      out << ind << " if (g_player_health > 0.0)\n";
      out << ind << "     if (d_dist > 5.0)\n";
      out << ind
          << "         world_angle = -fget_angle(0, 0, (g_player_x - "
             "world_x)*1000.0, (g_player_y - world_y)*1000.0) / 57295.78;\n";
      out << ind << "         if (collision_detected == 0)\n";
      out << ind << "             world_x += ((g_player_x - world_x) / d_dist) * "
          << speed << " * 3;\n";
      out << ind << "             world_y += ((g_player_y - world_y) / d_dist) * "
          << speed << " * 3;\n";
      if (entity.snapToFloor)
        out << ind
            << "             world_z = RAY_GET_FLOOR_HEIGHT(world_x, world_y);\n";
      out << ind << "         end\n";
      out << ind << "     end\n";
      // Walk animation: only when not in attack range/collision
      out << ind << "     if (current_anim_start != 0 and collision_detected == 0)\n";
      out << ind
          << "         current_anim_start = 0; current_anim_end = 14; "
             "current_anim_speed = 10;\n";
      out << ind << "         anim_current_frame = 0; anim_next_frame = 1;\n";
      out << ind << "         anim_interpolation = 0.0;\n";
      out << ind << "     end\n";
      out << ind << " end\n";
    } else if (current->type == "action_npc_attack") {
      // All-in-one attack node: proximity + animation + damage + cooldown
      QString range    = res.resolve(current->pins[2].pinId);
      QString damage   = res.resolve(current->pins[3].pinId);
      QString cooldown = res.resolve(current->pins[4].pinId);
      QString animStart= res.resolve(current->pins[5].pinId);
      QString animEnd  = res.resolve(current->pins[6].pinId);
      // Cooldown in frames (60fps), hit fires at half-period
      QString cooldownFrames = QString("((int)((%1)*60.0))").arg(cooldown);
      out << ind << " // ACTION_NPC_ATTACK: self-contained attack system\n";
      out << ind << " if (d_dist < " << range << " and g_player_health > 0.0)\n";
      out << ind << "     npc_path_active = 0;\n";
      out << ind << "     world_angle = -fget_angle(0, 0, (g_player_x - world_x)*1000.0, (g_player_y - world_y)*1000.0) / 57295.78;\n";
      out << ind << "     if (current_anim_start != " << animStart << " or current_anim_end != " << animEnd << ")\n";
      out << ind << "         current_anim_start = " << animStart << "; current_anim_end = " << animEnd << "; current_anim_speed = 10;\n";
      out << ind << "         anim_current_frame = " << animStart << "; anim_next_frame = " << animStart << " + 1;\n";
      out << ind << "         anim_interpolation = 0.0;\n";
      out << ind << "         is_attacking = 1;\n";
      out << ind << "         attack_hit_timer = " << cooldownFrames << "; // Reset timer on state start\n";
      out << ind << "     end\n";
      out << ind << "     is_attacking = 1;\n";
      out << ind << " else\n";
      out << ind << "     // Player out of range or player dead: reset attack state\n";
      out << ind << "     if (is_attacking == 1)\n";
      out << ind << "         is_attacking = 0; attack_hit_timer = 0;\n";
      out << ind << "         current_anim_start = 0; current_anim_end = 14; current_anim_speed = 10;\n";
      out << ind << "         anim_current_frame = 0; anim_next_frame = 1; anim_interpolation = 0.0;\n";
      out << ind << "     end\n";
      out << ind << " end\n";
      out << ind << " // Attack timing logic (only when player is alive)\n";
      out << ind << " if (is_attacking == 1 and g_player_health > 0.0)\n";
      out << ind << "     if (attack_hit_timer > 0)\n";
      out << ind << "         attack_hit_timer = attack_hit_timer - 1;\n";
      out << ind << "     else\n";
      out << ind << "         attack_hit_timer = " << cooldownFrames << "; // Auto-repeat\n";
      out << ind << "     end\n";
      out << ind << "     // HIT FRAME: Fires when timer reaches half-way\n";
      out << ind << "     if (attack_hit_timer == (" << cooldownFrames << " / 2))\n";
      out << ind << "         _npc_target = get_id(type " << playerTypeName << ");\n";
      out << ind << "         if (_npc_target > 0) _npc_target.health = _npc_target.health - ( " << damage << " ); end\n";
      out << ind << "     end\n";
      out << ind << " else\n";
      out << ind << "     // Player dead: force reset attack state\n";
      out << ind << "     if (is_attacking == 1 and g_player_health <= 0.0)\n";
      out << ind << "         is_attacking = 0; attack_hit_timer = 0;\n";
      out << ind << "         current_anim_start = 0; current_anim_end = 14; current_anim_speed = 10;\n";
      out << ind << "         anim_current_frame = 0; anim_next_frame = 1; anim_interpolation = 0.0;\n";
      out << ind << "     end\n";
      out << ind << " end\n";
    } else if (current->type == "ai_melee_director") {
      QString visionRange = res.resolve(current->pins[2].pinId);
      QString speed       = res.resolve(current->pins[3].pinId);
      QString attackRange = res.resolve(current->pins[4].pinId);
      QString damage      = res.resolve(current->pins[5].pinId);
      QString animStart   = res.resolve(current->pins[6].pinId);
      QString animEnd     = res.resolve(current->pins[7].pinId);
      QString animHit     = res.resolve(current->pins[8].pinId);
      QString cooldown    = res.resolve(current->pins[9].pinId);
      QString cooldownFrames = QString("((int)((%1)*60.0))").arg(cooldown);

      out << ind << " // --- AI MELEE DIRECTOR ---\n";
      out << ind << " if (g_player_health > 0.0 and recovery_timer == 0)\n";
      out << ind << "     if (d_dist < (" << attackRange << " * 2.2))\n";
      out << ind << "         // ESTADO: ATACAR\n";
      out << ind << "         npc_path_active = 0;\n";
      out << ind << "         world_angle = -fget_angle(0, 0, (g_player_x - world_x)*1000.0, (g_player_y - world_y)*1000.0) / 57295.78;\n";
      out << ind << "         if (current_anim_start != " << animStart << " or current_anim_end != " << animEnd << ")\n";
      out << ind << "             current_anim_start = " << animStart << "; current_anim_end = " << animEnd << "; current_anim_speed = 10;\n";
      out << ind << "             anim_current_frame = " << animStart << "; anim_next_frame = " << animStart << " + 1; anim_interpolation = 0.0;\n";
      out << ind << "         end\n";
      out << ind << "         // Aplicar Daño en exactamente el frame especificado\n";
      out << ind << "         if (anim_current_frame == (" << animHit << ") and attack_hit_timer == 0)\n";
      out << ind << "             _npc_target = get_id(type " << playerTypeName << ");\n";
      out << ind << "             if (_npc_target > 0 and d_dist <= (" << attackRange << " * 2.5))\n";
      out << ind << "                 _npc_target.health = _npc_target.health - ( " << damage << " );\n";
      out << ind << "             end\n";
      out << ind << "             attack_hit_timer = " << cooldownFrames << ";\n";
      out << ind << "         end\n";
      out << ind << "         if (attack_hit_timer > 0) attack_hit_timer = attack_hit_timer - 1; end\n";
      out << ind << "         // Reset timer si la animacion reinicia el ciclo (vuelve a empezar)\n";
      out << ind << "         if (anim_current_frame <= " << animStart << " + 1 and attack_hit_timer == 1) attack_hit_timer = 0; end\n";
      out << ind << "     elseif (d_dist < (" << visionRange << " * 4.0))\n";
      out << ind << "         // ESTADO: PERSEGUIR\n";
      out << ind << "         npc_path_active = 0;\n";
      out << ind << "         world_angle = -fget_angle(0, 0, (g_player_x - world_x)*1000.0, (g_player_y - world_y)*1000.0) / 57295.78;\n";
      out << ind << "         if (collision_detected == 0)\n";
      out << ind << "             world_x += ((g_player_x - world_x) / d_dist) * " << speed << " * 3;\n";
      out << ind << "             world_y += ((g_player_y - world_y) / d_dist) * " << speed << " * 3;\n";
      if (entity.snapToFloor)
        out << ind << "             world_z = RAY_GET_FLOOR_HEIGHT(world_x, world_y);\n";
      out << ind << "         end\n";
      out << ind << "         if (current_anim_start != 0 and collision_detected == 0)\n";
      out << ind << "             current_anim_start = 0; current_anim_end = 14; current_anim_speed = 10;\n";
      out << ind << "             anim_current_frame = 0; anim_next_frame = 1; anim_interpolation = 0.0;\n";
      out << ind << "         end\n";
      out << ind << "     else\n";
      out << ind << "         // ESTADO: PATRULLAR / IDLE\n";
      out << ind << "         if (" << (entity.npcPathId >= 0 ? "1" : "0") << " == 1)\n";
      out << ind << "             npc_path_active = 1;\n";
      out << ind << "         else\n";
      out << ind << "             if (current_anim_start != 0)\n";
      out << ind << "                 current_anim_start = 0; current_anim_end = 14; current_anim_speed = 3;\n";
      out << ind << "                 anim_current_frame = 0; anim_next_frame = 1; anim_interpolation = 0.0;\n";
      out << ind << "             end\n";
      out << ind << "         end\n";
      out << ind << "     end\n";
      out << ind << " elseif (g_player_health <= 0.0)\n";
      out << ind << "     // JUGADOR MUERTO: EXECUTAR RAMA UNA SOLA VEZ O IDLE POR DEFECTO\n";
      out << ind << "     if (player_death_triggered == 0)\n";
      out << ind << "         player_death_triggered = 1;\n";
      out << ind << "         npc_path_active = 0;\n";
      out << ind << "         // Reset a animación de reposo\n";
      out << ind << "         current_anim_start = 0; current_anim_end = 14; current_anim_speed = 3;\n";
      out << ind << "         anim_current_frame = 0; anim_next_frame = 1; anim_interpolation = 0.0;\n";
      if (current->pins.size() > 10) {
        const NodePinData &pdPin = current->pins[10];
        if (!pdPin.linkedPinIds.isEmpty()) {
            for (int lid : pdPin.linkedPinIds) {
                const NodePinData *next = pinMap.value(lid, nullptr);
                if (next) generateFlow(pinToNodeMap.value(next->pinId, nullptr), indent + 9, visited);
            }
        }
      }
      out << ind << "     end\n";
      out << ind << " end\n";
    } else if (current->type == "ai_damage_director") {
      QString painStart = res.resolve(current->pins[2].pinId);
      QString painEnd = res.resolve(current->pins[3].pinId);
      QString deathStart = res.resolve(current->pins[4].pinId);
      QString deathEnd = res.resolve(current->pins[5].pinId);
      QString speed = res.resolve(current->pins[6].pinId);

      out << ind << " // --- AI DAMAGE/DEATH DIRECTOR ---\n";
      out << ind << " if (health <= 0.0)\n";
      out << ind << "     // ESTADO: MUERTE\n";
      out << ind << "     if (current_anim_start != " << deathStart << " or current_anim_end != " << deathEnd << ")\n";
      out << ind << "         current_anim_start = " << deathStart << "; current_anim_end = " << deathEnd << "; current_anim_speed = " << speed << ";\n";
      out << ind << "         anim_current_frame = " << deathStart << "; anim_next_frame = " << deathStart << " + 1; anim_interpolation = 0.0;\n";
      out << ind << "     end\n";
      out << ind << " else\n";
      out << ind << "     // ESTADO: DAÑO (Pain)\n";
      out << ind << "     if (current_anim_start != " << painStart << " or current_anim_end != " << painEnd << ")\n";
      out << ind << "         current_anim_start = " << painStart << "; current_anim_end = " << painEnd << "; current_anim_speed = " << speed << ";\n";
      out << ind << "         anim_current_frame = " << painStart << "; anim_next_frame = " << painStart << " + 1; anim_interpolation = 0.0;\n";
      out << ind << "     end\n";
      out << ind << " end\n";
    } else if (current->type == "action_npc_flee") {
      QString speed = res.resolve(current->pins[3].pinId);
      out << ind
          << " // Flee: move away from player using 3D world coordinates\n";
      out << ind << " npc_path_active = 0;\n";
      out << ind << " if (d_dist > 5.0)\n";
      out << ind
          << "     world_angle = -fget_angle(0, 0, (world_x - "
             "g_player_x)*1000.0, (world_y - g_player_y)*1000.0) / 57295.78;\n";
      out << ind << "     world_x += ((world_x - g_player_x) / d_dist) * "
          << speed << " * 3;\n";
      out << ind << "     world_y += ((world_y - g_player_y) / d_dist) * "
          << speed << " * 3;\n";
      if (entity.snapToFloor)
        out << ind
            << "     world_z = RAY_GET_FLOOR_HEIGHT(world_x, world_y);\n";
      out << ind << " end\n";
    } else if (current->type == "action_sound") {
      QString file = res.resolve(current->pins[2].pinId);
      file.remove("\"");
      QString loops = res.resolve(current->pins[4].pinId);
      out << ind << " if (behavior_timer <= 0)\n";
      out << ind << "     s_id = SOUND_LOAD(\"" << file << "\");\n";
      out << ind << "     if (s_id > 0) SOUND_PLAY(s_id, " << loops
          << "); end\n";
      out << ind << "     behavior_timer = 30;\n";
      out << ind << " end\n";
    } else if (current->type == "action_set_animation") {
      QString gStart = res.resolve(current->pins[2].pinId);
      QString gEnd = res.resolve(current->pins[3].pinId);
      QString speed = res.resolve(current->pins[4].pinId);
      out << ind << " if (current_anim_start != " << gStart
          << " or current_anim_end != " << gEnd << ")\n";
      out << ind << "     current_anim_start = " << gStart
          << "; current_anim_end = " << gEnd
          << "; current_anim_speed = " << speed << ";\n";
      out << ind
          << "     anim_current_frame = current_anim_start; "
             "anim_next_frame "
             "= "
             "current_anim_start + 1;\n";
      out << ind
          << "     if (anim_next_frame > current_anim_end) anim_next_frame "
             "= "
             "current_anim_start; end\n";
      out << ind << "     anim_interpolation = 0.0;\n";
      out << ind << " end\n";

      // Following the flow to successor nodes (like action_damage)
      const NodePinData *outPin = nullptr;
      for (const auto &p : current->pins) {
        if (!p.isInput && p.isExecution && p.name == "Out") {
          outPin = &p;
          break;
        }
      }
      if (outPin && !outPin->linkedPinIds.isEmpty()) {
        const NodePinData *next =
            pinMap.value(outPin->linkedPinIds.first(), nullptr);
        generateFlow(next ? pinToNodeMap.value(next->pinId, nullptr) : nullptr,
                     indent, visited);
      }
    } else if (current->type == "action_wait") {
      QString secs = res.resolve(current->pins[2].pinId);
      // behavior_timer is decremented each frame at the top of the entity loop.
      // We wrap successors so they only execute when the timer is 0.
      out << ind << " if (behavior_timer <= 0)\n";
      out << ind << "     behavior_timer = (" << secs << ") * 60;\n";

      const NodePinData *outPin = nullptr;
      for (const auto &p : current->pins) {
        if (!p.isInput && p.isExecution) {
          outPin = &p;
          break;
        }
      }
      if (outPin && !outPin->linkedPinIds.isEmpty()) {
        const NodePinData *next =
            pinMap.value(outPin->linkedPinIds.first(), nullptr);
        generateFlow(next ? pinToNodeMap.value(next->pinId, nullptr) : nullptr,
                     indent + 4, visited);
      }
      out << ind << " end\n";
    } else if (current->type == "action_music") {
      QString file = res.resolve(current->pins[2].pinId);
      file.remove("\"");
      QString vol = res.resolve(current->pins[3].pinId);
      out << ind << " s_id = music_load(\"" << file << "\");\n";
      out << ind << " if (s_id > 0) music_play(s_id, -1); music_set_volume("
          << vol << "); end\n";
    } else if (current->type == "action_scene") {
      QString sceneName = res.resolve(current->pins[1].pinId);
      // Remove quotes if present from resolver to avoid double quotes
      sceneName.remove("\"");
      out << ind << " goto_scene(\"" << sceneName << "\");\n";
    } else if (current->type == "action_set_alpha") {
      QString alphaVal = res.resolve(current->pins[2].pinId);
      out << ind << " flags = " << alphaVal << "; // Set transparency\n";
      out << ind << " if (sprite_id >= 0) RAY_SET_SPRITE_FLAGS(sprite_id, "
          << alphaVal << "); end\n";
    } else if (current->type == "action_set_scale") {
      QString scaleVal = res.resolve(current->pins[2].pinId);
      out << ind << " size = " << scaleVal << "; // Set UI size\n";
      out << ind << " if (sprite_id >= 0) RAY_SET_SPRITE_SCALE(sprite_id, ("
          << scaleVal << ") / 100.0 * " << entity.scale << "); end\n";
    } else if (current->type == "action_set_path_active") {
      out << ind << " npc_path_active = " << res.resolve(current->pins[2].pinId)
          << ";\n";
    } else if (current->type == "action_set_resolution") {
      QString w = res.resolve(current->pins[2].pinId);
      QString h = res.resolve(current->pins[3].pinId);
      out << ind << " screen_w = " << w << "; screen_h = " << h
          << "; set_mode(screen_w, screen_h, 32);\n";
    } else if (current->type == "action_set_fullscreen") {
      QString val = res.resolve(current->pins[2].pinId);
      out << ind << " full_screen = " << val
          << "; set_mode(screen_w, screen_h, 32);\n";
    } else if (current->type == "action_set_music_volume") {
      QString vol = res.resolve(current->pins[2].pinId);
      out << ind << " music_set_volume(" << vol << ");\n";
    } else if (current->type == "action_set_sound_volume") {
      QString vol = res.resolve(current->pins[2].pinId);
      out << ind << " sound_set_volume(" << vol << ");\n";
    } else if (current->type == "action_set_ui_text") {
      QString target = res.resolve(current->pins[2].pinId);
      QString newText = res.resolve(current->pins[3].pinId);
      // For now, we assume "Self" (it's the most common for UI toggles)
      out << ind << " write_delete(txt_id);\n";
      out << ind << " txt_id = write(font_id, my_x, my_y, my_align, " << newText
          << ");\n";
      out << ind << " w = text_width(font_id, " << newText << ");\n";
      out << ind << " h = text_height(font_id, " << newText << ");\n";
    }

    if (current->type == "logic_if") {
      out << ind << " if (" << res.resolve(current->pins[3].pinId) << ")\n";
      const NodePinData *tPin = &current->pins[1];
      if (!tPin->linkedPinIds.isEmpty()) {
        for (int lid : tPin->linkedPinIds) {
            const NodePinData *next = pinMap.value(lid, nullptr);
            if (next) generateFlow(pinToNodeMap.value(next->pinId, nullptr), indent + 4, visited);
        }
      }
      const NodePinData *fPin = &current->pins[2];
      if (!fPin->linkedPinIds.isEmpty()) {
        out << ind << " else\n";
        for (int lid : fPin->linkedPinIds) {
            const NodePinData *next = pinMap.value(lid, nullptr);
            if (next) generateFlow(pinToNodeMap.value(next->pinId, nullptr), indent + 4, visited);
        }
      }
      out << ind << " end\n";
    } else {
      const NodePinData *outPin = nullptr;
      for (const auto &p : current->pins) {
        if (!p.isInput && p.isExecution) {
          outPin = &p;
          break;
        }
      }
      if (outPin && !outPin->linkedPinIds.isEmpty()) {
        for (int lid : outPin->linkedPinIds) {
            const NodePinData *next = pinMap.value(lid, nullptr);
            if (next) {
                generateFlow(pinToNodeMap.value(next->pinId, nullptr), indent, visited);
            }
        }
      }
    }
  };

  QSet<int> visited;
  const NodeData *start = nullptr;
  for (const auto &node : graph.nodes) {
    if (node.type == eventType) {
      start = &node;
      break;
    }
  }
  if (start)
    generateFlow(start, 8, visited);

  return code;
}
