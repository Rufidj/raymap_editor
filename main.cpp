#include "mainwindow.h"
#include <QApplication>

#include <QSurfaceFormat>

int main(int argc, char *argv[])
{
    // Ensure Compatibility Profile for legacy OpenGL (glBegin/glEnd)
#ifdef Q_OS_WIN
    qputenv("QT_OPENGL", "desktop");
#endif

    QSurfaceFormat format;
    format.setDepthBufferSize(24);
    format.setProfile(QSurfaceFormat::CompatibilityProfile);
    QSurfaceFormat::setDefaultFormat(format);

    QApplication app(argc, argv);
    
    app.setApplicationName("RayMap Editor");
    app.setApplicationVersion("1.0");
    app.setOrganizationName("BennuGD2");
    
    MainWindow window;
    window.show();
    
    return app.exec();
}
