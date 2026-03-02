#include "entitybehaviordialog.h"
#include "behaviornodeeditor.h"
#include "md3loader.h"
#include "processgenerator.h"
#include <QCheckBox>
#include <QComboBox>
#include <QDialogButtonBox>
#include <QDoubleSpinBox>
#include <QFileInfo>
#include <QFormLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QPoint>
#include <QPushButton>
#include <QRegularExpression> // Replaces QRegExp
#include <QScrollArea>
#include <QSpinBox>
#include <QTextEdit>
#include <QTextStream>
#include <QVBoxLayout>
#include <QVariant>

EntityBehaviorDialog::EntityBehaviorDialog(
    const EntityInstance &entity, const QString &projectPath,
    const QVector<NPCPath> *availablePaths,
    const QStringList &availableProcesses, QWidget *parent)
    : QDialog(parent), m_entity(entity), m_projectPath(projectPath),
      m_availablePaths(availablePaths),
      m_availableProcesses(availableProcesses) {
  setWindowTitle(tr("Editar Comportamiento - %1").arg(entity.processName));
  setMinimumWidth(500);
  setMinimumHeight(400);

  setupUI();
  updateVisibility();
}

void EntityBehaviorDialog::setupUI() {
  QVBoxLayout *outerLayout = new QVBoxLayout(this);
  QScrollArea *scrollArea = new QScrollArea();
  scrollArea->setWidgetResizable(true);
  QWidget *scrollContent = new QWidget();
  QVBoxLayout *mainLayout = new QVBoxLayout(scrollContent);
  scrollArea->setWidget(scrollContent);
  outerLayout->addWidget(scrollArea);

  // Entity Info
  QGroupBox *infoGroup = new QGroupBox(tr("Información de Entidad"));
  QFormLayout *infoLayout = new QFormLayout();
  infoLayout->addRow(tr("Nombre:"), new QLabel(m_entity.processName));
  infoLayout->addRow(tr("Tipo:"), new QLabel(m_entity.type));
  infoLayout->addRow(tr("Asset:"), new QLabel(m_entity.assetPath));
  infoGroup->setLayout(infoLayout);
  mainLayout->addWidget(infoGroup);

  // Behavior Settings
  QGroupBox *behaviorGroup =
      new QGroupBox(tr("Configuración de Comportamiento"));
  QFormLayout *behaviorLayout = new QFormLayout();

  // Activation Type
  m_activationTypeCombo = new QComboBox();
  m_activationTypeCombo->addItem(tr("Al inicio del juego"),
                                 EntityInstance::ACTIVATION_ON_START);
  m_activationTypeCombo->addItem(tr("Al colisionar"),
                                 EntityInstance::ACTIVATION_ON_COLLISION);
  m_activationTypeCombo->addItem(tr("Al entrar en área (trigger)"),
                                 EntityInstance::ACTIVATION_ON_TRIGGER);
  m_activationTypeCombo->addItem(tr("Por evento personalizado"),
                                 EntityInstance::ACTIVATION_ON_EVENT);
  m_activationTypeCombo->addItem(tr("Manual (código custom)"),
                                 EntityInstance::ACTIVATION_MANUAL);
  m_activationTypeCombo->setCurrentIndex(
      static_cast<int>(m_entity.activationType));
  connect(m_activationTypeCombo,
          QOverload<int>::of(&QComboBox::currentIndexChanged), this,
          &EntityBehaviorDialog::onActivationTypeChanged);
  behaviorLayout->addRow(tr("Tipo de Activación:"), m_activationTypeCombo);

  // Visibility
  m_visibilityCheck = new QCheckBox(tr("Visible al iniciar"));
  m_visibilityCheck->setChecked(m_entity.isVisible);
  behaviorLayout->addRow(tr("Visibilidad:"), m_visibilityCheck);

  // Intro Check (Only for Camera Paths)
  m_isIntroCheck =
      new QCheckBox(tr("Es secuencia de introducción (Bloquea Input)"));
  m_isIntroCheck->setChecked(m_entity.isIntro);

  bool isCameraPath =
      (m_entity.type == "campath") ||
      m_entity.assetPath.endsWith(".campath", Qt::CaseInsensitive) ||
      m_entity.assetPath.contains(".campath"); // Robust check

  if (isCameraPath) {
    behaviorLayout->addRow("", m_isIntroCheck);
  } else {
    m_isIntroCheck->setVisible(false);
  }

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
  m_controlTypeCombo->addItem(tr("Ninguno (Estático)"),
                              EntityInstance::CONTROL_NONE);
  m_controlTypeCombo->addItem(tr("Primera Persona (FPS)"),
                              EntityInstance::CONTROL_FIRST_PERSON);
  m_controlTypeCombo->addItem(tr("Tercera Persona"),
                              EntityInstance::CONTROL_THIRD_PERSON);
  m_controlTypeCombo->addItem(tr("Coche / Vehículo"),
                              EntityInstance::CONTROL_CAR);

  // Find index for current data
  int ctrlIdx = m_controlTypeCombo->findData(m_entity.controlType);
  if (ctrlIdx != -1)
    m_controlTypeCombo->setCurrentIndex(ctrlIdx);
  else
    m_controlTypeCombo->setCurrentIndex(0);
  playerLayout->addRow(tr("Tipo de Control:"), m_controlTypeCombo);

  m_cameraFollowCheck = new QCheckBox(tr("Cámara sigue a la entidad"));
  m_cameraFollowCheck->setChecked(m_entity.cameraFollow);
  playerLayout->addRow(tr("Cámara Follow:"), m_cameraFollowCheck);

  QHBoxLayout *offsetLayout = new QHBoxLayout();
  m_camOffsets[0] = new QLineEdit(QString::number(m_entity.cameraOffset_x));
  m_camOffsets[1] = new QLineEdit(QString::number(m_entity.cameraOffset_y));
  m_camOffsets[2] = new QLineEdit(QString::number(m_entity.cameraOffset_z));
  offsetLayout->addWidget(new QLabel("X:"));
  offsetLayout->addWidget(m_camOffsets[0]);
  offsetLayout->addWidget(new QLabel("Y:"));
  offsetLayout->addWidget(m_camOffsets[1]);
  offsetLayout->addWidget(new QLabel("Z:"));
  offsetLayout->addWidget(m_camOffsets[2]);
  playerLayout->addRow(tr("Offset de Cámara:"), offsetLayout);

  m_camRotationEdit = new QLineEdit(QString::number(m_entity.cameraRotation));
  playerLayout->addRow(tr("Rotación Cámara (deg):"), m_camRotationEdit);

  m_initialRotationEdit =
      new QLineEdit(QString::number(m_entity.initialRotation));
  m_initialRotationEdit->setToolTip(
      tr("Rotación inicial del modelo en grados (0-360)"));
  playerLayout->addRow(tr("Rotación Inicial Modelo (deg):"),
                       m_initialRotationEdit);

  m_playerGroup->setLayout(playerLayout);
  mainLayout->addWidget(m_playerGroup);

  // NPC Path Assignment
  QGroupBox *npcPathGroup = new QGroupBox(tr("Movimiento por Ruta (NPC Path)"));
  QFormLayout *npcPathLayout = new QFormLayout();

  m_npcPathCombo = new QComboBox();
  m_npcPathCombo->addItem(tr("(Sin ruta asignada)"), -1);

  // Populate with available paths
  if (m_availablePaths) {
    for (const NPCPath &path : *m_availablePaths) {
      m_npcPathCombo->addItem(path.name, path.path_id);
    }
  }

  // Set current selection
  int currentIndex = 0;
  for (int i = 0; i < m_npcPathCombo->count(); ++i) {
    if (m_npcPathCombo->itemData(i).toInt() == m_entity.npcPathId) {
      currentIndex = i;
      break;
    }
  }
  m_npcPathCombo->setCurrentIndex(currentIndex);

  m_autoStartPathCheck =
      new QCheckBox(tr("Iniciar ruta automáticamente al aparecer"));
  m_autoStartPathCheck->setChecked(m_entity.autoStartPath);

  m_snapToFloorCheck = new QCheckBox(tr("Pegar al suelo automáticamente"));
  m_snapToFloorCheck->setChecked(m_entity.snapToFloor);

  npcPathLayout->addRow(tr("Ruta a seguir:"), m_npcPathCombo);
  npcPathLayout->addRow(m_autoStartPathCheck);
  npcPathLayout->addRow(m_snapToFloorCheck);
  npcPathGroup->setLayout(npcPathLayout);
  mainLayout->addWidget(npcPathGroup);

  // Physics / Collision Box settings
  m_physicsGroup = new QGroupBox(tr("Física y Caja de Colisión (3D)"));
  QFormLayout *physicsLayout = new QFormLayout();
  m_colSize[0] = new QLineEdit(QString::number(m_entity.width));
  m_colSize[1] = new QLineEdit(QString::number(m_entity.depth));
  m_colSize[2] = new QLineEdit(QString::number(m_entity.height));
  physicsLayout->addRow(tr("Ancho (Width - X):"), m_colSize[0]);
  physicsLayout->addRow(tr("Fondo (Depth - Y):"), m_colSize[1]);
  physicsLayout->addRow(tr("Alto (Height - Z):"), m_colSize[2]);
  m_physicsGroup->setLayout(physicsLayout);
  mainLayout->addWidget(m_physicsGroup);

  if (m_entity.type != "model") {
    m_physicsGroup->setVisible(false);
  }

  // ===== PHYSICS ENGINE SECTION =====
  m_physicsEngineGroup = new QGroupBox(tr("Motor de Físicas"));
  m_physicsEngineGroup->setCheckable(true);
  m_physicsEngineGroup->setChecked(m_entity.physicsEnabled);
  QFormLayout *peLayout = new QFormLayout();

  m_massSpin = new QDoubleSpinBox();
  m_massSpin->setRange(0.0, 100000.0);
  m_massSpin->setDecimals(2);
  m_massSpin->setValue(m_entity.physicsMass);
  m_massSpin->setSuffix(" kg");
  m_massSpin->setToolTip(tr("Masa del objeto (0 = masa infinita/estático)"));
  peLayout->addRow(tr("Masa:"), m_massSpin);

  m_frictionSpin = new QDoubleSpinBox();
  m_frictionSpin->setRange(0.0, 1.0);
  m_frictionSpin->setDecimals(2);
  m_frictionSpin->setSingleStep(0.05);
  m_frictionSpin->setValue(m_entity.physicsFriction);
  m_frictionSpin->setToolTip(tr("Coeficiente de fricción (0=hielo, 1=goma)"));
  peLayout->addRow(tr("Fricción:"), m_frictionSpin);

  m_restitutionSpin = new QDoubleSpinBox();
  m_restitutionSpin->setRange(0.0, 1.0);
  m_restitutionSpin->setDecimals(2);
  m_restitutionSpin->setSingleStep(0.05);
  m_restitutionSpin->setValue(m_entity.physicsRestitution);
  m_restitutionSpin->setToolTip(tr("Rebote (0=ninguno, 1=rebote perfecto)"));
  peLayout->addRow(tr("Rebote:"), m_restitutionSpin);

  m_gravityScaleSpin = new QDoubleSpinBox();
  m_gravityScaleSpin->setRange(-10.0, 10.0);
  m_gravityScaleSpin->setDecimals(2);
  m_gravityScaleSpin->setSingleStep(0.1);
  m_gravityScaleSpin->setValue(m_entity.physicsGravityScale);
  m_gravityScaleSpin->setToolTip(
      tr("Multiplicador de gravedad (0=flotante, 1=normal, -1=invertida)"));
  peLayout->addRow(tr("Escala Gravedad:"), m_gravityScaleSpin);

  m_linearDampingSpin = new QDoubleSpinBox();
  m_linearDampingSpin->setRange(0.0, 1.0);
  m_linearDampingSpin->setDecimals(3);
  m_linearDampingSpin->setSingleStep(0.01);
  m_linearDampingSpin->setValue(m_entity.physicsLinearDamping);
  m_linearDampingSpin->setToolTip(tr("Resistencia del aire al movimiento"));
  peLayout->addRow(tr("Damping Lineal:"), m_linearDampingSpin);

  m_angularDampingSpin = new QDoubleSpinBox();
  m_angularDampingSpin->setRange(0.0, 1.0);
  m_angularDampingSpin->setDecimals(3);
  m_angularDampingSpin->setSingleStep(0.01);
  m_angularDampingSpin->setValue(m_entity.physicsAngularDamping);
  m_angularDampingSpin->setToolTip(tr("Resistencia del aire a la rotación"));
  peLayout->addRow(tr("Damping Angular:"), m_angularDampingSpin);

  // Mode flags
  m_staticCheck = new QCheckBox(tr("Estático (inmovible, como paredes)"));
  m_staticCheck->setChecked(m_entity.physicsIsStatic);
  peLayout->addRow(m_staticCheck);

  m_kinematicCheck =
      new QCheckBox(tr("Cinemático (movido por código, no por físicas)"));
  m_kinematicCheck->setChecked(m_entity.physicsIsKinematic);
  peLayout->addRow(m_kinematicCheck);

  m_triggerCheck =
      new QCheckBox(tr("Trigger (detecta colisión sin respuesta física)"));
  m_triggerCheck->setChecked(m_entity.physicsIsTrigger);
  peLayout->addRow(m_triggerCheck);

  // Rotation locks
  QHBoxLayout *lockLayout = new QHBoxLayout();
  m_lockRotXCheck = new QCheckBox(tr("X"));
  m_lockRotXCheck->setChecked(m_entity.physicsLockRotX);
  m_lockRotYCheck = new QCheckBox(tr("Y"));
  m_lockRotYCheck->setChecked(m_entity.physicsLockRotY);
  m_lockRotZCheck = new QCheckBox(tr("Z"));
  m_lockRotZCheck->setChecked(m_entity.physicsLockRotZ);
  lockLayout->addWidget(m_lockRotXCheck);
  lockLayout->addWidget(m_lockRotYCheck);
  lockLayout->addWidget(m_lockRotZCheck);
  peLayout->addRow(tr("Bloquear Rotación:"), lockLayout);

  // Collision layers
  m_collisionLayerSpin = new QSpinBox();
  m_collisionLayerSpin->setRange(0, 65535);
  m_collisionLayerSpin->setValue(m_entity.physicsCollisionLayer);
  m_collisionLayerSpin->setToolTip(
      tr("Capa de colisión de este objeto (bitmask)"));
  peLayout->addRow(tr("Capa de Colisión:"), m_collisionLayerSpin);

  m_collisionMaskSpin = new QSpinBox();
  m_collisionMaskSpin->setRange(0, 65535);
  m_collisionMaskSpin->setValue(m_entity.physicsCollisionMask);
  m_collisionMaskSpin->setToolTip(tr("Con qué capas colisiona (bitmask)"));
  peLayout->addRow(tr("Máscara de Colisión:"), m_collisionMaskSpin);

  m_physicsEngineGroup->setLayout(peLayout);
  mainLayout->addWidget(m_physicsEngineGroup);

  if (m_entity.type != "model") {
    m_physicsEngineGroup->setVisible(false);
  }

  // ===== MODEL ANIMATION SECTION =====
  m_animationGroup = new QGroupBox(tr("Animación del Modelo (MD3)"));
  QFormLayout *animLayout = new QFormLayout();

  m_animSelectCombo = new QComboBox();
  m_animSelectCombo->addItem(tr("(Personalizado / Manual)"), -1);
  animLayout->addRow(tr("Animaciones Detectadas:"), m_animSelectCombo);

  m_totalFramesLabel = new QLabel(tr("Total de Frames: ?"));
  animLayout->addRow(m_totalFramesLabel);

  m_playAnimCheck = new QCheckBox(tr("Reproducir Animación"));
  m_playAnimCheck->setChecked(m_entity.animSpeed != 0.0f);
  animLayout->addRow(m_playAnimCheck);

  m_startFrameSpin = new QSpinBox();
  m_startFrameSpin->setRange(0, 9999);
  m_startFrameSpin->setValue(m_entity.startGraph);
  animLayout->addRow(tr("Frame Inicial:"), m_startFrameSpin);

  m_endFrameSpin = new QSpinBox();
  m_endFrameSpin->setRange(0, 9999);
  m_endFrameSpin->setValue(m_entity.endGraph);
  animLayout->addRow(tr("Frame Final:"), m_endFrameSpin);

  m_animSpeedSpin = new QDoubleSpinBox();
  m_animSpeedSpin->setRange(0.0, 10.0);
  m_animSpeedSpin->setSingleStep(0.1);
  m_animSpeedSpin->setValue(m_entity.animSpeed == 0.0f ? 1.0f
                                                       : m_entity.animSpeed);
  animLayout->addRow(tr("Velocidad:"), m_animSpeedSpin);

  m_animationGroup->setLayout(animLayout);
  mainLayout->addWidget(m_animationGroup);

  if (m_entity.type != "model") {
    m_animationGroup->setVisible(false);
  } else {
    loadModelAnimations();
  }

  // Connect animation combo
  connect(m_animSelectCombo,
          QOverload<int>::of(&QComboBox::currentIndexChanged),
          [this](int index) {
            QVariant data = m_animSelectCombo->itemData(index);
            if (data.isValid() && data.type() == QVariant::Point) {
              QPoint range = data.toPoint();
              m_startFrameSpin->setValue(range.x());
              m_endFrameSpin->setValue(range.y());
            }
          });

  // Billboard Settings (conditional)
  QGroupBox *billboardGroup =
      new QGroupBox(tr("Configuración de Billboard (2D)"));
  QFormLayout *billboardLayout = new QFormLayout();

  QLineEdit *graphIdEdit = new QLineEdit(QString::number(m_entity.graphId));
  graphIdEdit->setObjectName("graphId");
  billboardLayout->addRow(tr("ID de Gráfico (Graph ID):"), graphIdEdit);

  QLineEdit *startGraphEdit =
      new QLineEdit(QString::number(m_entity.startGraph));
  startGraphEdit->setObjectName("startGraph");
  billboardLayout->addRow(tr("Gráfico Inicial Animación:"), startGraphEdit);

  QLineEdit *endGraphEdit = new QLineEdit(QString::number(m_entity.endGraph));
  endGraphEdit->setObjectName("endGraph");
  billboardLayout->addRow(tr("Gráfico Final Animación:"), endGraphEdit);

  QLineEdit *animSpeedEdit = new QLineEdit(QString::number(m_entity.animSpeed));
  animSpeedEdit->setObjectName("animSpeed");
  billboardLayout->addRow(tr("Velocidad Animación (0 = estático):"),
                          animSpeedEdit);

  QLineEdit *widthEdit = new QLineEdit(QString::number(m_entity.width));
  widthEdit->setObjectName("width");
  billboardLayout->addRow(tr("Ancho (Width):"), widthEdit);

  QLineEdit *heightEdit = new QLineEdit(QString::number(m_entity.height));
  heightEdit->setObjectName("height");
  billboardLayout->addRow(tr("Alto (Height):"), heightEdit);

  m_directionsSpin = new QSpinBox();
  m_directionsSpin->setRange(1, 32);
  m_directionsSpin->setValue(m_entity.billboard_directions);
  m_directionsSpin->setObjectName("directions");
  m_directionsSpin->setToolTip(
      tr("Número de direcciones en el FPG (1 para billboard simple, 8 para "
         "sprites estilo Doom/Duke3D)\n"
         "NOTA: El FPG debe estar intercalado (Frame1: N, E, S, W, Frame2: N, "
         "E...)\n"
         "Si pones 4, el script saltará de 4 en 4 graficos para animar."));
  billboardLayout->addRow(tr("Direcciones (1, 4, 8):"), m_directionsSpin);

  QLabel *helpLabel =
      new QLabel(tr("<small><i>* El FPG debe seguir el orden: Frente, Derecha, "
                    "Atrás, Izquierda.</i></small>"));
  helpLabel->setWordWrap(true);
  billboardLayout->addRow("", helpLabel);

  billboardGroup->setLayout(billboardLayout);
  mainLayout->addWidget(billboardGroup);

  if (m_entity.type != "billboard") {
    billboardGroup->setVisible(false);
  }

  // Custom Action
  QGroupBox *actionGroup =
      new QGroupBox(tr("Acción Personalizada (Código BennuGD)"));
  QVBoxLayout *actionLayout = new QVBoxLayout();
  m_customActionEdit = new QTextEdit();
  m_customActionEdit->setPlaceholderText(
      "// Código que se ejecuta cuando se activa la entidad\n"
      "// Puedes usar constantes como TYPE_PLAYER, TYPE_ENEMY, etc.\n"
      "// Ejemplo:\n"
      "say(\"¡Entidad activada!\");\n"
      "signal(id, s_kill);");
  m_customActionEdit->setPlainText(m_entity.customAction);
  m_customActionEdit->setMinimumHeight(150);
  actionLayout->addWidget(m_customActionEdit);
  actionGroup->setLayout(actionLayout);
  mainLayout->addWidget(actionGroup);

  // Preview Button
  m_previewButton = new QPushButton(tr("Vista Previa del Código"));
  connect(m_previewButton, &QPushButton::clicked, this,
          &EntityBehaviorDialog::onPreviewCode);

  m_nodeEditorButton = new QPushButton(tr("Editor de Nodos de Comportamiento"));
  m_nodeEditorButton->setIcon(QIcon(":/icons/nodes.png")); // Placeholder icon
  connect(m_nodeEditorButton, &QPushButton::clicked, this,
          &EntityBehaviorDialog::onOpenNodeEditor);

  QHBoxLayout *buttonRow = new QHBoxLayout();
  buttonRow->addWidget(m_previewButton);
  buttonRow->addWidget(m_nodeEditorButton);
  mainLayout->addLayout(buttonRow);

  // Connect signals for dynamic visibility
  connect(m_playerGroup, &QGroupBox::toggled, this,
          &EntityBehaviorDialog::updateVisibility);
  connect(m_controlTypeCombo,
          QOverload<int>::of(&QComboBox::currentIndexChanged), this,
          &EntityBehaviorDialog::updateVisibility);
  connect(m_cameraFollowCheck, &QCheckBox::toggled, this,
          &EntityBehaviorDialog::updateVisibility);

  // Dialog Buttons (added to outer layout, not scrolled)
  QDialogButtonBox *buttonBox =
      new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
  connect(buttonBox, &QDialogButtonBox::accepted, this,
          &EntityBehaviorDialog::onAccept);
  connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
  outerLayout->addWidget(buttonBox);
}

