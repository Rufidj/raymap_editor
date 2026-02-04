#include "entitybehaviordialog.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QMessageBox>
#include <QDialogButtonBox>

EntityBehaviorDialog::EntityBehaviorDialog(const EntityInstance &entity, const QStringList &availableProcesses, QWidget *parent)
    : QDialog(parent)
    , m_entity(entity)
    , m_availableProcesses(availableProcesses)
{
    setWindowTitle(tr("Editar Comportamiento - %1").arg(entity.processName));
    setMinimumWidth(500);
    setMinimumHeight(400);
    
    setupUI();
    updateVisibility();
}

void EntityBehaviorDialog::setupUI()
{
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    
    // Entity Info
    QGroupBox *infoGroup = new QGroupBox(tr("Información de Entidad"));
    QFormLayout *infoLayout = new QFormLayout();
    infoLayout->addRow(tr("Nombre:"), new QLabel(m_entity.processName));
    infoLayout->addRow(tr("Tipo:"), new QLabel(m_entity.type));
    infoLayout->addRow(tr("Asset:"), new QLabel(m_entity.assetPath));
    infoGroup->setLayout(infoLayout);
    mainLayout->addWidget(infoGroup);
    
    // Behavior Settings
    QGroupBox *behaviorGroup = new QGroupBox(tr("Configuración de Comportamiento"));
    QFormLayout *behaviorLayout = new QFormLayout();
    
    // Activation Type
    m_activationTypeCombo = new QComboBox();
    m_activationTypeCombo->addItem(tr("Al inicio del juego"), EntityInstance::ACTIVATION_ON_START);
    m_activationTypeCombo->addItem(tr("Al colisionar"), EntityInstance::ACTIVATION_ON_COLLISION);
    m_activationTypeCombo->addItem(tr("Al entrar en área (trigger)"), EntityInstance::ACTIVATION_ON_TRIGGER);
    m_activationTypeCombo->addItem(tr("Por evento personalizado"), EntityInstance::ACTIVATION_ON_EVENT);
    m_activationTypeCombo->addItem(tr("Manual (código custom)"), EntityInstance::ACTIVATION_MANUAL);
    m_activationTypeCombo->setCurrentIndex(static_cast<int>(m_entity.activationType));
    connect(m_activationTypeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &EntityBehaviorDialog::onActivationTypeChanged);
    behaviorLayout->addRow(tr("Tipo de Activación:"), m_activationTypeCombo);
    
    // Visibility
    m_visibilityCheck = new QCheckBox(tr("Entidad visible"));
    m_visibilityCheck->setChecked(m_entity.isVisible);
    behaviorLayout->addRow(tr("Visibilidad:"), m_visibilityCheck);
    
    behaviorGroup->setLayout(behaviorLayout);
    mainLayout->addWidget(behaviorGroup);
    
    // Collision Settings (conditional)
    m_collisionWidget = new QWidget();
    QFormLayout *collisionLayout = new QFormLayout(m_collisionWidget);
    m_collisionTargetCombo = new QComboBox();
    m_collisionTargetCombo->setEditable(true);
    m_collisionTargetCombo->addItem("TYPE_PLAYER");
    m_collisionTargetCombo->addItem("TYPE_ENEMY");
    
    // Add other existing processes
    for (const QString &p : m_availableProcesses) {
        if (p != "TYPE_PLAYER" && p != "TYPE_ENEMY") {
            m_collisionTargetCombo->addItem(p);
        }
    }
    
    m_collisionTargetCombo->setEditText(m_entity.collisionTarget);
    collisionLayout->addRow(tr("Colisionar con:"), m_collisionTargetCombo);
    mainLayout->addWidget(m_collisionWidget);
    
    // Event Settings (conditional)
    m_eventWidget = new QWidget();
    QFormLayout *eventLayout = new QFormLayout(m_eventWidget);
    m_eventNameEdit = new QLineEdit(m_entity.eventName);
    m_eventNameEdit->setPlaceholderText("nombre_evento");
    eventLayout->addRow(tr("Nombre del evento:"), m_eventNameEdit);
    mainLayout->addWidget(m_eventWidget);
    
    // Player & Control Settings
    m_playerGroup = new QGroupBox(tr("Configuración de Jugador y Control"));
    m_playerGroup->setCheckable(true);
    m_playerGroup->setChecked(m_entity.isPlayer);
    QFormLayout *playerLayout = new QFormLayout();
    
    m_controlTypeCombo = new QComboBox();
    m_controlTypeCombo->addItem(tr("Ninguno (Estático)"), EntityInstance::CONTROL_NONE);
    m_controlTypeCombo->addItem(tr("Primera Persona (FPS)"), EntityInstance::CONTROL_FIRST_PERSON);
    m_controlTypeCombo->addItem(tr("Tercera Persona"), EntityInstance::CONTROL_THIRD_PERSON);
    m_controlTypeCombo->addItem(tr("Coche / Vehículo"), EntityInstance::CONTROL_CAR);
    
    // Find index for current data
    int ctrlIdx = m_controlTypeCombo->findData(m_entity.controlType);
    if (ctrlIdx != -1) m_controlTypeCombo->setCurrentIndex(ctrlIdx);
    else m_controlTypeCombo->setCurrentIndex(0);
    playerLayout->addRow(tr("Tipo de Control:"), m_controlTypeCombo);
    
    m_cameraFollowCheck = new QCheckBox(tr("Cámara sigue a la entidad"));
    m_cameraFollowCheck->setChecked(m_entity.cameraFollow);
    playerLayout->addRow(tr("Cámara Follow:"), m_cameraFollowCheck);
    
    QHBoxLayout *offsetLayout = new QHBoxLayout();
    m_camOffsets[0] = new QLineEdit(QString::number(m_entity.cameraOffset_x));
    m_camOffsets[1] = new QLineEdit(QString::number(m_entity.cameraOffset_y));
    m_camOffsets[2] = new QLineEdit(QString::number(m_entity.cameraOffset_z));
    offsetLayout->addWidget(new QLabel("X:")); offsetLayout->addWidget(m_camOffsets[0]);
    offsetLayout->addWidget(new QLabel("Y:")); offsetLayout->addWidget(m_camOffsets[1]);
    offsetLayout->addWidget(new QLabel("Z:")); offsetLayout->addWidget(m_camOffsets[2]);
    playerLayout->addRow(tr("Offset de Cámara:"), offsetLayout);
    
    m_camRotationEdit = new QLineEdit(QString::number(m_entity.cameraRotation));
    playerLayout->addRow(tr("Rotación Cámara (deg):"), m_camRotationEdit);
    
    m_initialRotationEdit = new QLineEdit(QString::number(m_entity.initialRotation));
    m_initialRotationEdit->setToolTip(tr("Rotación inicial del modelo en grados (0-360)"));
    playerLayout->addRow(tr("Rotación Inicial Modelo (deg):"), m_initialRotationEdit);
    
    m_playerGroup->setLayout(playerLayout);
    mainLayout->addWidget(m_playerGroup);

    // Custom Action
    QGroupBox *actionGroup = new QGroupBox(tr("Acción Personalizada (Código BennuGD)"));
    QVBoxLayout *actionLayout = new QVBoxLayout();
    m_customActionEdit = new QTextEdit();
    m_customActionEdit->setPlaceholderText(
        "// Código que se ejecuta cuando se activa la entidad\n"
        "// Puedes usar constantes como TYPE_PLAYER, TYPE_ENEMY, etc.\n"
        "// Ejemplo:\n"
        "say(\"¡Entidad activada!\");\n"
        "signal(id, s_kill);"
    );
    m_customActionEdit->setPlainText(m_entity.customAction);
    m_customActionEdit->setMinimumHeight(150);
    actionLayout->addWidget(m_customActionEdit);
    actionGroup->setLayout(actionLayout);
    mainLayout->addWidget(actionGroup);
    
    // Preview Button
    m_previewButton = new QPushButton(tr("Vista Previa del Código"));
    connect(m_previewButton, &QPushButton::clicked, this, &EntityBehaviorDialog::onPreviewCode);
    mainLayout->addWidget(m_previewButton);

    // Connect signals for dynamic visibility
    connect(m_playerGroup, &QGroupBox::toggled, this, &EntityBehaviorDialog::updateVisibility);
    connect(m_controlTypeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &EntityBehaviorDialog::updateVisibility);
    connect(m_cameraFollowCheck, &QCheckBox::toggled, this, &EntityBehaviorDialog::updateVisibility);
    
    // Dialog Buttons
    QDialogButtonBox *buttonBox = new QDialogButtonBox(
        QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    connect(buttonBox, &QDialogButtonBox::accepted, this, &EntityBehaviorDialog::onAccept);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
    mainLayout->addWidget(buttonBox);
}

void EntityBehaviorDialog::updateVisibility()
{
    int index = m_activationTypeCombo->currentIndex();
    EntityInstance::ActivationType type = 
        static_cast<EntityInstance::ActivationType>(
            m_activationTypeCombo->itemData(index).toInt());
    
    // Show/hide collision settings
    m_collisionWidget->setVisible(type == EntityInstance::ACTIVATION_ON_COLLISION);
    
    // Show/hide event settings
    m_eventWidget->setVisible(type == EntityInstance::ACTIVATION_ON_EVENT);
    
    // Player/Control visibility context
    bool isPlayer = m_playerGroup->isChecked();
    m_controlTypeCombo->setEnabled(isPlayer);
    
    bool follow = m_cameraFollowCheck->isChecked();
    for (int i=0; i<3; ++i) m_camOffsets[i]->setEnabled(isPlayer && follow);
    m_camRotationEdit->setEnabled(isPlayer && !follow);
}

void EntityBehaviorDialog::onActivationTypeChanged(int index)
{
    updateVisibility();
}

void EntityBehaviorDialog::onAccept()
{
    // Update entity with current values
    int index = m_activationTypeCombo->currentIndex();
    m_entity.activationType = static_cast<EntityInstance::ActivationType>(
        m_activationTypeCombo->itemData(index).toInt());
    
    m_entity.isVisible = m_visibilityCheck->isChecked();
    m_entity.collisionTarget = m_collisionTargetCombo->currentText();
    m_entity.eventName = m_eventNameEdit->text();
    m_entity.customAction = m_customActionEdit->toPlainText();
    
    // Player & Control
    m_entity.isPlayer = m_playerGroup->isChecked();
    m_entity.controlType = static_cast<EntityInstance::ControlType>(
        m_controlTypeCombo->currentData().toInt());
    m_entity.cameraFollow = m_cameraFollowCheck->isChecked();
    m_entity.cameraOffset_x = m_camOffsets[0]->text().toFloat();
    m_entity.cameraOffset_y = m_camOffsets[1]->text().toFloat();
    m_entity.cameraOffset_z = m_camOffsets[2]->text().toFloat();
    m_entity.cameraRotation = m_camRotationEdit->text().toFloat();
    m_entity.initialRotation = m_initialRotationEdit->text().toFloat();
    
    accept();
}

void EntityBehaviorDialog::onPreviewCode()
{
    QString preview = generatePreviewCode();
    
    QMessageBox msgBox(this);
    msgBox.setWindowTitle(tr("Vista Previa del Código"));
    msgBox.setText(tr("Este código se generará en main.prg:"));
    msgBox.setDetailedText(preview);
    msgBox.setIcon(QMessageBox::Information);
    msgBox.exec();
}

QString EntityBehaviorDialog::generatePreviewCode() const
{
    QString code;
    code += QString("// Entidad: %1 (Tipo: %2)\n").arg(m_entity.processName).arg(m_entity.type);
    code += QString("Process %1()\n").arg(m_entity.processName);
    code += "Private\n";
    
    // Add private variables based on activation type
    if (m_entity.activationType == EntityInstance::ACTIVATION_ON_COLLISION) {
        code += QString("    int collision_target = %1;\n").arg(m_entity.collisionTarget);
    } else if (m_entity.activationType == EntityInstance::ACTIVATION_ON_EVENT) {
        code += QString("    int event_triggered = 0;\n");
    }
    
    code += "Begin\n";
    code += "    // Configuración inicial\n";
    
    // Position
    code += QString("    world_x = %1; world_y = %2; world_z = %3;\n").arg(m_entity.x).arg(m_entity.y).arg(m_entity.z);
    
    // Player variables
    if (m_entity.isPlayer) {
        code += "    float move_speed = 8.0;\n";
        code += "    float rot_speed = 0.08;\n";
    }
    
    code += "\n    LOOP\n";
    
    // Control logic
    if (m_entity.isPlayer) {
        code += "        // Lógica de control detectada...\n";
        code += "        if (key(_w)) ... end\n";
        if (m_entity.cameraFollow) {
            code += "        RAY_SET_CAMERA(world_x + offset, world_y + offset, ...);\n";
        }
    }
    
    code += "        FRAME;\n";
    code += "    END\n";
    
    code += "End\n";
    
    return code;
}
