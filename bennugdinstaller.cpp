#include "bennugdinstaller.h"
#include <QVBoxLayout>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QFile>
#include <QDir>
#include <QStandardPaths>
#include <QMessageBox>
#include <QProcess>
#include <QDebug>
#include <QPainter>
#include <QApplication>

BennuGDInstaller::BennuGDInstaller(QWidget *parent)
    : QDialog(parent)
    , m_networkManager(new QNetworkAccessManager(this))
    , m_currentReply(nullptr)
{
    setWindowTitle("Instalar Runtimes de BennuGD2");
    setMinimumWidth(600);
    
    QVBoxLayout *layout = new QVBoxLayout(this);
    
    // Banner Image
    QLabel *banner = new QLabel(this);
    QPixmap p(":/images/installer_banner.png"); // Placeholder path
    if (p.isNull()) {
        // Fallback or just text style?
        // Let's create a nice gradient placeholder if missing
        p = QPixmap(600, 150);
        p.fill(QColor(40, 40, 50));
        QPainter paint(&p);
        paint.setPen(Qt::white);
        paint.setFont(QFont("Arial", 24, QFont::Bold));
        paint.drawText(p.rect(), Qt::AlignCenter, "BennuGD2 Runtimes");
        paint.end();
    }
    banner->setPixmap(p.scaled(600, 150, Qt::KeepAspectRatioByExpanding, Qt::SmoothTransformation));
    banner->setAlignment(Qt::AlignCenter);
    layout->addWidget(banner);
    
    m_statusLabel = new QLabel("Comprobando instalación...", this);
    m_statusLabel->setWordWrap(true);
    m_statusLabel->setAlignment(Qt::AlignCenter);
    QFont f = m_statusLabel->font();
    f.setPointSize(10);
    m_statusLabel->setFont(f);
    
    m_progressBar = new QProgressBar(this);
    m_progressBar->setRange(0, 100);
    m_progressBar->setValue(0);
    m_progressBar->setTextVisible(true);
    
    m_cancelButton = new QPushButton("Cancelar", this);
    
    layout->addWidget(m_statusLabel);
    layout->addWidget(m_progressBar);
    layout->addWidget(m_cancelButton);
    
    connect(m_cancelButton, &QPushButton::clicked, this, &QDialog::reject);
}

BennuGDInstaller::~BennuGDInstaller()
{
    if (m_currentReply) {
        m_currentReply->abort();
        m_currentReply->deleteLater();
    }
}

bool BennuGDInstaller::checkMissingRuntimes()
{
    // Check if standard runtime folders exist
    QString root = QDir::homePath() + "/.bennugd2/runtimes";
    QStringList platforms = {"linux-gnu", "win64", "macos", "android", "switch", "web"};
    
    for (const QString &p : platforms) {
        if (!QDir(root + "/" + p).exists()) return true; // Missing
    }
    return false; // All present
}

void BennuGDInstaller::startInstallation()
{
    bool missing = checkMissingRuntimes();
    
    if (missing) {
        QMessageBox::StandardButton res = QMessageBox::question(this, "Runtimes Faltantes", 
            "Se han detectado runtimes faltantes necesarios para compilar y exportar.\n"
            "¿Desea descargarlos ahora desde el repositorio oficial? (Recomendado)",
            QMessageBox::Yes | QMessageBox::No);
            
        if (res == QMessageBox::Yes) {
            m_downloadQueue.clear();
            m_statusLabel->setText("Iniciando descarga...");
            fetchLatestRelease(); // Will use manual URLs
        } else {
            reject();
        }
    } else {
        // Already installed
         QMessageBox::StandardButton res = QMessageBox::question(this, "Runtimes Instalados", 
            "Parece que los runtimes ya están instalados.\n¿Desea volver a descargarlos y reinstalarlos?",
            QMessageBox::Yes | QMessageBox::No);
            
        if (res == QMessageBox::Yes) {
             m_downloadQueue.clear();
             fetchLatestRelease();
        } else {
             accept(); // Done
        }
    }
}

