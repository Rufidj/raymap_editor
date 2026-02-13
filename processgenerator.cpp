#include "processgenerator.h"
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QSet>
#include <QString>
#include <QStringList>
#include <QTextStream>
#include <QVector>

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
    // includes Caching is disabled for file stability

    out << "process " << processName << "(int spawn_id)\n";
    out << "PRIVATE\n";
    out << "    int model_id;\n";
    out << "    int texture_id;\n";
    out << "    int sprite_id;\n";
    out << "    double world_x, world_y, world_z;\n";
    out << "    double rotation;\n";
    out << "    double scale;\n";
    out << "begin\n";
    out << "    model_id = 0;\n";
    out << "    texture_id = 0;\n";
    out << "    rotation = 0.0;\n";
    out << "    scale = 1.0;\n";
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
           "0);\n";
    out << "    if (sprite_id < 0)\n";
    out << "        say(\"[" << processName
        << "] ERROR: Failed to create sprite\");\n";
    out << "        RAY_CLEAR_FLAG();\n";
    out << "        return;\n";
    out << "    end\n";
    out << "    \n";
    out << "    RAY_SET_SPRITE_MD2(sprite_id, model_id, texture_id);\n";
    out << "    RAY_SET_SPRITE_SCALE(sprite_id, scale);\n";
    out << "    RAY_SET_SPRITE_ANGLE(sprite_id, rotation);\n";
    out << "    \n";
    // out << "    say(\"[" << processName << "] Spawned successfully!
    // sprite_id=\" + sprite_id);\n";
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
  } else if (type == "campath" ||
             assetPath.endsWith(".campath", Qt::CaseInsensitive)) {
    // Camera Path Process wrapper using Native Engine Functions

    QFileInfo fi(assetPath);
    QString cleanPath = "assets/paths/" + fi.fileName();

    out << "process " << processName << "(int spawn_id)\n";
    out << "PRIVATE\n";
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
        << "(double world_x, double world_y, double world_z)\n";
    out << "PRIVATE\n";
    out << "    int sprite_id;\n";
    out << "    int texture_id = 1;  // TODO: Get from FPG\n";
    out << "begin\n";
    out << "    // Create sprite\n";
    out << "    sprite_id = RAY_ADD_SPRITE(world_x, world_y, world_z, "
           "texture_id, 64, 64);\n";
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
      out << "    // Entity " << entity.processName << " at flag "
          << entity.spawn_id << "\n";
      out << "    " << entity.processName << "(";
      out << "RAY_GET_FLAG_X(" << entity.spawn_id << "), ";
      out << "RAY_GET_FLAG_Y(" << entity.spawn_id << "), ";
      out << "RAY_GET_FLAG_Z(" << entity.spawn_id << "), ";
      out << entity.cameraRotation << ");\n";
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
      playerProcessName = entity.processName;
      out << "// DEBUG INFO: Found Player Entity -> Process Name: '"
          << playerProcessName << "' (Type: " << entity.type << ")\n";
      break;
    }
  }

  // Get unique process names
  QStringList uniqueProcesses = getUniqueProcessNames(entities);

  for (const QString &processName : uniqueProcesses) {
    // Find first entity with this process name (case insensitive match)
    for (const EntityInstance &entity : entities) {
      if (entity.processName.toLower() == processName) {
        QString procCode = generateProcessCodeWithBehavior(
            entity, wrapperOpen, wrapperClose, playerProcessName);

        out << procCode << "\n";
        break;
      }
    }
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

  QStringList uniqueProcesses = getUniqueProcessNames(entities);
  for (const QString &processName : uniqueProcesses) {
    out << "DECLARE PROCESS " << processName
        << "(double param_x, double param_y, double param_z, double "
           "param_angle)\n";
    out << "END\n";
  }

  return declarations;
}