void EntityBehaviorDialog::updateVisibility() {
  int index = m_activationTypeCombo->currentIndex();
  EntityInstance::ActivationType type =
      static_cast<EntityInstance::ActivationType>(
          m_activationTypeCombo->itemData(index).toInt());

  // Show/hide collision settings
  m_collisionWidget->setVisible(type ==
                                EntityInstance::ACTIVATION_ON_COLLISION);

  // Show/hide event settings
  m_eventWidget->setVisible(type == EntityInstance::ACTIVATION_ON_EVENT);

  if (m_animationGroup->isVisible()) {
    bool play = m_playAnimCheck->isChecked();
    m_startFrameSpin->setEnabled(play);
    m_endFrameSpin->setEnabled(play);
    m_animSpeedSpin->setEnabled(play);
    m_animSelectCombo->setEnabled(play);
  }

  // Player/Control visibility context
  bool isPlayer = m_playerGroup->isChecked();
  m_controlTypeCombo->setEnabled(isPlayer);

  bool follow = m_cameraFollowCheck->isChecked();
  for (int i = 0; i < 3; ++i)
    m_camOffsets[i]->setEnabled(isPlayer && follow);
  m_camRotationEdit->setEnabled(isPlayer && !follow);
}

void EntityBehaviorDialog::onActivationTypeChanged(int index) {
  updateVisibility();
}

