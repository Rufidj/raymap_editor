#include "mainwindow.h"
#include <QApplication>

#include <QSurfaceFormat>

int main(int argc, char *argv[])
{
    // Force clean environment to avoid theme/session crashes on Linux
    qputenv("QT_QPA_PLATFORMTHEME", "generic"); 
    qputenv("QT_STYLE_OVERRIDE", "Fusion");
    qputenv("QT_NO_SESSION_MANAGER", "1");

    // Standard OpenGL setup
    QSurfaceFormat format;
    format.setDepthBufferSize(24);
    format.setProfile(QSurfaceFormat::CompatibilityProfile);
    QSurfaceFormat::setDefaultFormat(format);
    
    // Disable Qt session management as it can cause crashes on some Linux setups
    qputenv("QT_NO_SESSION_MANAGER", "1");

    QApplication app(argc, argv);
    app.setStyle("Fusion"); // Force Fusion style early to avoid GTK theme issues
    qDebug() << "Application object created.";
    app.setApplicationName("RayMap Editor");
    app.setApplicationVersion("1.0");
    app.setOrganizationName("BennuGD2");
    
    qDebug() << "Creating MainWindow...";
    MainWindow window;
    qDebug() << "MainWindow created, showing...";
    window.show();
    qDebug() << "MainWindow shown, entering event loop...";
    
    int ret = app.exec();
    qDebug() << "Application finished with code:" << ret;
    return ret;
}
