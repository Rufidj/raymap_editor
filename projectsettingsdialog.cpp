#include "projectsettingsdialog.h"
#include "codegenerator.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QGroupBox>
#include <QFileDialog>
#include <QDir>
#include <QDirIterator>
#include <QFile>
#include <QDebug>
#include <QJsonDocument>
#include <QJsonObject>

ProjectSettingsDialog::ProjectSettingsDialog(const ProjectData &data, QWidget *parent)
    : QDialog(parent), m_data(data)
{
    setupUi();
    setWindowTitle(tr("Configuración del Proyecto"));
    resize(500, 600);
}

// Removed browsing slots
void ProjectSettingsDialog::loadSceneList()
{
    m_startupSceneCombo->clear();
    
    // Find all .scn files in project
    QString projectPath = m_data.path;
    QStringList filters; filters << "*.scn";
    QDirIterator it(projectPath, filters, QDir::Files | QDir::NoSymLinks, QDirIterator::Subdirectories);
    
    while(it.hasNext()) {
        QString filePath = it.next();
        QString baseName = QFileInfo(filePath).baseName();
        // Avoid duplicates if multiple .scn with same name exist in different folders
        if (m_startupSceneCombo->findText(baseName) == -1) {
             m_startupSceneCombo->addItem(baseName, baseName);
        }
    }
    
    // Select current
    int idx = m_startupSceneCombo->findText(m_data.startupScene);
    if (idx >= 0) m_startupSceneCombo->setCurrentIndex(idx);
    else if (m_startupSceneCombo->count() > 0) m_startupSceneCombo->setCurrentIndex(0);
}

void ProjectSettingsDialog::setupUi()
{
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    
    // --- General Settings ---
    QGroupBox *generalGroup = new QGroupBox(tr("General"), this);
    QFormLayout *generalLayout = new QFormLayout(generalGroup);
    
    m_nameEdit = new QLineEdit(m_data.name);
    m_versionEdit = new QLineEdit(m_data.version);
    
    // Startup Scene
    m_startupSceneCombo = new QComboBox();
    loadSceneList();
    
    generalLayout->addRow(tr("Nombre del Proyecto:"), m_nameEdit);
    generalLayout->addRow(tr("Versión:"), m_versionEdit);
    generalLayout->addRow(tr("Escena Inicial:"), m_startupSceneCombo);
    
    mainLayout->addWidget(generalGroup);

    // --- Android Settings ---
    QGroupBox *androidGroup = new QGroupBox(tr("Configuración Android"), this);
    QFormLayout *androidLayout = new QFormLayout(androidGroup);

    m_packageEdit = new QLineEdit(m_data.packageName);
    if (m_packageEdit->text().isEmpty()) m_packageEdit->setText("com.example.game");
    m_packageEdit->setPlaceholderText("com.company.game");

    m_androidSupportCheck = new QCheckBox(tr("Habilitar código compatible"));
    m_androidSupportCheck->setToolTip(tr("Genera lógica para detectar Android y usar rutas absolutas"));
    m_androidSupportCheck->setChecked(m_data.androidSupport);

    androidLayout->addRow(tr("Nombre de Paquete:"), m_packageEdit);
    androidLayout->addRow(tr(""), m_androidSupportCheck);

    mainLayout->addWidget(androidGroup);
    
    // --- Display ---
    QGroupBox *displayGroup = new QGroupBox(tr("Pantalla"), this);
    QGridLayout *displayLayout = new QGridLayout(displayGroup);
    
    // Window Resolution
    m_widthSpin = new QSpinBox();
    m_widthSpin->setRange(320, 3840);
    m_widthSpin->setValue(m_data.screenWidth);
    
    m_heightSpin = new QSpinBox();
    m_heightSpin->setRange(200, 2160);
    m_heightSpin->setValue(m_data.screenHeight);
    
    // Render Resolution (internal)
    m_renderWidthSpin = new QSpinBox();
    m_renderWidthSpin->setRange(160, 1920);
    m_renderWidthSpin->setValue(m_data.renderWidth > 0 ? m_data.renderWidth : m_data.screenWidth);
    m_renderWidthSpin->setToolTip(tr("Resolución interna de renderizado (para 3D)"));
    
    m_renderHeightSpin = new QSpinBox();
    m_renderHeightSpin->setRange(120, 1080);
    m_renderHeightSpin->setValue(m_data.renderHeight > 0 ? m_data.renderHeight : m_data.screenHeight);
    m_renderHeightSpin->setToolTip(tr("Resolución interna de renderizado (para 3D)"));
    
    m_fpsSpin = new QSpinBox();
    m_fpsSpin->setRange(0, 240); // 0 = Unlimited
    m_fpsSpin->setSpecialValueText(tr("Ilimitado"));
    m_fpsSpin->setValue(m_data.fps);
    
    displayLayout->addWidget(new QLabel(tr("Resolución Ventana:")), 0, 0, 1, 4);
    displayLayout->addWidget(new QLabel(tr("Ancho:")), 1, 0);
    displayLayout->addWidget(m_widthSpin, 1, 1);
    displayLayout->addWidget(new QLabel(tr("Alto:")), 1, 2);
    displayLayout->addWidget(m_heightSpin, 1, 3);
    
    displayLayout->addWidget(new QLabel(tr("Resolución Render (3D):")), 2, 0, 1, 4);
    displayLayout->addWidget(new QLabel(tr("Ancho:")), 3, 0);
    displayLayout->addWidget(m_renderWidthSpin, 3, 1);
    displayLayout->addWidget(new QLabel(tr("Alto:")), 3, 2);
    displayLayout->addWidget(m_renderHeightSpin, 3, 3);
    
    displayLayout->addWidget(new QLabel(tr("FPS:")), 4, 0);
    displayLayout->addWidget(m_fpsSpin, 4, 1);
    
    // Fullscreen checkbox
    m_fullscreenCheck = new QCheckBox(tr("Pantalla Completa"));
    m_fullscreenCheck->setChecked(m_data.fullscreen);
    displayLayout->addWidget(m_fullscreenCheck, 6, 0, 1, 4);
    
    mainLayout->addWidget(displayGroup);
    
    // Add stretch
    mainLayout->addStretch();
    
    // Buttons
    QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    connect(buttonBox, &QDialogButtonBox::accepted, this, &ProjectSettingsDialog::onAccept);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
    mainLayout->addWidget(buttonBox);
}

