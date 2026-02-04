#ifndef OBJIMPORTDIALOG_H
#define OBJIMPORTDIALOG_H

#include <QDialog>
#include <QLineEdit>
#include <QDoubleSpinBox>
#include <QSpinBox>
#include <QCheckBox>
#include <QLabel>
#include "modelpreviewwidget.h"

class ObjImportDialog : public QDialog
{
    Q_OBJECT

public:
    explicit ObjImportDialog(QWidget *parent = nullptr);
    
    QString inputPath() const;
    QString outputPath() const;
    double scale() const;
    bool generateAtlas() const;
    int rotation() const;  // Rotation in degrees (0-360)

private slots:
    void browseInput();
    void browseOutput();
    void convert();
    void onRotationChanged(int degrees);
    void onModelOrientationChanged();
    void resetModelOrientation();

private:
    QLineEdit *m_inputEdit;
    QLineEdit *m_outputEdit;
    QDoubleSpinBox *m_scaleSpin;
    QCheckBox *m_atlasCheck;
    QSpinBox *m_atlasSizeSpin;
    QSpinBox *m_rotationSpin;
    QLabel *m_rotationPreview;  // Visual arrow indicator
    ModelPreviewWidget *m_previewWidget;  // 3D model preview
    
    // Model orientation controls (for fixing vertical models, etc.)
    QSpinBox *m_orientXSpin;
    QSpinBox *m_orientYSpin;
    QSpinBox *m_orientZSpin;
};

#endif // OBJIMPORTDIALOG_H