void BennuGDInstaller::fetchLatestRelease()
{
    // Use Manual URLs provided
    m_downloadQueue.clear();
    
    // Linux
    m_downloadQueue.append({
        "https://github.com/Rufidj/raymap_editor/raw/main/runtimes/linux-gnu.tar.gz",
        "linux-gnu",
        "linux-gnu.tar.gz"
    });
    
    // Windows 
    m_downloadQueue.append({
        "https://github.com/Rufidj/raymap_editor/raw/main/runtimes/win64.tar.gz",
        "win64",
        "win64.tar.gz"
    });
    
    // MacOS
    m_downloadQueue.append({
        "https://github.com/Rufidj/raymap_editor/raw/main/runtimes/MacOsx.tar.gz",
        "macos",
        "macos.tar.gz"
    });
    
    // Android
    m_downloadQueue.append({
        "https://github.com/Rufidj/raymap_editor/raw/main/runtimes/android.tar.gz",
        "android",
        "android.tar.gz"
    });
    
    // Switch
     m_downloadQueue.append({
        "https://github.com/Rufidj/raymap_editor/raw/main/runtimes/switch.tar.gz",
        "switch",
        "switch.tar.gz"
    });
    
    // Web
     m_downloadQueue.append({
        "https://github.com/Rufidj/raymap_editor/raw/main/runtimes/web.tar.gz",
        "web",
        "web.tar.gz"
    });
    
    m_statusLabel->setText("Descargando runtimes desde repositorio...");
    m_progressBar->setValue(10);
    
    processDownloadQueue();
    return;
}

void BennuGDInstaller::processDownloadQueue()
{
    if (m_downloadQueue.isEmpty()) {
        // All done
        m_progressBar->setValue(100);
        m_statusLabel->setText("¡Instalación completada!");
        QMessageBox::information(this, "Éxito", "Todos los runtimes han sido descargados e instalados en ~/.bennugd2/runtimes.");
        emit installationFinished(true);
        accept();
        return;
    }
    
    m_currentTask = m_downloadQueue.takeFirst();
    downloadNextItem();
}

void BennuGDInstaller::downloadNextItem()
{
    m_statusLabel->setText("Descargando runtime para: " + m_currentTask.platform + "...");
    m_progressBar->setValue(20); // Reset roughly
    
    qDebug() << "Descending: " << m_currentTask.url;
    
    QNetworkRequest request(QUrl(m_currentTask.url));
    request.setHeader(QNetworkRequest::UserAgentHeader, "RayMapEditor");
    request.setAttribute(QNetworkRequest::RedirectPolicyAttribute, QNetworkRequest::NoLessSafeRedirectPolicy);
    
    // Determine extension
    QString extension = ".tgz";
    if (m_currentTask.filename.endsWith(".rar", Qt::CaseInsensitive)) extension = ".rar";
    else if (m_currentTask.filename.endsWith(".zip", Qt::CaseInsensitive)) extension = ".zip";
    else if (m_currentTask.filename.endsWith(".tar.gz", Qt::CaseInsensitive)) extension = ".tar.gz";
    
    m_tempFilePath = QStandardPaths::writableLocation(QStandardPaths::TempLocation) + "/bgd2_dl_" + m_currentTask.platform + extension;
    
    m_currentReply = m_networkManager->get(request);
    connect(m_currentReply, &QNetworkReply::downloadProgress, this, &BennuGDInstaller::onDownloadProgress);
    connect(m_currentReply, &QNetworkReply::finished, this, &BennuGDInstaller::onDownloadFinished);
}

void BennuGDInstaller::onDownloadProgress(qint64 bytesReceived, qint64 bytesTotal)
{
    if (bytesTotal > 0) {
        int percentage = (int)((bytesReceived * 100) / bytesTotal);
        m_progressBar->setValue(percentage);
        m_statusLabel->setText(QString("Descargando (%1): %2%").arg(m_currentTask.platform).arg(percentage));
    }
}

void BennuGDInstaller::onDownloadFinished()
{
    if (m_currentReply->error() != QNetworkReply::NoError) {
        QMessageBox::warning(this, "Error de Descarga", "Falló descarga de " + m_currentTask.platform + ": " + m_currentReply->errorString());
        // Continue with others?
        m_currentReply->deleteLater();
        m_currentReply = nullptr;
        processDownloadQueue();
        return;
    }
    
    QByteArray data = m_currentReply->readAll();
    m_currentReply->deleteLater();
    m_currentReply = nullptr;
    
    QFile file(m_tempFilePath);
    if (file.open(QIODevice::WriteOnly)) {
        file.write(data);
        file.close();
        extractAndInstall(m_tempFilePath, m_currentTask.platform);
    } else {
        qDebug() << "Failed to write temp file";
    }
    
    processDownloadQueue();
}

