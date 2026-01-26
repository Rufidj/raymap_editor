#ifndef BENNUGDINSTALLER_H
#define BENNUGDINSTALLER_H

#include <QDialog>
#include <QProgressBar>
#include <QLabel>
#include <QPushButton>
#include <QNetworkAccessManager>
#include <QNetworkReply>

class BennuGDInstaller : public QDialog
{
    Q_OBJECT

public:
    explicit BennuGDInstaller(QWidget *parent = nullptr);
    ~BennuGDInstaller();
    
    void startInstallation();

signals:
    void installationFinished(bool success);

private slots:
    void onDownloadProgress(qint64 bytesReceived, qint64 bytesTotal);
    void onDownloadFinished();

private:
    struct DownloadTask {
        QString url;
        QString platform; // "linux", "windows", "macos"
        QString filename;
    };

    void fetchLatestRelease();
    void processDownloadQueue();
    void downloadNextItem();
    void extractAndInstall(const QString &filePath, const QString &platform);
    
    QProgressBar *m_progressBar;
    QLabel *m_statusLabel;
    QPushButton *m_cancelButton;
    
    QNetworkAccessManager *m_networkManager;
    QNetworkReply *m_currentReply;
    
    QList<DownloadTask> m_downloadQueue;
    DownloadTask m_currentTask;
    QString m_tempFilePath;
    
    bool checkMissingRuntimes();
};

#endif // BENNUGDINSTALLER_H
