#ifndef NPCPATHEDITOR_H
#define NPCPATHEDITOR_H

#include "mapdata.h"
#include "npcpathcanvas.h"
#include <QCheckBox>
#include <QComboBox>
#include <QDialog>
#include <QDoubleSpinBox>
#include <QLineEdit>
#include <QListWidget>
#include <QPushButton>
#include <QSpinBox>

class NPCPathEditor : public QDialog {
  Q_OBJECT

public:
  explicit NPCPathEditor(NPCPath &path, const MapData *mapData = nullptr,
                         QWidget *parent = nullptr);
  ~NPCPathEditor();

  // Get the edited path
  const NPCPath &getPath() const { return m_path; }

private slots:
  void onAddWaypoint();
  void onRemoveWaypoint();
  void onWaypointSelected(int index);
  void onWaypointPropertyChanged();
  void onLoopModeChanged(int index);
  void onNameChanged(const QString &text);
  void onVisibleChanged(bool checked);

  // Canvas signals
  void onCanvasWaypointAdded(float x, float y);
  void onCanvasWaypointSelected(int index);
  void onCanvasWaypointMoved(int index, float x, float y);

private:
  void setupUI();
  void updateWaypointList();
  void updateWaypointProperties();
  void setup2DView();
  void draw2DMap();

  NPCPath &m_path;
  const MapData *m_mapData;

  // 2D View
  NPCPathCanvas *m_canvas;

  // UI Components
  QLineEdit *m_nameEdit;
  QComboBox *m_loopModeCombo;
  QCheckBox *m_visibleCheck;

  QListWidget *m_waypointList;
  QPushButton *m_addButton;
  QPushButton *m_removeButton;

  // Waypoint properties
  QDoubleSpinBox *m_xSpin;
  QDoubleSpinBox *m_ySpin;
  QDoubleSpinBox *m_zSpin;
  QSpinBox *m_waitTimeSpin;
  QDoubleSpinBox *m_speedSpin;
  QDoubleSpinBox *m_lookAngleSpin;

  int m_selectedWaypointIndex;
};

#endif // NPCPATHEDITOR_H
