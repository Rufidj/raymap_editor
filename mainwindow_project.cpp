#include "mainwindow.h"
#include "newprojectdialog.h"
#include "projectsettingsdialog.h"
#include "publishdialog.h"
#include "projectmanager.h"
#include "publishdialog.h"
#include "projectmanager.h"
#include "assetbrowser.h"
#include "codegenerator.h" // New
#include "processgenerator.h" // New
#include "grideditor.h" // New
#include <QMessageBox>
#include <QFileDialog>
#include <QInputDialog>

// ============================================================================
// PROJECT MANAGEMENT
// ============================================================================

void MainWindow::onNewProject()
{
    NewProjectDialog dialog(this);
    if (dialog.exec() == QDialog::Accepted) {
        QString name = dialog.getProjectName();
        QString path = dialog.getProjectPath();
        
        // Create ProjectManager if it doesn't exist
        if (!m_projectManager) {
            m_projectManager = new ProjectManager();
        }
        
        // Close current project if any
        if (m_projectManager->hasProject()) {
            onCloseProject();
        }
        
        // Create new project
        if (m_projectManager->createProject(path, name)) {
            QMessageBox::information(this, "Proyecto Creado",
                QString("Proyecto '%1' creado exitosamente en:\n%2")
                .arg(name)
                .arg(m_projectManager->getProjectPath()));
            
            if (m_assetBrowser) {
                m_assetBrowser->setProjectPath(m_projectManager->getProjectPath());
            }

            updateWindowTitle();
        } else {
            QMessageBox::warning(this, "Error",
                "No se pudo crear el proyecto.");
        }
    }
}

void MainWindow::onOpenProject()
{
    // Select project directory instead of file
    QString dirPath = QFileDialog::getExistingDirectory(
        this,
        "Seleccionar Carpeta del Proyecto BennuGD2",
        "",
        QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks
    );
    
    if (dirPath.isEmpty()) return;
    
    // Find .bgd2proj file in the selected directory
    QDir dir(dirPath);
    QStringList projFiles = dir.entryList(QStringList() << "*.bgd2proj", QDir::Files);
    
    QString fileName;
    if (projFiles.isEmpty()) {
        QMessageBox::warning(this, "Error",
            "No se encontró ningún archivo .bgd2proj en la carpeta seleccionada.");
        return;
    } else if (projFiles.size() == 1) {
        fileName = dirPath + "/" + projFiles.first();
    } else {
        // Multiple .bgd2proj files, let user choose
        bool ok;
        QString selected = QInputDialog::getItem(this, "Seleccionar Proyecto",
            "Se encontraron múltiples proyectos. Selecciona uno:",
            projFiles, 0, false, &ok);
        if (ok && !selected.isEmpty()) {
            fileName = dirPath + "/" + selected;
        } else {
            return;
        }
    }
    
    openProject(fileName);
}

void MainWindow::onCloseProject()
{
    if (!m_projectManager || !m_projectManager->hasProject()) {
        QMessageBox::information(this, "Sin Proyecto",
            "No hay ningún proyecto abierto.");
        return;
    }
    
    // Ask for confirmation
    QMessageBox::StandardButton reply = QMessageBox::question(
        this,
        "Cerrar Proyecto",
        QString("¿Cerrar el proyecto '%1'?")
        .arg(m_projectManager->getProject()->name),
        QMessageBox::Yes | QMessageBox::No
    );
    
    if (reply == QMessageBox::Yes) {
        m_projectManager->closeProject();
        
        // Cleanup UI
        if (m_assetBrowser) {
             m_assetBrowser->setProjectPath(""); // Clear asset browser
        }
        
        // Close all tabs (except maybe a default empty one)
        m_tabWidget->clear();
        onNewMap(); // Open a default empty map
        
        if (m_consoleWidget) {
             m_consoleWidget->clear();
             m_consoleDock->hide();
        }
        
        updateWindowTitle();
        
        QMessageBox::information(this, "Proyecto Cerrado",
            "El proyecto se ha cerrado.");
    }
}

void MainWindow::onProjectSettings()
{
    if (!m_projectManager || !m_projectManager->hasProject()) {
        QMessageBox::information(this, "Sin Proyecto",
            "No hay ningún proyecto abierto.\nCrea o abre un proyecto primero.");
        return;
    }
    
    // Load project configuration from file (or use defaults if not found)
    QString projectPath = m_projectManager->getProjectPath();
    ProjectData projectData = ProjectManager::loadProjectData(projectPath);
    
    // Update name and path from current project
    const Project *proj = m_projectManager->getProject();
    projectData.name = proj->name;
    projectData.path = proj->path;
    
    // Open settings dialog
    ProjectSettingsDialog dialog(projectData, this);
    if (dialog.exec() == QDialog::Accepted) {
        // Get updated data
        ProjectData updatedData = dialog.getProjectData();
        
        // The dialog already regenerated main.prg with new settings
        // Regenerate entity scripts (to apply Android support if needed)
        regenerateEntityScripts(&updatedData);
        
        QMessageBox::information(this, "Configuración Guardada",
            QString("Configuración del proyecto actualizada.\n"
                    "Resolución de ventana: %1x%2\n"
                    "Resolución de renderizado: %3x%4\n"
                    "El código ha sido regenerado con los nuevos valores.")
            .arg(updatedData.screenWidth)
            .arg(updatedData.screenHeight)
            .arg(updatedData.renderWidth)
            .arg(updatedData.renderHeight));
    }
}

