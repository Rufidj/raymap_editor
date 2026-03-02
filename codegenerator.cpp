#include "codegenerator.h"
#include "mapdata.h" // Needed for MapData structure
#include "processgenerator.h"
#include "raymapformat.h" // Needed to read map entities
#include "sceneeditor.h"
#include <QDateTime>
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
#include <QString>
#include <QTextStream>
#include <QVector>

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
        "BEGIN\n"
        "    IF (os_id == OS_ANDROID)\n"
        "        RETURN \"/data/data/\" + \"{{PACKAGE_NAME}}\" + \"/files/\" + "
        "relative_path;\n"
        "    ELSE\n"
        "        RETURN relative_path;\n"
        "    END\n"
        "END\n";

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
  return generateMainPrgWithEntities(QVector<EntityInstance>(),
                                     QVector<NPCPath>());
}

QString CodeGenerator::generateEntityProcess(const QString &entityName,
                                             const QString &entityType) {
  if (entityType == "player") {
    return processTemplate(getPlayerTemplate());
  } else if (entityType == "enemy") {
    return processTemplate(getEnemyTemplate());
  }

  // Generic entity
  QString code = QString("PROCESS %1(x, y, z)\n"
                         "PRIVATE\n"
                         "    int health = 100;\n"
                         "BEGIN\n"
                         "    LOOP\n"
                         "        // TODO: Add entity logic\n"
                         "        FRAME;\n"
                         "    END\n"
                         "END\n")
                     .arg(entityName);

  return code;
}

QString CodeGenerator::getMainTemplate() {
  return QString(
      "// [[ED_HEADER_START]]\n"
      "// Auto-generado por RayMap Editor\n"
      "// Proyecto: {{PROJECT_NAME}}\n"
      "// Fecha: {{DATE}}\n"
      "// [[ED_HEADER_END]]\n"
      "\n"
      "import \"libmod_gfx\";\n"
      "import \"libmod_input\";\n"
      "import \"libmod_misc\";\n"
      "import \"libmod_ray\";\n"
      "import \"libmod_sound\";\n"
      "\n"
      "// [[USER_IMPORTS_START]]\n"
      "// [[USER_IMPORTS_END]]\n"
      "\n"
      "// ---------------------------------------------------------\n"
      "// CONSTANTES\n"
      "// ---------------------------------------------------------\n"
      "CONST\n"
      "    // [[ED_CONSTANTS_START]]\n"
      "    TYPE_PLAYER = 1;\n"
      "    TYPE_ENEMY  = 2;\n"
      "    TYPE_OBJECT = 3;\n"
      "    TYPE_TRIGGER = 4;\n"
      "    DEBUG_HITBOXES = 0;\n"
      "    // [[ED_CONSTANTS_END]]\n"
      "    \n"
      "    // [[USER_CONSTANTS_START]]\n"
      "    // [[USER_CONSTANTS_END]]\n"
      "END\n"
      "\n"
      "// ---------------------------------------------------------\n"
      "// DECLARACIONES Y PROCESOS DEL EDITOR\n"
      "// ---------------------------------------------------------\n"
      "// [[ED_PROCESSES_START]]\n"
      "{{ENTITY_PROCESSES}}\n"
      "\n"
      "{{NPC_PATHS_CODE}}\n"
      "// [[ED_PROCESSES_END]]\n"
      "\n"
      "// [[USER_PROCESSES_START]]\n"
      "// [[USER_PROCESSES_END]]\n"
      "\n"
      "// ---------------------------------------------------------\n"
      "// RECURSOS Y FUNCIONES DINÁMICAS\n"
      "// ----------------─----------------------------------------\n"
      "// [[ED_RESOURCES_START]]\n"
      "{{INLINE_RESOURCES}}\n"
      "// [[ED_RESOURCES_END]]\n"
      "\n"
      "// ---------------------------------------------------------\n"
      "// VARIABLES GLOBALES\n"
      "// ---------------------------------------------------------\n"
      "GLOBAL\n"
      "    // [[ED_GLOBAL_START]]\n"
      "    int screen_w;\n"
      "    int screen_h;\n"
      "    int move_speed;\n"
      "    int rot_speed;\n"
      "    float cam_shake_intensity = 0.0;\n"
      "    int cam_shake_timer = 0;\n"
      "    // [[ED_GLOBAL_END]]\n"
      "\n"
      "    // [[USER_GLOBAL_START]]\n"
      "    // [[USER_GLOBAL_END]]\n"
      "END\n"
      "\n"
      "// --------------------------------─------------------------\n"
      "// PROGRAMA PRINCIPAL\n"
      "// ---------------------------------------------------------\n"
      "PROCESS main()\n"
      "BEGIN\n"
      "    // [[ED_INIT_START]]\n"
      "    screen_w = {{SCREEN_WIDTH}};\n"
      "    screen_h = {{SCREEN_HEIGHT}};\n"
      "    move_speed = 8000;\n"
      "    rot_speed = 2000;\n"
      "    \n"
      "    say(\"--- \" + \"{{PROJECT_NAME}}\" + \" START ---\");\n"
      "    set_mode(screen_w, screen_h, {{FULLSCREEN_MODE}});\n"
      "    set_fps({{FPS}}, 0);\n"
      "    \n"
      "    // Audio\n"
      "    sound.freq = 44100;\n"
      "    sound.channels = 32;\n"
      "    soundsys_init();\n"
      "    \n"
      "    // Cargar recursos e inicializar rutas\n"
      "    load_project_resources();\n"
      "    npc_paths_init();\n"
      "    // [[ED_INIT_END]]\n"
      "\n"
      "    // [[USER_INIT_START]]\n"
      "    // [[USER_INIT_END]]\n"
      "\n"
      "    // [[ED_SPAWN_START]]\n"
      "    {{STARTUP_SCENE}}();\n"
      "    // [[ED_SPAWN_END]]\n"
      "\n"
      "    LOOP\n"
      "        // [[ED_MAIN_LOOP_START]]\n"
      "        if (key(_esc)) exit(); end\n"
      "        // {{MOVEMENT_LOGIC}}\n"
      "        // [[ED_MAIN_LOOP_END]]\n"
      "\n"
      "        // [[USER_MAIN_LOOP_START]]\n"
      "        // [[USER_MAIN_LOOP_END]]\n"
      "        \n"
      "        FRAME;\n"
      "    END\n"
      "END\n");
}

