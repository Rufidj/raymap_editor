#ifndef CODEGENERATOR_H
#define CODEGENERATOR_H

#include <QString>
#include <QMap>
#include <QVector>
#include "projectmanager.h"
#include "mapdata.h"
struct SceneData;

class CodeGenerator
{
public:
    CodeGenerator();
    
    // Set project data
    void setProjectData(const ProjectData &data);
    
    // Template system
    void setVariable(const QString &name, const QString &value);
    QString processTemplate(const QString &templateText);
    
    // Main generation
    QString generateMainPrg();
    QString generateMainPrgWithEntities(const QVector<EntityInstance> &entities);
    QString patchMainPrg(const QString &existingCode, const QVector<EntityInstance> &entities);
    
    QString getWrapperOpen() const { return m_variables.value("ASSET_WRAPPER_OPEN"); }
    QString getWrapperClose() const { return m_variables.value("ASSET_WRAPPER_CLOSE"); }
    
    // Entity code generation
    QString generateEntityProcess(const QString &entityName, const QString &entityType);
    QString generateEntityModel(const QString &processName, const QString &modelPath);
    
    // Camera System
    QString generateCameraController(); // Generates includes/camera_controller.h
    QString generateCameraPathData(const QString &pathName, const struct CameraPath &path);

    // 2D Scene System
    QString generateScenePrg(const QString &sceneName, const SceneData &data, const QString &existingCode = QString());
    void generateAllScenes(const QString &projectPath); // Scan and generate all .scn code
    bool patchMainIncludeScenes(const QString &projectPath);
    bool setStartupScene(const QString &projectPath, const QString &sceneName);

private:
    ProjectData m_projectData;
    QMap<QString, QString> m_variables;
    
    // Helper functions
    QString getMainTemplate();
    QString getPlayerTemplate();
    QString getEnemyTemplate();
    QString getCameraControllerTemplate();
};

#endif // CODEGENERATOR_H
