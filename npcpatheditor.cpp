#include "npcpatheditor.h"
#include <QFormLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QMessageBox>
#include <QPainter>
#include <QVBoxLayout>

NPCPathEditor::NPCPathEditor(NPCPath &path, const MapData *mapData,
                             QWidget *parent)
    : QDialog(parent), m_path(path), m_mapData(mapData),
      m_selectedWaypointIndex(-1) {
  setWindowTitle("Editor de Rutas NPC - " + path.name);
  resize(900, 600);

  setupUI();
  if (m_mapData) {
    setup2DView();
  }
  updateWaypointList();
}

NPCPathEditor::~NPCPathEditor() {}

void NPCPathEditor::setupUI() {
  QVBoxLayout *mainLayout = new QVBoxLayout(this);

  // ===== PATH PROPERTIES =====
  QGroupBox *pathGroup = new QGroupBox("Propiedades de la Ruta");
  QFormLayout *pathForm = new QFormLayout(pathGroup);

  m_nameEdit = new QLineEdit(m_path.name);
  connect(m_nameEdit, &QLineEdit::textChanged, this,
          &NPCPathEditor::onNameChanged);
  pathForm->addRow("Nombre:", m_nameEdit);

  m_loopModeCombo = new QComboBox();
  m_loopModeCombo->addItem("Una vez (parar al final)", NPCPath::LOOP_NONE);
  m_loopModeCombo->addItem("Repetir (bucle)", NPCPath::LOOP_REPEAT);
  m_loopModeCombo->addItem("Ping-Pong (ida y vuelta)", NPCPath::LOOP_PINGPONG);
  m_loopModeCombo->addItem("Aleatorio", NPCPath::LOOP_RANDOM);
  m_loopModeCombo->setCurrentIndex(static_cast<int>(m_path.loop_mode));
  connect(m_loopModeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
          this, &NPCPathEditor::onLoopModeChanged);
  pathForm->addRow("Modo de bucle:", m_loopModeCombo);

  m_visibleCheck = new QCheckBox("Mostrar ruta en el editor");
  m_visibleCheck->setChecked(m_path.visible);
  connect(m_visibleCheck, &QCheckBox::toggled, this,
          &NPCPathEditor::onVisibleChanged);
  pathForm->addRow("", m_visibleCheck);

  mainLayout->addWidget(pathGroup);

  // ===== MAIN CONTENT: 2D VIEW + WAYPOINTS =====
  QHBoxLayout *contentLayout = new QHBoxLayout();

  // Left: 2D Map View
  m_canvas = new NPCPathCanvas();
  m_canvas->setMinimumSize(400, 400);
  connect(m_canvas, &NPCPathCanvas::waypointAdded, this,
          &NPCPathEditor::onCanvasWaypointAdded);
  connect(m_canvas, &NPCPathCanvas::waypointSelected, this,
          &NPCPathEditor::onCanvasWaypointSelected);
  connect(m_canvas, &NPCPathCanvas::waypointMoved, this,
          &NPCPathEditor::onCanvasWaypointMoved);
  contentLayout->addWidget(m_canvas, 2);

  // ===== WAYPOINTS =====
  QHBoxLayout *waypointLayout = new QHBoxLayout();

  // Left: Waypoint list
  QVBoxLayout *listLayout = new QVBoxLayout();
  QLabel *listLabel = new QLabel("Puntos de ruta:");
  listLayout->addWidget(listLabel);

  m_waypointList = new QListWidget();
  connect(m_waypointList, &QListWidget::currentRowChanged, this,
          &NPCPathEditor::onWaypointSelected);
  listLayout->addWidget(m_waypointList);

  QHBoxLayout *buttonLayout = new QHBoxLayout();
  m_addButton = new QPushButton("Añadir Punto");
  connect(m_addButton, &QPushButton::clicked, this,
          &NPCPathEditor::onAddWaypoint);
  buttonLayout->addWidget(m_addButton);

  m_removeButton = new QPushButton("Eliminar");
  m_removeButton->setEnabled(false);
  connect(m_removeButton, &QPushButton::clicked, this,
          &NPCPathEditor::onRemoveWaypoint);
  buttonLayout->addWidget(m_removeButton);

  listLayout->addLayout(buttonLayout);
  waypointLayout->addLayout(listLayout, 1);

  // Right: Waypoint properties
  QGroupBox *propsGroup = new QGroupBox("Propiedades del Punto");
  QFormLayout *propsForm = new QFormLayout(propsGroup);

  m_xSpin = new QDoubleSpinBox();
  m_xSpin->setRange(-10000, 10000);
  m_xSpin->setDecimals(2);
  connect(m_xSpin, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this,
          &NPCPathEditor::onWaypointPropertyChanged);
  propsForm->addRow("X:", m_xSpin);

  m_ySpin = new QDoubleSpinBox();
  m_ySpin->setRange(-10000, 10000);
  m_ySpin->setDecimals(2);
  connect(m_ySpin, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this,
          &NPCPathEditor::onWaypointPropertyChanged);
  propsForm->addRow("Y:", m_ySpin);

  m_zSpin = new QDoubleSpinBox();
  m_zSpin->setRange(-1000, 1000);
  m_zSpin->setDecimals(2);
  connect(m_zSpin, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this,
          &NPCPathEditor::onWaypointPropertyChanged);
  propsForm->addRow("Z:", m_zSpin);

  m_waitTimeSpin = new QSpinBox();
  m_waitTimeSpin->setRange(0, 10000);
  m_waitTimeSpin->setSuffix(" frames");
  m_waitTimeSpin->setToolTip("Frames a esperar en este punto (0 = sin espera)");
  connect(m_waitTimeSpin, QOverload<int>::of(&QSpinBox::valueChanged), this,
          &NPCPathEditor::onWaypointPropertyChanged);
  propsForm->addRow("Tiempo de espera:", m_waitTimeSpin);

  m_speedSpin = new QDoubleSpinBox();
  m_speedSpin->setRange(0.1, 100.0);
  m_speedSpin->setDecimals(2);
  m_speedSpin->setValue(5.0);
  m_speedSpin->setToolTip("Velocidad de movimiento para llegar a este punto");
  connect(m_speedSpin, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
          this, &NPCPathEditor::onWaypointPropertyChanged);
  propsForm->addRow("Velocidad:", m_speedSpin);

  m_lookAngleSpin = new QDoubleSpinBox();
  m_lookAngleSpin->setRange(-1, 360);
  m_lookAngleSpin->setDecimals(2);
  m_lookAngleSpin->setValue(-1);
  m_lookAngleSpin->setSuffix("°");
  m_lookAngleSpin->setToolTip(
      "Dirección a mirar (-1 = auto, mirar hacia el movimiento)");
  connect(m_lookAngleSpin, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
          this, &NPCPathEditor::onWaypointPropertyChanged);
  propsForm->addRow("Ángulo de mirada:", m_lookAngleSpin);

  // Disable properties initially
  m_xSpin->setEnabled(false);
  m_ySpin->setEnabled(false);
  m_zSpin->setEnabled(false);
  m_waitTimeSpin->setEnabled(false);
  m_speedSpin->setEnabled(false);
  m_lookAngleSpin->setEnabled(false);

  waypointLayout->addWidget(propsGroup, 1);
  contentLayout->addLayout(waypointLayout, 1);
  mainLayout->addLayout(contentLayout);

  // ===== DIALOG BUTTONS =====
  QHBoxLayout *dialogButtons = new QHBoxLayout();
  dialogButtons->addStretch();

  QPushButton *okButton = new QPushButton("Aceptar");
  connect(okButton, &QPushButton::clicked, this, &QDialog::accept);
  dialogButtons->addWidget(okButton);

  QPushButton *cancelButton = new QPushButton("Cancelar");
  connect(cancelButton, &QPushButton::clicked, this, &QDialog::reject);
  dialogButtons->addWidget(cancelButton);

  mainLayout->addLayout(dialogButtons);
}