void EntityBehaviorDialog::onAccept() {
  // Update entity with current values
  int index = m_activationTypeCombo->currentIndex();
  m_entity.activationType = static_cast<EntityInstance::ActivationType>(
      m_activationTypeCombo->itemData(index).toInt());

  m_entity.isVisible = m_visibilityCheck->isChecked();
  m_entity.collisionTarget = m_collisionTargetCombo->currentText();
  m_entity.eventName = m_eventNameEdit->text();
  m_entity.customAction = m_customActionEdit->toPlainText();

  m_entity.customAction = m_customActionEdit->toPlainText();

  // Intro
  if (m_isIntroCheck->isVisible()) {
    m_entity.isIntro = m_isIntroCheck->isChecked();
  }

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

  // NPC Path
  m_entity.npcPathId = m_npcPathCombo->currentData().toInt();
  m_entity.autoStartPath = m_autoStartPathCheck->isChecked();
  m_entity.snapToFloor = m_snapToFloorCheck->isChecked();

  // Billboard
  if (m_entity.type == "billboard") {
    QLineEdit *graphIdEdit = findChild<QLineEdit *>("graphId");
    QLineEdit *startGraphEdit = findChild<QLineEdit *>("startGraph");
    QLineEdit *endGraphEdit = findChild<QLineEdit *>("endGraph");
    QLineEdit *animSpeedEdit = findChild<QLineEdit *>("animSpeed");
    QLineEdit *widthEdit = findChild<QLineEdit *>("width");
    QLineEdit *heightEdit = findChild<QLineEdit *>("height");

    if (graphIdEdit)
      m_entity.graphId = graphIdEdit->text().toInt();
    if (startGraphEdit)
      m_entity.startGraph = startGraphEdit->text().toInt();
    if (endGraphEdit)
      m_entity.endGraph = endGraphEdit->text().toInt();
    if (animSpeedEdit)
      m_entity.animSpeed = animSpeedEdit->text().toFloat();
    if (widthEdit)
      m_entity.width = widthEdit->text().toInt();
    if (heightEdit)
      m_entity.height = heightEdit->text().toInt();
    if (m_directionsSpin)
      m_entity.billboard_directions = m_directionsSpin->value();
  }

  // Physics / Model Size
  if (m_colSize[0])
    m_entity.width = m_colSize[0]->text().toInt();
  if (m_colSize[1])
    m_entity.depth = m_colSize[1]->text().toInt();
  if (m_colSize[2])
    m_entity.height = m_colSize[2]->text().toInt();

  // Physics Engine
  m_entity.physicsEnabled = m_physicsEngineGroup->isChecked();
  m_entity.physicsMass = m_massSpin->value();
  m_entity.physicsFriction = m_frictionSpin->value();
  m_entity.physicsRestitution = m_restitutionSpin->value();
  m_entity.physicsGravityScale = m_gravityScaleSpin->value();
  m_entity.physicsLinearDamping = m_linearDampingSpin->value();
  m_entity.physicsAngularDamping = m_angularDampingSpin->value();
  m_entity.physicsIsStatic = m_staticCheck->isChecked();
  m_entity.physicsIsKinematic = m_kinematicCheck->isChecked();
  m_entity.physicsIsTrigger = m_triggerCheck->isChecked();
  m_entity.physicsLockRotX = m_lockRotXCheck->isChecked();
  m_entity.physicsLockRotY = m_lockRotYCheck->isChecked();
  m_entity.physicsLockRotZ = m_lockRotZCheck->isChecked();
  m_entity.physicsCollisionLayer = m_collisionLayerSpin->value();
  m_entity.physicsCollisionMask = m_collisionMaskSpin->value();

  // Model Animation
  if (m_entity.type == "model") {
    m_entity.startGraph = m_startFrameSpin->value();
    m_entity.endGraph = m_endFrameSpin->value();
    if (m_playAnimCheck->isChecked()) {
      m_entity.animSpeed = m_animSpeedSpin->value();
    } else {
      m_entity.animSpeed = 0.0f;
    }
  }

  accept();
}