QString CodeGenerator::getPlayerTemplate() {
  return QString("process player(x, y, z)\n"
                 "PRIVATE\n"
                 "    int health = 100;\n"
                 "    float speed = 5.0;\n"
                 "BEGIN\n"
                 "    LOOP\n"
                 "        // Player logic here\n"
                 "        FRAME;\n"
                 "    END\n"
                 "END\n");
}

QString CodeGenerator::getEnemyTemplate() {
  return QString("process enemy(x, y, z)\n"
                 "PRIVATE\n"
                 "    int health = 50;\n"
                 "    float speed = 3.0;\n"
                 "BEGIN\n"
                 "    LOOP\n"
                 "        // Enemy AI here\n"
                 "        FRAME;\n"
                 "    END\n"
                 "END\n");
}

QString CodeGenerator::generateMainPrgWithEntities(
    const QVector<EntityInstance> &entities, const QVector<NPCPath> &npcPaths) {
  if (m_projectData.name.isEmpty()) {
    qWarning() << "No project data set for code generation";
    return "";
  }

  // Basic project variables
  setVariable("PROJECT_NAME", m_projectData.name);
  setVariable("DATE",
              QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss"));
  setVariable("SCREEN_WIDTH", QString::number(m_projectData.screenWidth));
  setVariable("SCREEN_HEIGHT", QString::number(m_projectData.screenHeight));
  setVariable("FPS", QString::number(m_projectData.fps));
  setVariable("FULLSCREEN_MODE",
              m_projectData.fullscreen ? "MODE_FULLSCREEN" : "MODE_WINDOW");

  // Entity Declarations (Legacy or for extra info)
  QString entityDeclarations =
      ProcessGenerator::generateDeclarationsSection(entities);
  setVariable("ENTITY_DECLARATIONS", entityDeclarations);

  // Generate entity processes (Inline code)
  QString wrapperOpen = m_variables.value("ASSET_WRAPPER_OPEN", "");
  QString wrapperClose = m_variables.value("ASSET_WRAPPER_CLOSE", "");
  QString entityProcesses = ProcessGenerator::generateAllProcessesCode(
      entities, wrapperOpen, wrapperClose);
  setVariable("ENTITY_PROCESSES", entityProcesses);

  // Generate NPC Paths code
  QString npcPathsCode = ProcessGenerator::generateNPCPathsCode(npcPaths);
  setVariable("NPC_PATHS_CODE", npcPathsCode);

  // Combine all inlined code (Resources, Scenes, Commons)
  QString allInlinedCode =
      m_inlineCommons + "\n" + m_inlineResources + "\n" + m_inlineScenes;
  setVariable("INLINE_RESOURCES", allInlinedCode);

  // Generate spawn calls
  QString spawnCalls = ProcessGenerator::generateSpawnCalls(entities);
  setVariable("SPAWN_ENTITIES", spawnCalls);

  // Default scene if not set
  if (m_variables.value("STARTUP_SCENE").isEmpty()) {
    setVariable("STARTUP_SCENE", "intro"); // Default
  }

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
    movement = "// Movimiento asistido por cámara\n"
               "        RAY_CAMERA_UPDATE(0.017);";
  } else {
    movement = "// Movimiento manual de cámara\n"
               "        IF (key(_w)) RAY_MOVE_FORWARD(move_speed); END\n"
               "        IF (key(_s)) RAY_MOVE_BACKWARD(move_speed); END\n"
               "        IF (key(_a)) RAY_STRAFE_LEFT(move_speed); END\n"
               "        IF (key(_d)) RAY_STRAFE_RIGHT(move_speed); END\n"
               "        IF (key(_left)) RAY_ROTATE(-rot_speed); END\n"
               "        IF (key(_right)) RAY_ROTATE(rot_speed); END\n"
               "        RAY_CAMERA_UPDATE(0.017);";
  }
  setVariable("MOVEMENT_LOGIC", movement);

  return processTemplate(getMainTemplate());
}