QString ProcessGenerator::generateProcessCodeWithBehavior(
    const EntityInstance &entity, const QString &wrapperOpen,
    const QString &wrapperClose, const QString &playerTypeName) {
  QString code;
  QTextStream out(&code);

  out << "// ========================================\n";
  out << "// Entity: " << entity.processName << "\n";
  out << "// Type: " << entity.type << "\n";
  out << "// Asset: " << entity.assetPath << "\n";
  out << "// ========================================\n\n";

  out << "process " << entity.processName
      << "(double param_x, double param_y, double param_z, double "
         "param_angle)\n";
  out << "PRIVATE\n";

  // Common variables
  out << "    double world_x; double world_y; double world_z; double "
         "world_angle;\n";

  if (entity.type == "model") {
    out << "    int model_id;\n";
    out << "    int texture_id;\n";
    out << "    int sprite_id;\n";
    out << "    double rotation;\n";
    out << "    double scale;\n";
  } else if (entity.type == "campath") {
    out << "    int campath_id;\n";
  }

  // Behavior-specific variables
  if (entity.activationType == EntityInstance::ACTIVATION_ON_COLLISION) {
    out << "    int collision_target;\n";
    out << "    int collision_detected;\n";
  } else if (entity.activationType == EntityInstance::ACTIVATION_ON_EVENT) {
    out << "    int event_triggered;\n";
  }

  // Player-specific variables
  // Player or Car behavior-specific variables
  if (entity.isPlayer || entity.controlType == EntityInstance::CONTROL_CAR) {
    out << "    double move_speed;\n";
    out << "    double rot_speed;\n";
    out << "    double player_angle;\n";
    out << "    double turn_offset;\n";
    out << "    double dx, dy;\n";
    out << "    double angle_milis_car;\n";
  }

  if (entity.isPlayer) {
    out << "    double pitch_speed;\n";
    out << "    double player_pitch;\n";
    out << "    double cx, cy, cz, s, c, angle_deg;\n";
    out << "    double cam_angle_off;\n";
    out << "    double move_angle;\n";
    out << "    double angle_milis;\n";
  }

  // NPC Path variables (Declaration only)
  if (entity.npcPathId >= 0) {
    out << "    // NPC Path following variables\n";
    out << "    int npc_current_waypoint;\n";
    out << "    int npc_wait_counter;\n";
    out << "    int npc_direction; // For ping-pong mode\n";
  }

  out << "begin\n";
  if (entity.type == "model") {
    out << "    model_id = 0;\n";
    out << "    texture_id = 0;\n";
    out << "    sprite_id = -1;\n";
    out << "    rotation = 0.0;\n";
    out << "    scale = 1.0;\n";
  } else if (entity.type == "campath") {
    out << "    campath_id = 0;\n";
  }

  if (entity.activationType == EntityInstance::ACTIVATION_ON_COLLISION) {
    out << "    collision_target = " << entity.collisionTarget << ";\n";
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
  out << "    hook_" << entity.processName.toLower() << "_init(id);\n\n";

  out << "    say(\"Spawned Entity: " << entity.processName
      << " at \" + world_x + \",\" + world_y);\n";

  // NOTE: world_x, world_y, world_z are now parameters

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

    out << "    // Load Model and Texture\n";
    out << "    model_id = RAY_LOAD_MD3(" << wrapperOpen << "\"" << cleanPath
        << "\"" << wrapperClose << ");\n";
    out << "    texture_id = map_load(" << wrapperOpen << "\"" << texturePath
        << "\"" << wrapperClose << ");\n";
    out << "    \n";
    out << "    if (model_id == 0)\n";
    out << "        say(\"[" << entity.processName
        << "] ERROR: Failed to load model\");\n";
    out << "        // RAY_CLEAR_FLAG();\n";
    out << "        return;\n";
    out << "    end\n";
    out << "    \n";
    out << "    // Create sprite\n";
    out << "    sprite_id = RAY_ADD_SPRITE(world_x, world_y, world_z, 0, 0, "
           "0);\n";
    out << "    if (sprite_id < 0)\n";
    out << "        say(\"[" << entity.processName
        << "] ERROR: Failed to create sprite\");\n";
    out << "        // RAY_CLEAR_FLAG();\n";
    out << "        return;\n";
    out << "    end\n";
    out << "    \n";
    out << "    RAY_SET_SPRITE_MD2(sprite_id, model_id, texture_id);\n";
    out << "    RAY_SET_SPRITE_SCALE(sprite_id, scale);\n";
    out << "    RAY_SET_SPRITE_ANGLE(sprite_id, param_angle);\n";
    out << "    \n";

    // Visibility
    if (!entity.isVisible) {
      out << "    // Entity is invisible\n";
      out << "    RAY_SET_SPRITE_FLAGS(sprite_id, SPRITE_INVISIBLE);\n";
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
        // Updated to Tank-like forward motion (relative to player facing)
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

        out << "        // Apply movement with collision (Sliding) using "
               "object Z\n";
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

        // Inverted controls based on user feedback (A = Clockwise?, D =
        // Counter-Clockwise?)
        out << "        if (key(_left) || key(_a)) player_angle -= rot_speed; "
               "turn_offset -= 15.0; end\n";
        out << "        if (key(_right) || key(_d)) player_angle += rot_speed; "
               "turn_offset += 15.0; end\n";

        out << "        if (key(_w) || key(_up)) dx += cos(angle_milis_car) * "
               "move_speed; dy += sin(angle_milis_car) * move_speed; end\n";
        out << "        if (key(_s) || key(_down)) dx -= cos(angle_milis_car) "
               "* move_speed; dy -= sin(angle_milis_car) * move_speed; end\n";

        // Apply movement with collision (Sliding) using object Z at waist level
        // (+20)
        out << "        if (RAY_CHECK_COLLISION_Z(world_x, world_y, world_z + "
               "20.0, "
               "world_x + dx, world_y) == 0) world_x += dx; end\n";
        out << "        if (RAY_CHECK_COLLISION_Z(world_x, world_y, world_z + "
               "20.0, "
               "world_x, world_y + dy) == 0) world_y += dy; end\n";
        break;
      default:
        // Default sync for non-player cars
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
              (entity.cameraOffset_x == 0) ? -200.0f : entity.cameraOffset_x;
          float oy = entity.cameraOffset_y;
          float oz =
              (entity.cameraOffset_z == 0) ? 120.0f : entity.cameraOffset_z;

          out << "        // Chase Camera - Follows vehicle rotation\n";
          out << "        angle_deg = (player_angle + cam_angle_off) * "
                 "180000.0 / 3.14159;\n";
          out << "        s = sin(angle_deg); c = cos(angle_deg);\n";
          out << "        cx = world_x + c*(" << ox << ") - s*(" << oy
              << ");\n";
          out << "        cy = world_y + s*(" << ox << ") + c*(" << oy
              << ");\n";
          out << "        \n";
          out << "        RAY_SET_CAMERA(cx, cy, world_z + (" << oz
              << "), player_angle + cam_angle_off, player_pitch);\n";
        }
        // NOTA: Para First Person no llamamos a RAY_SET_CAMERA porque ya lo
        // gestiona el motor internamente
      }
    }

    if (entity.isPlayer || entity.controlType == EntityInstance::CONTROL_CAR) {
      out << "        world_angle = player_angle + (turn_offset * 0.017453);\n";
    }

    if (entity.type == "campath") {
      out << "        if (RAY_CAMERA_IS_PLAYING())\n";
      out << "            begin\n";
      out << "                RAY_CAMERA_PATH_UPDATE(0.0166);\n";
      out << "                world_x = RAY_GET_CAMERA_X();\n";
      out << "                world_y = RAY_GET_CAMERA_Y();\n";
      out << "                world_z = RAY_GET_CAMERA_Z();\n";
      out << "            end\n";
      out << "        end\n";
    }

    if (entity.npcPathId >= 0 && entity.autoStartPath) {
      out << "        // Automatic NPC Path Following\n";
      out << "        npc_follow_path(" << entity.npcPathId
          << ", &npc_current_waypoint, &npc_wait_counter, &npc_direction, "
             "&world_x, &world_y, &world_z, &world_angle);\n";
    }
    if (entity.snapToFloor) {
      out << "        world_z = RAY_GET_FLOOR_HEIGHT(world_x, world_y) + "
             "5.0;\n";
    }
    out << "        x = world_x; y = world_y; z = world_z;\n";
    if (entity.type == "model") {
      out << "        RAY_UPDATE_SPRITE_POSITION(sprite_id, world_x, world_y, "
             "world_z);\n";
      out << "        RAY_SET_SPRITE_ANGLE(sprite_id, world_angle * "
             "57.2957);\n";
    }

    out << "        // USER HOOK: Update\n";
    out << "        hook_" << entity.processName.toLower() << "_update(id);\n";
    out << "        frame;\n";
    out << "    end\n";
    break;

  case EntityInstance::ACTIVATION_ON_COLLISION:
    out << "    // Activate on collision\n";
    out << "    loop\n";
    out << "        if (collision(collision_target) && !collision_detected)\n";
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
    out << "        hook_" << entity.processName.toLower() << "_update(id);\n";
    out << "        frame;\n";
    out << "    end\n";
    break;

  case EntityInstance::ACTIVATION_ON_TRIGGER:
    out << "    // Activate on trigger (area detection)\n";
    out << "    loop\n";
    out << "        // TODO: Implement area trigger detection\n";
    out << "        // Check if player is within range\n";
    out << "        // USER HOOK: Update\n";
    out << "        hook_" << entity.processName.toLower() << "_update(id);\n";
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

  // Initialize path data helper
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

  // Helper function to follow a path - ALWAYS present to avoid undefined
  // procedure errors
  out << "// NPC Path Following Helper\n";
  out << "function npc_follow_path(int path_id, int pointer current_wp, int "
         "pointer wait_counter, int pointer direction, double pointer cur_x, "
         "double pointer cur_y, double pointer cur_z, double pointer "
         "cur_angle)\n";
  out << "private\n";
  out << "  int waypoint_count;\n";
  out << "  int loop_mode;\n";
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
      out << "      if (*current_wp >= 0 and *current_wp < waypoint_count)\n";
      out << "        target_x = npc_path_" << path.path_id
          << "_waypoints[*current_wp][0] / 1000.0;\n";
      out << "        target_y = npc_path_" << path.path_id
          << "_waypoints[*current_wp][1] / 1000.0;\n";
      out << "        target_z = npc_path_" << path.path_id
          << "_waypoints[*current_wp][2] / 1000.0;\n";
      out << "        speed = npc_path_" << path.path_id
          << "_waypoints[*current_wp][3] / 1000.0;\n";
      out << "        wait_time = npc_path_" << path.path_id
          << "_waypoints[*current_wp][4];\n";
      out << "        look_angle = npc_path_" << path.path_id
          << "_waypoints[*current_wp][5] / 1000.0;\n";
      out << "      end\n";
      out << "    end\n";
    }
    out << "    default:\n";
    out << "      return;\n";
    out << "    end\n";
    out << "  end\n\n";
  } else {
    out << "  return;\n";
  }

  out << "  if (*wait_counter > 0) \n";
  out << "      *wait_counter = *wait_counter - 1;\n";
  out << "      return;\n";
  out << "  end\n\n";

  out << "  dx = target_x - *cur_x;\n";
  out << "  dy = target_y - *cur_y;\n";
  out << "  // Use 2D distance for following and arrival\n";
  out << "  d_dist = sqrt(dx*dx + dy*dy);\n\n";

  out << "  if (d_dist < speed)\n";
  out << "    *cur_x = target_x; \n";
  out << "    *cur_y = target_y;\n";
  out << "    *wait_counter = wait_time;\n";
  out << "    switch (loop_mode)\n";
  out << "      case 0: if (*current_wp < waypoint_count - 1) *current_wp = "
         "*current_wp + 1; end; end\n";
  out << "      case 1: *current_wp = (*current_wp + 1) % waypoint_count; "
         "end\n";
  out << "      case 2: *current_wp = *current_wp + *direction;\n";
  out << "              if (*current_wp >= waypoint_count - 1) *direction = "
         "-1;\n";
  out << "              elseif (*current_wp <= 0) *direction = 1; end; end\n";
  out << "      case 3: *current_wp = rand(0, waypoint_count - 1); end\n";
  out << "    end\n";
  out << "  elseif (d_dist > 0.0)\n";
  out << "    *cur_x = *cur_x + (dx * speed / d_dist);\n";
  out << "    *cur_y = *cur_y + (dy * speed / d_dist);\n";
  out << "    // Move Z independently if needed\n";
  out << "    dz = target_z - *cur_z;\n";
  out << "    *cur_z = *cur_z + (dz * speed / (d_dist + 1.0));\n\n";
  out << "    if (look_angle >= 0.0)\n";
  out << "        *cur_angle = look_angle * 0.01745329;\n";
  out << "    else\n";
  out << "        *cur_angle = atan2(dy, dx) * 0.00001745329;\n";
  out << "    end\n";
  out << "  end\n";
  out << "end\n";

  return code;
}
