#include "bennugdinstaller.h"
#include "buildmanager.h"
#include "codegenerator.h"
#include "consolewidget.h"
#include "mainwindow.h"
#include "processgenerator.h"
#include "projectmanager.h"
#include "raymapformat.h"
#include "sceneeditor.h"
#include <QDebug>
#include <QDir>
#include <QDirIterator>
#include <QFileDialog>
#include <QInputDialog>
#include <QMessageBox>
#include <QSettings>
#include <QToolBar>

void MainWindow::setupBuildSystem() {
  // Create Build Manager
  m_buildManager = new BuildManager(this);

  // Create Console Widget
  m_consoleWidget = new ConsoleWidget(this);
  m_consoleDock = new QDockWidget(tr("Consola / Salida"), this);
  m_consoleDock->setObjectName("ConsoleDock");
  m_consoleDock->setWidget(m_consoleWidget);
  addDockWidget(Qt::BottomDockWidgetArea, m_consoleDock);
  m_consoleDock->show(); // Show by default for debugging

  // Connect signals
  connect(m_buildManager, &BuildManager::buildStarted, this, [this]() {
    // m_consoleWidget->clear(); // Don't clear on build, let user decide or
    // history persist
    m_consoleWidget->setBuildMode();
    m_consoleDock->show();
  });

  // Connect executeInTerminal signal
  connect(
      m_buildManager, &BuildManager::executeInTerminal, this,
      [this](const QString &command) { m_consoleWidget->sendText(command); });

  // Obsolete signals (buildFinished, runOutput, etc) are removed
  // because we now use a real terminal that handles its own output.

  connect(m_buildManager, &BuildManager::runStarted, this, [this]() {
    m_consoleWidget->setRunMode();
    m_consoleDock->show();
  });

  connect(m_consoleWidget, &ConsoleWidget::stopRequested, m_buildManager,
          &BuildManager::stopRunning);

  // Check if BennuGD2 is installed

  if (!m_buildManager->isBennuGD2Installed()) {
    // Automatically open installer. The installer will check for missing files
    // and prompt the user.
    onInstallBennuGD2();
  }
}

void MainWindow::onBuildProject() {
  // Auto-save map before build
  onSaveMap();

  QString projectPath;

  if (m_projectManager && m_projectManager->hasProject()) {
    projectPath = m_projectManager->getProjectPath();
  } else {
    // Fallback: Use current map directory
    // We need to access the tab widget and get the current file
    // Since mainwindow_build.cpp includes mainwindow.h, we can access members
    // if they are protected/public or we are in MainWindow scope (we are). But
    // we need to ensure we can get the filename. We previously used
    // getCurrentEditor()->fileName() We need to verify if getCurrentEditor is
    // available. It is a member function.
    GridEditor *editor = getCurrentEditor();
    if (editor) {
      QFileInfo fi(editor->fileName());
      if (fi.exists())
        projectPath = fi.absolutePath();
    }

    if (projectPath.isEmpty()) {
      projectPath = QDir::currentPath();
    }
  }

  // Auto-generate code before building to ensure it's up to date
  onGenerateCode();

  m_buildManager->buildProject(projectPath);
  m_buildManager->buildProject(projectPath);
}

void MainWindow::onRunProject() {
  // Auto-save map before run
  onSaveMap();

  QString projectPath;

  if (m_projectManager && m_projectManager->hasProject()) {
    projectPath = m_projectManager->getProjectPath();
  } else {
    GridEditor *editor = getCurrentEditor();
    if (editor) {
      QFileInfo fi(editor->fileName());
      if (fi.exists())
        projectPath = fi.absolutePath();
    }

    if (projectPath.isEmpty()) {
      projectPath = QDir::currentPath();
    }
  }

  m_buildManager->runProject(projectPath);
}

void MainWindow::onBuildAndRun() {
  // Auto-save map before build & run
  onSaveMap();

  QString projectPath;

  if (m_projectManager && m_projectManager->hasProject()) {
    projectPath = m_projectManager->getProjectPath();
  } else {
    GridEditor *editor = getCurrentEditor();
    if (editor) {
      QFileInfo fi(editor->fileName());
      if (fi.exists())
        projectPath = fi.absolutePath();
    }

    if (projectPath.isEmpty()) {
      projectPath = QDir::currentPath();
    }
  }

  // Auto-generate code before build & run
  onGenerateCode();

  // Build and Run combined
  m_buildManager->buildAndRunProject(projectPath);
}

void MainWindow::onStopRunning() { m_buildManager->stopRunning(); }