QString CodeGenerator::generateEntityModel(const QString &processName,
                                           const QString &modelPath) {
  QString wrapperOpen = m_variables.value("ASSET_WRAPPER_OPEN", "");
  QString wrapperClose = m_variables.value("ASSET_WRAPPER_CLOSE", "");

  // modelPath assumed relative (e.g. assets/foo.md3)
  QString loadStr =
      QString("%1\"%2\"%3").arg(wrapperOpen).arg(modelPath).arg(wrapperClose);

  return QString("PROCESS %1(x, y, z, angle);\n"
                 "PRIVATE\n"
                 "    int file_id = 0;\n"
                 "    int spr_id = 0;\n" // Added spr_id
                 "END\n"
                 "BEGIN\n"
                 "    file_id = load_md3(%2);\n" // Kept load_md3
                 "    IF (file_id > 0)\n"
                 "        spr_id = RAY_ADD_SPRITE(x, y, z, file_id, 0, 0, 0, "
                 "0);\n" // Changed to RAY_ADD_SPRITE with x,y,z
                 "    END\n"
                 "    LOOP\n"
                 "        FRAME;\n"
                 "    END\n"
                 "END\n")
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
      "    double x, y, z;\n"
      "    double yaw, pitch, roll;\n"
      "    double fov;\n"
      "    double time;\n"
      "    double duration;\n"
      "    int easeIn, easeOut;\n"
      "END\n" // Changed 'end' to 'END'
      "\n"
      "TYPE CameraPathData\n"
      "    int num_keyframes;\n"
      "    CameraKeyframe pointer keyframes;\n"
      "END\n" // Changed 'end' to 'END'
      "\n"
      "/* Load binary camera path (.cam) */\n"
      "function int LoadCameraPath(string filename, CameraPathData pointer "
      "out_data)\n"
      "PRIVATE\n"
      "    int f;\n"
      "    int count;\n"
      "    int i;\n"
      "BEGIN\n" // Changed 'begin' to 'BEGIN'
      "    f = fopen(filename, O_READ);\n"
      "    IF (f == 0) RETURN -1; END\n"
      "\n"
      "    fread(f, count);\n"
      "    out_data.num_keyframes = count;\n"
      "    IF (count > 0)\n"
      "        out_data.keyframes = alloc(count * sizeof(CameraKeyframe));\n"
      "    END\n"
      "\n"
      "    FOR (i=0; i<count; i++)\n"
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
      "    END\n"
      "\n"
      "    fclose(f);\n"
      "    RETURN 0;\n"
      // Changed 'return' to 'RETURN'
      "END\n" // Changed 'end' to 'END'
      "\n"
      "function FreeCameraPath(CameraPathData pointer data)\n"
      "BEGIN\n" // Changed 'begin' to 'BEGIN'
      "    IF (data.keyframes != NULL) free(data.keyframes); END\n" // Changed
                                                                    // 'if' to
                                                                    // 'IF',
                                                                    // 'end' to
                                                                    // 'END'
      "    data.num_keyframes = 0;\n"
      "END\n" // Changed 'end' to 'END'
      "\n"
      "/* Trigger PROCESS */\n"
      "process CameraTrigger(x, y, z, string file);\n"
      "PRIVATE\n"
      "    int player_id;\n"
      "    int dist;\n"
      "BEGIN\n"     // Changed 'begin' to 'BEGIN'
      "    LOOP\n"  // Changed 'loop' to 'LOOP'
      "    BEGIN\n" // Added BEGIN
      "        player_id = get_id(type player);\n"
      "        IF (player_id)\n" // Changed 'if' to 'IF'
      "        BEGIN\n"          // Added BEGIN
      "            dist = abs(player_id.x - x) + abs(player_id.y - y);\n"
      "            IF (dist < 64)\n" // Changed 'if' to 'IF'
      "            BEGIN\n"          // Added BEGIN
      "                // Start Cutscene\n"
      "                PlayCameraPath(file);\n"
      "                // Only run once?\n"
      "                BREAK;\n" // Changed 'break' to 'BREAK'
      "            END\n"        // Added END
      "        END\n"            // Added END
      "        FRAME;\n"
      "    END\n" // Changed 'end' to 'END'
      "END\n"     // Changed 'end' to 'END'
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
                                    const QVector<EntityInstance> &entities,
                                    const QVector<NPCPath> &npcPaths) {
  // 1. Generate clean code from Mega Template
  QString result = generateMainPrgWithEntities(entities, npcPaths);

  // 2. Map of blocks to preserve (User sections)
  // These are the areas where the user can legally write their own code
  QStringList userSections = {"USER_IMPORTS",   "USER_CONSTANTS",
                              "USER_PROCESSES", "USER_GLOBAL",
                              "USER_INIT",      "USER_MAIN_LOOP"};

  for (const QString &section : userSections) {
    QString startTag = "// [[" + section + "_START]]";
    QString endTag = "// [[" + section + "_END]]";

    int oldStart = existingCode.indexOf(startTag);
    int oldEnd = existingCode.indexOf(endTag);

    if (oldStart != -1 && oldEnd != -1 && oldStart < oldEnd) {
      // Extract user content from existing file
      int contentStart = oldStart + startTag.length();
      QString userContent =
          existingCode.mid(contentStart, oldEnd - contentStart);

      // Only inject if it's not just whitespace and comments
      if (userContent.trimmed().length() > 0) {
        int newStart = result.indexOf(startTag);
        int newEnd = result.indexOf(endTag);

        if (newStart != -1 && newEnd != -1) {
          int replaceStart = newStart + startTag.length();
          result.replace(replaceStart, newEnd - replaceStart, userContent);
        }
      }
    }
  }

  return result;
}

