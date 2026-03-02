#include "objimportdialog.h"
#include "md3generator.h"
#include "objtomd3converter.h"
#include <QApplication>
#include <QDir>
#include <QFileDialog>
#include <QFileInfo>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QMessageBox>
#include <QProgressDialog>
#include <QPushButton>
#include <QVBoxLayout>

ObjImportDialog::ObjImportDialog(QWidget *parent) : QDialog(parent) {
  setWindowTitle(tr("Conversor OBJ a MD3"));
  resize(400, 200);

  QVBoxLayout *mainLayout = new QVBoxLayout(this);

  // Input
  QHBoxLayout *inputLayout = new QHBoxLayout();
  m_inputEdit = new QLineEdit();
  QPushButton *browseInputBtn = new QPushButton(tr("Buscar OBJ..."));
  inputLayout->addWidget(new QLabel(tr("Entrada:")));
  inputLayout->addWidget(m_inputEdit);
  inputLayout->addWidget(browseInputBtn);
  mainLayout->addLayout(inputLayout);

  // Output
  QHBoxLayout *outputLayout = new QHBoxLayout();
  m_outputEdit = new QLineEdit();
  QPushButton *browseOutputBtn = new QPushButton(tr("Salida MD3..."));
  outputLayout->addWidget(new QLabel(tr("Salida:")));
  outputLayout->addWidget(m_outputEdit);
  outputLayout->addWidget(browseOutputBtn);
  mainLayout->addLayout(outputLayout);

  // Options
  QHBoxLayout *optionsLayout = new QHBoxLayout();
  m_scaleSpin = new QDoubleSpinBox();
  m_scaleSpin->setRange(0.01, 1000.0);
  m_scaleSpin->setValue(1.0);
  m_scaleSpin->setSingleStep(0.1);

  m_atlasCheck = new QCheckBox(tr("Generar Atlas de Textura (PNG)"));

  m_atlasSizeSpin = new QSpinBox();
  m_atlasSizeSpin->setRange(64, 4096);
  m_atlasSizeSpin->setValue(1024);
  m_atlasSizeSpin->setSingleStep(128);
  m_atlasSizeSpin->setSuffix(" px");

  optionsLayout->addWidget(new QLabel(tr("Escala:")));
  optionsLayout->addWidget(m_scaleSpin);
  optionsLayout->addWidget(new QLabel(tr("Tam. Atlas:")));
  optionsLayout->addWidget(m_atlasSizeSpin);
  optionsLayout->addStretch();
  optionsLayout->addWidget(m_atlasCheck);
  mainLayout->addLayout(optionsLayout);

  // Rotation Control with Visual Preview
  QHBoxLayout *rotationLayout = new QHBoxLayout();
  m_rotationSpin = new QSpinBox();
  m_rotationSpin->setRange(0, 359);
  m_rotationSpin->setValue(0);
  m_rotationSpin->setSuffix("°");
  m_rotationSpin->setToolTip(tr("Rotación inicial del modelo (0° = frente, 90° "
                                "= izquierda, 180° = atrás, 270° = derecha)"));

  m_rotationPreview = new QLabel("↑");
  m_rotationPreview->setStyleSheet(
      "QLabel { font-size: 48px; color: #2196F3; }");
  m_rotationPreview->setAlignment(Qt::AlignCenter);
  m_rotationPreview->setMinimumSize(80, 80);
  m_rotationPreview->setToolTip(tr("Dirección hacia donde mirará el modelo"));

  rotationLayout->addWidget(new QLabel(tr("Rotación Inicial:")));
  rotationLayout->addWidget(m_rotationSpin);
  rotationLayout->addStretch();
  rotationLayout->addWidget(m_rotationPreview);
  mainLayout->addLayout(rotationLayout);

  // 3D Model Preview
  m_previewWidget = new ModelPreviewWidget();
  m_previewWidget->setMinimumSize(300, 300);
  m_previewWidget->setMaximumSize(400, 400);
  m_previewWidget->setToolTip(
      tr("Preview 3D del modelo. Arrastra con botón izquierdo para rotar la "
         "vista, botón derecho para zoom."));
  mainLayout->addWidget(m_previewWidget, 0, Qt::AlignCenter);

  // Explanation note
  QLabel *noteLabel = new QLabel(
      tr("<b>Cómo usar:</b><br>"
         "1. Usa <b>Orientación</b> (X,Y) para acostar modelos verticales<br>"
         "2. Usa <b>Dirección</b> (Z) para que el FRENTE mire a la línea "
         "<b>ROJA</b><br>"
         "3. La rotación del preview (mouse) es solo visual"));
  noteLabel->setWordWrap(true);
  noteLabel->setStyleSheet(
      "QLabel { color: #cccccc; font-size: 11px; padding: 8px; background: "
      "#2d2d2d; border-radius: 4px; border: 1px solid #444; }");
  mainLayout->addWidget(noteLabel);

  // Model Orientation Controls (for fixing vertical models, etc.)
  QGroupBox *orientGroup =
      new QGroupBox(tr("Orientación del Modelo (Corrección)"));
  QVBoxLayout *orientMainLayout = new QVBoxLayout(orientGroup);

  // Spinboxes row
  QHBoxLayout *orientLayout = new QHBoxLayout();

  m_orientXSpin = new QSpinBox();
  m_orientXSpin->setRange(-180, 180);
  m_orientXSpin->setValue(0);
  m_orientXSpin->setSuffix("°");
  m_orientXSpin->setToolTip(tr("Rotación X (Pitch)"));

  m_orientYSpin = new QSpinBox();
  m_orientYSpin->setRange(-180, 180);
  m_orientYSpin->setValue(0);
  m_orientYSpin->setSuffix("°");
  m_orientYSpin->setToolTip(tr("Rotación Y (Yaw)"));

  m_orientZSpin = new QSpinBox();
  m_orientZSpin->setRange(-180, 180);
  m_orientZSpin->setValue(0);
  m_orientZSpin->setSuffix("°");
  m_orientZSpin->setToolTip(tr("Rotación Z (Roll)"));

  QPushButton *resetOrientBtn = new QPushButton(tr("Reset"));
  resetOrientBtn->setToolTip(tr("Restablecer orientación a 0°"));

  orientLayout->addWidget(new QLabel(tr("X:")));
  orientLayout->addWidget(m_orientXSpin);
  orientLayout->addWidget(new QLabel(tr("Y:")));
  orientLayout->addWidget(m_orientYSpin);
  orientLayout->addWidget(new QLabel(tr("Z:")));
  orientLayout->addWidget(m_orientZSpin);
  orientLayout->addWidget(resetOrientBtn);

  orientMainLayout->addLayout(orientLayout);

  // Quick fix buttons row
  QHBoxLayout *quickFixLayout = new QHBoxLayout();
  quickFixLayout->addWidget(new QLabel(tr("Corrección rápida:")));

  QPushButton *fix1Btn = new QPushButton(tr("↓ Acostar (+90°X)"));
  fix1Btn->setToolTip(tr("Acostar modelo vertical (X = +90°)"));
  connect(fix1Btn, &QPushButton::clicked, [this]() {
    m_orientXSpin->setValue(90);
    m_orientYSpin->setValue(0);
    m_orientZSpin->setValue(0);
  });

  QPushButton *fix2Btn = new QPushButton(tr("↑ Acostar (-90°X)"));
  fix2Btn->setToolTip(tr("Acostar modelo vertical (X = -90°)"));
  connect(fix2Btn, &QPushButton::clicked, [this]() {
    m_orientXSpin->setValue(-90);
    m_orientYSpin->setValue(0);
    m_orientZSpin->setValue(0);
  });

  QPushButton *fix3Btn = new QPushButton(tr("⟲ Voltear (180°Z)"));
  fix3Btn->setToolTip(tr("Voltear modelo boca abajo (Z = 180°)"));
  connect(fix3Btn, &QPushButton::clicked,
          [this]() { m_orientZSpin->setValue(180); });

  QPushButton *fix4Btn = new QPushButton(tr("↓⟲ +90°X +180°Z"));
  fix4Btn->setToolTip(tr("Acostar y voltear (X = +90°, Z = 180°)"));
  connect(fix4Btn, &QPushButton::clicked, [this]() {
    m_orientXSpin->setValue(90);
    m_orientYSpin->setValue(0);
    m_orientZSpin->setValue(180);
  });

  quickFixLayout->addWidget(fix1Btn);
  quickFixLayout->addWidget(fix2Btn);
  quickFixLayout->addWidget(fix3Btn);
  quickFixLayout->addWidget(fix4Btn);
  quickFixLayout->addStretch();

  orientMainLayout->addLayout(quickFixLayout);

  mainLayout->addWidget(orientGroup);

  // Buttons
  QHBoxLayout *btnLayout = new QHBoxLayout();
  QPushButton *convertBtn = new QPushButton(tr("Convertir"));
  QPushButton *cancelBtn = new QPushButton(tr("Cerrar"));
  btnLayout->addStretch();
  btnLayout->addWidget(convertBtn);
  btnLayout->addWidget(cancelBtn);
  mainLayout->addLayout(btnLayout);

  // Connections
  connect(browseInputBtn, &QPushButton::clicked, this,
          &ObjImportDialog::browseInput);
  connect(browseOutputBtn, &QPushButton::clicked, this,
          &ObjImportDialog::browseOutput);
  connect(convertBtn, &QPushButton::clicked, this, &ObjImportDialog::convert);
  connect(cancelBtn, &QPushButton::clicked, this, &QDialog::reject);
  connect(m_rotationSpin, QOverload<int>::of(&QSpinBox::valueChanged), this,
          &ObjImportDialog::onRotationChanged);
  connect(m_scaleSpin, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
          [this](double value) { m_previewWidget->setScale(value); });

  // Model orientation controls
  connect(m_orientXSpin, QOverload<int>::of(&QSpinBox::valueChanged), this,
          &ObjImportDialog::onModelOrientationChanged);
  connect(m_orientYSpin, QOverload<int>::of(&QSpinBox::valueChanged), this,
          &ObjImportDialog::onModelOrientationChanged);
  connect(m_orientZSpin, QOverload<int>::of(&QSpinBox::valueChanged), this,
          &ObjImportDialog::onModelOrientationChanged);
  connect(resetOrientBtn, &QPushButton::clicked, this,
          &ObjImportDialog::resetModelOrientation);

  // Initialize preview
  onRotationChanged(0);
}

