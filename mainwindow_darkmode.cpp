#include "mainwindow.h"
#include <QApplication>
#include <QPalette>
#include <QStyleFactory>
#include <QSettings>
#include "projectmanager.h"
#include "assetbrowser.h"
#include <QFileInfo>

void MainWindow::onToggleDarkMode(bool checked)
{
    if (checked) {
        // qApp->setStyle(QStyleFactory::create("Fusion")); // Already set in main.cpp

        QPalette p;
        p.setColor(QPalette::Window, QColor(53, 53, 53));
        p.setColor(QPalette::WindowText, Qt::white);
        p.setColor(QPalette::Base, QColor(42, 42, 42)); // Text inputs
        p.setColor(QPalette::AlternateBase, QColor(66, 66, 66));
        p.setColor(QPalette::ToolTipBase, Qt::white);
        p.setColor(QPalette::ToolTipText, Qt::white);
        p.setColor(QPalette::Text, Qt::white);
        p.setColor(QPalette::Button, QColor(53, 53, 53));
        p.setColor(QPalette::ButtonText, Qt::white);
        p.setColor(QPalette::BrightText, Qt::red);
        p.setColor(QPalette::Link, QColor(42, 130, 218));
        p.setColor(QPalette::Highlight, QColor(42, 130, 218));
        p.setColor(QPalette::HighlightedText, Qt::black);
        
        // Disabled colors
        p.setColor(QPalette::Disabled, QPalette::WindowText, QColor(128, 128, 128));
        p.setColor(QPalette::Disabled, QPalette::Text, QColor(128, 128, 128));
        p.setColor(QPalette::Disabled, QPalette::ButtonText, QColor(128, 128, 128));

        qApp->setPalette(p);
    } else {
        // qApp->setStyle(QStyleFactory::create("Fusion")); 
        qApp->setPalette(style()->standardPalette());
    }
}

void MainWindow::loadSettings()
{
    QSettings settings("BennuGD", "RayMapEditor");
    
    // Window geometry (using v2 to avoid crashes from old corrupt state)
    // restoreGeometry(settings.value("geometry_v2").toByteArray());
    // restoreState(settings.value("windowState_v2").toByteArray()); // DISABLED PERMANENTLY: Causes crashes on Linux
    
    // Force asset dock to left after restoration
    if (m_assetDock) {
        addDockWidget(Qt::LeftDockWidgetArea, m_assetDock);
    }
    
    // Dark Mode
    bool darkMode = settings.value("darkMode", true).toBool(); // Default to Dark Mode
    onToggleDarkMode(darkMode);
    
    // Recent files
    updateRecentMapsMenu();
    updateRecentFPGsMenu();
    
    
    // Last Open Project - DISABLED: User requested not to auto-load
    /*
    QString lastProject = settings.value("lastOpenProject").toString();
    if (!lastProject.isEmpty() && QFile::exists(lastProject)) {
        if (m_projectManager) {
            if (m_projectManager->openProject(lastProject)) {
                if (m_assetBrowser) {
                    m_assetBrowser->setProjectPath(m_projectManager->getProjectPath());
                }
                updateWindowTitle();
                 
                 // Also open main.prg if it exists?
                 // Maybe later.
            }
        }
    }
    */
}

void MainWindow::saveSettings()
{
    QSettings settings("BennuGD", "RayMapEditor");
    
    // Window geometry (using v2)
    // settings.setValue("geometry_v2", saveGeometry());
    // settings.setValue("windowState_v2", saveState()); // DISABLED PERMANENTLY: Causes crashes on Linux
    
    // Check if we are in dark mode (Fusion dark palette)
    // We can assume if window color is dark, mode is dark.
    bool specificDarkMode = (palette().color(QPalette::Window).lightness() < 128);
    settings.setValue("darkMode", specificDarkMode);
    
    // Last Open Project
    if (m_projectManager && m_projectManager->hasProject()) {
        settings.setValue("lastOpenProject", m_projectManager->getProjectPath() + "/" + m_projectManager->getProject()->name + ".bgd2proj");
    } else {
        settings.remove("lastOpenProject");
    }
}