void MainWindow::onPublishProject()
{
    if (!m_projectManager || !m_projectManager->hasProject()) {
        QMessageBox::information(this, "Sin Proyecto",
            "No hay ningún proyecto abierto.\nCrea o abre un proyecto primero.");
        return;
    }
    
    // Load project configuration
    QString projectPath = m_projectManager->getProjectPath();
    ProjectData projectData = ProjectManager::loadProjectData(projectPath);
    
    // Update name and path from current project
    const Project *proj = m_projectManager->getProject();
    projectData.name = proj->name;
    projectData.path = proj->path;
    
    // Open publish dialog
    PublishDialog dialog(&projectData, this);
    dialog.exec();
}

void MainWindow::regenerateEntityScripts(const ProjectData *customData)
{
    if (!m_projectManager || !m_projectManager->hasProject()) return;
    
    // Setup generator with current settings
    CodeGenerator generator;
    ProjectData data;
    
    if (customData) {
        data = *customData;
        // Ensure path is set (sometimes logic cleans it?)
        if (data.path.isEmpty()) data.path = m_projectManager->getProjectPath();
    } else {
        data = m_projectManager->loadProjectData(m_projectManager->getProjectPath());
    }
    
    generator.setProjectData(data);
    
    GridEditor* editor = getCurrentEditor();
    if (!editor) return;
    MapData* mapData = editor->mapData();
    if (!mapData) return;
    
    // Create includes directory
    QString includesDir = m_projectManager->getProjectPath() + "/src/includes";
    QDir().mkpath(includesDir);
    
    // Set to track generated files
    QSet<QString> generatedFiles;

    for (const EntityInstance &ent : mapData->entities) {
        if (ent.type == "model" && !ent.assetPath.isEmpty()) {
            
            // Determine .h filename (based on process name)
            QString hFileName = ent.processName + ".h";
            if (generatedFiles.contains(hFileName)) continue;
            generatedFiles.insert(hFileName);
            
            // Need relative asset path for the generator
            QDir projDir(m_projectManager->getProjectPath());
            QString relativeAssetPath = projDir.relativeFilePath(ent.assetPath);
            
            // Use generator to create the content suitable for Android if configured
            // Use ProcessGenerator which is the standard one, passing wrappers
            QString code = ProcessGenerator::generateProcessCode(ent.processName, 
                                                               relativeAssetPath, 
                                                               ent.type,
                                                               generator.getWrapperOpen(),
                                                               generator.getWrapperClose());
            
            // Save file
            QFile file(includesDir + "/" + hFileName);
            if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
                file.write(code.toUtf8());
                file.close();
                qDebug() << "Regenerated entity script:" << hFileName;
            } else {
                qWarning() << "Failed to write entity script:" << hFileName;
            }
        }
    }
    
    // Update main.prg includes
    QString mainPrgPath = m_projectManager->getProjectPath() + "/src/main.prg";
    QFile mainFile(mainPrgPath);
    if (mainFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QString mainCode = QString::fromUtf8(mainFile.readAll());
        mainFile.close();
        
        QString updatedCode = generator.patchMainPrg(mainCode, mapData->entities);
        
        if (updatedCode != mainCode) {
            if (mainFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
                mainFile.write(updatedCode.toUtf8());
                mainFile.close();
                qDebug() << "Updated main.prg includes";
            }
        }
    }
}

void MainWindow::openProject(const QString &path)
{
    // Create ProjectManager if it doesn't exist
    if (!m_projectManager) {
        m_projectManager = new ProjectManager();
    }
    
    // Close current project if any
    if (m_projectManager->hasProject()) {
        onCloseProject();
    }
    
    // Open project
    if (m_projectManager->openProject(path)) {
        addToRecentProjects(path);
        
        QMessageBox::information(this, "Proyecto Abierto",
            QString("Proyecto '%1' abierto exitosamente.")
            .arg(m_projectManager->getProject()->name));
        
        if (m_assetBrowser) {
            m_assetBrowser->setProjectPath(m_projectManager->getProjectPath());
        }

        updateWindowTitle();
        
        // Auto-open maps from project's maps folder
        QString mapsDir = m_projectManager->getProjectPath() + "/assets/maps";
        QDir dir(mapsDir);
        if (dir.exists()) {
            QStringList filters;
            filters << "*.raymap" << "*.rmap";
            QFileInfoList mapFiles = dir.entryInfoList(filters, QDir::Files);
            
            for (const QFileInfo &mapFile : mapFiles) {
                qDebug() << "Auto-opening map:" << mapFile.absoluteFilePath();
                openMapFile(mapFile.absoluteFilePath());
            }
            
            if (!mapFiles.isEmpty()) {
                m_statusLabel->setText(tr("Proyecto cargado: %1 mapas abiertos")
                                      .arg(mapFiles.size()));
            }
        }
    } else {
        QMessageBox::warning(this, "Error",
            "No se pudo abrir el proyecto: " + path);
    }
}
