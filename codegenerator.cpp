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
        "import \"mod_gfx\";\n"
        "import \"mod_input\";\n"
        "import \"mod_misc\";\n"
        "import \"libmod_ray\";\n"
        "\n"
        "{{ANDROID_HELPER_CODE}}\n"
        "\n"
        "{{ENTITY_INCLUDES}}\n"
        "\n"
        "GLOBAL\n"
        "    int screen_w = {{SCREEN_WIDTH}};\n"
        "    int screen_h = {{SCREEN_HEIGHT}};\n"
        "    int fpg_textures;\n"
        "    int fog_enabled = 0;\n"
        "    int minimap_enabled = 1;\n"
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
        "    {{SPAWN_ENTITIES}}\n"
        "\n"
        "    // Loop principal\n"
        "    LOOP\n"
        "        // Movimiento\n"
        "        if (key(_w)) RAY_MOVE_FORWARD(move_speed); end\n"
        "        if (key(_s)) RAY_MOVE_BACKWARD(move_speed); end\n"
        "        if (key(_a)) RAY_STRAFE_LEFT(move_speed); end\n"
        "        if (key(_d)) RAY_STRAFE_RIGHT(move_speed); end\n"
        "\n"
        "        // Cámara\n"
        "        if (key(_left)) RAY_ROTATE(-rot_speed); end\n"
        "        if (key(_right)) RAY_ROTATE(rot_speed); end\n"
        "        if (key(_up)) RAY_LOOK_UP_DOWN(pitch_speed); end\n"
        "        if (key(_down)) RAY_LOOK_UP_DOWN(-pitch_speed); end\n"
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
    
    // Generate entity includes
    QString entityIncludes = ProcessGenerator::generateIncludesSection(entities);
    setVariable("ENTITY_INCLUDES", entityIncludes);
    
    // Generate spawn calls
    QString spawnCalls = ProcessGenerator::generateSpawnCalls(entities);
    setVariable("SPAWN_ENTITIES", spawnCalls);
    
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