void BennuGDInstaller::extractAndInstall(const QString &filePath, const QString &platform)
{
    m_statusLabel->setText("Extrayendo: " + platform + "...");
    QApplication::processEvents(); // Update UI
    
    QString runtimeRoot = QDir::homePath() + "/.bennugd2/runtimes";
    QString targetDir = runtimeRoot + "/" + platform;
    
    // Cleanup old
    QDir(targetDir).removeRecursively();
    QDir().mkpath(targetDir);
    
    QProcess extractor;
    extractor.setWorkingDirectory(targetDir);
    
    QString program;
    QStringList args;
    
    if (filePath.endsWith(".rar", Qt::CaseInsensitive)) {
        program = "unrar";
        args << "x" << "-o+" << filePath;
    }
    else if (filePath.endsWith(".zip", Qt::CaseInsensitive)) {
        program = "unzip";
        args << "-o" << filePath;
    }
    else {
        // Assume tar
        program = "tar";
        // Always try to strip one level of folders, as most archives come with a root folder.
        // If this fails (archives without root folder), we might need retry logic or smart detection.
        // But for BennuGD2 official builds, they usually have root folder.
        args << "-xzf" << filePath << "--strip-components=1";
    }
    
    extractor.start(program, args);
    bool finish = extractor.waitForFinished(60000);
    
    if (!finish || extractor.exitCode() != 0) {
        qDebug() << "Extraction failed for " << platform << ". STDERR: " << extractor.readAllStandardError();
        // Warn user but continue
    } else {
        // Platform specific post-processing
        
        // MACOS SPECIAL HANDLING
        if (platform == "macos") {
             // The files inside MacOsx.tar.gz are seemingly bgdcMac.tgz, bgdiMac.tgz, moddescMac.tgz
             QDir macDir(targetDir);
             QStringList archives = {"bgdcMac.tgz", "bgdiMac.tgz", "moddescMac.tgz"};
             
             for (const QString &archiveName : archives) {
                 if (macDir.exists(archiveName)) {
                     // Extract TGZ
                     QProcess tar;
                     tar.setWorkingDirectory(targetDir);
                     tar.start("tar", QStringList() << "-xzf" << archiveName);
                     tar.waitForFinished();
                     macDir.remove(archiveName); // Clean archive
                 }
             }
             
             // Now we expect bgdc.app and bgdi.app (or similar)
             // Move binaries to root: bgdc.app/Contents/MacOS/bgdc -> ./bgdc
             auto flattenApp = [&](QString appName, QString binaryName) {
                 // Try to guess the app name if not standard. 
                 // Assuming bgdc.app and bgdi.app based on previous context, 
                 // but possibly bgdcMac.app? Let's try standard first.
                 
                 QStringList potentialNames = {appName, appName + "Mac", appName + "OSX"};
                 QString bundlePath;
                 
                 for(const QString &name : potentialNames) {
                     if (QDir(targetDir + "/" + name + ".app").exists()) {
                         bundlePath = targetDir + "/" + name + ".app";
                         break;
                     }
                 }
                 
                 if (!bundlePath.isEmpty()) {
                     QString binaryPath = bundlePath + "/Contents/MacOS/" + binaryName;
                     
                     if (QFile::exists(binaryPath)) {
                         // Move binary to root
                         QString targetPath = targetDir + "/" + binaryName;
                         QFile::remove(targetPath);
                         QFile::copy(binaryPath, targetPath);
                         QFile(targetPath).setPermissions(QFile::ExeUser | QFile::ExeGroup | QFile::ExeOther | QFile::ReadOwner | QFile::ReadGroup);
                     }
                 }
             };
             
             flattenApp("bgdc", "bgdc");
             flattenApp("bgdi", "bgdi");
             flattenApp("moddesc", "moddesc");
        }

        // LINUX SPECIAL HANDLING
        #ifdef Q_OS_LINUX
        if (platform == "linux") {
             QString binDir = QDir::homePath() + "/.bennugd2/bin";
             QDir().mkpath(binDir);
             // Copy binaries
             // TODO: Smarter copy. For now just copy everything from targetDir to binDir
             QProcess cp;
             cp.start("cp", QStringList() << "-r" << (targetDir + "/.") << binDir);
             cp.waitForFinished();
             
             // Set executable flags
             QProcess chmod;
             chmod.start("chmod", QStringList() << "+x" << binDir + "/bgdc" << binDir + "/bgdi");
             chmod.waitForFinished();
        }
        #endif
    }
    
    QFile::remove(filePath);
}