// ----------------------------------------------------------------------------
// 2D SCENE GENERATION
// ----------------------------------------------------------------------------

bool CodeGenerator::loadSceneJson(const QString &path, SceneData &data) {
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

  data.timeout = root["timeout"].toInt(0);
  data.nextScene = root["nextScene"].toString();

  QJsonArray entitiesArray = root["entities"].toArray();
  for (const QJsonValue &val : entitiesArray) {
    QJsonObject obj = val.toObject();

    SceneEntity *ent = new SceneEntity();
    ent->type = (SceneEntityType)obj["type"].toInt(ENTITY_SPRITE);
    ent->name = obj["name"].toString();
    ent->x = obj["x"].toDouble();
    ent->y = obj["y"].toDouble();
    ent->z = obj["z"].toDouble();
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
    } else if (ent->type == ENTITY_WORLD3D) {
      ent->sourceFile = obj["sourceFile"].toString();
    }

    // Visual Item (NONE) in generator
    ent->item = nullptr;

    data.entities.append(ent);
  }

  return true;
}

QString CodeGenerator::generateUserLogicStubs(const QStringList &processNames) {
  QString code =
      "// USER LOGIC - Edit this file to add custom behaviors\n"
      "// These hooks are called by the auto-generated processes\n\n";

  for (const QString &name : processNames) {
    QString lowerName = name.toLower();
    code += QString("// Hooks for %1\n").arg(lowerName);
    code +=
        QString("FUNCTION hook_%1_init(int p_id)\nBEGIN\n    // Called when "
                "entity is spawned\nEND\n\n")
            .arg(lowerName);
    code += QString("FUNCTION hook_%1_update(int p_id)\nBEGIN\n    // Called "
                    "every frame\nEND\n\n")
                .arg(lowerName);
  }

  return code;
}