void ObjImportDialog::browseInput() {
  QString path = QFileDialog::getOpenFileName(this, tr("Abrir Modelo"), "",
                                              tr("Modelos 3D (*.obj *.glb)"));
  if (!path.isEmpty()) {
    m_inputEdit->setText(path);
    // Auto-set output
    QFileInfo fi(path);
    QString out = fi.absolutePath() + "/" + fi.completeBaseName() + ".md3";
    m_outputEdit->setText(out);

    // Load model into preview
    ObjToMd3Converter converter;
    bool loaded = false;
    if (path.endsWith(".glb", Qt::CaseInsensitive)) {
      loaded = converter.loadGlb(path);
    } else {
      loaded = converter.loadObj(path);
    }

    if (loaded) {
      m_previewWidget->clearSurfaces();

      // Map global vertices to primitive-local surfaces
      // We group triangles by material index for the preview
      QMap<int, QVector<int>> materialTriGroups;
      for (int i = 0; i < converter.triangles().size(); ++i) {
        int matIdx = converter.faceMaterialIndices()[i];
        materialTriGroups[matIdx].append(i);
      }

      QList<int> matIndices = materialTriGroups.keys();
      for (int matIdx : matIndices) {
        const QVector<int> &triIndices = materialTriGroups[matIdx];
        MD3Generator::MeshData meshData;

        // Initialize animation frames if available
        bool hasAnim = !converter.animationFrames().isEmpty();
        if (hasAnim) {
          int numFrames = converter.animationFrames().size();
          meshData.animationFrames.resize(numFrames);
          for (int f = 0; f < numFrames; ++f) {
            const auto &frameVerts = converter.animationFrames()[f];
            for (int i = 0; i < frameVerts.size(); ++i) {
              MD3Generator::MeshData::VertexData vert;
              vert.pos = frameVerts[i];
              vert.normal =
                  QVector3D(0, 0, 1); // Normal support could be better
              if (i < converter.texCoords().size()) {
                vert.uv = converter.texCoords()[i];
              }
              meshData.animationFrames[f].append(vert);
            }
          }
        } else {
          // Legacy/Static Model
          for (int i = 0; i < converter.vertices().size(); ++i) {
            MD3Generator::MeshData::VertexData vert;
            vert.pos = converter.vertices()[i];
            vert.normal = QVector3D(0, 0, 1);
            if (i < converter.texCoords().size()) {
              vert.uv = converter.texCoords()[i];
            }
            meshData.vertices.append(vert);
          }
        }

        for (int triIdx : triIndices) {
          const auto &tri = converter.triangles()[triIdx];
          meshData.indices.append(tri.indices[0]);
          meshData.indices.append(tri.indices[1]);
          meshData.indices.append(tri.indices[2]);
        }

        // Get texture for this material
        QImage matTex;
        QString matName =
            (matIdx >= 0 && matIdx < converter.materialNames().size())
                ? converter.materialNames()[matIdx]
                : "";

        if (!matName.isEmpty()) {
          auto mat = converter.materials()[matName];
          if (mat.hasTexture)
            matTex = mat.textureImage;
        }

        m_previewWidget->addSurface(meshData, matTex);
      }
    }
  }
}