void NPCPathEditor::updateWaypointList() {
  m_waypointList->clear();

  for (int i = 0; i < m_path.waypoints.size(); ++i) {
    const Waypoint &wp = m_path.waypoints[i];
    QString label = QString("WP %1: (%.1f, %.1f, %.1f)")
                        .arg(i)
                        .arg(wp.x)
                        .arg(wp.y)
                        .arg(wp.z);
    m_waypointList->addItem(label);
  }

  // Update remove button state
  m_removeButton->setEnabled(m_path.waypoints.size() > 0);

  // Update canvas
  if (m_canvas) {
    m_canvas->update();
  }
}

void NPCPathEditor::updateWaypointProperties() {
  bool hasSelection = m_selectedWaypointIndex >= 0 &&
                      m_selectedWaypointIndex < m_path.waypoints.size();

  m_xSpin->setEnabled(hasSelection);
  m_ySpin->setEnabled(hasSelection);
  m_zSpin->setEnabled(hasSelection);
  m_waitTimeSpin->setEnabled(hasSelection);
  m_speedSpin->setEnabled(hasSelection);
  m_lookAngleSpin->setEnabled(hasSelection);

  if (hasSelection) {
    const Waypoint &wp = m_path.waypoints[m_selectedWaypointIndex];

    // Block signals to avoid triggering onWaypointPropertyChanged
    m_xSpin->blockSignals(true);
    m_ySpin->blockSignals(true);
    m_zSpin->blockSignals(true);
    m_waitTimeSpin->blockSignals(true);
    m_speedSpin->blockSignals(true);
    m_lookAngleSpin->blockSignals(true);

    m_xSpin->setValue(wp.x);
    m_ySpin->setValue(wp.y);
    m_zSpin->setValue(wp.z);
    m_waitTimeSpin->setValue(wp.wait_time);
    m_speedSpin->setValue(wp.speed);
    m_lookAngleSpin->setValue(wp.look_angle);

    m_xSpin->blockSignals(false);
    m_ySpin->blockSignals(false);
    m_zSpin->blockSignals(false);
    m_waitTimeSpin->blockSignals(false);
    m_speedSpin->blockSignals(false);
    m_lookAngleSpin->blockSignals(false);
  }
}

