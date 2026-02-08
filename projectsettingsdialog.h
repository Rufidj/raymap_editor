#ifndef PROJECTSETTINGSDIALOG_H
#define PROJECTSETTINGSDIALOG_H

#include <QDialog>
#include <QLineEdit>
#include <QComboBox>
#include <QSpinBox>
#include <QDoubleSpinBox>
#include <QCheckBox>
#include <QDialogButtonBox>
#include <QPushButton>
#include <QLabel>
#include "projectmanager.h"

class ProjectSettingsDialog : public QDialog
{
    Q_OBJECT

public:
    explicit ProjectSettingsDialog(const ProjectData &data, QWidget *parent = nullptr);
    
    // Returns the modified project data
    ProjectData getProjectData() const;

private slots:
    void onAccept();

private:
    void setupUi();
    void loadSceneList();
    
    ProjectData m_data; // Working copy

    // UI Elements
    QLineEdit *m_nameEdit;
    QLineEdit *m_versionEdit;
    QLineEdit *m_packageEdit; // Android Package Name
    
    // Startup
    QComboBox *m_startupSceneCombo;
    
    QSpinBox *m_widthSpin;
    QSpinBox *m_heightSpin;
    QSpinBox *m_renderWidthSpin;  // New: Internal render resolution (if applicable)
    QSpinBox *m_renderHeightSpin; // New: Internal render resolution
    QSpinBox *m_fpsSpin;
    
    QCheckBox *m_fullscreenCheck; // New: Fullscreen mode
    QCheckBox *m_androidSupportCheck; // New: Android compatibility
};

#endif // PROJECTSETTINGSDIALOG_H
