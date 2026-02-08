#include "projectmanager.h"
#include "codegenerator.h"
#include <QDir>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QFile>
#include <QDirIterator>
#include <QDebug>

ProjectManager::ProjectManager(QObject *parent) : QObject(parent), m_project(nullptr)
{
}

bool ProjectManager::createProject(const QString &path, const QString &name)
{
    if (m_project) delete m_project;
    m_project = new Project();
    m_project->name = name;
    m_project->path = path;
    
    // Create directory if it doesn't exist
    QDir dir(path);
    if (!dir.exists()) {
        dir.mkpath(".");
    }
    
    // Create src directory
    dir.mkpath("src");
    dir.mkpath("assets");
    
    // Generate default main.prg
    ProjectData defaultData;
    defaultData.name = name;
    defaultData.path = path;
    defaultData.startupScene = "scene1";
    
    // Create default scene
    QJsonObject sceneJson;
    sceneJson["width"] = 320;
    sceneJson["height"] = 240;
    sceneJson["entities"] = QJsonArray(); // Empty for now
    
    QFile sceneFile(path + "/scene1.scn");
    if(sceneFile.open(QIODevice::WriteOnly)) {
        sceneFile.write(QJsonDocument(sceneJson).toJson());
        sceneFile.close();
    }
    
    CodeGenerator generator;
    generator.setProjectData(defaultData);
    
    QString mainCode = generator.generateMainPrg();
    QFile srcFile(path + "/src/main.prg");
    if (srcFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
        srcFile.write(mainCode.toUtf8());
        srcFile.close();
    }
    
    // Generate scene scripts
    generator.generateAllScenes(path);
    
    // Save project file
    QJsonObject obj;
    obj["name"] = name;
    obj["version"] = "1.0";
    
    QJsonDocument doc(obj);
    QFile file(path + "/" + name + ".bgd2proj");
    if (file.open(QIODevice::WriteOnly)) {
        file.write(doc.toJson());
        file.close();
        
        // Also ensure createProject implies opening it? 
        // Logic usually separates creation from opening, but we can set the current logic to "open" it right here by setting m_project values correctly which we did.
        // m_project->name and m_project->path are set.
        return true;
    }
    return false;
}

bool ProjectManager::openProject(const QString &fileName)
{
    QFile file(fileName);
    if (!file.open(QIODevice::ReadOnly)) return false;
    
    QByteArray data = file.readAll();
    file.close();
    
    QJsonDocument doc = QJsonDocument::fromJson(data);
    if (doc.isNull()) return false;
    
    if (m_project) delete m_project;
    m_project = new Project();
    
    QJsonObject obj = doc.object();
    m_project->name = obj["name"].toString();
    m_project->path = QFileInfo(fileName).path();
    
    return true;
}

void ProjectManager::closeProject()
{
    if (m_project) {
        delete m_project;
        m_project = nullptr;
    }
}

bool ProjectManager::hasProject() const
{
    return m_project != nullptr;
}

const Project* ProjectManager::getProject() const
{
    return m_project;
}

QString ProjectManager::getProjectPath() const
{
    if (m_project) return m_project->path;
    return QString();
}

ProjectData ProjectManager::loadProjectData(const QString &projectPath)
{
    ProjectData data;
    
    // Set defaults
    data.path = projectPath;
    data.screenWidth = 800;
    data.screenHeight = 600;
    data.renderWidth = 800;
    data.renderHeight = 600;
    data.fps = 60;
    
    // Try to load from config file
    QString configPath = projectPath + "/project_config.json";
    QFile configFile(configPath);
    
    if (configFile.open(QIODevice::ReadOnly)) {
        QByteArray jsonData = configFile.readAll();
        configFile.close();
        
        QJsonDocument doc = QJsonDocument::fromJson(jsonData);
        if (!doc.isNull() && doc.isObject()) {
            QJsonObject config = doc.object();
            
            // Load all values (with defaults if not present)
            data.name = config.value("name").toString(data.name);
            data.version = config.value("version").toString("1.0");
            data.startupScene = config.value("startupScene").toString("");
            
            // Fallback: If no startup scene defined, try to find one
            if (data.startupScene.isEmpty()) {
                QDirIterator it(projectPath, QStringList() << "*.scn", QDir::Files, QDirIterator::Subdirectories);
                if (it.hasNext()) {
                    data.startupScene = QFileInfo(it.next()).baseName();
                } else {
                    data.startupScene = "scene1"; // Last resort
                }
            }
            
            data.screenWidth = config.value("screenWidth").toInt(data.screenWidth);
            data.screenHeight = config.value("screenHeight").toInt(data.screenHeight);
            data.renderWidth = config.value("renderWidth").toInt(data.renderWidth);
            data.renderHeight = config.value("renderHeight").toInt(data.renderHeight);
            data.fps = config.value("fps").toInt(data.fps);
            
            data.fullscreen = config.value("fullscreen").toBool(data.fullscreen);
            data.packageName = config.value("packageName").toString("com.example.game");
            data.androidSupport = config.value("androidSupport").toBool(true);
            
            qDebug() << "Loaded project configuration from" << configPath;
        }
    } else {
        qDebug() << "No project configuration found at" << configPath << "- using defaults";
    }
    
    return data;
}

bool ProjectManager::saveProjectData(const QString &projectPath, const ProjectData &data)
{
    QJsonObject config;
    config["name"] = data.name;
    config["version"] = data.version;
    config["startupScene"] = data.startupScene;
    config["screenWidth"] = data.screenWidth;
    config["screenHeight"] = data.screenHeight;
    config["renderWidth"] = data.renderWidth;
    config["renderHeight"] = data.renderHeight;
    config["fps"] = data.fps;
    config["fullscreen"] = data.fullscreen;
    config["packageName"] = data.packageName;
    config["androidSupport"] = data.androidSupport;
    
    QJsonDocument doc(config);
    QString configPath = projectPath + "/project_config.json";
    QFile configFile(configPath);
    if (configFile.open(QIODevice::WriteOnly)) {
        configFile.write(doc.toJson());
        configFile.close();
        return true;
    }
    return false;
}
