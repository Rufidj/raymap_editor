#include "buildmanager.h"
#include <QCoreApplication>
#include <QDebug>
#include <QDir>
#include <QSettings>
#include <QStandardPaths>
#ifdef Q_OS_LINUX
#include <sys/types.h>
#include <unistd.h>
#endif

BuildManager::BuildManager(QObject *parent)
    : QObject(parent), m_process(new QProcess(this)), m_isrunning(false),
      m_autoRunAfterBuild(false) {
  connect(m_process, &QProcess::readyReadStandardOutput, this,
          &BuildManager::onProcessOutput);
  connect(m_process, &QProcess::readyReadStandardError, this,
          &BuildManager::onProcessOutput);
  connect(m_process,
          QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished), this,
          &BuildManager::onProcessFinished);
  connect(m_process, &QProcess::errorOccurred, this,
          [this](QProcess::ProcessError error) {
            emit executeInTerminal(
                "Process Error: " + m_process->errorString() + "\n");
            m_isrunning = false;
            emit buildFinished(false);
          });

  detectBennuGD2();
}

void BuildManager::detectBennuGD2() {
  // Check settings first
  QSettings settings("BennuGD", "RayMapEditor");
  QString customPath = settings.value("bennugdPath").toString();

  // Migration Logic
  if (!customPath.isEmpty() && customPath.contains("/runtime/")) {
    QString testNew = QDir::homePath() + "/.bennugd2/runtime";
    if (QDir(testNew).exists()) {
      qDebug() << "Detected old runtime path in settings:" << customPath;
      qDebug() << "New runtimes found at" << testNew
               << ". Switching preferences.";
      customPath = "";
      settings.remove("bennugdPath");
    }
  }

  if (!customPath.isEmpty()) {
    m_bgdcPath = customPath + "/bgdc";
    m_bgdiPath = customPath + "/bgdi";

#ifdef Q_OS_WIN
    if (!m_bgdcPath.endsWith(".exe"))
      m_bgdcPath += ".exe";
    if (!m_bgdiPath.endsWith(".exe"))
      m_bgdiPath += ".exe";
#endif

    if (QFile::exists(m_bgdcPath) && QFile::exists(m_bgdiPath)) {
      qDebug() << "Using configured BennuGD2:" << m_bgdcPath;
      return;
    }
  }

  // Determine platform specific folder name
  QString platform = "unknown";
#ifdef Q_OS_LINUX
  platform = "linux-gnu";
#elif defined(Q_OS_WIN)
  platform = "win64";
#elif defined(Q_OS_MAC)
  platform = "macos";
#endif

  // Check standard paths
  QString homePath = QDir::homePath();
  QStringList paths;
  paths << homePath + "/.bennugd2/runtime/" + platform + "/bin";
  paths << homePath + "/.bennugd2/runtime/" + platform;
  paths << homePath + "/.bennugd2/bin";
  paths << QCoreApplication::applicationDirPath() + "/runtime/" + platform +
               "/bin";
  paths << QCoreApplication::applicationDirPath() + "/runtime/" + platform;
  paths << QCoreApplication::applicationDirPath() + "/bin";
  paths << QCoreApplication::applicationDirPath();

  paths << "/usr/local/bin";
  paths << "/usr/bin";
  paths << "/opt/bennugd2/bin";
  paths << homePath + "/bennugd2/bin";
  paths << homePath + "/.local/bin";
  paths << QDir::currentPath();

  qDebug() << "Detecting BennuGD2 for platform:" << platform;
  for (const QString &p : paths)
    qDebug() << "Checking path:" << p;

  for (const QString &path : paths) {
    QString bgdc = path + "/bgdc";
    QString bgdi = path + "/bgdi";

#ifdef Q_OS_WIN
    bgdc += ".exe";
    bgdi += ".exe";
#endif

    if (QFile::exists(bgdc) && QFile::exists(bgdi)) {
      m_bgdcPath = bgdc;
      m_bgdiPath = bgdi;
      qDebug() << "BennuGD2 found at:" << path;
      return;
    }
  }
}

bool BuildManager::isBennuGD2Installed() {
  return !m_bgdcPath.isEmpty() && !m_bgdiPath.isEmpty();
}

void BuildManager::setCustomBennuGDPath(const QString &path) {
  QSettings settings("BennuGD", "RayMapEditor");
  settings.setValue("bennugdPath", path);
  detectBennuGD2();
}

