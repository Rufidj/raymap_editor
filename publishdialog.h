#ifndef PUBLISHDIALOG_H
#define PUBLISHDIALOG_H

#include <QDialog>
#include <QComboBox>
#include <QLineEdit>
#include <QCheckBox>
#include <QProgressBar>
#include <QStandardPaths>
#include <QLabel>
#include <QPushButton>
#include "projectmanager.h"
#include "publisher.h"
#include "downloader.h"

class PublishDialog : public QDialog
{
    Q_OBJECT

public:
    explicit PublishDialog(ProjectData *project, QWidget *parent = nullptr);

private slots:
    void onBrowseOutput();
    void onBrowseIcon();
    void onPublish();
    void onPlatformChanged(int index);
    void onDownloadAppImageTool();
    void onDownloadNDK();
    void onInstallEmsdk();
    void onInstallJDK();
    void refreshWindowsTools();

private:
    QString findToolPath(const QString &toolName);
    ProjectData *m_project;
    Publisher m_publisher;
    
    // Paths
    QString m_appImageToolPath;
    QString m_ndkPath;
    
    // UI Elements
    QComboBox *m_platformCombo;
    QLineEdit *m_outputPathEdit;
    
    // Linux Options
    QWidget *m_linuxOptions;
    QCheckBox *m_chkLinuxArchive;
    QCheckBox *m_chkLinuxStandalone;
    QCheckBox *m_chkLinuxAppImage;

    // Switch Options
    QWidget *m_switchOptions;
    QLineEdit *m_switchAuthorEdit;

    // Windows Options
    QWidget *m_windowsOptions;
    QCheckBox *m_chkWindowsStandalone;
    QCheckBox *m_chkWindowsSfx;
    QCheckBox *m_chkWindowsZip;
    QPushButton *m_installMingwBtn;
    QPushButton *m_install7zBtn;

    // Android Options
    QWidget *m_androidOptions;
    QLineEdit *m_packageNameEdit;
    QLineEdit *m_iconPathEdit;
    QCheckBox *m_chkAndroidProject;
    QCheckBox *m_chkAndroidAPK;
    QCheckBox *m_chkInstallDevice;
    
    QLabel *m_jdkStatusLabel;
    QPushButton *m_installJdkBtn;
    QString m_currentJdkPath;

    // Web Options
    QWidget *m_webOptions;
    QLineEdit *m_webTitleEdit;
    QPushButton *m_installEmsdkBtn;
    QLabel *m_emsdkStatusLabel;

    QProgressBar *m_progressBar;
    QPushButton *m_publishButton;
    QPushButton *m_closeButton;

    void setupUI();
    void checkAndroidTools();
};

#endif // PUBLISHDIALOG_H