void MainWindow::onInstallBennuGD2() {
  BennuGDInstaller *installer = new BennuGDInstaller(this);

  connect(installer, &BennuGDInstaller::installationFinished, this,
          [this, installer](bool success) {
            if (success) {
              // Clear any custom override to force detection of new runtimes
              QSettings settings("BennuGD", "RayMapEditor");
              settings.remove("bennugdPath");

              // Re-detect BennuGD2
              m_buildManager->detectBennuGD2();
              QMessageBox::information(this, tr("Success"),
                                       tr("BennuGD2 installed successfully!"));
            }
            installer->deleteLater();
          });

  installer->show();
  installer->startInstallation();
}

void MainWindow::onConfigureBennuGD2() {
  QString dir = QFileDialog::getExistingDirectory(
      this, tr("Select BennuGD2 Installation Directory (bin folder)"),
      QDir::homePath(),
      QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks);

  if (!dir.isEmpty()) {
    m_buildManager->setCustomBennuGDPath(dir);
    if (m_buildManager->isBennuGD2Installed()) {
      QMessageBox::information(this, tr("Configuration Saved"),
                               tr("BennuGD2 path updated to:\n%1").arg(dir));
    } else {
      QMessageBox::warning(
          this, tr("Invalid Path"),
          tr("Could not find bgdc/bgdi in the selected directory:\n%1")
              .arg(dir));
    }
  }
}

