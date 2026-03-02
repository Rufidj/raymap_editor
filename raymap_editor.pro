QT       += core gui widgets network

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++11

# Nombre del ejecutable
TARGET = raymap_editor
TEMPLATE = app

# Definir directorio de salida
DESTDIR = $$PWD

# Archivos header
HEADERS += \
    assetbrowser.h \
    bennugdinstaller.h \
    bennurenderer.h \
    buildmanager.h \
    camerakeyframe.h \
    cameramarker.h \
    camerapath.h \
    camerapathcanvas.h \
    camerapatheditor.h \
    camerapathio.h \
    codeeditor.h \
    codeeditordialog.h \
    codegenerator.h \
    codepreviewpanel.h \
    consolewidget.h \
    downloader.h \
    effectgenerator.h \
    effectgeneratordialog.h \
    entitybehaviordialog.h \
    entitypropertypanel.h \
    fonteditordialog.h \
    fpgeditor.h \
    fpgloader.h \
    grideditor.h \
    insertboxdialog.h \
    mainwindow.h \
    mapdata.h \
    md3generator.h \
    md3loader.h \
    meshgeneratordialog.h \
    modelpreviewwidget.h \
    newprojectdialog.h \
    objimportdialog.h \
    objtomd3converter.h \
    processgenerator.h \
    projectmanager.h \
    projectsettingsdialog.h \
    publishdialog.h \
    publisher.h \
    rampgenerator.h \
    rampgeneratordialog.h \
    raycastrenderer.h \
    raymapformat.h \
    sceneeditor.h \
    spriteeditor.h \
    textureatlasgen.h \
    texturepalette.h \
    textureselector.h \
    visualmodewidget.h \
    visualrenderer.h \
    wldimporter.h

# Archivos fuente
SOURCES += \
    assetbrowser.cpp \
    bennugdinstaller.cpp \
    bennurenderer.cpp \
    buildmanager.cpp \
    cameramarker.cpp \
    camerapath.cpp \
    camerapathcanvas.cpp \
    camerapatheditor.cpp \
    camerapathio.cpp \
    codeeditor.cpp \
    codeeditordialog.cpp \
    codegenerator.cpp \
    codepreviewpanel.cpp \
    consolewidget.cpp \
    downloader.cpp \
    effectgenerator.cpp \
    effectgeneratordialog.cpp \
    entitybehaviordialog.cpp \
    entitypropertypanel.cpp \
    fonteditordialog.cpp \
    fpgeditor.cpp \
    fpgloader.cpp \
    grideditor.cpp \
    insertboxdialog.cpp \
    main.cpp \
    mainwindow.cpp \
    mainwindow_build.cpp \
    mainwindow_darkmode.cpp \
    mainwindow_project.cpp \
    md3generator.cpp \
    md3loader.cpp \
    meshgeneratordialog.cpp \
    modelpreviewwidget.cpp \
    newprojectdialog.cpp \
    objimportdialog.cpp \
    objtomd3converter.cpp \
    processgenerator.cpp \
    projectmanager.cpp \
    projectsettingsdialog.cpp \
    publishdialog.cpp \
    publisher.cpp \
    rampgenerator.cpp \
    rampgeneratordialog.cpp \
    rampgeneratordialog_texture_slots.cpp \
    raycastrenderer.cpp \
    raymapformat.cpp \
    sceneeditor.cpp \
    spriteeditor.cpp \
    textureatlasgen.cpp \
    texturepalette.cpp \
    textureselector.cpp \
    visualmodewidget.cpp \
    visualrenderer.cpp \
    wldimporter.cpp

# Archivos UI
FORMS += \
    mainwindow.ui

# Configuración de compilación
QMAKE_CXXFLAGS += -Wall -Wextra

# Default rules for deployment
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

RESOURCES += resources.qrc
