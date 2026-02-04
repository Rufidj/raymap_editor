cmake_minimum_required(VERSION 3.16)

project(raymap_editor VERSION 1.0 LANGUAGES CXX)

# Qt automatic tools
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

# C++ standard
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# -----------------------------
# Find dependencies
# -----------------------------

# ZLIB (requerido para FPG)
find_package(ZLIB REQUIRED)

# Qt (Qt6 preferred)
find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Widgets)

if(${QT_VERSION_MAJOR} GREATER_EQUAL 6)
    find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Widgets OpenGL Network OpenGLWidgets)
else()
    find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Widgets OpenGL Network)
endif()

# OpenGL
find_package(OpenGL REQUIRED)

# -----------------------------
# Project sources
# -----------------------------
set(PROJECT_SOURCES
    main.cpp
    mainwindow.cpp
    mainwindow.h
    mainwindow.ui
    mainwindow_darkmode.cpp
    mainwindow_build.cpp
    mainwindow_project.cpp
    newprojectdialog.h newprojectdialog.cpp
    projectsettingsdialog.h projectsettingsdialog.cpp
    consolewidget.h consolewidget.cpp
    assetbrowser.h assetbrowser.cpp
    buildmanager.h buildmanager.cpp
    bennugdinstaller.h bennugdinstaller.cpp
    codegenerator.h codegenerator.cpp
    processgenerator.h processgenerator.cpp
    projectmanager.h projectmanager.cpp
    codepreviewpanel.h codepreviewpanel.cpp
    codeeditor.h codeeditor.cpp
    codeeditordialog.h codeeditordialog.cpp
    md3loader.h md3loader.cpp
    entitypropertypanel.h entitypropertypanel.cpp
    entitybehaviordialog.h entitybehaviordialog.cpp
    objtomd3converter.h objtomd3converter.cpp
    objimportdialog.h objimportdialog.cpp
    publishdialog.h publishdialog.cpp
    publisher.h publisher.cpp
    downloader.h downloader.cpp
    fonteditordialog.h fonteditordialog.cpp
    resources.qrc
)

if(WIN32)
    list(APPEND PROJECT_SOURCES assets/app_icon.rc)
endif()

# -----------------------------
# Add executable
# -----------------------------
if(${QT_VERSION_MAJOR} GREATER_EQUAL 6)
    qt_add_executable(raymap_editor
        MANUAL_FINALIZATION
        ${PROJECT_SOURCES}
        fpgloader.h fpgloader.cpp
        fpgeditor.h fpgeditor.cpp
        effectgenerator.h effectgenerator.cpp
        effectgeneratordialog.h effectgeneratordialog.cpp
        camerakeyframe.h
        camerapath.h camerapath.cpp
        camerapathio.h camerapathio.cpp
        camerapatheditor.h camerapatheditor.cpp
        camerapathcanvas.h camerapathcanvas.cpp
        grideditor.h grideditor.cpp
        textureselector.h textureselector.cpp
        spriteeditor.h spriteeditor.cpp
        cameramarker.h cameramarker.cpp
        raymapformat.h raymapformat.cpp
        mapdata.h
        visualrenderer.h visualrenderer.cpp
        visualmodewidget.h visualmodewidget.cpp
        wldimporter.h wldimporter.cpp
        raycastrenderer.h raycastrenderer.cpp
        insertboxdialog.h insertboxdialog.cpp
        meshgeneratordialog.h meshgeneratordialog.cpp
        rampgenerator.h rampgenerator.cpp
        rampgeneratordialog.h rampgeneratordialog.cpp
        md3generator.h md3generator.cpp
        modelpreviewwidget.h modelpreviewwidget.cpp
        textureatlasgen.h textureatlasgen.cpp
    )
else()
    add_executable(raymap_editor
        ${PROJECT_SOURCES}
        fpgloader.h fpgloader.cpp
        fpgeditor.h fpgeditor.cpp
        effectgenerator.h effectgenerator.cpp
        effectgeneratordialog.h effectgeneratordialog.cpp
        camerakeyframe.h
        camerapath.h camerapath.cpp
        camerapathio.h camerapathio.cpp
        camerapatheditor.h camerapatheditor.cpp
        camerapathcanvas.h camerapathcanvas.cpp
        grideditor.h grideditor.cpp
        textureselector.h textureselector.cpp
        spriteeditor.h spriteeditor.cpp
        cameramarker.h cameramarker.cpp
        raymapformat.h raymapformat.cpp
        mapdata.h
        visualrenderer.h visualrenderer.cpp
        visualmodewidget.h visualmodewidget.cpp
        wldimporter.h wldimporter.cpp
        raycastrenderer.h raycastrenderer.cpp
        insertboxdialog.h insertboxdialog.cpp
        meshgeneratordialog.h meshgeneratordialog.cpp
        rampgenerator.h rampgenerator.cpp
        rampgeneratordialog.h rampgeneratordialog.cpp
        md3generator.h md3generator.cpp
        modelpreviewwidget.h modelpreviewwidget.cpp
        textureatlasgen.h textureatlasgen.cpp
    )
endif()

# -----------------------------
# Link libraries
# -----------------------------
target_link_libraries(raymap_editor PRIVATE
    Qt${QT_VERSION_MAJOR}::Widgets
    Qt${QT_VERSION_MAJOR}::OpenGL
    Qt${QT_VERSION_MAJOR}::Network
    ${OPENGL_LIBRARIES}
    ZLIB::ZLIB
)

if(${QT_VERSION_MAJOR} GREATER_EQUAL 6)
    target_link_libraries(raymap_editor PRIVATE Qt${QT_VERSION_MAJOR}::OpenGLWidgets)
endif()

# -----------------------------
# Properties
# -----------------------------
set_target_properties(raymap_editor PROPERTIES
    MACOSX_BUNDLE_GUI_IDENTIFIER com.bennugd.raymap_editor
    MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
    MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
    MACOSX_BUNDLE TRUE
    WIN32_EXECUTABLE TRUE
)

# -----------------------------
# Install
# -----------------------------
include(GNUInstallDirs)
install(TARGETS raymap_editor
    BUNDLE DESTINATION .
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

# -----------------------------
# Qt finalize for Qt6
# -----------------------------


# -----------------------------
# Copy loader_stub files (always)
# -----------------------------
add_custom_command(TARGET raymap_editor POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
    "${CMAKE_SOURCE_DIR}/loader_stub.exe"
    "$<TARGET_FILE_DIR:raymap_editor>/loader_stub.exe"
    COMMENT "Copying loader_stub.exe to output directory"
)

add_custom_command(TARGET raymap_editor POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
    "${CMAKE_SOURCE_DIR}/loader_stub"
    "$<TARGET_FILE_DIR:raymap_editor>/loader_stub"
    COMMENT "Copying loader_stub (Linux) to output directory"
)
