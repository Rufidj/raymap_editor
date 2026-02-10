#include "codegenerator.h"
#include "processgenerator.h"
#include "sceneeditor.h"
#include <QDebug>
#include <QDir>
#include <QDirIterator>
#include <QFile>
#include <QFileInfo>
#include <QImage>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QPainter>
#include <QPixmap>
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
                 "\n"
                 "\n"
                 "\n"
                 "\n"
                 "// Constantes de entidad\n"
                 "// [[ED_CONSTANTS_START]]\n"
                 "CONST\n"
                 "    TYPE_PLAYER = 1;\n"
                 "    TYPE_ENEMY  = 2;\n"
                 "    TYPE_OBJECT = 3;\n"
                 "    TYPE_TRIGGER = 4;\n"
                 "    \n"
                 "    // Debug\n"
                 "    DEBUG_HITBOXES = 0; // Set to 1 to see click areas\n"
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
      if (protectIfHasContent) {
        int contentStart = oldStart + startMarker.length();
        QString existingContent =
            result.mid(contentStart, oldEND - contentStart).trimmed();
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
    int firstEnd = result.indexOf("end", funcPos);
    if (firstEnd != -1) {
      int secondEnd = result.indexOf("end", firstEnd + 3);
      if (secondEnd != -1) {
        result.remove(funcPos, (secondEnd + 3) - funcPos);
      } else {
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

  // AGGRESSIVE FIX: Clean injection logic
  QString monolithicBlock;
  monolithicBlock += "// [[INLINED_CODE_START]]\n";
  if (!m_inlineCommons.isEmpty())
    monolithicBlock += m_inlineCommons + "\n";
  if (!m_inlineResources.isEmpty())
    monolithicBlock += m_inlineResources + "\n";
  if (!m_inlineScenes.isEmpty())
    monolithicBlock += m_inlineScenes + "\n";
  monolithicBlock += "// [[INLINED_CODE_END]]\n";

  QString cleanCode;
  QStringList originalLines = result.split('\n');
  bool skipping = false;
  bool inlinedFound = false;

  // 1. Keep imports and non-inlined code
  for (const QString &line : originalLines) {
    QString trimmed = line.trimmed();
    if (trimmed == "// [[INLINED_CODE_START]]") {
      skipping = true;
      if (!inlinedFound) {
        cleanCode += monolithicBlock;
        inlinedFound = true;
      }
      continue;
    }
    if (trimmed == "// [[INLINED_CODE_END]]") {
      skipping = false;
      continue;
    }
    if (skipping)
      continue;

    // Check for legacy includes we want to remove
    if (trimmed.startsWith("include \"includes/"))
      continue;

    // Avoid double-empty lines
    if (trimmed.isEmpty() && cleanCode.endsWith("\n\n"))
      continue;

    cleanCode += line + "\n";
  }

  // 2. If no inlined block was found, insert it after imports or at top
  if (!inlinedFound) {
    int lastImport = cleanCode.lastIndexOf("import \"");
    if (lastImport != -1) {
      int endOfLine = cleanCode.indexOf("\n", lastImport);
      cleanCode.insert(endOfLine + 1, "\n" + monolithicBlock + "\n");
    } else {
      cleanCode.prepend(monolithicBlock + "\n");
    }
  }

  // 3. Ensure essential imports are present
  QStringList essentialImports = {"libmod_gfx", "libmod_input", "libmod_misc",
                                  "libmod_ray", "libmod_sound"};
  for (const QString &imp : essentialImports) {
    if (!cleanCode.contains("import \"" + imp + "\"")) {
      cleanCode.prepend("import \"" + imp + "\";\n");
    }
  }

  // 4. Ensure essential audio/resource calls in main()
  if (!cleanCode.contains("load_project_resources")) {
    int mainStart = cleanCode.indexOf("process main()");
    if (mainStart != -1) {
      int beginPos = cleanCode.indexOf("begin", mainStart);
      if (beginPos != -1) {
        cleanCode.insert(cleanCode.indexOf("\n", beginPos) + 1,
                         "    load_project_resources();\n");
      }
    }
  }

  // 5. Final Safety Check: Ensure process main() exists
  if (!cleanCode.contains("process main()")) {
    cleanCode += "\n// Safety Fallback: Main Process\n";
    cleanCode += "process main()\n";
    cleanCode += "begin\n";
    cleanCode += "    load_project_resources();\n";
    cleanCode += "    // [[ED_STARTUP_SCENE_START]]\n";
    QString startScene = m_variables.value("STARTUP_SCENE");
    if (!startScene.isEmpty() && !startScene.contains("//"))
      cleanCode += "    " + startScene + "();\n";
    else
      cleanCode +=
          "    if (exists(type scene1)) scene1(); end\n"; // Intelligent try
    cleanCode += "    // [[ED_STARTUP_SCENE_END]]\n";
    cleanCode += "    loop\n";
    cleanCode += "        if (key(_esc)) exit(\"\", 0); end\n";
    cleanCode += "        frame;\n";
    cleanCode += "    end\n";
    cleanCode += "end\n";
  }

  return cleanCode;
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
    ent->scaleX = obj["scaleX"].toDouble(ent->scale);
    ent->scaleY = obj["scaleY"].toDouble(ent->scale);
    ent->script = obj["script"].toString(); // Relative path usually
    ent->onClick = obj["onClick"].toString();

    if (ent->type == ENTITY_SPRITE) {
      ent->sourceFile = obj["sourceFile"].toString(); // Check if relative?
      ent->graphId = obj["graphId"].toInt();
    } else if (ent->type == ENTITY_TEXT) {
      ent->text = obj["text"].toString();
      ent->fontId = obj["fontId"].toInt();
      ent->alignment = obj["alignment"].toInt(0);

      if (obj.contains("fontFile")) {
        ent->fontFile = obj["fontFile"].toString();
      }
    }

    // Visual Item (NONE) in generator
    ent->item = nullptr;

    data.entities.append(ent);
  }

  return true;
}

QString CodeGenerator::generateScenePrg(const QString &sceneName,
                                        const SceneData &data,
                                        const QString &interactionMapPath,
                                        const QString &existingCode) {
  QString code;
  code += QString("process %1()\n").arg(sceneName);
  code += "private\n    int ent_id;\n    string w_title;\n";

  QString userSetup, userLoop;

  // Preserve existing user code
  if (!existingCode.isEmpty()) {
    int startSetup = existingCode.indexOf("// [[USER_SETUP]]");
    int endSetup = existingCode.indexOf("// [[USER_SETUP_END]]");
    if (startSetup != -1 && endSetup != -1) {
      userSetup =
          existingCode.mid(startSetup + 17, endSetup - (startSetup + 17));
    }
    int startLoop = existingCode.indexOf("// [[USER_LOOP]]");
    int endLoop = existingCode.indexOf("// [[USER_LOOP_END]]");
    if (startLoop != -1 && endLoop != -1) {
      userLoop = existingCode.mid(startLoop + 16, endLoop - (startLoop + 16));
    }
  }

  // Define unique variables for interactive entities to allow robust coordinate
  // checking
  int interactiveVarCount = 0;
  for (auto ent : data.entities) {
    if (!ent->onClick.isEmpty()) {
      interactiveVarCount++;
      code += QString("    int i_ent_%1;\n").arg(interactiveVarCount);
    }
  }

  code += "begin\n";
  code += "    // 1. Radical Cleanup: Ensure we are the only process running\n";
  code += "    let_me_alone();\n";
  code += "    // Wait a frame to ensure cleanup propagates if needed "
          "(sometimes safe)\n";
  code += "    frame;\n\n";

  code += "    // Scene Setup\n";
  code +=
      QString("    set_mode(%1, %2, 32);\n").arg(data.width).arg(data.height);

  // Load Resources
  QMap<QString, QString> resMap;
  QSet<QString> loadedResources;
  for (const auto &ent : data.entities) {
    if (ent->sourceFile.isEmpty())
      continue;
    if (loadedResources.contains(ent->sourceFile))
      continue;

    QString ext = QFileInfo(ent->sourceFile).suffix().toLower();
    QString safeName = QFileInfo(ent->sourceFile).fileName().replace(".", "_");
    safeName.replace("-", "_").replace(" ", "_");
    QString resVar = "res_" + safeName;

    if (ext == "fpg") {
      code += QString("    int %1 = fpg_load(\"%2\");\n")
                  .arg(resVar)
                  .arg(ent->sourceFile);
    } else if (ext == "png" || ext == "jpg" || ext == "jpeg" || ext == "tga" ||
               ext == "bmp") {
      code += QString("    int %1 = map_load(\"%2\");\n")
                  .arg(resVar)
                  .arg(ent->sourceFile);
    } else {
      continue;
    }
    resMap[ent->sourceFile] = resVar;
    loadedResources.insert(ent->sourceFile);
  }

  // Load Fonts
  for (const auto &ent : data.entities) {
    if (ent->type != ENTITY_TEXT || ent->fontFile.isEmpty())
      continue;
    if (loadedResources.contains(ent->fontFile))
      continue;

    QString cleanFontName =
        QFileInfo(ent->fontFile).baseName().replace(" ", "_").replace("-", "_");
    QString resVar = "fnt_" + cleanFontName;
    code += QString("    int %1 = fnt_load(\"%2\");\n")
                .arg(resVar)
                .arg(ent->fontFile);
    resMap[ent->fontFile] = resVar;
    loadedResources.insert(ent->fontFile);
  }

  // Load Music
  if (!data.musicFile.isEmpty() && !loadedResources.contains(data.musicFile)) {
    QString safeName = QFileInfo(data.musicFile)
                           .fileName()
                           .replace(".", "_")
                           .replace(" ", "_")
                           .replace("-", "_");
    QString resVar = "mus_" + safeName;
    code += QString("    int %1 = music_load(\"%2\");\n")
                .arg(resVar)
                .arg(data.musicFile);
    resMap[data.musicFile] = resVar;
    loadedResources.insert(data.musicFile);
  }

  // Set Cursor
  if (!data.cursorFile.isEmpty()) {
    QString ext = QFileInfo(data.cursorFile).suffix().toLower();
    QString resVar = resMap.value(data.cursorFile);
    if (resVar.isEmpty()) {
      resVar = "mouse_graph";
      if (ext == "fpg")
        code += QString("    int %1 = fpg_load(\"%2\");\n")
                    .arg(resVar)
                    .arg(data.cursorFile);
      else
        code += QString("    int %1 = map_load(\"%2\");\n")
                    .arg(resVar)
                    .arg(data.cursorFile);
    }
    if (ext == "fpg") {
      code += QString("    mouse.file = %1;\n").arg(resVar);
      code += QString("    mouse.graph = %1;\n").arg(data.cursorGraph);
    } else {
      code += "    mouse.file = 0;\n";
      code += QString("    mouse.graph = %1;\n").arg(resVar);
    }
  } else {
    code += "    mouse.graph = 0; // System cursor\n";
  }

  // Cleanup Previous Scene (Critical)
  code += "    // Clean up previous scene\n";
  code += "    let_me_alone();\n";
  code += "\n";

  // Music Playback
  if (!data.musicFile.isEmpty() && resMap.contains(data.musicFile)) {
    code += "\n    // Music Playback\n";
    QString varName = resMap[data.musicFile];
    int loops = data.musicLoop ? -1 : 0;
    code += QString("    if (%1 > 0)\n").arg(varName);
    code += "        music_set_volume(128);\n";
    code += QString("        music_play(%1, %2);\n").arg(varName).arg(loops);
    code += "    end\n";
  }

  struct SceneEventData {
    QString code;
    QString varName;
    int hw, hh, hx, hy;
    int x, y, align; // For text-only fallback
    bool isSprite;
  };
  QList<SceneEventData> sceneEvents;
  int currentInteractiveIdx = 1;

  code += "\n    // Instantiate Entities\n";
  for (auto ent : data.entities) {
    if (ent->type == ENTITY_SPRITE) {
      QString processName = ent->script.isEmpty()
                                ? "StaticSprite"
                                : QFileInfo(ent->script).baseName();

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
      code += QString("    ent_id.angle = %1; ent_id.size_x = %2; "
                      "ent_id.size_y = %3;\n")
                  .arg(int(ent->angle * 1000))
                  .arg(int(ent->scaleX * 100))
                  .arg(int(ent->scaleY * 100));

      if (!ent->onClick.isEmpty()) {
        QString vName = QString("i_ent_%1").arg(currentInteractiveIdx++);
        code += QString("    %1 = ent_id;\n").arg(vName);

        SceneEventData ev;
        ev.code = ent->onClick;
        ev.varName = vName;
        ev.hw = ent->hitW;
        ev.hh = ent->hitH;
        ev.hx = ent->hitX;
        ev.hy = ent->hitY;
        ev.isSprite = true;
        sceneEvents.append(ev);
      }
      code += "\n";

    } else if (ent->type == ENTITY_TEXT) {
      QString fontId = "0";
      if (!ent->fontFile.isEmpty() && resMap.contains(ent->fontFile)) {
        fontId = resMap[ent->fontFile];
      }

      // Text Entity Logic
      // If it has onClick, we generate a dedicated Auto_Btn process!
      if (!ent->onClick.isEmpty()) {
        // Use scene name + unique index to allow multiple scenes with buttons
        QString btnProcName = QString("Auto_Btn_%1_%2")
                                  .arg(sceneName)
                                  .arg(currentInteractiveIdx++);
        // Sanitize name to be a valid identifier
        btnProcName =
            btnProcName.replace(" ", "_").replace("-", "_").replace(".", "_");

        code += QString("    // Text Button: %1\n").arg(ent->name);
        // Pass the font ID (variable name) as argument
        code += QString("    %1(%2);\n").arg(btnProcName).arg(fontId);

        // Generate the process in commons or at end of file?
        // We'll append it to m_inlineCommons for now or a new m_inlineScenes
        // section. Better: Append to m_inlineScenes, but as a separate process
        // block.

        QString btnCode;
        // Process updates to accept font_id
        btnCode += QString("process %1(int font_id)\n").arg(btnProcName);
        btnCode += "PRIVATE\n";
        btnCode += "    int txt_id;\n";
        btnCode += "    int w, h;\n";
        btnCode += "    int my_x, my_y;\n";
        btnCode += "begin\n";
        btnCode += QString("    my_x = %1; my_y = %2;\n")
                       .arg(int(ent->x))
                       .arg(int(ent->y));
        // Note: write returns the text ID
        btnCode +=
            QString("    txt_id = write(font_id, my_x, my_y, %1, \"%2\");\n")
                .arg(ent->alignment)
                .arg(ent->text);
        btnCode +=
            QString("    w = text_width(font_id, \"%1\");\n").arg(ent->text);
        btnCode +=
            QString("    h = text_height(font_id, \"%1\");\n").arg(ent->text);
        btnCode += "    loop\n";
        // Logic based on Alignment for Hitbox Check
        // Align 0 (Left): x to x+w
        // Align 1 (Center): x-w/2 to x+w/2
        // Align 2 (Right): x-w to x

        QString xCondition;
        if (ent->alignment == 1) // Center
          xCondition = "mouse.x > (my_x - w/2) AND mouse.x < (my_x + w/2)";
        else if (ent->alignment == 2) // Right
          xCondition = "mouse.x > (my_x - w) AND mouse.x < my_x";
        else // Left (0)
          xCondition = "mouse.x > my_x AND mouse.x < (my_x + w)";

        btnCode += QString("        if (mouse.left AND (%1) AND (mouse.y >= "
                           "(my_y - 5) AND mouse.y <= (my_y + h + 5)))\n")
                       .arg(xCondition);
        btnCode += "            // Click Detected\n";
        btnCode += "            while(mouse.left) frame; end // Wait release\n";
        btnCode += QString("            %1\n").arg(ent->onClick); // User Action
        // After action, if scene changes, this process dies via let_me_alone in
        // next scene. But if action is just logic, we continue loop.
        btnCode += "        end\n";
        btnCode += "        frame;\n";
        btnCode += "    end\n"; // End Loop
        btnCode += "OnExit:\n";
        btnCode += "    write_delete(txt_id);\n";
        btnCode += "end\n\n";

        m_inlineScenes += btnCode; // Append this process to the file

      } else {
        // Static Text (No interaction)
        code += QString("    // Text Entity: %1\n").arg(ent->name);
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
      if (mapPath.contains("assets/"))
        mapPath = mapPath.mid(mapPath.indexOf("assets/"));

      code += QString("    if (RAY_LOAD_MAP(\"%1\", 0) == 0) say(\"Error "
                      "loading hybrid 3D Map\"); end\n")
                  .arg(mapPath);
      code += "    ray_display();\n\n";
    }
  }

  // Inject User Setup
  code += "\n    // [[USER_SETUP]]" + userSetup + "// [[USER_SETUP_END]]\n";

  // Loop Logic
  code += "    int mouse_last_state = 0;\n";
  code += "    int scene_exit = 0;\n";
  code += "    loop\n";
  code += "        if (scene_exit) break; end\n";
  if (data.exitOnEsc) {
    code += "        if (key(_esc)) exit(\"\", 0); end\n";
  }

  // 100% Robust Click Detection (No interaction map needed)
  if (!sceneEvents.isEmpty()) {
    code += "\n        // Robust Click Handling\n";
    code += "        if (mouse.left && !mouse_last_state)\n";
    code += "            mouse_last_state = 1;\n";
    code += "            // Check each interactive entity\n";

    // We check events in reverse order (topmost/last created first)
    for (int i = sceneEvents.size() - 1; i >= 0; i--) {
      const auto &ev = sceneEvents[i];
      if (ev.isSprite) {
        code +=
            QString("            if (check_scene_click(%1, %2, %3, %4, %5))\n")
                .arg(ev.varName)
                .arg(ev.hw)
                .arg(ev.hh)
                .arg(ev.hx)
                .arg(ev.hy);
      } else {
        // SIMPLIFIED HITBOX LOGIC (CENTER-BASED)
        int w = ev.hw > 0 ? ev.hw : 120;
        int h = ev.hh > 0 ? ev.hh : 30;

        // Force Center Alignment for Hitbox
        // We assume the user placed the entity where they want the CENTER of
        // the click to be.
        int xmin = (ev.x + ev.hx) - (w / 2);
        int ymin = (ev.y + ev.hy) - (h / 2);
        int xmax = xmin + w;
        int ymax = ymin + h;

        // DEBUG VISUALS: REMOVED
        // box(xmin, ymin, xmax, ymax, map_new(1,1,16), 64);

        code += QString("            if (mouse.x >= %1 && mouse.x <= %2 && "
                        "mouse.y >= %3 && mouse.y <= %4)\n")
                    .arg(xmin)
                    .arg(xmax)
                    .arg(ymin)
                    .arg(ymax);
      }
      QString sanitizedCode = ev.code;
      sanitizedCode.replace("\"", "'"); // Sanitize for safe 'say' display
      code += QString("                // Action: %1\n")
                  .arg(sanitizedCode.left(30));
      code += QString("                %1\n").arg(ev.code);
      code += "                scene_exit = 1;\n";
      code += "                break; // Stop after first click\n";
      code += "            end\n"; // Close the hit-test IF
    }
    code += "        end\n"; // Close the mouse.left IF
    code += "        if (!mouse.left) mouse_last_state = 0; end\n";
  }

  // Inject User Loop
  code += "        // [[USER_LOOP]]" + userLoop + "// [[USER_LOOP_END]]\n";

  code += "        frame;\n";
  code += "    end\n";
  code += "end\n";

  return code;
}

void CodeGenerator::generateAllScenes(const QString &projectPath,
                                      const QSet<QString> &extraResources) {
  // Clear inline buffers for Monolithic generation
  m_inlineCommons.clear();
  m_inlineResources.clear();
  m_inlineScenes.clear();

  // We do NOT create subdirectories anymore if we want to be clean,
  // but let's keep them if user wants to see them? No, user said "dejarnos de
  // includes". So we skip writing files.

  // ---------------------------------------------------------
  // 1. GENERATE COMMONS
  // ---------------------------------------------------------
  {
    m_inlineCommons +=
        "// =============================================================\n";
    m_inlineCommons += "// COMMONS (Inlined)\n";
    m_inlineCommons +=
        "// =============================================================\n\n";

    QString pkgName = m_projectData.packageName.isEmpty()
                          ? "com.example.game"
                          : m_projectData.packageName;

    m_inlineCommons += "// Android Path Helper\n";
    m_inlineCommons += "function string get_asset_path(string relative_path)\n";
    m_inlineCommons += "begin\n";
    m_inlineCommons += "    if (os_id == OS_ANDROID)\n";
    m_inlineCommons += "        return \"/data/data/\" + \"" + pkgName +
                       "\" + \"/files/\" + relative_path;\n";
    m_inlineCommons += "    else\n";
    m_inlineCommons += "        // Robust Desktop Path Check (src/ vs root)\n";
    m_inlineCommons += "        if (!fexists(relative_path) && fexists(\"../\" "
                       "+ relative_path))\n";
    m_inlineCommons += "            return \"../\" + relative_path;\n";
    m_inlineCommons += "        end\n";
    m_inlineCommons += "    end\n";
    m_inlineCommons += "    return relative_path;\n";
    m_inlineCommons += "end\n\n";

    m_inlineCommons +=
        "process StaticSprite()\nbegin\n    loop\n        frame;\n    "
        "end\nend\n\n";

    m_inlineCommons += "// Shared 3D Renderer Process\n";
    m_inlineCommons += "process ray_display()\nbegin\n    loop\n";
    m_inlineCommons += "        graph = RAY_RENDER(0);\n";
    m_inlineCommons += "        if (graph)\n";
    m_inlineCommons += "            x = 320; y = 240; // Default center\n";
    m_inlineCommons += "        end\n";
    m_inlineCommons += "        frame;\n";
    m_inlineCommons += "    end\n";
    m_inlineCommons += "end\n\n";
    m_inlineCommons += "// Shared 2D Click Detection Helper\n";
    m_inlineCommons += "function int check_scene_click(int id, int hw, int hh, "
                       "int hx, int hy)\n";
    m_inlineCommons += "private\n    int w, h, xmin, ymin, xmax, ymax;\n";
    m_inlineCommons += "begin\n";
    m_inlineCommons += "    if (id == 0 || !exists(id)) return 0; end\n\n";
    m_inlineCommons += "    if (hw > 0 && hh > 0)\n";
    m_inlineCommons += "        w = hw; h = hh;\n";
    m_inlineCommons += "    else\n";
    m_inlineCommons += "        // Auto size from graphic (if exists)\n";
    m_inlineCommons +=
        "        w = graphic_info(id.file, id.graph, G_WIDTH);\n";
    m_inlineCommons +=
        "        h = graphic_info(id.file, id.graph, G_HEIGHT);\n";
    m_inlineCommons += "        if (w <= 0) w = 64; end \n";
    m_inlineCommons += "        if (h <= 0) h = 32; end\n";
    m_inlineCommons += "    end\n\n";
    m_inlineCommons += "    // Scale Adjustment\n";
    m_inlineCommons += "    w = w * id.size_x / 100;\n";
    m_inlineCommons += "    // Simplified Check:\n";
    m_inlineCommons += "    // If it is a process with a graphic (id > 0), use "
                       "native collision!\n";
    m_inlineCommons +=
        "    // This handles rotation, scale and transparency automatically.\n";
    m_inlineCommons += "    if (id > 0 && exists(id))\n";
    m_inlineCommons += "        return collision(type mouse);\n";
    m_inlineCommons += "    end\n\n";
    m_inlineCommons +=
        "    // Fallback for manual areas (Text or No-Graphic)\n";
    m_inlineCommons += "    // Scale Adjustment for manual sizes\n";
    m_inlineCommons += "    w = w * id.size_x / 100;\n";
    m_inlineCommons += "    h = h * id.size_y / 100;\n\n";
    m_inlineCommons += "    // Alignment-aware X calculation\n";
    m_inlineCommons += "    // BennuGD write() aligns relative to X.\n";
    m_inlineCommons += "    // If Align 0 (Left): Box starts at X\n";
    m_inlineCommons += "    // If Align 1 (Center): Box starts at X - W/2\n";
    m_inlineCommons += "    // If Align 2 (Right): Box starts at X - W\n";
    m_inlineCommons += "    \n";
    m_inlineCommons += "    // We use a safe default if alignment is unknown "
                       "(assume center like sprites)\n";
    m_inlineCommons += "    // But for text, we generated 'align' into the "
                       "event check earlier? \n";
    m_inlineCommons += "    // Wait, 'id' is a process, it doesn't know "
                       "'align' in this scope easily unless passed.\n";
    m_inlineCommons +=
        "    // However, typical text entities in my generator use:\n";
    m_inlineCommons += "    // write(..., x, y, alignment, \"text\");\n";
    m_inlineCommons += "    \n";
    m_inlineCommons += "    // Let's assume standard centering for SAFETY "
                       "unless offset manually.\n";
    m_inlineCommons += "    // Ideally, for text, users check a box that "
                       "matches the text visual.\n";
    m_inlineCommons += "    // If this is a manual hitbox (hx, hy, hw, hh), we "
                       "trust hx/hy are relative to x,y.\n";
    m_inlineCommons += "    \n";
    m_inlineCommons += "    // If the user says \"click is only top-left\", it "
                       "means the detected area is too small\n";
    m_inlineCommons += "    // or shifted.\n";
    m_inlineCommons += "    \n";
    m_inlineCommons +=
        "    // Let's try to center the box on X,Y by default, effectively:\n";
    m_inlineCommons += "    xmin = id.x + (hx * id.size_x / 100) - (w / 2);\n";
    m_inlineCommons += "    ymin = id.y + (hy * id.size_y / 100) - (h / 2);\n";
    m_inlineCommons += "    xmax = xmin + w;\n";
    m_inlineCommons += "    ymax = ymin + h;\n";
    m_inlineCommons += "    \n";
    m_inlineCommons +=
        "    // DEBUG: Expand the box slightly to be forgiving?\n";
    m_inlineCommons += "    // No, let's stick to exact math first.\n";
    m_inlineCommons += "    \n";
    m_inlineCommons += "    if (mouse.x >= xmin && mouse.x <= xmax && mouse.y "
                       ">= ymin && mouse.y <= ymax)\n";
    m_inlineCommons += "        return 1;\n";
    m_inlineCommons += "    end\n";
    m_inlineCommons += "    return 0;\n";
    m_inlineCommons += "end\n\n";
  }

  // ---------------------------------------------------------
  // 2. COLLECT RESOURCES & GENERATE SCENES
  // ---------------------------------------------------------
  QStringList filters;
  filters << "*.scn";

  QSet<QString> processedBaseNames;
  QSet<QString> allProjectResources = extraResources;
  QStringList sceneNames;

  QDir projDir(projectPath);
  QDirIterator it(projectPath, filters, QDir::Files | QDir::NoSymLinks,
                  QDirIterator::Subdirectories);

  while (it.hasNext()) {
    QString filePath = it.next();
    QString baseName = QFileInfo(filePath).baseName();

    if (processedBaseNames.contains(baseName))
      continue;
    processedBaseNames.insert(baseName);

    SceneData data;
    if (loadSceneJson(filePath, data)) {
      QDir sceneDir = QFileInfo(filePath).absoluteDir();

      // Resource Collection Logic
      auto fix = [&](QString path) -> QString {
        if (path.isEmpty())
          return QString();
        QFileInfo info(path);
        QString absPath = info.isRelative()
                              ? QFileInfo(sceneDir, path).absoluteFilePath()
                              : info.absoluteFilePath();
        QString projAbs = projDir.absolutePath();

        if (absPath.startsWith(projAbs)) {
          QString rel = projDir.relativeFilePath(absPath);
          if (!rel.startsWith("assets/") && rel.contains("assets/")) {
            rel = rel.mid(rel.indexOf("assets/"));
          }
          return rel;
        }
        int assetsIdx = absPath.lastIndexOf("assets/");
        if (assetsIdx != -1)
          return absPath.mid(assetsIdx);

        return projDir.relativeFilePath(absPath);
      };

      if (!data.cursorFile.isEmpty()) {
        data.cursorFile = fix(data.cursorFile);
        allProjectResources.insert(data.cursorFile);
      }
      if (!data.musicFile.isEmpty()) {
        data.musicFile = fix(data.musicFile);
        allProjectResources.insert(data.musicFile);
      }
      for (auto ent : data.entities) {
        if (!ent->sourceFile.isEmpty()) {
          ent->sourceFile = fix(ent->sourceFile);
          allProjectResources.insert(ent->sourceFile);
        }
        if (!ent->fontFile.isEmpty()) {
          ent->fontFile = fix(ent->fontFile);
          allProjectResources.insert(ent->fontFile);
        }
      }

      // Generate Code for Scene
      // Note: We don't read existing .prg file if we are inlining,
      // UNLESS we want to preserve user edits.
      // If the user wants to "stop using includes", they likely accept
      // that manual edits in separate files might be lost or need to be moved.
      // However, for safety, let's try to read it if it exists in the old
      // location?
      QString oldFile = projectPath + "/src/scenes/" + baseName + ".prg";
      QString existingCode;
      if (QFile::exists(oldFile)) {
        QFile f(oldFile);
        if (f.open(QIODevice::ReadOnly))
          existingCode = f.readAll();
      }
      // Generate Interaction Map
      QString mapBaseName = baseName + "_input.png";
      QString fullMapPath = sceneDir.absoluteFilePath(mapBaseName);
      generateInteractionMap(data, fullMapPath, filePath);

      // Determine relative path for Bennu load
      // We use the same 'fix' logic or just simpler if we know it is in same
      // dir
      QString relMapPath = fix(mapBaseName);

      // Generate Code for Scene
      QString sceneCode =
          generateScenePrg(baseName.toLower(), data, relMapPath, existingCode);
      sceneNames.append(baseName);

      m_inlineScenes +=
          "\n// "
          "=============================================================\n";
      m_inlineScenes += "// SCENE: " + baseName + "\n";
      m_inlineScenes +=
          "// =============================================================\n";
      m_inlineScenes += sceneCode + "\n";
    }
  }

  // VALIDATE STARTUP SCENE (Smart falling back if renamed/deleted)
  QString currentStart = m_variables.value("STARTUP_SCENE");
  if (!sceneNames.isEmpty()) {
    bool found = false;
    for (const QString &sn : sceneNames) {
      if (sn.toLower() == currentStart.toLower()) {
        found = true;
        // Ensure exact casing matches process name
        setVariable("STARTUP_SCENE", sn);
        break;
      }
    }
    if (!found) {
      // Fallback to first available scene
      setVariable("STARTUP_SCENE", sceneNames.first());
    }
  } else {
    // No scenes found? Use a safe placeholder to avoid compiler error
    setVariable("STARTUP_SCENE", "// No scenes found - check assets/scenes/");
  }

  // ---------------------------------------------------------
  // 3. GENERATE RESOURCES
  // ---------------------------------------------------------
  {
    m_inlineResources +=
        "// =============================================================\n";
    m_inlineResources += "// GLOBAL RESOURCES (Inlined)\n";
    m_inlineResources +=
        "// =============================================================\n\n";

    QString assetOpen = m_projectData.androidSupport ? "get_asset_path(" : "";
    QString assetClose = m_projectData.androidSupport ? ")" : "";

    // Global Declarations
    m_inlineResources += "GLOBAL\n";
    QMap<QString, QString> resMap;
    for (const QString &res : allProjectResources) {
      QString cleanName = QFileInfo(res).baseName().toLower();
      cleanName = cleanName.replace(".", "_").replace(" ", "_");
      QString ext = QFileInfo(res).suffix().toLower();
      QString varName = "id_" + cleanName + "_" + ext;
      resMap[res] = varName;
      m_inlineResources += "    int " + varName + ";\n";
    }
    m_inlineResources += "end\n\n";

    // Load Function
    m_inlineResources += "function load_project_resources()\nbegin\n";
    for (auto it = resMap.begin(); it != resMap.end(); ++it) {
      QString res = it.key();
      QString varName = it.value();
      QString ext = QFileInfo(res).suffix().toLower();

      QString loadFunc;
      if (ext == "fpg" || ext == "map" || ext == "png" || ext == "jpg" ||
          ext == "jpeg" || ext == "bmp" || ext == "tga")
        loadFunc = "map_load";
      else if (ext == "fnt" || ext == "fnx")
        loadFunc = "fnt_load";
      else if (ext == "mp3" || ext == "ogg" || ext == "wav")
        loadFunc = "music_load";

      if (!loadFunc.isEmpty()) {
        m_inlineResources += "    if (" + varName + " <= 0) " + varName +
                             " = " + loadFunc + "(" + assetOpen + "\"" + res +
                             "\"" + assetClose + "); end\n";
        m_inlineResources += "    if (" + varName +
                             " > 0) say(\"Loaded resource: " + res +
                             " ID: \" + " + varName + "); end\n";
      }
    }
    m_inlineResources += "end\n\n";

    // Unload Function
    m_inlineResources += "function unload_project_resources()\nbegin\n";
    for (auto it = resMap.begin(); it != resMap.end(); ++it) {
      QString res = it.key();
      QString varName = it.value();
      QString ext = QFileInfo(res).suffix().toLower();
      if (ext == "fpg")
        m_inlineResources +=
            "    if(" + varName + ">0) fpg_unload(" + varName + "); end\n";
      else if (ext == "fnt" || ext == "fnx")
        m_inlineResources +=
            "    if(" + varName + ">0) fnt_unload(" + varName + "); end\n";
      else if (ext == "mp3" || ext == "ogg" || ext == "wav")
        m_inlineResources +=
            "    if(" + varName + ">0) music_unload(" + varName + "); end\n";
      else if (ext == "png" || ext == "jpg" || ext == "jpeg" || ext == "bmp" ||
               ext == "tga") {
        m_inlineResources +=
            "    if(" + varName + ">0) map_unload(0, " + varName + "); end\n";
      }
      m_inlineResources += "    " + varName + " = 0;\n";
    }
    m_inlineResources += "end\n\n";
  }

  // Scene Navigation Helper (Inlined into scenes block or resources block?
  // Scenes block seems mostly appropriate)
  {
    m_inlineScenes += "\n// Scene Navigation Helper\n";
    m_inlineScenes += "function goto_scene(string name)\nbegin\n";
    m_inlineScenes += "    let_me_alone();\n    write_delete(all_text);\n\n";
    for (const QString &sn : sceneNames)
      m_inlineScenes +=
          "    if (name == \"" + sn + "\") " + sn + "(); return; end\n";
    m_inlineScenes += "    say(\"Scene not found: \" + name);\nend\n";
  }
}

bool CodeGenerator::patchMainIncludeScenes(const QString &projectPath) {
  QString mainPath = projectPath + "/src/main.prg";
  QFile f(mainPath);

  if (!f.exists()) {
    // Create Default Main if missing (INLINED STYLE)
    CodeGenerator generator; // Use fresh generator for template
    QString content = generator.generateMainPrg();

    if (f.open(QIODevice::WriteOnly)) {
      f.write(content.toUtf8());
      f.close();
      return true;
    }
    return false;
  }

  if (!f.open(QIODevice::ReadOnly))
    return false;
  QString content = f.readAll();
  f.close();

  // We reuse patchMainPrg to inject the monolithic blocks
  // Since we are likely called after generateAllScenes, 'this' instance
  // should have the m_inline... variables populated.
  // However, if called statically or from a fresh instance, we might miss data.
  // But typically this is called from MainWindow context where generator might
  // be short-lived. WAIT: This function seems to be used for 2D scene
  // additions? If we are strictly monolithic, we don't need
  // "patchMainIncludeScenes" to add includes. We just need to ensure the
  // monolithic block is up to date.

  // If 'this' generator has no inline data, we can't really patch effectively
  // without regenerating everything.
  // Let's assume the caller has set up the generator correctly (e.g. called
  // generateAllScenes).

  QString newContent = this->patchMainPrg(content, QVector<EntityInstance>());

  if (f.open(QIODevice::WriteOnly)) {
    f.write(newContent.toUtf8());
    f.close();
    return true;
  }

  return false;
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

  // UPDATE CURRENT INSTANCE DATA using loading if needed, or just specific
  // field We assume generateAllScenes was called on 'this' so we have the
  // inline data. But we need ensuring project data is up to date with the
  // startup scene.
  ProjectData data = ProjectManager::loadProjectData(projectPath);
  data.startupScene = sceneName;
  this->setProjectData(data);

  // Save updated data
  ProjectManager::saveProjectData(projectPath, data);

  QString existingContent;
  if (f.open(QIODevice::ReadOnly | QIODevice::Text)) {
    existingContent = QTextStream(&f).readAll();
    f.close();
  }

  QString newCode;
  if (existingContent.isEmpty() ||
      !existingContent.contains("// [[ED_STARTUP_SCENE_START]]")) {
    // Generate Base + Patch (Inject Monolithic Block)
    QString baseCode = this->generateMainPrg();
    newCode = this->patchMainPrg(baseCode, QVector<EntityInstance>());
  } else {
    // Pass empty entities vector if we are just setting startup scene,
    // but typically we'd want the entities.
    // Ideally we should have entities stored?
    // For now we assume patchMainPrg handles the structure preservation
    // and our inline injection logic adds the content.
    newCode = this->patchMainPrg(existingContent, QVector<EntityInstance>());
  }

  if (f.open(QIODevice::WriteOnly | QIODevice::Text)) {
    f.write(newCode.toUtf8());
    f.close();
    return true;
  }
  return false;
}

// ----------------------------------------------------------------------------
// Interaction Map Generation
// ----------------------------------------------------------------------------
#include <QBitmap>
#include <QPainter>

void CodeGenerator::generateInteractionMap(const SceneData &data,
                                           const QString &fullPath,
                                           const QString &scenePath) {
  // Use RGB32 for colorful map
  QImage img(data.width, data.height, QImage::Format_RGB32);
  img.fill(Qt::black); // Background 0

  QPainter p(&img);
  p.setRenderHint(QPainter::Antialiasing, false);
  p.setRenderHint(QPainter::SmoothPixmapTransform, false);

  int nextEventId = 1;

  for (SceneEntity *ent : data.entities) {
    // Check for Manual Hitbox Override
    bool hasManualHitbox = (ent->hitW > 0 && ent->hitH > 0);

    // Filter non-interactive entities if no manual hitbox
    if (!hasManualHitbox) {
      if (ent->type == ENTITY_SPRITE) {
        if (ent->onClick.isEmpty())
          continue;
      } else if (ent->type == ENTITY_TEXT) {
        bool isInteractive =
            !ent->onClick.trimmed().isEmpty() && ent->onClick != "NONE";
        if (!isInteractive)
          continue;
      } else {
        continue; // Skip other types if no manual hitbox
      }
    } else {
      // Manual hitbox implies interactivity check
      if (ent->onClick.isEmpty() || ent->onClick == "NONE")
        continue;
    }

    int id = nextEventId++;
    if (id > 255)
      continue;

    // Color Generation
    int hue = (id * 137) % 360;
    QColor col = QColor::fromHsv(hue, 255, 255);

    p.save();
    p.translate(ent->x, ent->y);

    // Draw Manual Hitbox
    if (hasManualHitbox) {
      p.setPen(Qt::NoPen);
      p.setBrush(col);
      p.rotate(ent->angle); // Rotate manual box with entity
      // Draw centered at offset
      p.drawRect(ent->hitX - ent->hitW / 2, ent->hitY - ent->hitH / 2,
                 ent->hitW, ent->hitH);
      p.restore();
      continue;
    }

    // Default Rendering Logic
    if (ent->type == ENTITY_SPRITE) {
      QPixmap pm(ent->sourceFile);
      if (pm.isNull()) {
        p.restore();
        continue;
      }

      p.rotate(ent->angle);
      p.scale(ent->scaleX, ent->scaleY);

      // Draw Sprite Bounding Box (No Mask for heavier interaction)
      QPixmap solid(pm.size());
      solid.fill(col);
      // solid.setMask(mask); // Removed to make interaction area the full box

      // Center the sprite (Bennu sprites are centered)
      p.drawPixmap(-pm.width() / 2, -pm.height() / 2, solid);

    } else if (ent->type == ENTITY_TEXT) {
      p.setPen(QPen(col, 1));
      p.setBrush(QBrush(col));

      // Rough estimation: 14px width per char? 30px height?
      int w = ent->text.length() * 14;
      int h = 30;

      // Handle Alignment
      if (ent->alignment == 0) // Left
        p.drawRect(0, 0, w, h);
      else if (ent->alignment == 1) // Center
        p.drawRect(-w / 2, 0, w, h);
      else // Right
        p.drawRect(-w, 0, w, h);
    }
    p.restore();
  }

  // Blend Manual Painted Layer
  QFileInfo sceneInfo(scenePath);
  QString layerPath = sceneInfo.absolutePath() + "/" + sceneInfo.baseName() +
                      "_interaction.png";
  if (QFile::exists(layerPath)) {

    QImage layer(layerPath);
    if (!layer.isNull()) {
      p.setCompositionMode(QPainter::CompositionMode_SourceOver);
      p.drawImage(0, 0, layer);
    }
  }

  p.end();

  img.save(fullPath);
}
