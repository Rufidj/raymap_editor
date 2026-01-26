#include "buildmanager.h"
#include <QDebug>
#include <QDir>
#include <QStandardPaths>
#include <QSettings>
#include <QCoreApplication>

BuildManager::BuildManager(QObject *parent) 
    : QObject(parent), m_process(new QProcess(this)), m_isrunning(false), m_autoRunAfterBuild(false)
{
    connect(m_process, &QProcess::readyReadStandardOutput, this, &BuildManager::onProcessOutput);
    connect(m_process, &QProcess::readyReadStandardError, this, &BuildManager::onProcessOutput);
    connect(m_process, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this, &BuildManager::onProcessFinished);
    connect(m_process, &QProcess::errorOccurred, this, [this](QProcess::ProcessError error){
        emit executeInTerminal("Process Error: " + m_process->errorString() + "\n");
        m_isrunning = false;
        emit buildFinished(false);
    });
            
    detectBennuGD2();
}

void BuildManager::detectBennuGD2()
{
    // Check settings first
    QSettings settings("BennuGD", "RayMapEditor");
    QString customPath = settings.value("bennugdPath").toString();
    
    // Migration Logic: If custom path points to old "runtime" folder (singular) 
    // but we have new standard "runtimes" (plural), prefer the new one.
    if (!customPath.isEmpty() && customPath.contains("/runtime/")) {
       QString testNew = QDir::homePath() + "/.bennugd2/runtimes";
       if (QDir(testNew).exists()) {
           qDebug() << "Detected old runtime path in settings:" << customPath;
           qDebug() << "New runtimes found at" << testNew << ". Switching preferences.";
           customPath = ""; // Ignore old setting
           settings.remove("bennugdPath"); // Clear it permanently
       }
    }
    
    if (!customPath.isEmpty()) {
        m_bgdcPath = customPath + "/bgdc";
        m_bgdiPath = customPath + "/bgdi";
        
        #ifdef Q_OS_WIN
        if (!m_bgdcPath.endsWith(".exe")) m_bgdcPath += ".exe";
        if (!m_bgdiPath.endsWith(".exe")) m_bgdiPath += ".exe";
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

    // Check standard paths with priority to ~/.bennugd2/runtimes
    QString homePath = QDir::homePath();
    QStringList paths;
    paths << homePath + "/.bennugd2/runtimes/" + platform + "/bin";
    paths << homePath + "/.bennugd2/runtimes/" + platform; // Check root too
    paths << homePath + "/.bennugd2/bin";
    // Also check relative to executable (Portable Mode)
    paths << QCoreApplication::applicationDirPath() + "/runtimes/" + platform + "/bin";
    paths << QCoreApplication::applicationDirPath() + "/runtimes/" + platform;
    paths << QCoreApplication::applicationDirPath() + "/bin";
    paths << QCoreApplication::applicationDirPath(); // Check app dir itself (for deployed releases)
    
    paths << "/usr/local/bin";
    paths << "/usr/bin";
    paths << "/opt/bennugd2/bin";
    paths << homePath + "/bennugd2/bin";
    paths << homePath + "/.local/bin";
    paths << QDir::currentPath();
    
    qDebug() << "Detecting BennuGD2 for platform:" << platform;
    for(const QString& p : paths) qDebug() << "Checking path:" << p;
    
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

bool BuildManager::isBennuGD2Installed()
{
    return !m_bgdcPath.isEmpty() && !m_bgdiPath.isEmpty();
}

void BuildManager::setCustomBennuGDPath(const QString &path)
{
    QSettings settings("BennuGD", "RayMapEditor");
    settings.setValue("bennugdPath", path);
    detectBennuGD2();
}

void BuildManager::buildProject(const QString &projectPath)
{
    if (m_isrunning) return;
    if (m_bgdcPath.isEmpty()) {
        emit executeInTerminal("Error: BennuGD2 compilers not found!\n");
        return;
    }
    
    m_currentProjectPath = projectPath;
    m_autoRunAfterBuild = false; // Just build
    
    QString mainFile = projectPath + "/src/main.prg";
    
    emit buildStarted();
    emit executeInTerminal("Compiling: " + mainFile + "\n");
    
    // Set environment vars
    QFileInfo bgdcInfo(m_bgdcPath);
    QString bgdcDir = bgdcInfo.absolutePath();
    
    // Generic Wrapper Logic for ALL platforms
    QString scriptExtension = "";
    #ifdef Q_OS_WIN
    scriptExtension = ".bat";
    #else
    scriptExtension = ".sh";
    #endif
    
    QString script = bgdcDir + "/compile" + scriptExtension;
    QString srcDir = QDir::toNativeSeparators(projectPath + "/src");
    // mainFile is already defined above
    
    QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
    
    // Clean environment logic (Linux/Mac) or Path extension (Windows)
    #ifdef Q_OS_WIN
        QString currentPath = env.value("PATH");
        env.insert("PATH", QDir::toNativeSeparators(bgdcDir) + ";" + currentPath);
    #else
        env.insert("PATH", "/usr/bin:/bin:/usr/local/bin");
        env.insert("HOME", QDir::homePath());
    #endif
    
    m_process->setProcessEnvironment(env);
    
    if (QFile::exists(script)) {
        // Wrapper exists (Old bundled style or manually added)
        script = QDir::toNativeSeparators(script);
        emit executeInTerminal("Wrapper: " + script + " " + srcDir + "\n");
        m_process->start(script, QStringList() << srcDir);
    } else {
        // Direct execution (New downloaded style)
        
        #ifndef Q_OS_WIN
        // Unix Lib Path Setup
        QString libDir = QDir(bgdcDir + "/../lib").canonicalPath();
        if (libDir.isEmpty()) libDir = bgdcDir; // Fallback to same dir
        
        QString ldVar = "LD_LIBRARY_PATH";
        #ifdef Q_OS_MAC
        ldVar = "DYLD_LIBRARY_PATH";
        #endif
        
        QString ldPath = libDir;
        if (env.contains(ldVar)) ldPath += ":" + env.value(ldVar);
        env.insert(ldVar, ldPath);
        env.insert("BENNU_LIB_PATH", libDir); // Often needed
        m_process->setProcessEnvironment(env); // Re-set env with lib path
        #endif

        QString exe = QDir::toNativeSeparators(m_bgdcPath);
        emit executeInTerminal("Compiling (Direct): " + exe + " " + mainFile + "\n");
        m_process->setWorkingDirectory(srcDir);
        m_process->start(exe, QStringList() << "main.prg");
    }
    
    m_isrunning = true;
}

void BuildManager::runProject(const QString &projectPath)
{
    if (m_isrunning) return;
    if (m_bgdiPath.isEmpty()) {
        emit executeInTerminal("Error: BennuGD2 interpreter not found!\n");
        return;
    }
    
    m_currentProjectPath = projectPath;
    QString dcbFile = projectPath + "/src/main.dcb";
    
    emit runStarted();
    emit executeInTerminal("Running: " + dcbFile + "\n");
    
    // Set environment vars to help bgdi find modules (in same dir as executable)
    QFileInfo bgdiInfo(m_bgdiPath);
    QString bgdiDir = bgdiInfo.absolutePath();

    // Generic Wrapper Logic for ALL platforms
    QString scriptExtension = "";
    #ifdef Q_OS_WIN
    scriptExtension = ".bat";
    #else
    scriptExtension = ".sh";
    #endif
    
    QString script = bgdiDir + "/run" + scriptExtension;
    QString rootDir = QDir::toNativeSeparators(projectPath);
    
    QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
    
    // Clean environment logic (Linux/Mac) or Path extension (Windows)
    #ifdef Q_OS_WIN
        QString currentPath = env.value("PATH");
        env.insert("PATH", QDir::toNativeSeparators(bgdiDir) + ";" + currentPath);
    #else
        env.insert("PATH", "/usr/bin:/bin:/usr/local/bin");
        env.insert("HOME", QDir::homePath());
        
        QProcessEnvironment sys = QProcessEnvironment::systemEnvironment();
        QStringList keepVars = {
            "DISPLAY", "WAYLAND_DISPLAY", "XDG_RUNTIME_DIR", "XDG_SESSION_TYPE",
            "PULSE_SERVER", "DBUS_SESSION_BUS_ADDRESS", "LANG", "LC_ALL"
        };
        for(const QString &var : keepVars) {
            if(sys.contains(var)) env.insert(var, sys.value(var));
        }
    #endif
    
    m_process->setProcessEnvironment(env);
    
    // Important: setWorkingDirectory requires native separators on some Windows configs if rootDir comes from URI
    m_process->setWorkingDirectory(rootDir);

    if (QFile::exists(script)) {
        // Wrapper exists
        script = QDir::toNativeSeparators(script);
        emit executeInTerminal("Wrapper: " + script + " " + rootDir + "\n");
        #ifdef Q_OS_WIN
        m_process->start(script, QStringList() << rootDir); 
        #else
        m_process->start(script, QStringList() << rootDir);
        #endif
    } else {
         // Direct execution fallback
        
        #ifndef Q_OS_WIN
        // Unix Lib Path Setup
        QString libDir = QDir(bgdiDir + "/../lib").canonicalPath();
        if (libDir.isEmpty()) libDir = bgdiDir;
        
        QString ldVar = "LD_LIBRARY_PATH";
        #ifdef Q_OS_MAC
        ldVar = "DYLD_LIBRARY_PATH";
        #endif
        
        QString ldPath = libDir;
        if (env.contains(ldVar)) ldPath += ":" + env.value(ldVar); 
        
        env.insert(ldVar, ldPath);
        env.insert("BENNU_LIB_PATH", libDir);
        m_process->setProcessEnvironment(env); // Re-set env with lib path
        #endif
        
        QString exe = QDir::toNativeSeparators(m_bgdiPath);
        emit executeInTerminal("Running (Direct): " + exe + " src/main.dcb\n");
        m_process->start(exe, QStringList() << "src/main.dcb");
    }
    
    m_isrunning = true;
}

void BuildManager::buildAndRunProject(const QString &projectPath)
{
    if (m_isrunning) return;
    m_autoRunAfterBuild = true;
    buildProject(projectPath); // Will trigger run in onProcessFinished
}

void BuildManager::stopRunning()
{
    if (m_isrunning && m_process->state() != QProcess::NotRunning) {
        m_process->kill();
        emit executeInTerminal("\nProcess terminated by user.\n");
    }
}

void BuildManager::onProcessOutput()
{
    QByteArray data = m_process->readAllStandardOutput();
    emit executeInTerminal(QString::fromLocal8Bit(data));
    data = m_process->readAllStandardError();
    emit executeInTerminal(QString::fromLocal8Bit(data));
}

void BuildManager::onProcessFinished(int exitCode, QProcess::ExitStatus exitStatus)
{
    m_isrunning = false;
    
    if (m_autoRunAfterBuild && exitCode == 0 && exitStatus == QProcess::NormalExit) {
        m_autoRunAfterBuild = false; // Clear flag
        emit buildFinished(true);
        runProject(m_currentProjectPath);
    } else if (m_autoRunAfterBuild) {
        // Build failed
        m_autoRunAfterBuild = false;
        emit buildFinished(false);
        emit executeInTerminal("\nBuild Failed. Cannot run.\n");
    } else {
        // Just finished
        emit executeInTerminal(QString("\nProcess finished with exit code %1\n").arg(exitCode));
        emit runFinished();
    }
}