void BuildManager::buildProject(const QString &projectPath,
                                const QString &mainFile) {
  if (m_isrunning)
    return;
  if (m_bgdcPath.isEmpty()) {
    emit executeInTerminal("Error: BennuGD2 compilers not found!\n");
    return;
  }

  m_currentProjectPath = projectPath;
  // m_autoRunAfterBuild is set by caller (buildAndRun or runScene)

  QString fullMainPath = projectPath + "/src/" + mainFile;

  emit buildStarted();
  emit executeInTerminal("Compiling: " + fullMainPath + "\n");

  // Set environment vars
  QFileInfo bgdcInfo(m_bgdcPath);
  QString bgdcDir = bgdcInfo.absolutePath();

  // Generic Wrapper Logic
  QString scriptExtension = "";
#ifdef Q_OS_WIN
  scriptExtension = ".bat";
#else
  scriptExtension = ".sh";
#endif

  QString script = bgdcDir + "/compile" + scriptExtension;
  QString srcDir = QDir::toNativeSeparators(projectPath + "/src");

  QProcessEnvironment env = QProcessEnvironment::systemEnvironment();

#ifdef Q_OS_WIN
  QString currentPath = env.value("PATH");
  env.insert("PATH", QDir::toNativeSeparators(bgdcDir) + ";" + currentPath);
#else
  env.insert("PATH", "/usr/bin:/bin:/usr/local/bin");
  env.insert("HOME", QDir::homePath());
#endif

  m_process->setProcessEnvironment(env);

  bool useWrapper = QFile::exists(script);
  if (mainFile != "main.prg") {
    useWrapper = false;
    emit executeInTerminal(
        "Note: Bypassing wrapper for custom build target.\n");
  }

  if (useWrapper) {
    // Wrapper exists
    script = QDir::toNativeSeparators(script);
    emit executeInTerminal("Wrapper: " + script + " " + srcDir + "\n");
    m_process->start(script, QStringList() << srcDir);
  } else {
    // Direct execution

#ifndef Q_OS_WIN
    QString libDir = QDir(bgdcDir + "/../lib").canonicalPath();
    if (libDir.isEmpty())
      libDir = bgdcDir;

    QString ldVar = "LD_LIBRARY_PATH";
#ifdef Q_OS_MAC
    ldVar = "DYLD_LIBRARY_PATH";
#endif

    QString ldPath = libDir;
    if (env.contains(ldVar))
      ldPath += ":" + env.value(ldVar);
    env.insert(ldVar, ldPath);
    env.insert("BENNU_LIB_PATH", libDir);
    m_process->setProcessEnvironment(env);
#endif

    QString exe = QDir::toNativeSeparators(m_bgdcPath);
    emit executeInTerminal("Compiling (Direct): " + exe + " " + fullMainPath +
                           "\n");
    m_process->setWorkingDirectory(srcDir);
    m_process->start(exe, QStringList() << mainFile);
  }

  m_isrunning = true;
}