QString CodeGenerator::generateScenePrg(const QString &sceneName,
                                        const SceneData &data,
                                        const QString &interactionMapPath,
                                        const QString &existingCode) {
  QString code;
  code += QString("PROCESS SCENE_%1()\n").arg(sceneName);
  code += "PRIVATE\n    int ent_id;\n    string w_title;\n";
  code += "    int mouse_last_state;\n";
  code += "    int scene_exit;\n";
  code += "    int fpg_map;\n";
  code += "    int spawn_ent_id;\n";
  code += "    int player_id;\n";
  if (data.timeout > 0 && !data.nextScene.isEmpty()) {
    code += "    int scene_timer;\n";
  }

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

  code += "BEGIN\n";
  code += "    mouse_last_state = 0;\n";
  code += "    scene_exit = 0;\n";
  if (data.timeout > 0 && !data.nextScene.isEmpty()) {
    code += "    scene_timer = 0;\n";
  }
  code += "    // Cleanup: Ensure we are the only process running\n";
  code += "    let_me_alone();\n";
  code += "    // Wait a frame to ensure cleanup propagates\n";
  code += "    FRAME;\n\n";

  code += "    // Load global resources (only loads if not already loaded)\n";
  code += "    load_project_resources();\n\n";

  code += "    // Scene Setup\n";
  code +=
      QString("    set_mode(%1, %2, 32);\n").arg(data.width).arg(data.height);

  // Build a resMap that maps resource files to their global variable names
  // (matching the names generated in generateAllScenes section 3)
  QMap<QString, QString> resMap;
  QSet<QString> loadedResources;
  for (const auto &ent : data.entities) {
    if (!ent->sourceFile.isEmpty() &&
        !loadedResources.contains(ent->sourceFile)) {
      QString cleanName = QFileInfo(ent->sourceFile).baseName().toLower();
      cleanName = cleanName.replace(".", "_").replace(" ", "_");
      QString ext = QFileInfo(ent->sourceFile).suffix().toLower();
      QString varName = "id_" + cleanName + "_" + ext;
      resMap[ent->sourceFile] = varName;
      loadedResources.insert(ent->sourceFile);
    }
    if (!ent->fontFile.isEmpty() && !loadedResources.contains(ent->fontFile)) {
      QString cleanName = QFileInfo(ent->fontFile).baseName().toLower();
      cleanName = cleanName.replace(".", "_").replace(" ", "_");
      QString ext = QFileInfo(ent->fontFile).suffix().toLower();
      QString varName = "id_" + cleanName + "_" + ext;
      resMap[ent->fontFile] = varName;
      loadedResources.insert(ent->fontFile);
    }
  }
  if (!data.musicFile.isEmpty() && !loadedResources.contains(data.musicFile)) {
    QString cleanName = QFileInfo(data.musicFile).baseName().toLower();
    cleanName = cleanName.replace(".", "_").replace(" ", "_");
    QString ext = QFileInfo(data.musicFile).suffix().toLower();
    QString varName = "id_" + cleanName + "_" + ext;
    resMap[data.musicFile] = varName;
    loadedResources.insert(data.musicFile);
  }
  if (!data.cursorFile.isEmpty() &&
      !loadedResources.contains(data.cursorFile)) {
    QString cleanName = QFileInfo(data.cursorFile).baseName().toLower();
    cleanName = cleanName.replace(".", "_").replace(" ", "_");
    QString ext = QFileInfo(data.cursorFile).suffix().toLower();
    QString varName = "id_" + cleanName + "_" + ext;
    resMap[data.cursorFile] = varName;
    loadedResources.insert(data.cursorFile);
  }

  // Set Cursor (using global resource variable)
  if (!data.cursorFile.isEmpty()) {
    QString ext = QFileInfo(data.cursorFile).suffix().toLower();
    QString resVar = resMap.value(data.cursorFile, "0");
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
      // ALWAYS generate a dedicated Auto_Btn process for text to ensure
      // Z-ordering (-500) works independent of the main 3D world process
      // (Z=1000).

      // Use scene name + unique index to allow multiple scenes with buttons
      QString btnProcName =
          QString("Auto_Btn_%1_%2").arg(sceneName).arg(currentInteractiveIdx++);
      // Sanitize name to be a valid identifier
      btnProcName =
          btnProcName.replace(" ", "_").replace("-", "_").replace(".", "_");

      code += QString("    // Text Button: %1\n").arg(ent->name);
      // Pass the font ID (variable name) as argument
      code += QString("    %1(%2);\n").arg(btnProcName).arg(fontId);

      QString btnCode;
      // Process updates to accept font_id
      btnCode += QString("PROCESS %1(int font_id)\n").arg(btnProcName);
      btnCode += "PRIVATE\n";
      btnCode += "    int txt_id;\n";
      btnCode += "    int w, h;\n";
      btnCode += "    int my_x, my_y;\n";
      btnCode += "begin\n";
      btnCode += QString("    my_x = %1; my_y = %2;\n")
                     .arg(int(ent->x))
                     .arg(int(ent->y));
      btnCode += "    z = -500;\n"; // Force foreground
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

      // Only generate click logic if there is an action or a destination
      if (!ent->onClick.isEmpty()) {
        // Logic based on Alignment for Hitbox Check
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
        btnCode += "        end\n";
      }

      btnCode += "        frame;\n";
      btnCode += "    end\n"; // End Loop
      btnCode += "OnExit:\n";
      btnCode += "    write_delete(txt_id);\n";
      btnCode += "end\n\n";

      m_inlineScenes += btnCode; // Append this process to the file

    } else if (ent->type == ENTITY_WORLD3D) {
      code += QString("\n    // 3D World Hybrid Entity: %1\n").arg(ent->name);
      code += "    RAY_SHUTDOWN();\n";
      code += QString("    RAY_INIT(%1, %2, 70, 1);\n")
                  .arg(data.width)
                  .arg(data.height);
      QString mapPath = ent->sourceFile;
      if (mapPath.contains("assets/"))
        mapPath = mapPath.mid(mapPath.lastIndexOf("assets/"));

      // Auto-load FPG textures (Assumed to be in assets/fpg/ with same name)
      QString fpgPath = "assets/fpg/" + QFileInfo(mapPath).baseName() + ".fpg";
      code += QString("    fpg_map = fpg_load(\"%1\");\n").arg(fpgPath);
      code +=
          "    if (fpg_map == 0) say(\"Warning: FPG texture file not found: " +
          fpgPath + "\"); end\n";

      code += QString("    if (RAY_LOAD_MAP(\"%1\", fpg_map) == 0) say(\"Error "
                      "loading hybrid 3D Map\"); end\n")
                  .arg(mapPath);

      // --- Auto-Spawn Entities from Map ---
      MapData internalData;
      RayMapFormat loader;
      // We need the full path to read the file NOW (during code generation)
      // Resolve absolute path for loading
      QString fullPath = ent->sourceFile;
      if (fullPath.contains("assets/")) {
        // Clean up doubled assets if necessary
        QString cleanPart = fullPath.mid(fullPath.lastIndexOf("assets/"));
        QString testPath = m_projectData.path + "/" + cleanPart;
        if (QFile::exists(testPath)) {
          fullPath = testPath;
        } else if (!QFileInfo(fullPath).isAbsolute() &&
                   !m_projectData.path.isEmpty()) {
          fullPath = m_projectData.path + "/" + fullPath;
        }
      } else if (!QFileInfo(fullPath).isAbsolute() &&
                 !m_projectData.path.isEmpty()) {
        fullPath = m_projectData.path + "/" + fullPath;
      }

      if (loader.loadMap(fullPath, internalData, nullptr)) {
        // Processes for these entities are now handled by autogen_entities.prg
        // via a centralized scan during project generation.

        if (!internalData.entities.isEmpty()) {
          code += "    // Spawning Map Entities (Auto-generated)\n";
          // Variables spawn_ent_id and player_id are now in PRIVATE section
          code += "    player_id = 0;\n";

          for (const auto &mapEnt : internalData.entities) {
            QString procName = mapEnt.processName;

            // Fallback if process name is empty (use asset name)
            if (procName.isEmpty()) {
              QFileInfo assetInfo(mapEnt.assetPath);
              procName = assetInfo.baseName();
            }
            if (procName.isEmpty())
              procName = "UnknownEntity";

            // Sanitize name
            procName =
                procName.replace(" ", "_").replace("-", "_").replace(".", "_");

            // Append spawn_id for unique per-instance process name
            QString uniqueProcName =
                procName + "_" + QString::number(mapEnt.spawn_id);

            // Generate Process Call
            // Assumptions: Process(x, y, z, angle)
            // Note: mapEnt.cameraRotation is float. Passing as argument.
            code += QString("    spawn_ent_id = %1(%2, %3, %4, %5);\n")
                        .arg(uniqueProcName)
                        .arg(mapEnt.x)
                        .arg(mapEnt.y)
                        .arg(mapEnt.z)
                        .arg(mapEnt.cameraRotation);

            // --- Handle Behaviors ---
            code += "    IF (spawn_ent_id > 0)\n";

            // Player Flag
            if (mapEnt.isPlayer) {
              code += "        // Is Player\n";
              // Assuming 'player_id' is a global or common var in game project
              code += "        player_id = spawn_ent_id;\n";
            }

            // Camera Follow
            if (mapEnt.cameraFollow) {
              code += "        // Camera Follow\n";
              // Assuming standard Ray function or custom helper
              code += QString("        // RAY_CAM_SET_TARGET(spawn_ent_id, %1, "
                              "%2, %3);\n")
                          .arg(mapEnt.cameraOffset_x)
                          .arg(mapEnt.cameraOffset_y)
                          .arg(mapEnt.cameraOffset_z);
              // Use a generic Ray function if known, or leave as comment for
              // user to uncomment/implement
            }

            // Control Type (Pass as Setup call?)
            if (mapEnt.controlType != EntityInstance::CONTROL_NONE) {
              code += QString("        // Control Type: %1\n")
                          .arg(mapEnt.controlType);
            }

            code += "    END\n";
          }
          code += "\n";
        }
      } else {
        // Optional warning in generated code comment
        code += QString("    // Warning: Could not read map file to spawn "
                        "entities: %1\n")
                    .arg(ent->sourceFile);
      }

      // Replace generic ray_display() with our dedicated renderer process
      code += QString("    Ray_Renderer_Process(%1, %2);\n\n")
                  .arg(data.width)
                  .arg(data.height);

      // Inject the process definition
      QString rendererCode;
      rendererCode += "PROCESS Ray_Renderer_Process(int w, int h)\n";
      rendererCode += "PRIVATE\n";
      rendererCode += "    float old_cx, old_cy, off_x, off_y;\n";
      rendererCode += "BEGIN\n";
      rendererCode += "    // Create rendering surface\n";
      rendererCode += "    graph = map_new(w, h, 32);\n";
      rendererCode += "    x = w / 2;\n";
      rendererCode += "    y = h / 2;\n";
      rendererCode += "    z = 1000; // Force 3D background depth\n";
      rendererCode += "    LOOP\n";
      rendererCode += "        RAY_PHYSICS_STEP(16.0);\n";
      rendererCode += "        IF (cam_shake_timer > 0)\n";
      rendererCode += "            old_cx = RAY_GET_CAMERA_X();\n";
      rendererCode += "            old_cy = RAY_GET_CAMERA_Y();\n";
      rendererCode += "            off_x = (rand(-100, 100) / 100.0) * "
                      "cam_shake_intensity;\n";
      rendererCode += "            off_y = (rand(-100, 100) / 100.0) * "
                      "cam_shake_intensity;\n";
      rendererCode += "            RAY_SET_CAMERA(old_cx + off_x, old_cy + "
                      "off_y, RAY_GET_CAMERA_Z(), RAY_GET_CAMERA_ROT(), "
                      "RAY_GET_CAMERA_PITCH());\n";
      rendererCode += "            RAY_RENDER(graph);\n";
      rendererCode +=
          "            RAY_SET_CAMERA(old_cx, old_cy, RAY_GET_CAMERA_Z(), "
          "RAY_GET_CAMERA_ROT(), RAY_GET_CAMERA_PITCH());\n";
      rendererCode += "            cam_shake_timer--;\n";
      rendererCode += "        ELSE\n";
      rendererCode += "            RAY_RENDER(graph);\n";
      rendererCode += "        END\n";
      rendererCode += "        FRAME;\n";
      rendererCode += "    END\n";
      rendererCode += "OnExit:\n";
      rendererCode += "    IF (graph > 0) map_unload(0, graph); END\n";
      rendererCode += "END\n\n";

      m_inlineScenes += rendererCode;
    }
  }

  // Inject User Setup
  code += "\n    // [[USER_SETUP]]" + userSetup + "// [[USER_SETUP_END]]\n";

  // Loop Logic
  code += "    LOOP\n";
  code += "        IF (scene_exit) BREAK; END\n";
  if (data.exitOnEsc) {
    code += "        IF (key(_esc)) exit(\"\", 0); END\n";
  }
  if (data.timeout > 0 && !data.nextScene.isEmpty()) {
    code += "        scene_timer++;\n";
    code += QString("        IF (scene_timer > %1) goto_scene(\"%2\"); END\n")
                .arg(data.timeout * 60)
                .arg(data.nextScene);
  }

  // 100% Robust Click Detection (No interaction map needed)
  if (!sceneEvents.isEmpty()) {
    code += "\n        // Robust Click Handling\n";
    code += "        IF (mouse.left && !mouse_last_state)\n";
    code += "            mouse_last_state = 1;\n";
    code += "            // Check each interactive entity\n";

    // We check events in reverse order (topmost/last created first)
    for (int i = sceneEvents.size() - 1; i >= 0; i--) {
      const auto &ev = sceneEvents[i];
      if (ev.isSprite) {
        code += QString("            IF (check_scene_click(%1, %2, %3, %4, "
                        "%5))\n")
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

        code += QString("            IF (mouse.x >= %1 && mouse.x <= %2 && "
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
      code += "                BREAK; // Stop after first click\n";
      code += "            END\n"; // Close the hit-test IF
    }
    code += "        END\n"; // Close the mouse.left IF
    code += "        IF (!mouse.left) mouse_last_state = 0; END\n";
  }

  // Inject User Loop
  code += "        // [[USER_LOOP]]" + userLoop + "// [[USER_LOOP_END]]\n";

  code += "        FRAME;\n";
  code += "    END\n";
  code += "END\n";

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
    m_inlineCommons += "FUNCTION string get_asset_path(string relative_path)\n";
    m_inlineCommons += "BEGIN\n";
    m_inlineCommons += "    IF (os_id == OS_ANDROID)\n";
    m_inlineCommons += "        RETURN \"/data/data/\" + \"" + pkgName +
                       "\" + \"/files/\" + relative_path;\n";
    m_inlineCommons += "    ELSE\n";
    m_inlineCommons += "        // Robust Desktop Path Check (src/ vs root)\n";
    m_inlineCommons += "        IF (!fexists(relative_path) && fexists(\"../\" "
                       "+ relative_path))\n";
    m_inlineCommons += "            RETURN \"../\" + relative_path;\n";
    m_inlineCommons += "        END\n";
    m_inlineCommons += "    END\n";
    m_inlineCommons += "    RETURN relative_path;\n";
    m_inlineCommons += "END\n\n";

    m_inlineCommons +=
        "PROCESS StaticSprite()\nBEGIN\n    LOOP\n        FRAME;\n    "
        "END\nEND\n\n";

    m_inlineCommons += "// Shared 3D Renderer Process\n";
    m_inlineCommons += "PROCESS ray_display()\nBEGIN\n    LOOP\n";
    m_inlineCommons += "        graph = RAY_RENDER(0);\n";
    m_inlineCommons += "        IF (graph)\n";
    m_inlineCommons += "            x = 320; y = 240; // Default center\n";
    m_inlineCommons += "        END\n";
    m_inlineCommons += "        FRAME;\n";
    m_inlineCommons += "    END\n";
    m_inlineCommons += "END\n\n";
    m_inlineCommons += "// Shared 2D Click Detection Helper\n";
    m_inlineCommons += "FUNCTION int check_scene_click(int id, int hw, int hh, "
                       "int hx, int hy)\n";
    m_inlineCommons += "PRIVATE\n    int w, h, xmin, ymin, xmax, ymax;\n";
    m_inlineCommons += "BEGIN\n";
    m_inlineCommons += "    IF (id == 0 || !exists(id)) RETURN 0; END\n\n";
    m_inlineCommons += "    IF (hw > 0 && hh > 0)\n";
    m_inlineCommons += "        w = hw; h = hh;\n";
    m_inlineCommons += "    ELSE\n";
    m_inlineCommons += "        // Auto size from graphic (if exists)\n";
    m_inlineCommons +=
        "        w = graphic_info(id.file, id.graph, G_WIDTH);\n";
    m_inlineCommons +=
        "        h = graphic_info(id.file, id.graph, G_HEIGHT);\n";
    m_inlineCommons += "        IF (w <= 0) w = 64; END \n";
    m_inlineCommons += "        IF (h <= 0) h = 32; END\n";
    m_inlineCommons += "    END\n\n";
    m_inlineCommons += "    // Scale Adjustment\n";
    m_inlineCommons += "    w = w * id.size_x / 100;\n";
    m_inlineCommons += "    // Simplified Check:\n";
    m_inlineCommons += "    // If it is a process with a graphic (id > 0), use "
                       "native collision!\n";
    m_inlineCommons +=
        "    // This handles rotation, scale and transparency automatically.\n";
    m_inlineCommons += "    IF (id > 0 && EXISTS(id))\n";
    m_inlineCommons += "        RETURN collision(type mouse);\n";
    m_inlineCommons += "    END\n\n";
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
    m_inlineCommons += "    IF (mouse.x >= xmin && mouse.x <= xmax && mouse.y "
                       ">= ymin && mouse.y <= ymax)\n";
    m_inlineCommons += "        RETURN 1;\n";
    m_inlineCommons += "    END\n";
    m_inlineCommons += "    RETURN 0;\n";
    m_inlineCommons += "END\n\n";
    m_inlineCommons += "PROCESS Billboard_Effect_Process(float px, float py, "
                       "float pz, int file, "
                       "int g_start, int g_end, float speed, float scale)\n";
    m_inlineCommons +=
        "PRIVATE\n    int spr_id;\n    int cur_g;\n    float timer;\n";
    m_inlineCommons += "BEGIN\n    timer = 0;\n";
    m_inlineCommons += "    cur_g = g_start;\n";
    m_inlineCommons +=
        "    spr_id = RAY_ADD_SPRITE(px, py, pz, file, cur_g, 0, 0, 0);\n";
    m_inlineCommons += "    RAY_SET_SPRITE_SCALE(spr_id, scale);\n";
    m_inlineCommons += "    WHILE (cur_g <= g_end)\n";
    m_inlineCommons += "        RAY_SET_SPRITE_GRAPH(spr_id, cur_g);\n";
    m_inlineCommons += "        timer = 0;\n";
    m_inlineCommons +=
        "        WHILE (timer < speed) timer += 0.016; FRAME; END\n";
    m_inlineCommons += "        cur_g++;\n";
    m_inlineCommons += "    END\n";
    m_inlineCommons += "    RAY_REMOVE_SPRITE(spr_id);\n";
    m_inlineCommons += "END\n\n";
  }

  // ---------------------------------------------------------
  // 2. COLLECT RESOURCES & GENERATE SCENES
  // ---------------------------------------------------------
  // 2. COLLECT RESOURCES & GENERATE SCENES
  QStringList filters;
  filters << "*.scn" << "*.scene";

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
      if (sn.toLower() == currentStart.toLower() ||
          ("scene_" + sn.toLower()) == currentStart.toLower()) {
        found = true;
        // Ensure exact casing matches process name with prefix
        setVariable("STARTUP_SCENE", "scene_" + sn.toLower());
        break;
      }
    }
    if (!found) {
      // Fallback to first available scene
      setVariable("STARTUP_SCENE", "scene_" + sceneNames.first().toLower());
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
    m_inlineResources += "END\n\n";

    // Load Function
    m_inlineResources += "FUNCTION load_project_resources()\nBEGIN\n";
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
        m_inlineResources += "    IF (" + varName + " <= 0) " + varName +
                             " = " + loadFunc + "(" + assetOpen + "\"" + res +
                             "\"" + assetClose + "); END\n";
        m_inlineResources += "    IF (" + varName +
                             " > 0) say(\"Loaded resource: " + res +
                             " ID: \" + " + varName + "); END\n";
      }
    }
    m_inlineResources += "END\n\n";

    // Unload Function
    m_inlineResources += "FUNCTION unload_project_resources()\nBEGIN\n";
    for (auto it = resMap.begin(); it != resMap.end(); ++it) {
      QString res = it.key();
      QString varName = it.value();
      QString ext = QFileInfo(res).suffix().toLower();
      if (ext == "fpg")
        m_inlineResources +=
            "    IF(" + varName + ">0) fpg_unload(" + varName + "); END\n";
      else if (ext == "fnt" || ext == "fnx")
        m_inlineResources +=
            "    IF(" + varName + ">0) fnt_unload(" + varName + "); END\n";
      else if (ext == "mp3" || ext == "ogg" || ext == "wav")
        m_inlineResources +=
            "    IF(" + varName + ">0) music_unload(" + varName + "); END\n";
      else if (ext == "png" || ext == "jpg" || ext == "jpeg" || ext == "bmp" ||
               ext == "tga") {
        m_inlineResources +=
            "    IF(" + varName + ">0) map_unload(0, " + varName + "); END\n";
      }
      m_inlineResources += "    " + varName + " = 0;\n";
    }
    m_inlineResources += "END\n\n";
  }

  // Scene Navigation Helper (Inlined into scenes block or resources block?
  // Scenes block seems mostly appropriate)
  {
    m_inlineScenes += "\n// Scene Navigation Helper\n";
    m_inlineScenes += "FUNCTION goto_scene(string name)\nBEGIN\n";
    m_inlineScenes += "    // Stop music and clean up previous scene\n";
    m_inlineScenes += "    music_stop();\n";
    m_inlineScenes += "    let_me_alone();\n";
    m_inlineScenes += "    write_delete(all_text);\n\n";
    for (const QString &sn : sceneNames)
      m_inlineScenes += "    IF (name == \"" + sn + "\") scene_" +
                        sn.toLower() + "(); RETURN; END\n";
    m_inlineScenes += "    say(\"Scene not found: \" + name);\nEND\n";
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