void NPCPathEditor::onAddWaypoint() {
  // Add waypoint at origin (or last waypoint position + offset)
  Waypoint newWp;
  if (!m_path.waypoints.isEmpty()) {
    const Waypoint &last = m_path.waypoints.last();
    newWp.x = last.x + 100;
    newWp.y = last.y;
    newWp.z = last.z;
  }

  m_path.waypoints.append(newWp);
  updateWaypointList();

  // Select the new waypoint
  m_waypointList->setCurrentRow(m_path.waypoints.size() - 1);
}

void NPCPathEditor::onRemoveWaypoint() {
  if (m_selectedWaypointIndex >= 0 &&
      m_selectedWaypointIndex < m_path.waypoints.size()) {
    m_path.waypoints.removeAt(m_selectedWaypointIndex);
    updateWaypointList();

    // Select previous or clear selection
    if (m_path.waypoints.isEmpty()) {
      m_selectedWaypointIndex = -1;
      updateWaypointProperties();
    } else {
      int newIndex = qMin(m_selectedWaypointIndex, m_path.waypoints.size() - 1);
      m_waypointList->setCurrentRow(newIndex);
    }
  }
}

void NPCPathEditor::onWaypointSelected(int index) {
  m_selectedWaypointIndex = index;
  updateWaypointProperties();
  if (m_canvas) {
    m_canvas->setSelectedWaypoint(index);
  }
}

void NPCPathEditor::onWaypointPropertyChanged() {
  if (m_selectedWaypointIndex >= 0 &&
      m_selectedWaypointIndex < m_path.waypoints.size()) {
    Waypoint &wp = m_path.waypoints[m_selectedWaypointIndex];

    wp.x = m_xSpin->value();
    wp.y = m_ySpin->value();
    wp.z = m_zSpin->value();
    wp.wait_time = m_waitTimeSpin->value();
    wp.speed = m_speedSpin->value();
    wp.look_angle = m_lookAngleSpin->value();

    // Update list item text
    QListWidgetItem *item = m_waypointList->item(m_selectedWaypointIndex);
    if (item) {
      item->setText(QString("WP %1: (%.1f, %.1f, %.1f)")
                        .arg(m_selectedWaypointIndex)
                        .arg(wp.x)
                        .arg(wp.y)
                        .arg(wp.z));
    }
  }
}

void NPCPathEditor::onLoopModeChanged(int index) {
  m_path.loop_mode = static_cast<NPCPath::LoopMode>(index);
}

void NPCPathEditor::onNameChanged(const QString &text) {
  m_path.name = text;
  setWindowTitle("Editor de Rutas NPC - " + text);
}

void NPCPathEditor::onVisibleChanged(bool checked) { m_path.visible = checked; }

void NPCPathEditor::setup2DView() {
  m_canvas->setMapData(m_mapData);
  m_canvas->setPath(&m_path);
}

void NPCPathEditor::draw2DMap() {
  // No longer needed - canvas handles drawing
}

void NPCPathEditor::onCanvasWaypointAdded(float x, float y) {
  Waypoint newWp;
  newWp.x = x;
  newWp.y = y;
  newWp.z = 64.0f; // Default height
  newWp.wait_time = 0;
  newWp.speed = 5.0f;
  newWp.look_angle = -1.0f;

  m_path.waypoints.append(newWp);
  updateWaypointList();
  m_waypointList->setCurrentRow(m_path.waypoints.size() - 1);
  m_canvas->update();
}

void NPCPathEditor::onCanvasWaypointSelected(int index) {
  m_waypointList->setCurrentRow(index);
}

void NPCPathEditor::onCanvasWaypointMoved(int index, float x, float y) {
  if (index >= 0 && index < m_path.waypoints.size()) {
    m_path.waypoints[index].x = x;
    m_path.waypoints[index].y = y;
    updateWaypointList();
    updateWaypointProperties();
  }
}