void BuildManager::runProject(const QString &projectPath,
                              const QString &dcbFile) {
  if (m_isrunning)
    return;
  if (m_bgdiPath.isEmpty()) {
    emit executeInTerminal("Error: BennuGD2 interpreter not found!\n");
    return;
  }

  m_currentProjectPath = projectPath;
  // dcbFile is treated as relative to src/ if it does not contain "/"
  QString dcbRelative = "src/" + dcbFile;
  QString fullDcbPath = projectPath + "/" + dcbRelative;

  emit runStarted();
  emit executeInTerminal("Running: " + fullDcbPath + "\n");

  QFileInfo bgdiInfo(m_bgdiPath);
  QString bgdiDir = bgdiInfo.absolutePath();

  QString scriptExtension = "";
#ifdef Q_OS_WIN
  scriptExtension = ".bat";
#else
  scriptExtension = ".sh";
#endif

  QString script = bgdiDir + "/run" + scriptExtension;
  QString rootDir = QDir::toNativeSeparators(projectPath);

  QProcessEnvironment env = QProcessEnvironment::systemEnvironment();

// NUCLEAR OPTION: Blindly copy EVERYTHING from the current process environment
#ifdef Q_OS_LINUX
  extern char **environ;
  for (char **current = environ; *current; current++) {
    QString entry = *current;
    int posequals = entry.indexOf('=');
    if (posequals != -1) {
      QString key = entry.left(posequals);
      QString val = entry.mid(posequals + 1);

      // SANITIZATION: Do NOT propagate LD_LIBRARY_PATH from Qt Creator!
      if (key == "LD_LIBRARY_PATH" || key == "DYLD_LIBRARY_PATH")
        continue;

      env.insert(key, val);
    }
  }
#endif

  // If running the main project and wrapper exists, use it!
  // BUT ONLY if we are using a local runtime, NOT a system installation
  bool isSystemInstall = m_bgdiPath.startsWith("/usr/") ||
                         m_bgdiPath.startsWith("/bin") ||
                         !script.contains("/.bennugd2/");

  // Force direct execution if system install detected
  bool useWrapper = !isSystemInstall &&
                    (dcbFile == "main.dcb" || dcbFile == "src/main.dcb") &&
                    QFile::exists(script);

#ifndef Q_OS_WIN
  if (!useWrapper) {
    // ONLY set library paths if NOT using wrapper
    // because wrapper sets its own and we don't want to interfere
    QString ldVar = "LD_LIBRARY_PATH";
#ifdef Q_OS_MAC
    ldVar = "DYLD_LIBRARY_PATH";
#endif

    QString ldPath = bgdiDir;
    if (env.contains(ldVar))
      ldPath = ldPath + ":" + env.value(ldVar);
    env.insert(ldVar, ldPath);

    // Also add runtime dir to PATH to find helpers if needed
    QString path = env.value("PATH");
    env.insert("PATH", bgdiDir + ":" + path + ":/usr/bin:/bin:/usr/local/bin");

    env.insert("HOME", QDir::homePath());
  }
#endif

  m_process->setProcessEnvironment(env);
  m_process->setWorkingDirectory(rootDir);

  QString exe = QDir::toNativeSeparators(m_bgdiPath);

  // EXTERNAL TERMINAL LAUNCH FAILED DUE TO SANDBOX.
  // FALLBACK TO INTERNAL EXECUTION BUT FIXING THE ENVIRONMENT MANUALLY.

  // 1. Force Wrapper Logic (Always use run.sh if available)
  QString cmd;
  if (QFile::exists(script)) {
    cmd = QString("%1 %2").arg(script).arg(rootDir);
  } else {
    cmd = QString("%1 %2").arg(exe).arg(dcbRelative);
  }

#ifdef Q_OS_LINUX
  emit executeInTerminal("Running (SANDBOX MODE): " + cmd + "\n");

  // CRITICAL: Inject Audio Environment Variables manually
  // If we are in a sandbox, these might be missing or pointing to wrong places.
  // We try to guess the host pulse socket.
  QProcessEnvironment fixedEnv = env;

  QString runDir = QString("/run/user/%1").arg(getuid());
  if (!fixedEnv.contains("XDG_RUNTIME_DIR")) {
    fixedEnv.insert("XDG_RUNTIME_DIR", runDir);
    emit executeInTerminal("DEBUG: Injected XDG_RUNTIME_DIR=" + runDir + "\n");
  }

  if (!fixedEnv.contains("PULSE_SERVER")) {
    // Try standard pulse socket
    QString pulseSock = "unix:" + runDir + "/pulse/native";
    fixedEnv.insert("PULSE_SERVER", pulseSock);
    emit executeInTerminal("DEBUG: Injected PULSE_SERVER=" + pulseSock + "\n");
  }

  // Force SDL to try PulseAudio
  fixedEnv.insert("SDL_AUDIODRIVER", "pulseaudio");

  m_process->setProcessEnvironment(fixedEnv);

  // Launch via simple shell (no interactive, no external)
  m_process->start("/bin/sh", QStringList() << "-c" << cmd);
#else
  emit executeInTerminal("Running (Direct): " + exe + " " + dcbRelative + "\n");
  m_process->start(exe, QStringList() << dcbRelative);
#endif

  m_isrunning = true;
}

void BuildManager::buildAndRunProject(const QString &projectPath) {
  if (m_isrunning)
    return;
  m_autoRunAfterBuild = true;
  m_targetDcbName = "main.dcb";
  buildProject(projectPath, "main.prg");
}

#include <QDirIterator>
#include <QJsonDocument>
#include <QJsonObject>

// ... (existing code up to runScene)