void MainWindow::onGenerateCode() {
  if (!m_projectManager || !m_projectManager->hasProject())
    return;

  CodeGenerator generator;

  // Load project configuration from file (or use defaults if not found)
  QString projectPath = m_projectManager->getProjectPath();
  ProjectData projectData = ProjectManager::loadProjectData(projectPath);

  // Update name and path from current project
  const Project *proj = m_projectManager->getProject();
  projectData.name = proj->name;
  projectData.path = proj->path;

  generator.setProjectData(projectData);

  // Get current map and FPG paths
  GridEditor *editor = getCurrentEditor();
  QString mapPath = "assets/maps/map.raymap";  // Default
  QString fpgPath = "assets/fpg/textures.fpg"; // Default

  if (editor && !editor->fileName().isEmpty()) {
    // Get relative path from project root
    QString projectPath = m_projectManager->getProjectPath();
    QFileInfo mapInfo(editor->fileName());

    // Calculate relative path
    QDir projectDir(projectPath);
    mapPath = projectDir.relativeFilePath(mapInfo.absoluteFilePath());

    qDebug() << "Map path:" << mapPath;
  }

  // Get FPG path if one is loaded
  if (!m_currentFPGPath.isEmpty()) {
    QString projectPath = m_projectManager->getProjectPath();
    QFileInfo fpgInfo(m_currentFPGPath);
    QDir projectDir(projectPath);
    fpgPath = projectDir.relativeFilePath(fpgInfo.absoluteFilePath());

    qDebug() << "FPG path:" << fpgPath;
  }

  // Set paths in generator
  generator.setVariable("INITIAL_MAP", mapPath);
  generator.setVariable("FPG_PATH", fpgPath);

  // Get entities and NPC paths from current map
  QVector<EntityInstance> entities;
  QVector<NPCPath> npcPaths;
  if (editor && editor->mapData()) {
    entities = editor->mapData()->entities;
    npcPaths = editor->mapData()->npcPaths;
  }

  // Entity process generation moved down to consolidate with hybrid map
  // entities

  // Generate main.prg with entities
  QString mainPath = m_projectManager->getProjectPath() + "/src/main.prg";
  bool fileExists = QFile::exists(mainPath);

  // Determine if we are in "Scene" mode (2D UI/Scenes)
  bool isScene = mapPath.endsWith(".scn", Qt::CaseInsensitive);

  // Determine the primary directory for source files
  QString srcDir = m_projectManager->getProjectPath() + "/src";
  QDir().mkpath(srcDir);

  // Collect resources from CURRENT map to pass to global generator
  QSet<QString> currentMapResources;
  for (const auto &ent : entities) {
    if (!ent.assetPath.isEmpty())
      currentMapResources.insert(ent.assetPath);
  }
  if (!fpgPath.isEmpty())
    currentMapResources.insert(fpgPath);

  // ALWAYS generate common files (resources, scenes_list, scene_commons)
  // This will populate m_inlineResources and m_inlineScenes
  generator.generateAllScenes(m_projectManager->getProjectPath(),
                              currentMapResources);

  // Handle building from a SCENE map
  if (isScene) {
    QFileInfo sceneInfo(mapPath);
    QString sceneName = sceneInfo.baseName();

    // UPDATE Project Data with new startup scene
    projectData.startupScene = sceneName;
    ProjectManager::saveProjectData(m_projectManager->getProjectPath(),
                                    projectData);

    // Ensure the variable is set with prefix for spawning in main.prg
    generator.setVariable("STARTUP_SCENE", "scene_" + sceneName.toLower());
  } else {
    // If we are in a 3D map, ensure we don't accidentally use a scene as
    // startup
    if (projectData.startupScene.isEmpty()) {
      generator.setVariable("STARTUP_SCENE", "// No startup scene set");
    } else {
      generator.setVariable("STARTUP_SCENE",
                            "scene_" + projectData.startupScene.toLower());
    }
  }

  // 1. Consolidated Entity generation is now handled after project scan.
  // We'll generate forward declarations first to avoid parameter type errors
  QString forwardDecls =
      ProcessGenerator::generateDeclarationsSection(entities);
  {
    QFile fileDecls(srcDir + "/autogen_decl.prg");
    if (fileDecls.open(QIODevice::WriteOnly | QIODevice::Text)) {
      fileDecls.write(forwardDecls.toUtf8());
      fileDecls.close();
    }
  }

  // 2. Generate and save paths
  QString npcPathsCode = ProcessGenerator::generateNPCPathsCode(npcPaths);
  bool pathsFileExists = QFile::exists(srcDir + "/autogen_paths.prg");
  if (!isScene || !pathsFileExists) {
    if (npcPathsCode.isEmpty()) {
      npcPathsCode = "function npc_paths_init()\nbegin\nend\n";
    }
    QFile filePaths(srcDir + "/autogen_paths.prg");
    if (filePaths.open(QIODevice::WriteOnly | QIODevice::Text)) {
      filePaths.write(npcPathsCode.toUtf8());
      filePaths.close();
    }
  }

  // 3. Save monolithic resources/scenes file (ALWAYS update this)
  QFile fileRes(srcDir + "/autogen_resources.prg");
  if (fileRes.open(QIODevice::WriteOnly | QIODevice::Text)) {
    QString resCode = generator.getInlineCommons() + "\n" +
                      generator.getInlineResources() + "\n" +
                      generator.getInlineScenes();
    fileRes.write(resCode.toUtf8());
    fileRes.close();
  }

  // 4. Handle USER LOGIC stub (Smart Update)
  QString userLogicPath = srcDir + "/user_logic.prg";
  QStringList uniqueProcesses =
      ProcessGenerator::getUniqueProcessNames(entities);

  // CRITICAL FIX: Also scan all scenes in the project for ENTITY_WORLD3D
  // to ensure their hybrid map entities also get stubs.
  QDir projectDir(m_projectManager->getProjectPath());
  QStringList scnFilters;
  scnFilters << "*.scn" << "*.scene";
  QDirIterator itScn(m_projectManager->getProjectPath(), scnFilters,
                     QDir::Files | QDir::NoSymLinks,
                     QDirIterator::Subdirectories);
  while (itScn.hasNext()) {
    SceneData scnData;
    if (CodeGenerator::loadSceneJson(itScn.next(), scnData)) {
      for (auto scnEnt : scnData.entities) {
        if (scnEnt->type == ENTITY_WORLD3D) {
          MapData hybridMap;
          RayMapFormat loader;
          QString fullMapPath = scnEnt->sourceFile;
          if (!QFileInfo(fullMapPath).isAbsolute())
            fullMapPath = projectDir.absolutePath() + "/" + fullMapPath;
          if (loader.loadMap(fullMapPath, hybridMap, nullptr)) {
            for (const auto &hEnt : hybridMap.entities) {
              // Add to global entity list for process generation if not already
              // there (Based on process name to avoid redundant code)
              entities.append(hEnt);

              QString pName = hEnt.processName;
              if (pName.isEmpty())
                pName = QFileInfo(hEnt.assetPath).baseName();
              if (!pName.isEmpty()) {
                pName =
                    pName.replace(" ", "_").replace("-", "_").replace(".", "_");
                if (!uniqueProcesses.contains(pName.toLower()))
                  uniqueProcesses << pName.toLower();
              }
            }
          }
        }
      }
    }
  }

  // Generate ALL entity processes consolidated from main map and hybrid maps
  QString allEntityProcesses = ProcessGenerator::generateAllProcessesCode(
      entities, generator.getWrapperOpen(), generator.getWrapperClose());

  // 1. Save (or Overwrite) consolidated entities file
  {
    QFile fileEntities(srcDir + "/autogen_entities.prg");
    if (fileEntities.open(QIODevice::WriteOnly | QIODevice::Text)) {
      fileEntities.write(allEntityProcesses.toUtf8());
      fileEntities.close();
    }
  }

  if (!QFile::exists(userLogicPath)) {
    QFile fileUser(userLogicPath);
    if (fileUser.open(QIODevice::WriteOnly | QIODevice::Text)) {
      QString userLogicStubs =
          generator.generateUserLogicStubs(uniqueProcesses);
      fileUser.write(userLogicStubs.toUtf8());
      fileUser.close();
    }
  } else {
    // Append only missing hooks to avoid breaking user code
    QFile fileUser(userLogicPath);
    if (fileUser.open(QIODevice::ReadWrite | QIODevice::Text)) {
      QString content = fileUser.readAll();
      QString toAppend;
      for (const QString &name : uniqueProcesses) {
        QString lowerName = name.toLower();
        bool hasInit = content.contains("hook_" + lowerName + "_init",
                                        Qt::CaseInsensitive);
        bool hasUpdate = content.contains("hook_" + lowerName + "_update",
                                          Qt::CaseInsensitive);

        if (!hasInit || !hasUpdate) {
          toAppend +=
              QString("\n// Hooks for %1 (Auto-added)\n").arg(lowerName);
          if (!hasInit) {
            toAppend += QString("function hook_%1_init(int p_id) begin end\n")
                            .arg(lowerName);
          }
          if (!hasUpdate) {
            toAppend += QString("function hook_%1_update(int p_id) begin end\n")
                            .arg(lowerName);
          }
        }
      }
      if (!toAppend.isEmpty()) {
        fileUser.seek(fileUser.size());
        fileUser.write(toAppend.toUtf8());
      }
      fileUser.close();
    }
  }

  // 5. Generate or UPDATE MAIN.PRG master
  if (!fileExists) {
    QFile fileMain(mainPath);
    if (fileMain.open(QIODevice::WriteOnly | QIODevice::Text)) {
      QString masterMain =
          "// " + projectData.name +
          "\n"
          "// Auto-generado por RayMap Editor\n\n"
          "import \"libmod_gfx\";\n"
          "import \"libmod_input\";\n"
          "import \"libmod_misc\";\n"
          "import \"libmod_ray\";\n"
          "import \"libmod_sound\";\n\n"
          "include \"autogen_decl.prg\";\n"
          "include \"autogen_resources.prg\";\n"
          "include \"autogen_entities.prg\";\n"
          "include \"autogen_paths.prg\";\n"
          "include \"user_logic.prg\";\n\n"
          "GLOBAL\n"
          "    int screen_w = " +
          QString::number(projectData.screenWidth) +
          ";\n"
          "    int screen_h = " +
          QString::number(projectData.screenHeight) +
          ";\n"
          "END\n\n"
          "PROCESS main()\n"
          "BEGIN\n"
          "    set_mode(screen_w, screen_h, " +
          (projectData.fullscreen ? "MODE_FULLSCREEN" : "MODE_WINDOW") +
          ");\n"
          "    set_fps(" +
          QString::number(projectData.fps) +
          ", 0);\n"
          "    soundsys_init();\n\n"
          "    load_project_resources();\n"
          "    npc_paths_init();\n\n"
          "    // [[ED_STARTUP_SCENE_START]]\n"
          "    " +
          generator.processTemplate("{{STARTUP_SCENE}}") +
          "();\n"
          "    // [[ED_STARTUP_SCENE_END]]\n\n"
          "    LOOP\n"
          "        if (key(_esc)) exit(); end\n"
          "        RAY_CAMERA_UPDATE(0.017);\n"
          "        FRAME;\n"
          "    END\n"
          "END\n";
      fileMain.write(masterMain.toUtf8());
      fileMain.close();
    }
  } else if (isScene) {
    // If editing a scene, update the startup call in the master file using
    // markers
    QFile f(mainPath);
    if (f.open(QIODevice::ReadOnly | QIODevice::Text)) {
      QString content = QTextStream(&f).readAll();
      f.close();

      QString startMarker = "// [[ED_STARTUP_SCENE_START]]";
      QString endMarker = "// [[ED_STARTUP_SCENE_END]]";
      int startIdx = content.indexOf(startMarker);
      int endIdx = content.indexOf(endMarker);

      if (startIdx != -1 && endIdx != -1) {
        QString newStartup = startMarker + "\n    scene_" +
                             QFileInfo(mapPath).baseName().toLower() +
                             "();\n    " + endMarker;
        content.replace(startIdx, (endIdx + endMarker.length()) - startIdx,
                        newStartup);

        if (f.open(QIODevice::WriteOnly | QIODevice::Text)) {
          f.write(content.toUtf8());
          f.close();
        }
      }
    }
  }

  if (m_consoleWidget) {
    m_consoleWidget->sendText("Code generated successfully!\n");
    if (isScene) {
      m_consoleWidget->sendText("  Mode: Scene (" +
                                QFileInfo(mapPath).baseName() + ")\n");
    } else {
      m_consoleWidget->sendText("  Mode: Map (" +
                                QFileInfo(mapPath).baseName() + ")\n");
    }
  }
}