void ObjImportDialog::browseOutput() {
  QString path = QFileDialog::getSaveFileName(this, tr("Guardar MD3"),
                                              m_outputEdit->text(),
                                              tr("Quake 3 Model (*.md3)"));
  if (!path.isEmpty()) {
    m_outputEdit->setText(path);
  }
}

void ObjImportDialog::convert() {
  QString inPath = m_inputEdit->text();
  QString outPath = m_outputEdit->text();

  if (inPath.isEmpty() || outPath.isEmpty()) {
    QMessageBox::warning(
        this, tr("Error"),
        tr("Por favor selecciona archivos de entrada y salida correctly."));
    return;
  }

  QProgressDialog progress("Iniciando conversión...", "Cancelar", 0, 100, this);
  progress.setWindowModality(Qt::WindowModal);
  progress.setMinimumDuration(0);
  progress.setValue(0);
  QApplication::processEvents();

  ObjToMd3Converter converter;
  converter.onProgress = [&](int p, QString s) {
    progress.setLabelText(s);
    progress.setValue(p);
    QApplication::processEvents();
    if (progress.wasCanceled()) {
      // Abort logic not fully implemented in converter yet
    }
  };

  bool success = false;

  if (inPath.endsWith(".glb", Qt::CaseInsensitive)) {
    success = converter.loadGlb(inPath);
  } else {
    success = converter.loadObj(inPath);
  }

  if (!success) {
    progress.close();
    QMessageBox::critical(
        this, tr("Error"),
        tr("No se pudo cargar el archivo de entrada.\nAsegúrate de que existe "
           "y es un formato válido."));
    return;
  }

  // Prepare atlas path based on INPUT file name (not output MD3)
  QFileInfo fiInput(inPath);
  QFileInfo fiOutput(outPath);
  QString atlasPath =
      fiOutput.absolutePath() + "/" + fiInput.completeBaseName() + ".png";
  bool atlasCreated = false;

  // Merge textures if multiple materials found (GLB)
  if (converter.mergeTextures(atlasPath, m_atlasSizeSpin->value())) {
    atlasCreated = true;
  }
  // Fallback to single texture baking if merge wasn't needed (1 material)
  else if (m_atlasCheck->isChecked()) {
    converter.setProgress(80, "Generando textura única...");
    if (converter.generateTextureAtlas(atlasPath, m_atlasSizeSpin->value())) {
      atlasCreated = true;
    }
  }

  converter.setProgress(90, "Guardando MD3...");
  if (!converter.saveMd3(outPath, m_scaleSpin->value(), m_rotationSpin->value(),
                         m_orientXSpin->value(), m_orientYSpin->value(),
                         m_orientZSpin->value(),
                         m_previewWidget->getCameraXRotation(),
                         m_previewWidget->getCameraYRotation())) {
    progress.close();
    QMessageBox::critical(this, tr("Error"),
                          tr("No se pudo guardar el archivo MD3."));
    return;
  }

  progress.setValue(100);
  progress.close();

  QString msg =
      tr("Conversión completada con éxito!\n") + converter.debugInfo();
  if (atlasCreated) {
    msg += "\nAtlas texture: " + fiInput.completeBaseName() + ".png";
  }
}

