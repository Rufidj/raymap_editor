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
    out << "    double d_dist;\n";
    out << "    int behavior_timer;\n";
    out << "    int _npc_ang;\n"; // Used by action_npc_chase / action_npc_flee
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
    out << "    npc_path_active = 1;\n";
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

  // Global player position for NPC AI (avoids buggy inter-process _id.x access)
  out << "// Global player position (shared with NPCs for reliable distance "
         "calculation)\n";
  out << "global\n";
  out << "  double g_player_x;\n";
  out << "  double g_player_y;\n";
  out << "  double g_player_z;\n";
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

  out << "process " << entity.processName
      << "(int param_x, int param_y, int param_z, int param_angle)\n";
  out << "private\n";

  // Common variables - double for radians/subpixel precision
  out << "    double world_x; double world_y; double world_z; double "
         "world_angle;\n";

  if (entity.type == "model" || entity.type == "gltf") {
    out << "    int model_id;\n";
    out << "    int texture_id;\n";
    out << "    int sprite_id;\n";
    out << "    double rotation;\n";
    out << "    double scale;\n";
    out << "    double anim_interpolation;\n";
    out << "    int anim_current_frame;\n";
    out << "    int anim_next_frame;\n";
    out << "    int current_anim_start;\n";
    out << "    int current_anim_end;\n";
    out << "    double current_anim_speed;\n";
  } else if (entity.type == "campath") {
    out << "    int campath_id;\n";
  }

  // Behavior-specific variables (always present for sound/engine actions)
  out << "    int car_engine_id;\n";
  if (entity.activationType == EntityInstance::ACTIVATION_ON_EVENT) {
    out << "    int event_triggered;\n";
  }

  // Player or Car behavior-specific variables
  if (entity.isPlayer || entity.controlType == EntityInstance::CONTROL_CAR) {
    out << "    double move_speed;\n";
    out << "    double rot_speed;\n";
    out << "    double player_angle;\n";
    out << "    double turn_offset;\n";
    out << "    double angle_milis_car;\n";
    out << "    double speed;\n";
  }

  if (entity.isPlayer) {
    out << "    double pitch_speed;\n";
    out << "    double player_pitch;\n";
    out << "    double cx, cy, cz, s, c, angle_deg;\n";
    out << "    double cam_angle_off;\n";
    out << "    double move_angle;\n";
    out << "    double cam_safe_x, cam_safe_y, cam_dx, cam_dy;\n";
    out << "    double cam_test_x, cam_test_y;\n";
    out << "    int cam_steps, cam_i;\n";
    out << "    double angle_milis;\n";
  }

  if (entity.npcPathId >= 0) {
    out << "    // NPC Path following variables\n";
    out << "    int npc_current_waypoint;\n";
    out << "    int npc_wait_counter;\n";
    out << "    int npc_direction; // For ping-pong mode\n";
    out << "    int npc_path_active;\n";
  }

  // Utility variables for asset loading and behavior
  out << "    string texture_path_base;\n";
  out << "    string alt_path;\n";
  out << "    int s_id;\n";
  out << "    int cur_speed;\n";
  out << "    int speed_vol;\n";
  out << "    int collision_target;\n";
  out << "    int collision_detected;\n";
  out << "    double dx, dy, dz, d_dist;\n";
  out << "    double angle_to_target;\n";
  out << "    int behavior_timer;\n";
  out << "    int _npc_ang;\n"; // Used by action_npc_chase / action_npc_flee

  out << "begin\n";
  out << "    car_engine_id = 0;\n";
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

  if (entity.activationType == EntityInstance::ACTIVATION_ON_COLLISION) {
    QString target = entity.collisionTarget;
    if (target == "TYPE_PLAYER" && !playerTypeName.isEmpty()) {
      target = "type " + playerTypeName;
    } else if (target.toLower() == "player" && !playerTypeName.isEmpty()) {
      target = "type " + playerTypeName;
    }
    out << "    collision_target = " << target << ";\n";
    out << "    collision_detected = 0;\n";
  } else if (entity.activationType == EntityInstance::ACTIVATION_ON_EVENT) {
    out << "    event_triggered = 0;\n";
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
    out << "    texture_id = map_load(texture_path_base + \".png\");\n";
    out << "    if (texture_id <= 0) texture_id = map_load(texture_path_base + "
           "\".jpg\"); end\n";

    out << "    if (texture_id <= 0)\n";
    out << "       // Try same directory as model\n";
    out << "       alt_path = \"assets/md3/\" + \""
        << QFileInfo(entity.assetPath).baseName() << "\";\n";
    out << "       texture_id = map_load(alt_path + \".png\");\n";
    out << "       if (texture_id <= 0) texture_id = map_load(alt_path + "
           "\".jpg\"); end\n";
    out << "    end\n";

    out << "    if (model_id == 0) say(\"[" << entity.processName
        << "] ERROR: Failed to load model: " << cleanPath << "\"); end\n";
    out << "    if (texture_id == 0) say(\"[" << entity.processName
        << "] WARNING: Failed to load texture: \" + texture_path_base); end\n";
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

  // Behavior implementation
  out << "    // ===== BEHAVIOR =====\n";

  QString actionCode = entity.customAction;

  // Use Behavior Graph if it has nodes
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

  switch (entity.activationType) {
  case EntityInstance::ACTIVATION_ON_START:
    out << "    // Activate on start\n";
    if (!actionCode.isEmpty()) {
      QString customCode = actionCode;
      out << "    " << customCode.replace("\n", "\n    ") << "\n";
    }
    out << "    loop\n";

    if (entity.isPlayer) {
      out << "        move_angle = player_angle + cam_angle_off;\n";

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
        out << "        world_x = RAY_GET_CAMERA_X(); world_y = "
               "RAY_GET_CAMERA_Y(); world_z = RAY_GET_CAMERA_Z();\n";
        out << "        player_angle = RAY_GET_CAMERA_ROT(); player_pitch = "
               "RAY_GET_CAMERA_PITCH();\n";
        break;

      case EntityInstance::CONTROL_THIRD_PERSON:
        out << "        angle_milis = player_angle * 57295.8;\n";
        out << "        dx = 0; dy = 0;\n";

        out << "        if (key(_left)) player_angle += rot_speed; end\n";
        out << "        if (key(_right)) player_angle -= rot_speed; end\n";

        out << "        if (key(_w)) dx += cos(angle_milis) * move_speed; dy "
               "+= sin(angle_milis) * move_speed; end\n";
        out << "        if (key(_s)) dx -= cos(angle_milis) * move_speed; dy "
               "-= sin(angle_milis) * move_speed; end\n";
        out << "        if (key(_a)) dx += cos(angle_milis + 90000) * "
               "move_speed; dy += sin(angle_milis + 90000) * move_speed; end\n";
        out << "        if (key(_d)) dx += cos(angle_milis - 90000) * "
               "move_speed; dy += sin(angle_milis - 90000) * move_speed; end\n";

        out << "        // Apply movement with collision (Sliding)\n";
        out << "        if (RAY_CHECK_COLLISION_Z(world_x, world_y, world_z, "
               "world_x + dx, world_y) == 0) world_x += dx; end\n";
        out << "        if (RAY_CHECK_COLLISION_Z(world_x, world_y, world_z, "
               "world_x, world_y + dy) == 0) world_y += dy; end\n";

        out << "        if (key(_up)) player_pitch += pitch_speed; if "
               "(player_pitch > 1.2) player_pitch = 1.2; end end\n";
        out << "        if (key(_down)) player_pitch -= pitch_speed; if "
               "(player_pitch < -1.2) player_pitch = -1.2; end end\n";
        break;

      case EntityInstance::CONTROL_CAR:
        out << "        angle_milis_car = player_angle * 57295.8;\n";
        out << "        dx = 0; dy = 0;\n";
        out << "        turn_offset *= 0.8;\n";

        out << "        if (key(_left) || key(_a)) player_angle -= rot_speed; "
               "turn_offset -= 5.0; end\n";
        out << "        if (key(_right) || key(_d)) player_angle += rot_speed; "
               "turn_offset += 5.0; end\n";

        if (entity.physicsEnabled) {
          // Physics-based movement (Force = gradual acceleration)
          out << "        if (key(_w) || key(_up))\n";
          out << "            RAY_PHYSICS_APPLY_FORCE(sprite_id, "
                 "cos(angle_milis_car) * move_speed * "
              << (entity.physicsMass * 200.0f)
              << ", "
                 "sin(angle_milis_car) * move_speed * "
              << (entity.physicsMass * 200.0f) << ", 0);\n";
          out << "        end\n";
          out << "        if (key(_s) || key(_down))\n";
          out << "            RAY_PHYSICS_APPLY_FORCE(sprite_id, "
                 "-cos(angle_milis_car) * move_speed * "
              << (entity.physicsMass * 120.0f)
              << ", "
                 "-sin(angle_milis_car) * move_speed * "
              << (entity.physicsMass * 120.0f) << ", 0);\n";
          out << "        end\n";
          // Sync world position from physics engine
          out << "        world_x = RAY_GET_SPRITE_X(sprite_id); world_y = "
                 "RAY_GET_SPRITE_Y(sprite_id); world_z = "
                 "RAY_GET_SPRITE_Z(sprite_id);\n";
          // Broadcast player position to global vars for NPC AI
          out << "        g_player_x = world_x; g_player_y = world_y; "
                 "g_player_z = world_z;\n";
        } else {
          // Manual Tank-Drive Move (Coordinate-based)
          out << "        if (key(_w) || key(_up))\n";
          out << "            dx += cos(player_angle * 57295.8) * "
                 "move_speed;\n";
          out << "            dy += sin(player_angle * 57295.8) * "
                 "move_speed;\n";
          out << "        end\n";
          out << "        if (key(_s) || key(_down))\n";
          out << "            dx -= cos(player_angle * 57295.8) * "
                 "move_speed;\n";
          out << "            dy -= sin(player_angle * 57295.8) * "
                 "move_speed;\n";
          out << "        end\n";

          // Cars should NOT step up walls - use very low step height
          out << "        RAY_SET_STEP_HEIGHT(5.0);\n";
          // Apply collision against sectors AND sprites
          out << "        if (RAY_CHECK_COLLISION_Z(world_x, world_y, world_z "
                 "+ 5.0, world_x + dx, world_y) == 0 and "
                 "RAY_CHECK_SPRITE_COLLISION(sprite_id, world_x + dx, world_y, "
              << (entity.width / 2.0f) << ") < 0) world_x += dx; end\n";
          out << "        if (RAY_CHECK_COLLISION_Z(world_x, world_y, world_z "
                 "+ 5.0, world_x, world_y + dy) == 0 and "
                 "RAY_CHECK_SPRITE_COLLISION(sprite_id, world_x, world_y + dy, "
              << (entity.width / 2.0f) << ") < 0) world_y += dy; end\n";
          out << "        RAY_SET_STEP_HEIGHT(32.0);\n";
        }
        break;

      default:
        // Default sync for non-player entities
        if (!entity.isPlayer &&
            entity.controlType == EntityInstance::CONTROL_CAR) {
          out << "        player_angle = world_angle;\n";
        }
        break;
      }

      if (entity.cameraFollow) {
        if (entity.controlType == EntityInstance::CONTROL_THIRD_PERSON ||
            entity.controlType == EntityInstance::CONTROL_CAR) {
          float ox =
              (entity.cameraOffset_x == 0) ? -400.0f : entity.cameraOffset_x;
          float oy = entity.cameraOffset_y;
          float oz =
              (entity.cameraOffset_z == 0) ? 150.0f : entity.cameraOffset_z;

          out << "        // Chase Camera - Follows vehicle rotation\n";
          out << "        angle_deg = (player_angle + cam_angle_off) * "
                 "180000.0 / 3.14159;\n";
          out << "        s = sin(angle_deg); c = cos(angle_deg);\n";
          out << "        cx = world_x + c*(" << ox << ") - s*(" << oy
              << ");\n";
          out << "        cy = world_y + s*(" << ox << ") + c*(" << oy
              << ");\n";
          out << "        \n";
          // Camera collision avoidance
          out << "        // Camera collision avoidance\n";
          out << "        cam_safe_x = world_x; cam_safe_y = world_y;\n";
          out << "        cam_dx = cx - world_x; cam_dy = cy - world_y;\n";
          out << "        cam_steps = 10;\n";
          out << "        from cam_i = 1 to cam_steps;\n";
          out << "            cam_test_x = world_x + (cam_dx * cam_i / "
                 "cam_steps);\n";
          out << "            cam_test_y = world_y + (cam_dy * cam_i / "
                 "cam_steps);\n";
          out << "            // Use high step_height (100) to ignore "
                 "curbs/low walls\n";
          out << "            if (RAY_CHECK_COLLISION_EXT(world_x, world_y, "
                 "world_z + "
              << oz << ", cam_test_x, cam_test_y, 100.0))\n";
          out << "                break;\n";
          out << "            end\n";
          out << "            cam_safe_x = cam_test_x;\n";
          out << "            cam_safe_y = cam_test_y;\n";
          out << "        end\n";

          // Safety: Don't let the camera get TOO close to the car
          out << "        cx = cam_safe_x; cy = cam_safe_y;\n";
          out << "        if (abs(cx - world_x) < 50 and abs(cy - world_y) < "
                 "50)\n";
          out << "            cx = world_x; cy = world_y;\n";
          out << "        end\n";

          out << "        RAY_SET_CAMERA(cx, cy, world_z + (" << oz
              << "), player_angle + cam_angle_off, player_pitch);\n";
        }
        // For First Person, RAY_SET_CAMERA is handled internally by the engine
      }
    }

    if (entity.isPlayer || entity.controlType == EntityInstance::CONTROL_CAR) {
      out << "        world_angle = player_angle + (turn_offset * 0.005);\n";
    }

    if (entity.type == "campath") {
      out << "        if (RAY_CAMERA_IS_PLAYING())\n";
      out << "            RAY_CAMERA_PATH_UPDATE(0.0166);\n";
      out << "            world_x = RAY_GET_CAMERA_X();\n";
      out << "            world_y = RAY_GET_CAMERA_Y();\n";
      out << "            world_z = RAY_GET_CAMERA_Z();\n";
      out << "        end\n";
    }

    if (entity.snapToFloor && (entity.npcPathId < 0 || !entity.autoStartPath)) {
      out << "        // Snap to floor for non-path entities\n";
      out << "        world_z = RAY_GET_FLOOR_HEIGHT(world_x, world_y);\n";
    }

    if (entity.npcPathId >= 0) {
      out << "        // Automatic NPC Path Following\n";
      out << "        if (npc_path_active == 1)\n";
      out << "            npc_follow_path(" << entity.npcPathId
          << ", &npc_current_waypoint, &npc_wait_counter, &npc_direction, "
             "&world_x, &world_y, &world_z, &world_angle, "
          << (entity.snapToFloor ? "1" : "0") << ");\n";
      out << "        else\n";
      out << "            // if (entity.isPlayer == 0) say(\"Entity \" + name "
             "+ \" path inactive\"); end\n";
      out << "        end\n";
    }

    out << "        if (behavior_timer > 0) behavior_timer = behavior_timer - "
           "1; end\n";

    // Pre-compute distance to player using global vars (reliable)
    if (!entity.isPlayer && !updateCode.isEmpty()) {
      out << "        // Distance to player (global position - avoids "
             "inter-process bug)\n";
      out << "        dx = world_x - g_player_x;\n";
      out << "        dy = world_y - g_player_y;\n";
      out << "        d_dist = sqrt(dx*dx + dy*dy);\n";
    }

    if (!updateCode.isEmpty()) {
      out << "        // Behavior Update (Each Frame)\n";
      out << "        " << updateCode.replace("\n", "\n        ") << "\n";
    }

    // Auto return to patrol when player is far away
    if (!entity.isPlayer && entity.npcPathId >= 0 && !updateCode.isEmpty()) {
      out << "        // Auto-disengage: return to patrol if player is far\n";
      out << "        if (npc_path_active == 0 && d_dist > 2000)\n";
      out << "            npc_path_active = 1;\n";
      out << "        end\n";
      out << "        // Restore walk animation when patrolling\n";
      out << "        if (npc_path_active == 1 && current_anim_start != 0)\n";
      out << "            current_anim_start = 0; current_anim_end = 14; "
             "current_anim_speed = 10;\n";
      out << "            anim_current_frame = 0; anim_next_frame = 1;\n";
      out << "            anim_interpolation = 0.0;\n";
      out << "        end\n";
    }

    if (entity.type == "model" || entity.type == "gltf") {
      out << "        x = world_x; y = world_y; z = world_z;\n";
      out << "        RAY_UPDATE_SPRITE_POSITION(sprite_id, world_x, world_y, "
             "world_z);\n";
      out << "        RAY_SET_SPRITE_ANGLE(sprite_id, world_angle * "
             "57.2957);\n";

      out << "        // Animation Logic\n";
      if (entity.type == "model") {
        out << "        if (current_anim_speed != 0)\n";
        out << "            anim_interpolation = anim_interpolation + "
               "(abs(current_anim_speed) / 60.0);\n";
        out << "            if (anim_interpolation >= 1.0)\n";
        out << "                anim_interpolation = 0.0;\n";
        out << "                anim_current_frame = anim_next_frame;\n";
        out << "                anim_next_frame = anim_current_frame + 1;\n";
        out << "                if (anim_next_frame > current_anim_end) "
               "anim_next_frame = current_anim_start; end\n";
        out << "            end\n";
        out << "        end\n";
        out << "        RAY_SET_SPRITE_ANIM(sprite_id, anim_current_frame, "
               "anim_next_frame, anim_interpolation);\n";
      } else {
        out << "        RAY_SET_SPRITE_GLB_ANIM(sprite_id, current_anim_start, "
               "anim_interpolation);\n";
      }
    }

    out << "        // USER HOOK: Update\n";
    out << "        hook_" << hookBaseName << "_update(id);\n";
    out << "        frame;\n";
    out << "    end\n";
    break;

  case EntityInstance::ACTIVATION_ON_COLLISION:
    out << "    // Activate on collision\n";
    if (!collisionCode.isEmpty()) {
      actionCode = collisionCode;
    }
    out << "    loop\n";
    out << "        if (collision(collision_target) and collision_detected == "
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

  // Helper functions for NPC behavior (Safe from "Process 0 not active" errors)
  out << "// Safe property access helpers\n";
  out << "function float get_val_x(int _id) begin if (exists(_id)) return "
         "_id.x; end return 0.0; end\n";
  out << "function float get_val_y(int _id) begin if (exists(_id)) return "
         "_id.y; end return 0.0; end\n";
  out << "function float get_val_z(int _id) begin if (exists(_id)) return "
         "_id.z; end return 0.0; end\n\n";

  out << "// Safe 3D Distance calculation (returns distance in editor units)\n";
  out << "function float get_dist_3d(int id1, int id2)\n";
  out << "begin\n";
  out << "    if (not exists(id1) or not exists(id2)) return 999999.0; end\n";
  out << "    return sqrt(pow(get_val_x(id1)-get_val_x(id2),2) + "
         "pow(get_val_y(id1)-get_val_y(id2),2) + "
         "pow((get_val_z(id1)-get_val_z(id2))/16.0,2)) / 100.0;\n";
  out << "end\n\n";

  // Helper function to follow a path - ALWAYS present to avoid undefined
  // procedure errors
  out << "// NPC Path Following Helper\n";
  out << "function npc_follow_path(int path_id, int pointer current_wp, int "
         "pointer wait_counter, int pointer direction, double pointer cur_x, "
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
  out << "      case 0: if (*current_wp < waypoint_count - 1) *current_wp = "
         "*current_wp + 1; end; end\n";
  out << "      case 1: *current_wp = (*current_wp + 1) % waypoint_count; "
         "end\n";
  out << "      case 2: *current_wp = *current_wp + *direction;\n";
  out << "              if (*current_wp >= waypoint_count - 1) *direction = "
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
  out << "        target_x = atan2(-dy, dx); // Negate Y to match engine "
         "coordinate system\n";
  out << "    else\n";
  out << "        target_x = *cur_angle;\n";
  out << "    end\n";
  out << "    \n";
  out << "    // Correct angular difference for smooth Lerp (handles 0-2PI "
         "wrap)\n";
  out << "    target_y = target_x - *cur_angle; \n";
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
        const NodePinData *linkedPin =
            pm.value(pin->linkedPinIds.first(), nullptr);
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
                // (divide by 4 so user values like 60/500 map to real 240/2000)
                return "(d_dist / 4.0)";
              }

              if (target == "TYPE_PLAYER" && !playerProcessName.isEmpty())
                target = "get_id(type " + playerProcessName + ")";
              return QString("get_dist_3d(id, %1)").arg(target);
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
    if (!current || visited.contains(current->nodeId))
      return;
    visited.insert(current->nodeId);
    QString ind = QString(indent, ' ');

    if (current->type == "action_say") {
      out << ind << "say(" << res.resolve(current->pins[2].pinId) << ");\n";
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
    } else if (current->type == "action_npc_chase") {
      QString target = res.resolve(current->pins[2].pinId);
      QString speed = res.resolve(current->pins[3].pinId);
      out << ind << " if (exists(" << target << "))\n";
      out << ind << "     _npc_ang = fget_angle(x, y, " << target << ".x, "
          << target << ".y);\n";
      out << ind << "     x += get_distx(_npc_ang, " << speed << ");\n";
      out << ind << "     y += get_disty(_npc_ang, " << speed << ");\n";
      out << ind << " end\n";
    } else if (current->type == "action_npc_flee") {
      QString target = res.resolve(current->pins[2].pinId);
      QString speed = res.resolve(current->pins[3].pinId);
      out << ind << " if (exists(" << target << "))\n";
      out << ind << "     _npc_ang = fget_angle(x, y, " << target << ".x, "
          << target << ".y) + 180000;\n";
      out << ind << "     x += get_distx(_npc_ang, " << speed << ");\n";
      out << ind << "     y += get_disty(_npc_ang, " << speed << ");\n";
      out << ind << " end\n";
    } else if (current->type == "action_sound") {
      QString file = res.resolve(current->pins[2].pinId);
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
      // Stop path and face player during animation (e.g. attack)
      out << ind << " npc_path_active = 0;\n";
      out << ind
          << " world_angle = atan2(g_player_y - world_y, "
             "g_player_x - world_x) / 57295.78;\n";
      out << ind << " if (current_anim_start != " << gStart
          << " or current_anim_end != " << gEnd << ")\n";
      out << ind << "     current_anim_start = " << gStart
          << "; current_anim_end = " << gEnd
          << "; current_anim_speed = " << speed << ";\n";
      out << ind
          << "     anim_current_frame = current_anim_start; anim_next_frame = "
             "current_anim_start + 1;\n";
      out << ind
          << "     if (anim_next_frame > current_anim_end) anim_next_frame = "
             "current_anim_start; end\n";
      out << ind << "     anim_interpolation = 0.0;\n";
      out << ind << " end\n";
    } else if (current->type == "action_npc_chase") {
      QString chaseSpeed = res.resolve(current->pins[3].pinId);
      out << ind << " // Chase: use global player position\n";
      out << ind << " npc_path_active = 0;\n";
      out << ind << " if (d_dist > 5.0)\n";
      out << ind
          << "     world_angle = atan2(g_player_y - world_y, "
             "g_player_x - world_x) / 57295.78;\n";
      out << ind << "     world_x += ((g_player_x - world_x) / d_dist) * "
          << chaseSpeed << " * 3;\n";
      out << ind << "     world_y += ((g_player_y - world_y) / d_dist) * "
          << chaseSpeed << " * 3;\n";
      if (entity.snapToFloor)
        out << ind
            << "     world_z = RAY_GET_FLOOR_HEIGHT(world_x, world_y);\n";
      out << ind << " end\n";
      // Set walk animation during chase
      out << ind << " if (current_anim_start != 0)\n";
      out << ind
          << "     current_anim_start = 0; current_anim_end = 14; "
             "current_anim_speed = 10;\n";
      out << ind << "     anim_current_frame = 0; anim_next_frame = 1;\n";
      out << ind << "     anim_interpolation = 0.0;\n";
      out << ind << " end\n";
    } else if (current->type == "action_wait") {
      QString secs = res.resolve(current->pins[2].pinId);
      out << ind << " behavior_timer = (" << secs << ") * 60;\n";
      out << ind << " while(behavior_timer > 0) behavior_timer--; frame; end\n";
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
        const NodePinData *next =
            pinMap.value(tPin->linkedPinIds.first(), nullptr);
        generateFlow(next ? pinToNodeMap.value(next->pinId, nullptr) : nullptr,
                     indent + 4, visited);
      }
      const NodePinData *fPin = &current->pins[2];
      if (!fPin->linkedPinIds.isEmpty()) {
        out << ind << " else\n";
        const NodePinData *next =
            pinMap.value(fPin->linkedPinIds.first(), nullptr);
        generateFlow(next ? pinToNodeMap.value(next->pinId, nullptr) : nullptr,
                     indent + 4, visited);
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
        const NodePinData *next =
            pinMap.value(outPin->linkedPinIds.first(), nullptr);
        generateFlow(next ? pinToNodeMap.value(next->pinId, nullptr) : nullptr,
                     indent, visited);
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
