#include "codegenerator.h"
#include "processgenerator.h"
#include "sceneeditor.h"
#include <QDebug>
#include <QDir>
#include <QDirIterator>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSet>
#include <QTextStream>

CodeGenerator::CodeGenerator() {}

void CodeGenerator::setProjectData(const ProjectData &data) {
  m_projectData = data;

  // Set all variables from project data
  setVariable("PROJECT_NAME", m_projectData.name);
  setVariable("PROJECT_VERSION", m_projectData.version);
  setVariable("SCREEN_WIDTH", QString::number(m_projectData.screenWidth));
  setVariable("SCREEN_HEIGHT", QString::number(m_projectData.screenHeight));
  setVariable("RENDER_WIDTH", QString::number(m_projectData.renderWidth));
  setVariable("RENDER_HEIGHT", QString::number(m_projectData.renderHeight));
  setVariable("FPS", QString::number(m_projectData.fps));

  QString startScene = m_projectData.startupScene;
  if (startScene.isEmpty())
    startScene = "scene1"; // Default fallback
  setVariable("STARTUP_SCENE", startScene);

  setVariable("FULLSCREEN_MODE",
              m_projectData.fullscreen ? "MODE_FULLSCREEN" : "MODE_WINDOW");
  setVariable("DATE",
              QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss"));
  setVariable("PACKAGE_NAME", m_projectData.packageName.isEmpty()
                                  ? "com.example.game"
                                  : m_projectData.packageName);

  // Android Helper Code Logic
  if (m_projectData.androidSupport) {
    QString helperCode =
        "// Helper para rutas Android\n"
        "// Se usa ruta absoluta hardcodeada basada en el nombre del paquete\n"
        "function string get_asset_path(string relative_path)\n"
        "begin\n"
        "    if (os_id == OS_ANDROID)\n"
        "        return \"/data/data/\" + \"{{PACKAGE_NAME}}\" + \"/files/\" + "
        "relative_path;\n"
        "    end\n"
        "    return relative_path;\n"
        "end";

    setVariable("ANDROID_HELPER_CODE", helperCode);
    setVariable("ASSET_WRAPPER_OPEN", "get_asset_path(");
    setVariable("ASSET_WRAPPER_CLOSE", ")");
  } else {
    setVariable("ANDROID_HELPER_CODE", "");
    setVariable("ASSET_WRAPPER_OPEN", "");
    setVariable("ASSET_WRAPPER_CLOSE", "");
  }
}

void CodeGenerator::setVariable(const QString &name, const QString &value) {
  m_variables[name] = value;
}

QString CodeGenerator::processTemplate(const QString &templateText) {
  QString result = templateText;

  // Replace all variables {{VAR_NAME}} with their values
  for (auto it = m_variables.begin(); it != m_variables.end(); ++it) {
    QString placeholder = "{{" + it.key() + "}}";
    result.replace(placeholder, it.value());
  }

  return result;
}

QString CodeGenerator::generateMainPrg() {
  if (m_projectData.name.isEmpty()) {
    qWarning() << "No project data set for code generation";
    return "";
  }

  QString template_text = getMainTemplate();

  // Clear legacy variables to prevent redefinitions in main.prg
  setVariable("ENTITY_DECLARATIONS", "");
  setVariable("ENTITY_PROCESSES", "");
  setVariable("SPAWN_ENTITIES", "");

  return processTemplate(template_text);
}

QString CodeGenerator::generateEntityProcess(const QString &entityName,
                                             const QString &entityType) {
  if (entityType == "player") {
    return processTemplate(getPlayerTemplate());
  } else if (entityType == "enemy") {
    return processTemplate(getEnemyTemplate());
  }

  // Generic entity
  QString code = QString("process %1(x, y, z)\n"
                         "PRIVATE\n"
                         "    int health = 100;\n"
                         "begin\n"
                         "    loop\n"
                         "        // TODO: Add entity logic\n"
                         "        frame;\n"
                         "    end\n"
                         "end\n")
                     .arg(entityName);

  return code;
}

QString CodeGenerator::getMainTemplate() {
  return QString("// Auto-generado por RayMap Editor\n"
                 "// Proyecto: {{PROJECT_NAME}}\n"
                 "// Fecha: {{DATE}}\n"
                 "\n"
                 "import \"libmod_gfx\";\n"
                 "import \"libmod_input\";\n"
                 "import \"libmod_misc\";\n"
                 "import \"libmod_ray\";\n"
                 "import \"libmod_sound\";\n"
                 "include \"includes/scene_commons.prg\";\n"
                 "include \"includes/resources.prg\";\n"
                 "include \"includes/scenes_list.prg\";\n"
                 "\n"
                 "\n"
                 "// Constantes de entidad\n"
                 "// [[ED_CONSTANTS_START]]\n"
                 "CONST\n"
                 "    TYPE_PLAYER = 1;\n"
                 "    TYPE_ENEMY  = 2;\n"
                 "    TYPE_OBJECT = 3;\n"
                 "    TYPE_TRIGGER = 4;\n"
                 "end\n"
                 "// [[ED_CONSTANTS_END]]\n"
                 "\n"
                 "// [[ED_DECLARATIONS_START]]\n"
                 "// [[ED_DECLARATIONS_END]]\n"
                 "\n"
                 "GLOBAL\n"
                 "    int screen_w = {{SCREEN_WIDTH}};\n"
                 "    int screen_h = {{SCREEN_HEIGHT}};\n"
                 "end\n"
                 "\n"
                 "process main()\n"
                 "begin\n"
                 "    say(\"--- PROGRAM START ---\");\n"
                 "    say(\"CWD: \" + cd());\n"
                 "    // Inicializar pantalla\n"
                 "    set_mode(screen_w, screen_h, {{FULLSCREEN_MODE}});\n"
                 "    set_fps({{FPS}}, 0);\n"
                 "    window_set_title(\"{{PROJECT_NAME}}\");\n"
                 "\n"
                 "    // Inicializar sistema de audio (Estilo Joselkiller)\n"
                 "    sound.freq = 44100;\n"
                 "    sound.channels = 32;\n"
                 "    int audio_status = soundsys_init();\n"
                 "    reserve_channels(24);\n"
                 "    set_master_volume(128);\n"
                 "    music_set_volume(128);\n"
                 "    say(\"AUDIO: Init status \" + audio_status + \" (Driver: "
                 "\" + getenv(\"SDL_AUDIODRIVER\") + \")\");\n"
                 "\n"
                 "    // Load all project resources\n"
                 "    load_project_resources();\n"
                 "\n"
                 "    // Start Initial Scene\n"
                 "    // [[ED_STARTUP_SCENE_START]]\n"
                 "    {{STARTUP_SCENE}}();\n"
                 "    // [[ED_STARTUP_SCENE_END]]\n"
                 "\n"
                 "    // Main Loop (just in case scene returns)\n"
                 "    loop\n"
                 "        if (key(_esc)) exit(\"\", 0); end\n"
                 "        frame;\n"
                 "    end\n"
                 "end\n");
}

QString CodeGenerator::getPlayerTemplate() {
  return QString("process player(x, y, z)\n"
                 "PRIVATE\n"
                 "    int health = 100;\n"
                 "    float speed = 5.0;\n"
                 "begin\n"
                 "    loop\n"
                 "        // Player logic here\n"
                 "        frame;\n"
                 "    end\n"
                 "end\n");
}

QString CodeGenerator::getEnemyTemplate() {
  return QString("process enemy(x, y, z)\n"
                 "PRIVATE\n"
                 "    int health = 50;\n"
                 "    float speed = 3.0;\n"
                 "begin\n"
                 "    loop\n"
                 "        // Enemy AI here\n"
                 "        frame;\n"
                 "    end\n"
                 "end\n");
}

QString CodeGenerator::generateMainPrgWithEntities(
    const QVector<EntityInstance> &entities) {
  if (m_projectData.name.isEmpty()) {
    qWarning() << "No project data set for code generation";
    return "";
  }

  // Generate entity declarations
  QString entityDeclarations =
      ProcessGenerator::generateDeclarationsSection(entities);
  setVariable("ENTITY_DECLARATIONS", entityDeclarations);

  // Generate entity processes (Inline instead of includes)
  QString wrapperOpen = m_variables.value("ASSET_WRAPPER_OPEN", "");
  QString wrapperClose = m_variables.value("ASSET_WRAPPER_CLOSE", "");
  QString entityProcesses = ProcessGenerator::generateAllProcessesCode(
      entities, wrapperOpen, wrapperClose);
  setVariable("ENTITY_PROCESSES", entityProcesses);
  setVariable("ENTITY_INCLUDES", ""); // Clear old variable for safety

  // Generate spawn calls
  QString spawnCalls = ProcessGenerator::generateSpawnCalls(entities);
  setVariable("SPAWN_ENTITIES", spawnCalls);

  // Determine movement logic
  bool hasPlayer = false;
  for (const auto &e : entities) {
    if (e.isPlayer) {
      hasPlayer = true;
      break;
    }
  }

  QString movement;
  if (hasPlayer) {
    movement = "        // Movimiento por entidad (Jugador detectado)\n";
    movement += "        RAY_CAMERA_UPDATE(0.017);";
  } else {
    movement = "        // Movimiento libre de cámara\n"
               "        if (key(_w)) RAY_MOVE_FORWARD(move_speed); end\n"
               "        if (key(_s)) RAY_MOVE_BACKWARD(move_speed); end\n"
               "        if (key(_a)) RAY_STRAFE_LEFT(move_speed); end\n"
               "        if (key(_d)) RAY_STRAFE_RIGHT(move_speed); end\n"
               "        if (key(_left)) RAY_ROTATE(-rot_speed); end\n"
               "        if (key(_right)) RAY_ROTATE(rot_speed); end\n"
               "        if (key(_up)) RAY_LOOK_UP_DOWN(pitch_speed); end\n"
               "        if (key(_down)) RAY_LOOK_UP_DOWN(-pitch_speed); end\n"
               "        RAY_CAMERA_UPDATE(0.017);";
  }
  setVariable("MOVEMENT_LOGIC", movement);

  QString template_text = getMainTemplate();
  return processTemplate(template_text);
}

QString CodeGenerator::generateEntityModel(const QString &processName,
                                           const QString &modelPath) {
  QString wrapperOpen = m_variables.value("ASSET_WRAPPER_OPEN", "");
  QString wrapperClose = m_variables.value("ASSET_WRAPPER_CLOSE", "");

  // modelPath assumed relative (e.g. assets/foo.md3)
  QString loadStr =
      QString("%1\"%2\"%3").arg(wrapperOpen).arg(modelPath).arg(wrapperClose);

  return QString("process %1(x, y, z, angle);\n"
                 "PRIVATE\n"
                 "    int file_id = 0;\n"
                 "end\n"
                 "begin\n"
                 "    file_id = load_md3(%2);\n"
                 "    if (file_id > 0)\n"
                 "        graph = file_id;\n"
                 "    end\n"
                 "    loop\n"
                 "        frame;\n"
                 "    end\n"
                 "end\n")
      .arg(processName)
      .arg(loadStr);
}

QString CodeGenerator::generateCameraController() {
  return getCameraControllerTemplate();
}

QString CodeGenerator::getCameraControllerTemplate() {
  return QString(
      "/* Camera Controller Module - Auto-generated */\n"
      "#ifndef CAMERA_CONTROLLER_H\n"
      "#define CAMERA_CONTROLLER_H\n"
      "\n"
      "import \"libmod_file\";\n"
      "import \"libmod_mem\";\n"
      "import \"libmod_math\";\n"
      "\n"
      "TYPE CameraKeyframe\n"
      "    float x, y, z;\n"
      "    float yaw, pitch, roll;\n"
      "    float fov;\n"
      "    float time;\n"
      "    float duration;\n"
      "    int easeIn, easeOut;\n"
      "end\n"
      "\n"
      "TYPE CameraPathData\n"
      "    int num_keyframes;\n"
      "    CameraKeyframe pointer keyframes;\n"
      "end\n"
      "\n"
      "/* Load binary camera path (.cam) */\n"
      "function int LoadCameraPath(string filename, CameraPathData pointer "
      "out_data)\n"
      "PRIVATE\n"
      "    int f;\n"
      "    int count;\n"
      "    int i;\n"
      "begin\n"
      "    f = fopen(filename, O_READ);\n"
      "    if (f == 0) return -1; end\n"
      "\n"
      "    fread(f, count);\n"
      "    out_data.num_keyframes = count;\n"
      "    if (count > 0)\n"
      "        out_data.keyframes = alloc(count * sizeof(CameraKeyframe));\n"
      "    end\n"
      "\n"
      "    for (i=0; i<count; i++)\n"
      "        fread(f, out_data.keyframes[i].x);\n"
      "        fread(f, out_data.keyframes[i].y);\n"
      "        fread(f, out_data.keyframes[i].z);\n"
      "        fread(f, out_data.keyframes[i].yaw);\n"
      "        fread(f, out_data.keyframes[i].pitch);\n"
      "        fread(f, out_data.keyframes[i].roll);\n"
      "        fread(f, out_data.keyframes[i].fov);\n"
      "        fread(f, out_data.keyframes[i].time);\n"
      "        fread(f, out_data.keyframes[i].duration);\n"
      "        fread(f, out_data.keyframes[i].easeIn);\n"
      "        fread(f, out_data.keyframes[i].easeOut);\n"
      "    end\n"
      "\n"
      "    fclose(f);\n"
      "    return 0;\n"
      "end\n"
      "\n"
      "function FreeCameraPath(CameraPathData pointer data)\n"
      "begin\n"
      "    if (data.keyframes != NULL) free(data.keyframes); end\n"
      "    data.num_keyframes = 0;\n"
      "end\n"
      "\n"
      "/* Trigger PROCESS */\n"
      "process CameraTrigger(x, y, z, string file);\n"
      "PRIVATE\n"
      "    int player_id;\n"
      "    int dist;\n"
      "begin\n"
      "    loop\n"
      "        player_id = get_id(type player);\n"
      "        if (player_id)\n"
      "            dist = abs(player_id.x - x) + abs(player_id.y - y);\n"
      "            if (dist < 64)\n"
      "                // Start Cutscene\n"
      "                PlayCameraPath(file);\n"
      "                // Only run once?\n"
      "                break;\n"
      "            end\n"
      "        end\n"
      "        frame;\n"
      "    end\n"
      "end\n"
      "\n"
      "// Placeholder for PlayCameraPath implementation\n"
      "process PlayCameraPath(string filename);\n"
      "PRIVATE\n"
      "    CameraPathData data;\n"
      "begin\n"
      "    if (LoadCameraPath(filename, &data) < 0) return; end\n"
      "    // TODO: Interpolation LOOP\n"
      "    say(\"Playing cutscene: \" + filename);\n"
      "    // ...\n"
      "    FreeCameraPath(&data);\n"
      "end\n"
      "\n"
      "#endif\n");
}

QString CodeGenerator::generateCameraPathData(const QString &pathName,
                                              const struct CameraPath &path) {
  // Implementation needed if generating .h files for paths
  return "";
}
QString CodeGenerator::patchMainPrg(const QString &existingCode,
                                    const QVector<EntityInstance> &entities) {
  QString newFullCode = generateMainPrgWithEntities(entities);

  // If the existing file doesn't have any markers, it's safer to return the
  // existing code to prevent overwriting manual changes.
  if (!existingCode.contains("// [[ED_CONSTANTS_START]]")) {
    return existingCode;
  }

  QString result = existingCode;
  auto replaceBlock = [&](const QString &startMarker, const QString &endMarker,
                          bool protectIfHasContent = false) {
    int newStart = newFullCode.indexOf(startMarker);
    int newEND = newFullCode.indexOf(endMarker);
    if (newStart == -1 || newEND == -1)
      return;
    newEND += endMarker.length();
    QString newBlock = newFullCode.mid(newStart, newEND - newStart);

    int oldStart = result.indexOf(startMarker);
    int oldEND = result.indexOf(endMarker);
    if (oldStart != -1 && oldEND != -1) {
      // Protect user's manual code
      if (protectIfHasContent) {
        int contentStart = oldStart + startMarker.length();
        QString existingContent =
            result.mid(contentStart, oldEND - contentStart).trimmed();
        // If there's ANY meaningful content, don't touch it
        if (existingContent.length() > 10) {
          return; // Skip - user has code here
        }
      }

      oldEND += endMarker.length();
      result.replace(oldStart, oldEND - oldStart, newBlock);
    }
  };

  replaceBlock("// [[ED_CONSTANTS_START]]", "// [[ED_CONSTANTS_END]]");
  replaceBlock("// [[ED_DECLARATIONS_START]]", "// [[ED_DECLARATIONS_END]]");
  replaceBlock("// [[ED_CONTROL_START]]", "// [[ED_CONTROL_END]]");
  replaceBlock("// [[ED_SPAWN_START]]", "// [[ED_SPAWN_END]]");
  replaceBlock("// [[ED_PROCESSES_START]]", "// [[ED_PROCESSES_END]]",
               true); // NEVER overwrite if has content
  replaceBlock("// [[ED_STARTUP_SCENE_START]]", "// [[ED_STARTUP_SCENE_END]]");

  // --- CLEANUP OLD INLINE CODE ---
  // Remove old get_asset_path if it exists as inline function (now in
  // scene_commons.prg)
  int funcPos = result.indexOf("function string get_asset_path");
  if (funcPos != -1) {
    // Find the second "end" (one for 'if', one for 'function')
    int firstEnd = result.indexOf("end", funcPos);
    if (firstEnd != -1) {
      int secondEnd = result.indexOf("end", firstEnd + 3);
      if (secondEnd != -1) {
        result.remove(funcPos, (secondEnd + 3) - funcPos);
      } else {
        // Fallback: just remove the line/block start if we can't find clear end
        int nextBegin = result.indexOf("begin", funcPos);
        if (nextBegin != -1)
          result.remove(funcPos, nextBegin - funcPos);
      }
    }
  }

  // Ensure resources are initialized in main() specifically
  if (!result.contains("load_project_resources")) {
    int mainPos = result.indexOf("process main()");
    if (mainPos != -1) {
      int beginPos = result.indexOf("begin", mainPos);
      if (beginPos != -1) {
        int nextLine = result.indexOf("\n", beginPos);
        if (nextLine != -1) {
          QString initCode = "    // Inicializar sistema de audio\n"
                             "    sound.freq = 44100;\n"
                             "    sound.channels = 32;\n"
                             "    int audio_status = soundsys_init();\n"
                             "    say(\"AUDIO: Status \" + audio_status + \" "
                             "(\" + getenv(\"SDL_AUDIODRIVER\") + \")\");\n"
                             "    reserve_channels(24);\n"
                             "    load_project_resources();\n";
          result.insert(nextLine + 1, initCode);
        }
      }
    }
  } else if (!result.contains("soundsys_init")) {
    // If it has resources but not sound init, insert sound init before
    // resources
    int resPos = result.indexOf("load_project_resources");
    if (resPos != -1) {
      result.insert(resPos,
                    "sound.freq = 44100;\n    sound.channels = 32;\n    "
                    "soundsys_init();\n    reserve_channels(24);\n    ");
    }
  }

  // Ensure sound module is present if music might be used
  if (!result.contains("import \"libmod_sound\"")) {
    int pos = result.lastIndexOf("import \"");
    if (pos != -1) {
      int endPos = result.indexOf(";", pos);
      if (endPos != -1)
        result.insert(endPos + 1, "\nimport \"libmod_sound\";");
    }
  }

  // Ensure include commons, resources and scenes_list are present
  if (!result.contains("includes/scene_commons.prg")) {
    int pos = result.indexOf("include \"includes/resources.prg\";");
    if (pos == -1)
      pos = result.indexOf("include \"includes/scenes_list.prg\";");

    if (pos != -1) {
      result.insert(pos, "include \"includes/scene_commons.prg\";\n");
    } else {
      int lastImport = result.lastIndexOf("import \"");
      if (lastImport != -1) {
        int endImport = result.indexOf(";", lastImport);
        if (endImport != -1)
          result.insert(endImport + 1,
                        "\ninclude \"includes/scene_commons.prg\";\n");
      }
    }
  }

  if (!result.contains("includes/resources.prg")) {
    int pos = result.indexOf("include \"includes/scenes_list.prg\";");
    if (pos != -1) {
      result.insert(pos, "include \"includes/resources.prg\";\n");
    } else {
      int lastInc =
          result.lastIndexOf("include \"includes/scene_commons.prg\";");
      if (lastInc != -1) {
        int endInc = result.indexOf(";", lastInc);
        if (endInc != -1)
          result.insert(endInc + 1, "\ninclude \"includes/resources.prg\";\n");
      }
    }
  }

  if (!result.contains("includes/scenes_list.prg")) {
    QString includeLine = "include \"includes/scenes_list.prg\";";
    int pos = result.indexOf("include \"includes/resources.prg\";");
    if (pos != -1) {
      int endInc = result.indexOf(";", pos);
      if (endInc != -1)
        result.insert(endInc + 1, "\n" + includeLine + "\n");
    } else {
      result.prepend(includeLine + "\n");
    }
  }

  return result;
}

// ----------------------------------------------------------------------------
// 2D SCENE GENERATION
// ----------------------------------------------------------------------------

static bool loadSceneJson(const QString &path, SceneData &data) {
  QFile file(path);
  if (!file.open(QIODevice::ReadOnly))
    return false;

  QByteArray jsonData = file.readAll();
  file.close();

  QJsonDocument doc = QJsonDocument::fromJson(jsonData);
  if (doc.isNull())
    return false;

  QJsonObject root = doc.object();

  data.width = root["width"].toInt(320);
  data.height = root["height"].toInt(240);
  data.inputMode = root["inputMode"].toInt(INPUT_MOUSE);
  data.exitOnEsc = root["exitOnEsc"].toBool(true);
  data.cursorFile = root["cursorFile"].toString();
  data.cursorGraph = root["cursorGraph"].toInt(0);
  data.musicFile = root["musicFile"].toString();
  data.musicLoop = root["musicLoop"].toBool(true);

  QJsonArray entitiesArray = root["entities"].toArray();
  for (const QJsonValue &val : entitiesArray) {
    QJsonObject obj = val.toObject();

    SceneEntity *ent = new SceneEntity();
    ent->type = (SceneEntityType)obj["type"].toInt(ENTITY_SPRITE);
    ent->name = obj["name"].toString();
    ent->x = obj["x"].toDouble();
    ent->y = obj["y"].toDouble();
    ent->z = obj["z"].toInt();
    ent->angle = obj["angle"].toDouble();
    ent->scale = obj["scale"].toDouble(1.0);
    ent->script = obj["script"].toString(); // Relative path usually
    ent->onClick = obj["onClick"].toString();

    if (ent->type == ENTITY_SPRITE) {
      ent->sourceFile = obj["sourceFile"].toString(); // Check if relative?
      ent->graphId = obj["graphId"].toInt();
    } else if (ent->type == ENTITY_TEXT) {
      ent->text = obj["text"].toString();
      ent->fontId = obj["fontId"].toInt();
      ent->fontFile = obj["fontFile"].toString();
      ent->alignment = obj["alignment"].toInt(0);
    }

    data.entities.append(ent);
  }
  return true;
}

QString CodeGenerator::generateScenePrg(const QString &sceneName,
                                        const SceneData &data,
                                        const QString &existingCode) {
  // 1. Extract existing user code to preserve it
  QString userSetup = "\n    // Add your setup code here\n    ";
  QString userLoop = "\n        // Add your LOOP code here\n        ";

  if (!existingCode.isEmpty()) {
    int s1 = existingCode.indexOf("// [[USER_SETUP]]");
    int e1 = existingCode.indexOf("// [[USER_SETUP_END]]");
    if (s1 != -1 && e1 != -1) {
      userSetup = existingCode.mid(s1 + 17, e1 - (s1 + 17));
    }

    int s2 = existingCode.indexOf("// [[USER_LOOP]]");
    int e2 = existingCode.indexOf("// [[USER_LOOP_END]]");
    if (s2 != -1 && e2 != -1) {
      userLoop = existingCode.mid(s2 + 16, e2 - (s2 + 16));
    }
  }

  // Android Wrappers
  QString assetOpen = m_projectData.androidSupport ? "get_asset_path(" : "";
  QString assetClose = m_projectData.androidSupport ? ")" : "";

  QString code;
  code += "// Scene: " + sceneName + "\n";
  code += "// Generated automatically by RayMap Editor. DO NOT EDIT sections "
          "outside User Blocks.\n\n";

  // code += "declare FUNCTION string get_asset_path(string path) end\n\n"; //
  // Removed

  // code += "include \"includes/scene_commons.prg\";\n\n"; // REMOVED: Included
  // globally in scenes_list.prg

  // Forward declarations for helper processes
  for (auto ent : data.entities) {
    if (!ent->onClick.isEmpty()) {
      QString safeName = ent->name.isEmpty()
                             ? (ent->type == ENTITY_TEXT ? "Btn" : "Ent")
                             : ent->name;
      safeName =
          safeName.replace(" ", "_").replace("[", "").replace("]", "").replace(
              ":", "");
      if (ent->type == ENTITY_TEXT) {
        QString uniqueId = QString::number(
            qHash(QString("%1_%2").arg(ent->x).arg(ent->y)) % 10000);
        code += "declare process Auto_Btn_" + safeName + "_" + uniqueId +
                "(int font_id);\n";
      } else {
        code += "declare process Auto_" + safeName + "();\n";
      }
    }
  }
  code += "\n";

  code += "process " + sceneName + "()\n";
  code += "PRIVATE\n";
  code += "    int ent_id;\n";

  // Resources Variables (Handled globally now)
  QSet<QString> resources;
  for (auto ent : data.entities) {
    if (ent->type == ENTITY_SPRITE && !ent->sourceFile.isEmpty())
      resources.insert(ent->sourceFile);
    if (ent->type == ENTITY_TEXT && !ent->fontFile.isEmpty())
      resources.insert(ent->fontFile);
  }
  if (!data.cursorFile.isEmpty())
    resources.insert(data.cursorFile);
  if (!data.musicFile.isEmpty())
    resources.insert(data.musicFile);

  // Resource IDs are now GLOBAL - we use the mapping to refer to them
  QMap<QString, QString> resMap;
  for (const QString &res : resources) {
    QString cleanName = QFileInfo(res).baseName().toLower();
    cleanName = cleanName.replace(".", "_").replace(" ", "_");
    QString ext = QFileInfo(res).suffix().toLower();
    resMap[res] = "id_" + cleanName + "_" + ext;
  }

  code += "begin\n";
  code += "    write_delete(all_text);\n";

  code += "    // Load Resources - MOVED TO GLOBAL resources.prg\n";
  // We already generate the resources.prg in a separate step or we can assume
  // it's there. In generateAllScenes we will make sure resources.prg is
  // generated.

  if (!data.musicFile.isEmpty() && resMap.contains(data.musicFile)) {
    code += "\n    // Music Loading & Playback (Inside scene as per wiki)\n";
    QString varName = resMap[data.musicFile];
    QString res = data.musicFile;
    int loops = data.musicLoop ? -1 : 0;

    code += "    if (" + varName + " <= 0)\n";
    code += "        if (fexists(get_asset_path(\"../" + res + "\"))) " +
            varName + " = music_load(get_asset_path(\"../" + res +
            "\")); end\n";
    code += "        if (" + varName + " <= 0) if (fexists(get_asset_path(\"" +
            res + "\"))) " + varName + " = music_load(get_asset_path(\"" + res +
            "\")); end end\n";
    code += "        if (" + varName + " <= 0) if (fexists(\"../" + res +
            "\")) " + varName + " = music_load(\"../" + res + "\"); end end\n";
    code += "        if (" + varName + " <= 0) if (fexists(\"" + res + "\")) " +
            varName + " = music_load(\"" + res + "\"); end end\n";
    code += "    end\n";
    code += "    if (" + varName + " > 0)\n";
    code += "        say(\"DEBUG: Playing music ID \" + " + varName + ");\n";
    code += "        music_set_volume(128);\n";
    code += "        music_play(" + varName + ", " + QString::number(loops) +
            ");\n";
    code += "    end\n";
  }

  if (data.inputMode == INPUT_MOUSE || data.inputMode == INPUT_HYBRID) {
    code += "\n    // Setup Input (Mouse)\n";
    if (!data.cursorFile.isEmpty()) {
      QString resVar = resMap[data.cursorFile];
      QString ext = QFileInfo(data.cursorFile).suffix().toLower();

      if (ext == "fpg") {
        code += QString("    mouse.file = %1;\n").arg(resVar);
        code += QString("    mouse.graph = %1;\n").arg(data.cursorGraph);
      } else {
        // PNG directly
        code += "    mouse.file = 0;\n";
        code += QString("    mouse.graph = %1;\n").arg(resVar);
      }
    } else {
      code += "    mouse.graph = 0; // System cursor\n";
    }
  }

  QStringList auxProcesses;

  code += "\n    // Instantiate Entities\n";
  for (auto ent : data.entities) {
    if (ent->type == ENTITY_SPRITE) {
      QString processName = "StaticSprite";
      if (!ent->script.isEmpty()) {
        processName = QFileInfo(ent->script).baseName();
      } else if (!ent->onClick.isEmpty()) {
        // Generate Auto Process
        QString safeName = ent->name.isEmpty() ? "Ent" : ent->name;
        safeName = safeName.replace(" ", "_");
        processName = "Auto_" + safeName;

        QString aux = "process " + processName + "()\nbegin\n";
        aux += "    loop\n";
        aux += "        if (mouse.left AND collision(type mouse))\n";
        aux += "             begin\n";
        aux += "                 " + ent->onClick + ";\n";
        aux += "                 while(mouse.left) frame; end\n";
        aux += "             end\n";
        aux += "        end\n";
        aux += "        frame;\n";
        aux += "    end\nend\n";
        auxProcesses.append(aux);
      }

      code += QString("    // Entity: %1\n").arg(ent->name);
      code += QString("    ent_id = %1();\n").arg(processName);

      QString resVar = resMap.value(ent->sourceFile, "0");

      QString ext = QFileInfo(ent->sourceFile).suffix().toLower();
      if (ext == "png" || ext == "jpg" || ext == "jpeg" || ext == "bmp" ||
          ext == "tga") {
        code += "    ent_id.file = 0;\n";
        code += QString("    ent_id.graph = %1;\n").arg(resVar);
      } else {
        code += QString("    ent_id.file = %1;\n").arg(resVar);
        code += QString("    ent_id.graph = %1;\n").arg(ent->graphId);
      }

      code += QString("    ent_id.x = %1; ent_id.y = %2; ent_id.z = %3;\n")
                  .arg(ent->x)
                  .arg(ent->y)
                  .arg(ent->z);
      code += QString("    ent_id.angle = %1; ent_id.size = %2;\n")
                  .arg(int(ent->angle * 1000))
                  .arg(int(ent->scale * 100));
      code += "\n";
    } else if (ent->type == ENTITY_TEXT) {
      QString fontId = "0";
      if (!ent->fontFile.isEmpty() && resMap.contains(ent->fontFile)) {
        fontId = resMap[ent->fontFile];
      }

      code += QString("    // Text Entity: %1\n").arg(ent->name);

      // Robust check for interactivity
      bool isInteractive = !ent->onClick.trimmed().isEmpty() &&
                           !ent->name.startsWith("txt_") &&
                           ent->onClick != "NONE";

      if (isInteractive) {
        // Interactive Text Button Process
        QString safeName = ent->name.isEmpty() ? "Btn" : ent->name;
        safeName = safeName.replace(" ", "_")
                       .replace("[", "")
                       .replace("]", "")
                       .replace(":", "");

        // Make unique by adding position hash to avoid collisions
        QString uniqueId = QString::number(
            qHash(QString("%1_%2").arg(ent->x).arg(ent->y)) % 10000);
        QString procName = "Auto_Btn_" + safeName + "_" + uniqueId;

        QString aux = "process " + procName + "(int font_id)\n";
        aux += "PRIVATE\n";
        aux += "    int txt_id;\n";
        aux += "    int w; \n";
        aux += "    int h;\n";
        aux += "begin\n";
        aux += QString("    x = %1; y = %2;\n").arg(ent->x).arg(ent->y);

        // Make text persistent for this process
        aux += QString("    txt_id = write(font_id, x, y, %1, \"%2\");\n")
                   .arg(ent->alignment)
                   .arg(ent->text);

        // Calc Hitbox logic
        // Note: text_width/height require font loaded.
        aux += QString("    if(font_id >= 0)\n");
        aux += QString("        w = text_width(font_id, \"%1\");\n")
                   .arg(ent->text);
        aux += QString("        h = text_height(font_id, \"%1\");\n")
                   .arg(ent->text);
        aux += QString("    end\n");

        int align = ent->alignment;
        QString xCheck;
        if (align == 1)
          xCheck = "abs(mouse.x - x) < (w/2)"; // Center
        else if (align == 2)
          xCheck = "(mouse.x > (x - w) AND mouse.x < x)"; // Right
        else
          xCheck = "(mouse.x > x AND mouse.x < (x + w))"; // Left (Default 0)

        aux += "    loop\n";

        // Hover effect? Optional

        // Click logic
        aux += "        if (" + xCheck +
               " AND abs(mouse.y - y) < (h/2) AND mouse.left)\n";
        aux += "             " + ent->onClick + "\n";         // Execute code
        aux += "             while(mouse.left) frame; end\n"; // Debounce
        aux += "        end\n";
        aux += "        frame;\n";
        aux += "    end\n";

        // Cleanup text when process dies
        aux += "OnExit:\n";
        aux += "    write_delete(txt_id);\n";
        aux += "end\n";

        // Add to aux list
        auxProcesses.append(aux);

        // Call it
        code += QString("    %1(%2);\n").arg(procName).arg(fontId);

      } else {
        // STATIC TEXT (Label)
        // Just write it in the scene main body
        code += QString("    write(%1, %2, %3, %4, \"%5\");\n")
                    .arg(fontId)
                    .arg(int(ent->x))
                    .arg(int(ent->y))
                    .arg(ent->alignment)
                    .arg(ent->text);
      }
    } else if (ent->type == ENTITY_WORLD3D) {
      code += QString("\n    // 3D World Hybrid Entity: %1\n").arg(ent->name);
      code += "    RAY_SHUTDOWN();\n";
      code += "    RAY_INIT(640, 480, 70, 1);\n";

      QString mapPath = ent->sourceFile;
      if (mapPath.contains("/assets/"))
        mapPath = mapPath.mid(mapPath.indexOf("/assets/") + 1);

      code += QString("    if (RAY_LOAD_MAP(\"%1\", 0) == 0) say(\"Error "
                      "loading hybrid 3D Map\"); end\n")
                  .arg(mapPath);
      code += "    ray_display();\n\n";
    }
  }

  // Inject User Setup
  code += "\n    // [[USER_SETUP]]" + userSetup + "// [[USER_SETUP_END]]\n";

  code += "    loop\n";
  if (data.exitOnEsc) {
    code += "        if (key(_esc)) exit(\"\", 0); end\n";
  }
  // Inject User Loop
  code += "        // [[USER_LOOP]]" + userLoop + "// [[USER_LOOP_END]]\n";

  code += "        frame;\n";
  code += "    end\n";
  code += "end\n";

  if (!auxProcesses.isEmpty()) {
    code += "\n// --- Auto-Generated Helper Processes ---\n\n";
    code += auxProcesses.join("\n\n");
  }

  return code;
}

void CodeGenerator::generateAllScenes(const QString &projectPath) {
  QString scenesOutDir = projectPath + "/src/scenes";
  QDir().mkpath(scenesOutDir);

  QString includesDir = projectPath + "/src/includes";
  QDir().mkpath(includesDir);

  // Helper
  QFile helper(includesDir + "/scene_commons.prg");
  if (helper.open(QIODevice::WriteOnly)) {
    QTextStream out(&helper);
    out << "// BennuGD 2 - RayMap Editor Commons\n\n";

    out << "// Helper para rutas Android\n";
    out << "// Se usa ruta absoluta hardcodeada basada en el nombre del "
           "paquete\n";
    out << "function string get_asset_path(string relative_path)\n";
    out << "begin\n";
    out << "    if (os_id == OS_ANDROID)\n";
    out << "        return \"/data/data/\" + \"{{PACKAGE_NAME}}\" + "
           "\"/files/\" + relative_path;\n";
    out << "    end\n";
    out << "    return relative_path;\n";
    out << "end\n\n";

    out << "process StaticSprite()\nbegin\n    loop\n        frame;\n    "
           "end\nend\n\n";

    out << "// Proceso de renderizado 3D compartido\n";
    out << "process ray_display()\nbegin\n    loop\n";
    out << "        graph = RAY_RENDER(0);\n";
    out << "        if (graph)\n";
    out << "            x = 320; y = 240; // Default center\n";
    out << "        end\n";
    out << "        frame;\n";
    out << "    end\n";
    out << "end\n";
    helper.close();
  }

  QStringList filters;
  filters << "*.scn";

  QStringList generatedIncludes;
  QStringList sceneNames;
  QSet<QString> processedBaseNames;
  QSet<QString> allProjectResources;

  // First pass: collect all resources from all scenes
  QDir projDir(projectPath);
  QDirIterator it(projectPath, filters, QDir::Files | QDir::NoSymLinks,
                  QDirIterator::Subdirectories);
  while (it.hasNext()) {
    QString filePath = it.next();
    SceneData data;
    if (loadSceneJson(filePath, data)) {
      QDir sceneDir = QFileInfo(filePath).absoluteDir();
      auto fix = [&](QString path) -> QString {
        if (path.isEmpty())
          return QString();
        QFileInfo info(path);
        QString absPath = info.isRelative()
                              ? QFileInfo(sceneDir, path).absoluteFilePath()
                              : info.absoluteFilePath();
        QString projAbs = projDir.absolutePath();

        // If it's outside the project, we can't reliably load it with relative
        // paths. We should warn, but for now we try to at least make it a
        // project-relative path if possible or keep it as is if it's already
        // inside assets.
        if (absPath.startsWith(projAbs)) {
          QString rel = projDir.relativeFilePath(absPath);
          if (!rel.startsWith("assets/") && rel.contains("assets/")) {
            rel = rel.mid(rel.indexOf("assets/"));
          }
          return rel;
        }

        // Fallback for files outside project: try to find 'assets' in path
        int assetsIdx = absPath.lastIndexOf("assets/");
        if (assetsIdx != -1)
          return absPath.mid(assetsIdx);

        return projDir.relativeFilePath(absPath);
      };
      if (!data.cursorFile.isEmpty())
        allProjectResources.insert(fix(data.cursorFile));
      if (!data.musicFile.isEmpty())
        allProjectResources.insert(fix(data.musicFile));
      for (auto ent : data.entities) {
        if (!ent->sourceFile.isEmpty())
          allProjectResources.insert(fix(ent->sourceFile));
        if (!ent->fontFile.isEmpty())
          allProjectResources.insert(fix(ent->fontFile));
      }
    }
  }

  // Generate resources.prg
  QFile resFile(includesDir + "/resources.prg");
  if (resFile.open(QIODevice::WriteOnly)) {
    QTextStream ts(&resFile);
    ts << "// Global Project Resources\n\n";

    // Android Helpers
    QString assetOpen = m_projectData.androidSupport ? "get_asset_path(" : "";
    QString assetClose = m_projectData.androidSupport ? ")" : "";

    // 1. GLOBAL Declarations
    ts << "GLOBAL\n";
    QMap<QString, QString> resMap;
    for (const QString &res : allProjectResources) {
      QString cleanName = QFileInfo(res).baseName().toLower();
      cleanName = cleanName.replace(".", "_").replace(" ", "_");
      QString ext = QFileInfo(res).suffix().toLower();
      QString varName = "id_" + cleanName + "_" + ext;
      resMap[res] = varName;
      ts << "    int " << varName << ";\n";
    }
    ts << "end\n\n";

    // 2. Load Function
    ts << "function load_project_resources()\nbegin\n";
    for (auto it = resMap.begin(); it != resMap.end(); ++it) {
      QString res = it.key();
      QString varName = it.value();
      QString ext = QFileInfo(res).suffix().toLower();
      QString loadFunc;
      if (ext == "fpg")
        loadFunc = "fpg_load";
      else if (ext == "png" || ext == "jpg" || ext == "jpeg" || ext == "bmp" ||
               ext == "tga")
        loadFunc = "map_load";
      else if (ext == "fnt" || ext == "fnx")
        loadFunc = "fnt_load";
      // Music is HANDLED IN SCENE directly as per wiki recommendation

      if (!loadFunc.isEmpty()) {
        ts << "    // Loading: " << res << "\n";
        ts << "    " << varName << " = 0;\n";
        ts << "    if (" << varName << " <= 0)\n";
        ts << "        say(\"DEBUG: Checking " << res << " in ../\");\n";
        ts << "        if (fexists(\"../" << res << "\"))\n";
        ts << "            " << varName << " = " << loadFunc << "(\"../" << res
           << "\");\n";
        ts << "            say(\"  -> Result ID: \" + " << varName << ");\n";
        ts << "        else\n";
        ts << "            say(\"  -> File NOT found in ../\");\n";
        ts << "        end\n";
        ts << "    end\n";

        ts << "    if (" << varName << " <= 0)\n";
        ts << "        say(\"DEBUG: Checking " << res << " in root\");\n";
        ts << "        if (fexists(\"" << res << "\"))\n";
        ts << "            " << varName << " = " << loadFunc << "(\"" << res
           << "\");\n";
        ts << "            say(\"  -> Result ID: \" + " << varName << ");\n";
        ts << "        else\n";
        ts << "            say(\"  -> File NOT found in root\");\n";
        ts << "        end\n";
        ts << "    end\n";
      }
    }
    ts << "end\n\n";

    // 3. Unload Function
    ts << "function unload_project_resources()\nbegin\n";
    for (auto it = resMap.begin(); it != resMap.end(); ++it) {
      QString res = it.key();
      QString varName = it.value();
      QString ext = QFileInfo(res).suffix().toLower();
      if (ext == "fpg")
        ts << "    if(" << varName << ">0) fpg_unload(" << varName
           << "); end\n";
      else if (ext == "fnt" || ext == "fnx")
        ts << "    if(" << varName << ">0) fnt_unload(" << varName
           << "); end\n";
      else if (ext == "mp3" || ext == "ogg" || ext == "wav")
        ts << "    if(" << varName << ">0) music_unload(" << varName
           << "); end\n";
      else if (ext == "png" || ext == "jpg" || ext == "jpeg" || ext == "bmp" ||
               ext == "tga") {
        ts << "    if(" << varName << ">0) map_unload(0, " << varName
           << "); end\n";
      }
      ts << "    " << varName << " = 0;\n";
    }
    ts << "end\n\n";

    resFile.close();
  }

  // Second pass: generate each scene PRG
  QDirIterator it2(projectPath, filters, QDir::Files | QDir::NoSymLinks,
                   QDirIterator::Subdirectories);
  while (it2.hasNext()) {
    QString filePath = it2.next();
    QString baseName = QFileInfo(filePath).baseName();
    if (processedBaseNames.contains(baseName))
      continue;
    processedBaseNames.insert(baseName);

    SceneData data;
    if (loadSceneJson(filePath, data)) {
      // Fix paths again for local generation context
      QDir sceneDir = QFileInfo(filePath).absoluteDir();
      auto fixPath = [&](QString &path) {
        if (path.isEmpty())
          return;
        path = projDir.relativeFilePath(
            QFileInfo(sceneDir, path).absoluteFilePath());
      };
      fixPath(data.cursorFile);
      fixPath(data.musicFile);
      for (auto ent : data.entities) {
        fixPath(ent->sourceFile);
        fixPath(ent->fontFile);
      }

      QString outFile = scenesOutDir + "/" + baseName + ".prg";
      QString existingCode;
      if (QFile::exists(outFile)) {
        QFile f(outFile);
        if (f.open(QIODevice::ReadOnly)) {
          QString content = f.readAll();
          if (content.contains("Generated automatically"))
            existingCode = content;
          f.close();
        }
      }

      QString code = generateScenePrg(baseName, data, existingCode);
      QFile f(outFile);
      if (f.open(QIODevice::WriteOnly)) {
        QTextStream out(&f);
        out << code;
        f.close();
        sceneNames.append(baseName);
        generatedIncludes.append("include \"scenes/" + baseName + ".prg\"");
      }
    }
  }

  // Generate scenes_list.prg
  QFile listFile(includesDir + "/scenes_list.prg");
  if (listFile.open(QIODevice::WriteOnly)) {
    QTextStream ts(&listFile);
    ts << "// Auto-generated list of scenes\n\n";

    // Ensure resources are available to all scenes - MOVED TO MAIN.PRG to avoid
    // double include ts << "include \"includes/resources.prg\";\n\n";

    for (const QString &sn : sceneNames)
      ts << "declare process " << sn << "();\n";
    ts << "\n";
    for (const QString &inc : generatedIncludes)
      ts << inc << "\n";
    ts << "\n";

    ts << "// Scene Navigation Helper\n";
    ts << "function goto_scene(string name)\nbegin\n";
    ts << "    let_me_alone();\n    write_delete(all_text);\n\n";
    for (const QString &sn : sceneNames)
      ts << "    if (name == \"" << sn << "\") " << sn << "(); return; end\n";
    ts << "    say(\"Scene not found: \" + name);\nend\n";
    listFile.close();
  }
}

bool CodeGenerator::patchMainIncludeScenes(const QString &projectPath) {
  QString mainPath = projectPath + "/src/main.prg";
  QFile f(mainPath);

  if (!f.exists()) {
    // Create Default Main if missing
    QString content = "import \"libmod_gfx\";\n"
                      "import \"libmod_input\";\n"
                      "import \"libmod_misc\";\n"
                      "import \"libmod_sound\";\n"
                      "import \"libmod_ray\";\n\n"
                      "include \"includes/resources.prg\";\n"
                      "include \"includes/scenes_list.prg\";\n\n"
                      "process main();\n"
                      "begin\n"
                      "    set_mode(640,480,32);\n"
                      "    // Start your scene here\n"
                      "    // e.g. MyScene();\n"
                      "    while(!key(_esc)) frame; end;\n"
                      "end\n";

    if (f.open(QIODevice::WriteOnly)) {
      f.write(content.toUtf8());
      f.close();
      return true;
    }
    return false;
  }

  if (!f.open(QIODevice::ReadWrite))
    return false;
  QString content = f.readAll();

  // Ensure libmod_sound is imported
  if (!content.contains("libmod_sound")) {
    content.prepend("import \"libmod_sound\";\n");
  }

  // Ensure resources.prg is included (Fix for missing Get_Asset_Path)
  if (!content.contains("resources.prg")) {
    QString resInc = "include \"includes/resources.prg\";";
    // Try to insert after imports
    int pos = content.lastIndexOf("import \"");
    if (pos != -1) {
      int endPos = content.indexOf(";", pos);
      if (endPos != -1)
        content.insert(endPos + 1, "\n" + resInc + "\n");
      else
        content.prepend(resInc + "\n");
    } else {
      // No imports? Prepend
      content.prepend(resInc + "\n");
    }
  }

  QString includeLine = "include \"includes/scenes_list.prg\";";

  // Remove redundant direct include of scene_commons if present (it's now in
  // scenes_list)
  content.remove("include \"includes/scene_commons.prg\";");
  content.remove("include \"includes/scene_commons.prg\"");

  if (content.contains("scenes_list.prg")) { // robust check
    f.resize(0);
    f.write(content.toUtf8());
    f.close();
    return true;
  }

  // Insert after imports
  int pos = content.lastIndexOf("import \"");
  if (pos != -1) {
    int endPos = content.indexOf(";", pos);
    if (endPos != -1) {
      content.insert(endPos + 1, "\n" + includeLine + "\n");
    } else {
      content.prepend(includeLine + "\n");
    }
  } else {
    content.prepend(includeLine + "\n");
  }

  f.resize(0);
  f.seek(0);
  f.write(content.toUtf8());
  f.close();
  return true;
}

bool CodeGenerator::setStartupScene(const QString &projectPath,
                                    const QString &sceneName) {
  QString mainPath = projectPath + "/src/main.prg";
  QFile f(mainPath);

  // Backup if exists
  if (f.exists()) {
    QFile::remove(mainPath + ".bak");
    f.copy(mainPath + ".bak");
  }

  CodeGenerator generator;
  ProjectData data = ProjectManager::loadProjectData(projectPath);
  data.startupScene = sceneName;
  generator.setProjectData(data);

  ProjectManager::saveProjectData(projectPath, data);

  QString existingContent;
  if (f.open(QIODevice::ReadOnly | QIODevice::Text)) {
    existingContent = QTextStream(&f).readAll();
    f.close();
  }

  QString newCode;
  if (existingContent.isEmpty() ||
      !existingContent.contains("// [[ED_STARTUP_SCENE_START]]")) {
    newCode = generator.generateMainPrg();
  } else {
    newCode =
        generator.patchMainPrg(existingContent, QVector<EntityInstance>());
  }

  if (f.open(QIODevice::WriteOnly | QIODevice::Text)) {
    f.write(newCode.toUtf8());
    f.close();
    return true;
  }
  return false;
}