void BuildManager::runScene(const QString &projectPath,
                            const QString &sceneName) {
  if (m_isrunning)
    return;

  // Detect Scene Resolution
  int w = 640;
  int h = 480;

  QString sceneFile;
  QDirIterator it(projectPath, QStringList() << "*.scn",
                  QDir::Files | QDir::NoSymLinks, QDirIterator::Subdirectories);
  while (it.hasNext()) {
    it.next();
    if (it.fileInfo().baseName() == sceneName) {
      sceneFile = it.filePath();
      break;
    }
  }

  if (!sceneFile.isEmpty()) {
    QFile sFile(sceneFile);
    if (sFile.open(QIODevice::ReadOnly)) {
      QJsonDocument doc = QJsonDocument::fromJson(sFile.readAll());
      if (!doc.isNull()) {
        QJsonObject root = doc.object();
        w = root["width"].toInt(640);
        h = root["height"].toInt(480);
      }
      sFile.close();
    }
  }

  // 0. Generate debug helper (get_asset_path)
  QString includesDir = projectPath + "/src/includes";
  QDir().mkpath(includesDir);
  QString helperPath = includesDir + "/debug_assets.prg";
  QFile hf(helperPath);
  if (hf.open(QIODevice::WriteOnly)) {
    QTextStream out(&hf);
    out << "// Helper temporal para debug de escenas\n";
    out << "function string get_asset_path(string path)\n";
    out << "begin\n";
    out << "    return path;\n";
    out << "end\n";
    hf.close();
  }

  // 1. Generate temp main in src/
  QString tempMainPath = projectPath + "/src/main_debug_scene.prg";
  QFile f(tempMainPath);
  if (f.open(QIODevice::WriteOnly)) {
    QTextStream out(&f);
    out << "import \"libmod_gfx\";\nimport \"libmod_input\";\nimport "
           "\"libmod_misc\";\nimport \"libmod_ray\";\n\n";

    out << "include \"includes/debug_assets.prg\";\n";
    out << "include \"includes/scenes_list.prg\";\n\n";

    QString initCode =
        "    // Inicializar sistema de audio (Estilo Joselkiller)\n"
        "    sound.freq = 44100;\n"
        "    sound.channels = 32;\n"
        "    int audio_status = soundsys_init();\n"
        "    reserve_channels(24);\n"
        "    set_master_volume(128);\n"
        "    music_set_volume(128);\n"
        "    say(\"AUDIO: Init status \" + audio_status + \" (Driver: \" + "
        "getenv(\"SDL_AUDIODRIVER\") + \")\");\n"
        "\n"
        "    say(\"CWD: \" + cd());\n";

    out << "process main()\nbegin\n";
    out << initCode; // Insert the initialization code here
    out << "    " + sceneName + "();\n";
    out << "    loop frame; end\n";
    out << "end\n";
    f.close();
  }

  m_autoRunAfterBuild = true;
  m_targetDcbName = "main_debug_scene.dcb";
  buildProject(projectPath, "main_debug_scene.prg");
}

void BuildManager::stopRunning() {
  if (m_isrunning && m_process->state() != QProcess::NotRunning) {
    m_process->kill();
    emit executeInTerminal("\nProcess terminated by user.\n");
  }
}

void BuildManager::onProcessOutput() {
  QByteArray data = m_process->readAllStandardOutput();
  emit executeInTerminal(QString::fromLocal8Bit(data));
  data = m_process->readAllStandardError();
  emit executeInTerminal(QString::fromLocal8Bit(data));
}

void BuildManager::onProcessFinished(int exitCode,
                                     QProcess::ExitStatus exitStatus) {
  m_isrunning = false;

  if (m_autoRunAfterBuild && exitCode == 0 &&
      exitStatus == QProcess::NormalExit) {
    m_autoRunAfterBuild = false; // Clear flag
    emit buildFinished(true);

    QString dcb = m_targetDcbName.isEmpty() ? "main.dcb" : m_targetDcbName;
    m_targetDcbName = ""; // Reset

    runProject(m_currentProjectPath, dcb);
  } else if (m_autoRunAfterBuild) {
    // Build failed
    m_autoRunAfterBuild = false;
    m_targetDcbName = "";
    emit buildFinished(false);
    emit executeInTerminal("\nBuild Failed. Cannot run.\n");
  } else {
    // Just finished
    emit executeInTerminal(
        QString("\nProcess finished with exit code %1\n").arg(exitCode));
    emit runFinished();
  }
}
