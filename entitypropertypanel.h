#ifndef ENTITYPROPERTYPANEL_H
#define ENTITYPROPERTYPANEL_H

#include "mapdata.h"
#include <QDoubleSpinBox>
#include <QFormLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QWidget>

class EntityPropertyPanel : public QWidget {
  Q_OBJECT

public:
  explicit EntityPropertyPanel(QWidget *parent = nullptr);

  void setEntity(int index, const EntityInstance &entity);
  void clearSelection();

signals:
  void entityChanged(int index, const EntityInstance &entity);
  void editBehaviorRequested(int index, const EntityInstance &entity);

private slots:
  void onValueChanged();
  void onTypeChanged();

private:
  int m_currentIndex;
  EntityInstance m_currentEntity;
  bool m_updating;

  QLineEdit *m_typeEdit;
  QLineEdit *m_nameEdit; // processName
  QLineEdit *m_assetEdit;

  QDoubleSpinBox *m_xSpin;
  QDoubleSpinBox *m_ySpin; // Depth (RayMap Z / GL Z)
  QDoubleSpinBox
      *m_zSpin; // Height (RayMap Y / GL Y) - wait, check coordinate system
  QDoubleSpinBox *m_angleSpin; // Rotation

  // In RayMap data:
  // x, y (2D map coordinates), z (height)
  // Actually in RayMapFormat:
  // x = x, y = z (height), z = y (depth) ?
  // Let's stick to the struct fields: x, y, z.

  QPushButton *m_editBehaviorButton;

  // Helper to setup spins
  void setupSpinBox(QDoubleSpinBox *spin, const QString &suffix);
};

#endif // ENTITYPROPERTYPANEL_H
