#include "processgenerator.h"
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QSet>
#include <QTextStream>

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
    out << "    int model_id = 0;\n";
    out << "    int texture_id = 0;\n";
    out << "    int sprite_id;\n";
    out << "    float world_x, world_y, world_z;\n";
    out << "    float rotation = 0.0;\n";
    out << "    float scale = 1.0;\n";
    out << "begin\n";
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
    out << "    float pos_x, pos_y, pos_z;\n";
    out << "begin\n";
    out << "    pos_x = RAY_GET_FLAG_X(spawn_id);\n";
    out << "    pos_y = RAY_GET_FLAG_Y(spawn_id);\n";
    out << "    pos_z = RAY_GET_FLAG_Z(spawn_id);\n";
    out << "    \n";
    out << "    // Preload path\n";
    out << "    path_id = ray_camera_load(path_file);\n";
    out << "    if (path_id < 0) say(\"Error loading path: \" + path_file); "
           "return; end\n";
    out << "    \n";
    out << "    loop\n";
    out << "        p_id = get_id(type player);\n";
    out << "        if (p_id)\n";
    out << "            dist = abs(p_id.x - pos_x) + abs(p_id.y - pos_y);\n";
    out << "            if (dist < 64)\n";
    out << "                // Trigger Cutscene\n";
    out << "                ray_camera_play(path_id);\n";
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
    out << "    // Spawn entities\n";
    for (const EntityInstance &entity : entities) {
      out << "    " << entity.processName << "(" << entity.spawn_id
          << ");  // Entity at (" << entity.x << ", " << entity.y << ", "
          << entity.z << ")\n";
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

  // Get unique process names
  QStringList uniqueProcesses = getUniqueProcessNames(entities);

  for (const QString &processName : uniqueProcesses) {
    // Find first entity with this process name
    for (const EntityInstance &entity : entities) {
      if (entity.processName == processName) {
        QString procCode =
            generateProcessCodeWithBehavior(entity, wrapperOpen, wrapperClose);

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
    uniqueNames.insert(entity.processName);
  }

  return uniqueNames.values();
}

QString ProcessGenerator::generateDeclarationsSection(
    const QVector<EntityInstance> &entities) {
  QString declarations;
  QTextStream out(&declarations);

  QStringList uniqueProcesses = getUniqueProcessNames(entities);
  for (const QString &processName : uniqueProcesses) {
    out << "process " << processName << "(int spawn_id);\n";
  }

  return declarations;
}

QString
ProcessGenerator::generateProcessCodeWithBehavior(const EntityInstance &entity,
                                                  const QString &wrapperOpen,
                                                  const QString &wrapperClose) {
  QString code;
  QTextStream out(&code);

  out << "// ========================================\n";
  out << "// Entity: " << entity.processName << "\n";
  out << "// Type: " << entity.type << "\n";
  out << "// Asset: " << entity.assetPath << "\n";
  out << "// ========================================\n\n";

  out << "process " << entity.processName << "(int spawn_id)\n";
  out << "PRIVATE\n";

  // Common variables
  out << "    float world_x, world_y, world_z;\n";

  if (entity.type == "model") {
    out << "    int model_id = 0;\n";
    out << "    int texture_id = 0;\n";
    out << "    int sprite_id = -1;\n";
    out << "    float rotation = 0.0;\n";
    out << "    float scale = 1.0;\n";
  } else if (entity.type == "campath") {
    out << "    int campath_id = 0;\n";
  }

  // Behavior-specific variables
  if (entity.activationType == EntityInstance::ACTIVATION_ON_COLLISION) {
    out << "    int collision_target = " << entity.collisionTarget << ";\n";
    out << "    int collision_detected = 0;\n";
  } else if (entity.activationType == EntityInstance::ACTIVATION_ON_EVENT) {
    out << "    int event_triggered = 0;\n";
  }

  // Player-specific variables
  if (entity.isPlayer) {
    out << "    float move_speed = 8.0;\n";
    out << "    float rot_speed = 0.05;\n";
    out << "    float pitch_speed = 0.05;\n";
    out << "    float player_angle = " << (entity.angle * 3.14159 / 180.0)
        << ";\n";
    out << "    float player_pitch = 0.0;\n";
    out << "    float turn_offset = 0.0;\n";
    out << "    float cx, cy, cz, s, c, angle_deg;\n";
    out << "    float dx = 0.0; float dy = 0.0;\n";
  }

  out << "begin\n";

  // Get spawn position
  out << "    // Get spawn position from flag\n";
  out << "    world_x = RAY_GET_FLAG_X(spawn_id);\n";
  out << "    world_y = RAY_GET_FLAG_Y(spawn_id);\n";
  out << "    world_z = RAY_GET_FLAG_Z(spawn_id);\n";
  out << "    \n";

  // Load assets based on type
  if (entity.type == "model") {
    QString cleanPath = entity.assetPath;
    QFileInfo fi(entity.assetPath);
    if (fi.isAbsolute()) {
      cleanPath = "assets/models/" + fi.fileName();
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
    out << "        RAY_CLEAR_FLAG();\n";
    out << "        return;\n";
    out << "    end\n";
    out << "    \n";
    out << "    // Create sprite\n";
    out << "    sprite_id = RAY_ADD_SPRITE(world_x, world_y, world_z, 0, 0, "
           "0);\n";
    out << "    if (sprite_id < 0)\n";
    out << "        say(\"[" << entity.processName
        << "] ERROR: Failed to create sprite\");\n";
    out << "        RAY_CLEAR_FLAG();\n";
    out << "        return;\n";
    out << "    end\n";
    out << "    \n";
    out << "    RAY_SET_SPRITE_MD2(sprite_id, model_id, texture_id);\n";
    out << "    RAY_SET_SPRITE_SCALE(sprite_id, scale);\n";
    out << "    RAY_SET_SPRITE_ANGLE(sprite_id, " << entity.angle << ");\n";
    out << "    \n";

    // Visibility
    if (!entity.isVisible) {
      out << "    // Entity is invisible\n";
      out << "    RAY_SET_SPRITE_FLAGS(sprite_id, SPRITE_INVISIBLE);\n";
      out << "    \n";
    }
  } else if (entity.type == "campath") {
    QFileInfo fi(entity.assetPath);
    QString cleanPath = "assets/paths/" + fi.fileName();

    out << "    // Load Camera Path\n";
    out << "    campath_id = RAY_LOAD_CAMPATH(" << wrapperOpen << "\""
        << cleanPath << "\"" << wrapperClose << ");\n";
    if (wrapperOpen != "") {
      // If we have wrappers, maybe we need extra check or something,
      // but for now simple check is fine.
    }
    out << "    if (campath_id < 0)\n";
    out << "        say(\"[" << entity.processName
        << "] ERROR: Failed to load campath\");\n";
    out << "        RAY_CLEAR_FLAG();\n";
    out << "        return;\n";
    out << "    end\n";
    out << "    \n";
    out << "    // Start playing automatically\n";
    out << "    RAY_PLAY_CAMPATH(campath_id);\n";
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
      out << "        // Movimiento relativo al angulo de la camara\n";
      out << "        float cam_angle_off = "
          << (entity.cameraRotation * 0.0174f) << ";\n";
      out << "        float move_angle = player_angle + cam_angle_off;\n";

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
        out << "        // [DEBUG] CONTROLS V4 - COLLISIONS\n";
        out << "        float angle_milis = player_angle * 57295.8;\n";
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
        // WASD + Arrow controls for Car (Tank controls) WITH COLLISION
        // Motion is always relative to CAR chassis (player_angle), NOT camera
        out << "        float angle_milis_car = player_angle * 57295.8;\n";
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

        out << "        // Apply movement with collision (Sliding) using "
               "object Z\n";
        out << "        if (RAY_CHECK_COLLISION_Z(world_x, world_y, world_z, "
               "world_x + dx, world_y) == 0) world_x += dx; end\n";
        out << "        if (RAY_CHECK_COLLISION_Z(world_x, world_y, world_z, "
               "world_x, world_y + dy) == 0) world_y += dy; end\n";
        break;
      default:
        break;
      }

      out << "        RAY_UPDATE_SPRITE_POSITION(sprite_id, world_x, world_y, "
             "world_z);\n";
      out << "        RAY_SET_SPRITE_ANGLE(sprite_id, (player_angle * 180.0 / "
             "3.14159) + turn_offset);\n";

      if (entity.cameraFollow) {
        if (entity.controlType == EntityInstance::CONTROL_THIRD_PERSON ||
            entity.controlType == EntityInstance::CONTROL_CAR) {
          float ox =
              (entity.cameraOffset_x == 0) ? -200.0f : entity.cameraOffset_x;
          float oy = entity.cameraOffset_y;
          float oz =
              (entity.cameraOffset_z == 0) ? 120.0f : entity.cameraOffset_z;

          out << "        // Chase Camera - Follows vehicle rotation\n";
          out << "        float angle_deg = (player_angle + cam_angle_off) * "
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
    out << "        frame;\n";
    out << "    end\n";
    break;

  case EntityInstance::ACTIVATION_ON_TRIGGER:
    out << "    // Activate on trigger (area detection)\n";
    out << "    loop\n";
    out << "        // TODO: Implement area trigger detection\n";
    out << "        // Check if player is within range\n";
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