void ProjectSettingsDialog::onAccept()
{
    // Update m_data with UI values
    m_data.name = m_nameEdit->text();
    m_data.version = m_versionEdit->text();
    m_data.startupScene = m_startupSceneCombo->currentText();
    
    m_data.screenWidth = m_widthSpin->value();
    m_data.screenHeight = m_heightSpin->value();
    m_data.renderWidth = m_renderWidthSpin->value();
    m_data.renderHeight = m_renderHeightSpin->value();
    m_data.fps = m_fpsSpin->value();
    
    m_data.fullscreen = m_fullscreenCheck->isChecked();
    
    // Android
    m_data.packageName = m_packageEdit->text();
    m_data.androidSupport = m_androidSupportCheck->isChecked();
    
    // Save configuration to JSON file
    if (ProjectManager::saveProjectData(m_data.path, m_data)) {
        qDebug() << "Saved project configuration via ProjectManager";
    } else {
        qWarning() << "Failed to save project configuration";
    }
    
    // Regenerate main.prg with new settings (Patching to preserve user code)
    CodeGenerator generator;
    generator.setProjectData(m_data);
    
    QString mainPath = m_data.path + "/src/main.prg";
    QFile mainFile(mainPath);
    QString existingContent;
    if (mainFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        existingContent = QTextStream(&mainFile).readAll();
        mainFile.close();
    }
    
    QString newCode;
    if (existingContent.isEmpty() || !existingContent.contains("// [[ED_STARTUP_SCENE_START]]")) {
        newCode = generator.generateMainPrg();
    } else {
        // Use patching if markers exist
        newCode = generator.patchMainPrg(existingContent, QVector<EntityInstance>());
    }
    
    if (mainFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
        mainFile.write(newCode.toUtf8());
        mainFile.close();
        qDebug() << "Updated main.prg (patched/regenerated)";
    } else {
        qWarning() << "Failed to write main.prg";
    }
    
    accept();
}

ProjectData ProjectSettingsDialog::getProjectData() const
{
    return m_data;
}