QString ObjImportDialog::inputPath() const { return m_inputEdit->text(); }
QString ObjImportDialog::outputPath() const { return m_outputEdit->text(); }
double ObjImportDialog::scale() const { return m_scaleSpin->value(); }
bool ObjImportDialog::generateAtlas() const {
  return m_atlasCheck->isChecked();
}
int ObjImportDialog::rotation() const { return m_rotationSpin->value(); }

void ObjImportDialog::onRotationChanged(int degrees) {
  // Update visual arrow based on rotation
  // 0° = Up (North), 90° = Left (West), 180° = Down (South), 270° = Right
  // (East)
  QString arrow;
  QString color;

  if (degrees >= 0 && degrees < 45) {
    arrow = "↑";
    color = "#2196F3"; // Blue - North
  } else if (degrees >= 45 && degrees < 135) {
    arrow = "←";
    color = "#FF9800"; // Orange - West
  } else if (degrees >= 135 && degrees < 225) {
    arrow = "↓";
    color = "#F44336"; // Red - South
  } else if (degrees >= 225 && degrees < 315) {
    arrow = "→";
    color = "#4CAF50"; // Green - East
  } else {
    arrow = "↑";
    color = "#2196F3"; // Blue - North
  }

  m_rotationPreview->setText(arrow);
  m_rotationPreview->setStyleSheet(
      QString("QLabel { font-size: 48px; color: %1; font-weight: bold; }")
          .arg(color));

  // Update 3D preview
  m_previewWidget->setRotation(degrees);
}

void ObjImportDialog::onModelOrientationChanged() {
  m_previewWidget->setModelOrientation(
      m_orientXSpin->value(), m_orientYSpin->value(), m_orientZSpin->value());
}

void ObjImportDialog::resetModelOrientation() {
  m_orientXSpin->setValue(0);
  m_orientYSpin->setValue(0);
  m_orientZSpin->setValue(0);
}