void EntityBehaviorDialog::loadModelAnimations() {
  if (m_entity.type != "model")
    return;

  QString assetPath = m_entity.assetPath;
  QString fullPath;

  if (QFileInfo(assetPath).isRelative()) {
    fullPath = m_projectPath + "/" + assetPath;
  } else {
    fullPath = assetPath;
  }

  qDebug() << "Loading MD3 for frames info:" << fullPath;

  MD3Loader loader;
  if (loader.load(fullPath)) {
    int totalFrames = loader.getNumFrames();
    m_totalFramesLabel->setText(tr("Total de Frames: %1").arg(totalFrames));
    m_startFrameSpin->setMaximum(totalFrames > 0 ? totalFrames - 1 : 0);
    m_endFrameSpin->setMaximum(totalFrames > 0 ? totalFrames - 1 : 0);

    // Try to find animation config: modelname.cfg first, then animation.cfg
    QFileInfo md3Info(fullPath);
    QString cfgPath =
        md3Info.absolutePath() + "/" + md3Info.baseName() + ".cfg";
    if (!QFile::exists(cfgPath)) {
      cfgPath = md3Info.absolutePath() + "/animation.cfg";
    }

    QFile cfgFile(cfgPath);
    if (cfgFile.open(QIODevice::ReadOnly)) {
      QTextStream stream(&cfgFile);
      m_animSelectCombo->clear();
      m_animSelectCombo->addItem(tr("(Personalizado / Manual)"), -1);
      while (!stream.atEnd()) {
        QString line = stream.readLine().trimmed();
        if (line.isEmpty() || line.startsWith("//"))
          continue;

        // Format: first-frame, num-frames, looping-frames, frames-per-second
        // Quake 3 MD3 standard
        QStringList parts =
            line.split(QRegularExpression("\\s+"), Qt::SkipEmptyParts);
        if (parts.size() >= 2) {
          QString name = "";
          int commentIdx = line.indexOf("//");
          if (commentIdx != -1) {
            name = line.mid(commentIdx + 2).trimmed();
          } else {
            name = tr("Anim %1").arg(m_animSelectCombo->count());
          }

          bool ok1, ok2;
          int first = parts[0].toInt(&ok1);
          int length = parts[1].toInt(&ok2);
          if (ok1 && ok2) {
            m_animSelectCombo->addItem(name, QPoint(first, first + length - 1));
          }
        }
      }
      cfgFile.close();
    }
  } else {
    m_totalFramesLabel->setText(
        tr("Error cargando MD3 para lectura de frames."));
  }
}

