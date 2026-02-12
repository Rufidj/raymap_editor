#ifndef ENTITYBEHAVIORDIALOG_H
#define ENTITYBEHAVIORDIALOG_H

#include "mapdata.h"
#include <QCheckBox>
#include <QComboBox>
#include <QDialog>
#include <QFormLayout>
#include <QGroupBox>
#include <QLineEdit>
#include <QPushButton>
#include <QTextEdit>

class EntityBehaviorDialog : public QDialog {
  Q_OBJECT

public:
  explicit EntityBehaviorDialog(
      const EntityInstance &entity,
      const QVector<NPCPath> *availablePaths = nullptr,
      const QStringList &availableProcesses = QStringList(),
      QWidget *parent = nullptr);

  EntityInstance getEntity() const { return m_entity; }

private slots:
  void onActivationTypeChanged(int index);
  void onAccept();
  void onPreviewCode();

private:
  EntityInstance m_entity;
  const QVector<NPCPath> *m_availablePaths;
  QStringList m_availableProcesses;

  // UI Components
  QComboBox *m_activationTypeCombo;
  QCheckBox *m_visibilityCheck;

  // Collision settings (visible only for ACTIVATION_ON_COLLISION)
  QWidget *m_collisionWidget;
  QComboBox *m_collisionTargetCombo;

  // Event settings (visible only for ACTIVATION_ON_EVENT)
  QWidget *m_eventWidget;
  QLineEdit *m_eventNameEdit;

  // Custom action
  QTextEdit *m_customActionEdit;

  // Player & Control settings
  QGroupBox *m_playerGroup;
  QCheckBox *m_isPlayerCheck;
  QComboBox *m_controlTypeCombo;
  QCheckBox *m_cameraFollowCheck;
  QLineEdit *m_camOffsets[3]; // X, Y, Z
  QLineEdit *m_camRotationEdit;
  QLineEdit *m_initialRotationEdit; // Initial model rotation
  QCheckBox *m_isIntroCheck;        // Intro cutscene mode

  // NPC Path settings
  QComboBox *m_npcPathCombo;
  QCheckBox *m_autoStartPathCheck;
  QCheckBox *m_snapToFloorCheck;

  // Preview
  QPushButton *m_previewButton;

  void setupUI();
  void updateVisibility();
  QString generatePreviewCode() const;
};

#endif // ENTITYBEHAVIORDIALOG_H
