#include "codegenerator.h"
#include "processgenerator.h"
#include <QDateTime>
#include <QDebug>

CodeGenerator::CodeGenerator()
{
}

void CodeGenerator::setProjectData(const ProjectData &data)
{
    m_projectData = data;
    
    // Set all variables from project data
    setVariable("PROJECT_NAME", m_projectData.name);
    setVariable("PROJECT_VERSION", m_projectData.version);
    setVariable("SCREEN_WIDTH", QString::number(m_projectData.screenWidth));
    setVariable("SCREEN_HEIGHT", QString::number(m_projectData.screenHeight));
    setVariable("RENDER_WIDTH", QString::number(m_projectData.renderWidth));
    setVariable("RENDER_HEIGHT", QString::number(m_projectData.renderHeight));
    setVariable("FPS", QString::number(m_projectData.fps));
    setVariable("FOV", QString::number(m_projectData.fov));
    setVariable("RAYCAST_QUALITY", QString::number(m_projectData.raycastQuality));
    setVariable("FPG_PATH", m_projectData.fpgFile.isEmpty() ? "assets.fpg" : m_projectData.fpgFile);
    setVariable("INITIAL_MAP", m_projectData.initialMap.isEmpty() ? "map.raymap" : m_projectData.initialMap);
    setVariable("CAM_X", QString::number(m_projectData.cameraX));
    setVariable("CAM_Y", QString::number(m_projectData.cameraY));
    setVariable("CAM_Z", QString::number(m_projectData.cameraZ));
    setVariable("CAM_ROT", QString::number(m_projectData.cameraRot));
    setVariable("CAM_PITCH", QString::number(m_projectData.cameraPitch));
    setVariable("FULLSCREEN_MODE", m_projectData.fullscreen ? "MODE_FULLSCREEN" : "MODE_WINDOW");
    setVariable("DATE", QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss"));
    setVariable("PACKAGE_NAME", m_projectData.packageName.isEmpty() ? "com.example.game" : m_projectData.packageName);

    // Android Helper Code Logic
    if (m_projectData.androidSupport) {
        QString helperCode = 
            "// Helper para rutas Android\n"
            "// Se usa ruta absoluta hardcodeada basada en el nombre del paquete\n"
            "Function string get_asset_path(string relative_path)\n"
            "Begin\n"
            "    if (os_id == OS_ANDROID)\n"
            "        return \"/data/data/{{PACKAGE_NAME}}/files/\" + relative_path;\n"
            "    end\n"
            "    return relative_path;\n"
            "End";
            
        setVariable("ANDROID_HELPER_CODE", helperCode);
        setVariable("ASSET_WRAPPER_OPEN", "get_asset_path(");
        setVariable("ASSET_WRAPPER_CLOSE", ")");
    } else {
        setVariable("ANDROID_HELPER_CODE", "");
        setVariable("ASSET_WRAPPER_OPEN", "");
        setVariable("ASSET_WRAPPER_CLOSE", "");
    }
}

void CodeGenerator::setVariable(const QString &name, const QString &value)
{
    m_variables[name] = value;
}

QString CodeGenerator::processTemplate(const QString &templateText)
{
    QString result = templateText;
    
    // Replace all variables {{VAR_NAME}} with their values
    for (auto it = m_variables.begin(); it != m_variables.end(); ++it) {
        QString placeholder = "{{" + it.key() + "}}";
        result.replace(placeholder, it.value());
    }
    
    return result;
}

QString CodeGenerator::generateMainPrg()
{
    if (m_projectData.name.isEmpty()) {
        qWarning() << "No project data set for code generation";
        return "";
    }
    
    QString template_text = getMainTemplate();
    
    // Add entity spawns (placeholder for now)
    setVariable("SPAWN_ENTITIES", "// TODO: Add entity spawns here");
    
    // Add entity processes (placeholder for now)
    setVariable("ENTITY_PROCESSES", "// TODO: Add entity process definitions here");
    
    return processTemplate(template_text);
}

QString CodeGenerator::generateEntityProcess(const QString &entityName, const QString &entityType)
{
    if (entityType == "player") {
        return processTemplate(getPlayerTemplate());
    } else if (entityType == "enemy") {
        return processTemplate(getEnemyTemplate());
    }
    
    // Generic entity
    QString code = QString(
        "PROCESS %1(x, y, z)\n"
        "PRIVATE\n"
        "    int health = 100;\n"
        "END\n"
        "BEGIN\n"
        "    LOOP\n"
        "        // TODO: Add entity logic\n"
        "        FRAME;\n"
        "    END\n"
        "END\n"
    ).arg(entityName);
    
    return code;
}

QString CodeGenerator::getMainTemplate()
{
    return QString(
        "// Auto-generado por RayMap Editor\n"
        "// Proyecto: {{PROJECT_NAME}}\n"
        "// Fecha: {{DATE}}\n"
        "\n"
        "import \"libmod_gfx\";\n"
        "import \"libmod_input\";\n"
        "import \"libmod_misc\";\n"
        "import \"libmod_ray\";\n"
        "\n"
        "{{ANDROID_HELPER_CODE}}\n"
        "\n"
        "// Constantes de entidad\n"
        "// [[ED_CONSTANTS_START]]\n"
        "CONST\n"
        "    TYPE_PLAYER = 1;\n"
        "    TYPE_ENEMY  = 2;\n"
        "    TYPE_OBJECT = 3;\n"
        "    TYPE_TRIGGER = 4;\n"
        "END\n"
        "// [[ED_CONSTANTS_END]]\n"
        "\n"
        "// [[ED_DECLARATIONS_START]]\n"
        "{{ENTITY_DECLARATIONS}}\n"
        "// [[ED_DECLARATIONS_END]]\n"
        "\n"
        "GLOBAL\n"
        "    int screen_w = {{SCREEN_WIDTH}};\n"
        "    int screen_h = {{SCREEN_HEIGHT}};\n"
        "    int fpg_textures;\n"
        "    int fog_enabled = 0;\n"
        "    int minimap_enabled = 1;\n"
        "\n"
        "    // Variables globales de mundo\n"
        "    float WORLD_X, WORLD_Y, WORLD_Z;\n"
        "END\n"
        "\n"
        "PROCESS main()\n"
        "PRIVATE\n"
        "    float move_speed = 5.0;\n"
        "    float rot_speed = 0.05;\n"
        "    float pitch_speed = 0.02;\n"
        "    int fog_key_pressed = 0;\n"
        "    int minimap_key_pressed = 0;\n"
        "BEGIN\n"
        "    // Inicializar pantalla\n"
        "    set_mode(screen_w, screen_h, {{FULLSCREEN_MODE}});\n"
        "    set_fps({{FPS}}, 0);\n"
        "    window_set_title(\"{{PROJECT_NAME}}\");\n"
        "\n"
        "    // Cargar FPG de texturas\n"
        "    fpg_textures = fpg_load({{ASSET_WRAPPER_OPEN}}\"{{FPG_PATH}}\"{{ASSET_WRAPPER_CLOSE}});\n"
        "    if (fpg_textures < 0)\n"
        "        say(\"ERROR: No se pudo cargar FPG\");\n"
        "        exit();\n"
        "    end\n"
        "\n"
        "    // Inicializar motor raycasting\n"
        "    // Usa resolución de renderizado interna (puede ser menor que la ventana para mejor rendimiento)\n"
        "    if (RAY_INIT({{RENDER_WIDTH}}, {{RENDER_HEIGHT}}, {{FOV}}, {{RAYCAST_QUALITY}}) == 0)\n"
        "        say(\"ERROR: No se pudo inicializar motor\");\n"
        "        exit();\n"
        "    end\n"
        "\n"
        "    // Cargar mapa inicial\n"
        "    if (RAY_LOAD_MAP({{ASSET_WRAPPER_OPEN}}\"{{INITIAL_MAP}}\"{{ASSET_WRAPPER_CLOSE}}, fpg_textures) == 0)\n"
        "        say(\"ERROR: No se pudo cargar mapa\");\n"
        "        RAY_SHUTDOWN();\n"
        "        exit();\n"
        "    end\n"
        "\n"
        "    // Configuración Inicial\n"
        "    RAY_SET_FOG(fog_enabled, 0, 0, 0, 0, 0);\n"
        "    RAY_SET_DRAW_MINIMAP(minimap_enabled);\n"
        "\n"
        "    // Configurar cámara inicial (comentado - el mapa ya tiene la cámara configurada)\n"
        "    // RAY_SET_CAMERA({{CAM_X}}, {{CAM_Y}}, {{CAM_Z}}, {{CAM_ROT}}, {{CAM_PITCH}});\n"
        "\n"
        "\n"
        "\n"
        "    // Iniciar renderizado\n"
        "    ray_display();\n"
        "\n"
        "    // [[ED_SPAWN_START]]\n"
        "    {{SPAWN_ENTITIES}}\n"
        "    // [[ED_SPAWN_END]]\n"
        "\n"
        "    // Loop principal\n"
        "    LOOP\n"
        "        // [[ED_CONTROL_START]]\n"
        "        {{MOVEMENT_LOGIC}}\n"
        "        // [[ED_CONTROL_END]]\n"
        "\n"
        "        // Salto\n"
        "       \n"
        "\n"
        "        if (key(_esc)) let_me_alone(); exit(\"\", 0); end\n"
        "\n"
        "        FRAME;\n"
        "    END\n"
        "\n"
        "    // Cleanup\n"
        "    RAY_FREE_MAP();\n"
        "    RAY_SHUTDOWN();\n"
        "    fpg_unload(fpg_textures);\n"
        "END\n"
        "\n"
        "PROCESS ray_display()\n"
        "BEGIN\n"
        "    LOOP\n"
        "        graph = RAY_RENDER(0);\n"
        "        if (graph)\n"
        "            x = screen_w / 2;\n"
        "            y = screen_h / 2;\n"
        "        end\n"
        "        FRAME;\n"
        "    END\n"
        "END\n"
        "\n"
        "// [[ED_PROCESSES_START]]\n"
        "{{ENTITY_PROCESSES}}\n"
        "// [[ED_PROCESSES_END]]\n"
    );
}

QString CodeGenerator::getPlayerTemplate()
{
    return QString(
        "PROCESS player(x, y, z)\n"
        "PRIVATE\n"
        "    int health = 100;\n"
        "    float speed = 5.0;\n"
        "END\n"
        "BEGIN\n"
        "    LOOP\n"
        "        // Player logic here\n"
        "        FRAME;\n"
        "    END\n"
        "END\n"
    );
}

QString CodeGenerator::getEnemyTemplate()
{
    return QString(
        "PROCESS enemy(x, y, z)\n"
        "PRIVATE\n"
        "    int health = 50;\n"
        "    float speed = 3.0;\n"
        "END\n"
        "BEGIN\n"
        "    LOOP\n"
        "        // Enemy AI here\n"
        "        FRAME;\n"
        "    END\n"
        "END\n"
    );
}

QString CodeGenerator::generateMainPrgWithEntities(const QVector<EntityInstance> &entities)
{
    if (m_projectData.name.isEmpty()) {
        qWarning() << "No project data set for code generation";
        return "";
    }
    
    // Generate entity declarations
    QString entityDeclarations = ProcessGenerator::generateDeclarationsSection(entities);
    setVariable("ENTITY_DECLARATIONS", entityDeclarations);
    
    // Generate entity processes (Inline instead of includes)
    QString wrapperOpen = m_variables.value("ASSET_WRAPPER_OPEN", "");
    QString wrapperClose = m_variables.value("ASSET_WRAPPER_CLOSE", "");
    QString entityProcesses = ProcessGenerator::generateAllProcessesCode(entities, wrapperOpen, wrapperClose);
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
        movement =  "        // Movimiento libre de cámara\n"
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

QString CodeGenerator::generateEntityModel(const QString &processName, const QString &modelPath)
{
    QString wrapperOpen = m_variables.value("ASSET_WRAPPER_OPEN", "");
    QString wrapperClose = m_variables.value("ASSET_WRAPPER_CLOSE", "");
    
    // modelPath assumed relative (e.g. assets/foo.md3)
    QString loadStr = QString("%1\"%2\"%3").arg(wrapperOpen).arg(modelPath).arg(wrapperClose);
    
    return QString(
        "PROCESS %1(x, y, z, angle)\n"
        "PRIVATE\n"
        "    int file_id = 0;\n"
        "END\n"
        "BEGIN\n"
        "    file_id = load_md3(%2);\n"
        "    if (file_id > 0)\n"
        "        graph = file_id;\n"
        "    end\n"
        "    LOOP\n"
        "        FRAME;\n"
        "    END\n"
        "END\n"
    ).arg(processName).arg(loadStr);
}

QString CodeGenerator::generateCameraController()
{
    return getCameraControllerTemplate();
}

QString CodeGenerator::getCameraControllerTemplate()
{
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
        "END\n"
        "\n"
        "TYPE CameraPathData\n"
        "    int num_keyframes;\n"
        "    CameraKeyframe pointer keyframes;\n"
        "END\n"
        "\n"
        "/* Load binary camera path (.cam) */\n"
        "FUNCTION int LoadCameraPath(string filename, CameraPathData pointer out_data)\n"
        "PRIVATE\n"
        "    int f;\n"
        "    int count;\n"
        "    int i;\n"
        "BEGIN\n"
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
        "END\n"
        "\n"
        "FUNCTION FreeCameraPath(CameraPathData pointer data)\n"
        "BEGIN\n"
        "    if (data.keyframes != NULL) free(data.keyframes); end\n"
        "    data.num_keyframes = 0;\n"
        "END\n"
        "\n"
        "/* Trigger Process */\n"
        "PROCESS CameraTrigger(x, y, z, string file)\n"
        "PRIVATE\n"
        "    int player_id;\n"
        "    int dist;\n"
        "BEGIN\n"
        "    LOOP\n"
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
        "        FRAME;\n"
        "    END\n"
        "END\n"
        "\n"
        "// Placeholder for PlayCameraPath implementation\n"
        "PROCESS PlayCameraPath(string filename)\n"
        "PRIVATE\n"
        "    CameraPathData data;\n"
        "BEGIN\n"
        "    if (LoadCameraPath(filename, &data) < 0) return; end\n"
        "    // TODO: Interpolation loop\n"
        "    say(\"Playing cutscene: \" + filename);\n"
        "    // ...\n"
        "    FreeCameraPath(&data);\n"
        "END\n"
        "\n"
        "#endif\n"
    );
}

QString CodeGenerator::generateCameraPathData(const QString &pathName, const struct CameraPath &path)
{
    // Implementation needed if generating .h files for paths
    return ""; 
}
QString CodeGenerator::patchMainPrg(const QString &existingCode, const QVector<EntityInstance> &entities)
{
    QString newFullCode = generateMainPrgWithEntities(entities);
    
    // If the existing file doesn't have any markers, it's safer to return the existing code
    // to prevent overwriting manual changes.
    if (!existingCode.contains("// [[ED_CONSTANTS_START]]")) {
        return existingCode;
    }
    
    QString result = existingCode;
    auto replaceBlock = [&](const QString &startMarker, const QString &endMarker, bool protectIfHasContent = false) {
        int newStart = newFullCode.indexOf(startMarker);
        int newEnd = newFullCode.indexOf(endMarker);
        if (newStart == -1 || newEnd == -1) return;
        newEnd += endMarker.length();
        QString newBlock = newFullCode.mid(newStart, newEnd - newStart);
        
        int oldStart = result.indexOf(startMarker);
        int oldEnd = result.indexOf(endMarker);
        if (oldStart != -1 && oldEnd != -1) {
            // Protect user's manual code
            if (protectIfHasContent) {
                int contentStart = oldStart + startMarker.length();
                QString existingContent = result.mid(contentStart, oldEnd - contentStart).trimmed();
                // If there's ANY meaningful content, don't touch it
                if (existingContent.length() > 10) {
                    return; // Skip - user has code here
                }
            }
            
            oldEnd += endMarker.length();
            result.replace(oldStart, oldEnd - oldStart, newBlock);
        }
    };

    replaceBlock("// [[ED_CONSTANTS_START]]", "// [[ED_CONSTANTS_END]]");
    replaceBlock("// [[ED_DECLARATIONS_START]]", "// [[ED_DECLARATIONS_END]]");
    replaceBlock("// [[ED_CONTROL_START]]", "// [[ED_CONTROL_END]]");
    replaceBlock("// [[ED_SPAWN_START]]", "// [[ED_SPAWN_END]]");
    replaceBlock("// [[ED_PROCESSES_START]]", "// [[ED_PROCESSES_END]]", true); // NEVER overwrite if has content
    
    return result;
}