void EntityBehaviorDialog::onPreviewCode() {
  QString preview = generatePreviewCode();

  QMessageBox msgBox(this);
  msgBox.setWindowTitle(tr("Vista Previa del Código"));
  msgBox.setText(tr("Este código se generará en main.prg:"));
  msgBox.setDetailedText(preview);
  msgBox.setIcon(QMessageBox::Information);
  msgBox.exec();
}

QString EntityBehaviorDialog::generatePreviewCode() const {
  QString code;
  code += QString("// Entidad: %1 (Tipo: %2)\n")
              .arg(m_entity.processName)
              .arg(m_entity.type);
  code += QString("Process %1()\n").arg(m_entity.processName);
  code += "Private\n";

  // Add private variables based on activation type
  if (m_entity.activationType == EntityInstance::ACTIVATION_ON_COLLISION) {
    code += QString("    int collision_target = %1;\n")
                .arg(m_entity.collisionTarget);
  } else if (m_entity.activationType == EntityInstance::ACTIVATION_ON_EVENT) {
    code += QString("    int event_triggered = 0;\n");
  }

  code += "Begin\n";
  code += "    // Configuración inicial\n";

  QString actionCode = m_entity.customAction;
  if (!m_entity.behaviorGraph.nodes.isEmpty()) {
    actionCode = ProcessGenerator::generateGraphCode(m_entity.behaviorGraph);
  }

  // Position
  code += QString("    world_x = %1; world_y = %2; world_z = %3;\n")
              .arg(m_entity.x)
              .arg(m_entity.y)
              .arg(m_entity.z);

  if (!actionCode.isEmpty()) {
    code += "\n    // Lógica del Grafo de Comportamiento:\n";
    code += "    " + actionCode.replace("\n", "\n    ") + "\n";
  }

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
      code +=
          "        RAY_SET_CAMERA(world_x + offset, world_y + offset, ...);\n";
    }
  }

  code += "        FRAME;\n";
  code += "    END\n";

  code += "End\n";

  return code;
}

void EntityBehaviorDialog::onOpenNodeEditor() {
  BehaviorNodeEditor editor(m_entity.behaviorGraph, m_projectPath, this);
  if (editor.exec() == QDialog::Accepted) {
    // Logic for when the editor is closed/saved if needed
  }
}
