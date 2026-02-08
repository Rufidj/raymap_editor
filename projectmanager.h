#ifndef PROJECTMANAGER_H
#define PROJECTMANAGER_H

#include <QObject>
#include <QString>
#include <QVector>

struct Project {
    QString name;
    QString path;
    // Add other project fields as needed
};

struct ProjectData {
    QString name;
    QString version;
    QString path;
    QString startupScene; // Name of the scene to launch on start
    QString mainScript = "src/main.prg";  // Main script to compile
    
    // Display settings
    int screenWidth = 800;
    int screenHeight = 600;
    int renderWidth = 800;   // Internal render resolution
    int renderHeight = 600;  // Internal render resolution
    int fps = 60;
    bool fullscreen = false;  // Fullscreen mode
    
    // Publishing
    QString packageName;  // For Android publishing
    QString iconPath;     // For publishing
    bool androidSupport = true; // Generate Android compatibility code
};

class ProjectManager : public QObject
{
    Q_OBJECT
public:
    explicit ProjectManager(QObject *parent = nullptr);
    
    bool createProject(const QString &path, const QString &name);
    bool openProject(const QString &fileName);
    void closeProject();
    bool hasProject() const;
    const Project* getProject() const;
    QString getProjectPath() const;
    
    // Load/Save project configuration (resolution, etc.)
    static ProjectData loadProjectData(const QString &projectPath);
    static bool saveProjectData(const QString &projectPath, const ProjectData &data);

private:
    Project *m_project;
};

#endif // PROJECTMANAGER_H
