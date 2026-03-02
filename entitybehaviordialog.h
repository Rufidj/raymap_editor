#ifndef ENTITYBEHAVIORDIALOG_H
#define ENTITYBEHAVIORDIALOG_H

#include "mapdata.h"
#include <QCheckBox>
#include <QComboBox>
#include <QDialog>
#include <QFormLayout>
#include <QGroupBox>
#include <QLabel>
#include <QLineEdit>
#include <QPoint>
#include <QPushButton>
#include <QSpinBox>
#include <QTextEdit>
#include <QVariant>

class EntityBehaviorDialog : public QDialog {
  Q_OBJECT

public:
  explicit EntityBehaviorDialog(
      const EntityInstance &entity, const QString &projectPath,
      const QVector<NPCPath> *availablePaths = nullptr,
      const QStringList &availableProcesses = QStringList(),
      QWidget *parent = nullptr);

  EntityInstance getEntity() const { return m_entity; }

private slots:
  void onActivationTypeChanged(int index);
  void onAccept();
  void onPreviewCode();
  void onOpenNodeEditor();

private:
  EntityInstance m_entity;
  QString m_projectPath;
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
  QSpinBox *m_directionsSpin;

  // Physics / Collision Box settings
  QGroupBox *m_physicsGroup;
  QLineEdit *m_colSize[3]; // 0: Width, 1: Depth, 2: Height

  // Physics Engine settings
  QGroupBox *m_physicsEngineGroup;
  QCheckBox *m_physicsEnabledCheck;
  QDoubleSpinBox *m_massSpin;
  QDoubleSpinBox *m_frictionSpin;
  QDoubleSpinBox *m_restitutionSpin;
  QDoubleSpinBox *m_gravityScaleSpin;
  QDoubleSpinBox *m_linearDampingSpin;
  QDoubleSpinBox *m_angularDampingSpin;
  QCheckBox *m_staticCheck;
  QCheckBox *m_kinematicCheck;
  QCheckBox *m_triggerCheck;
  QCheckBox *m_lockRotXCheck;
  QCheckBox *m_lockRotYCheck;
  QCheckBox *m_lockRotZCheck;
  QSpinBox *m_collisionLayerSpin;
  QSpinBox *m_collisionMaskSpin;

  // Model Animation settings
  QGroupBox *m_animationGroup;
  QCheckBox *m_playAnimCheck;
  QSpinBox *m_startFrameSpin;
  QSpinBox *m_endFrameSpin;
  QDoubleSpinBox *m_animSpeedSpin;
  QComboBox *m_animSelectCombo;
  QLabel *m_totalFramesLabel;

  // Preview
  QPushButton *m_previewButton;
  QPushButton *m_nodeEditorButton;

  void setupUI();
  void updateVisibility();
  void loadModelAnimations();
  QString generatePreviewCode() const;
};

#endif // ENTITYBEHAVIORDIALOG_H
