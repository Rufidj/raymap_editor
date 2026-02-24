#ifndef FONTEDITORDIALOG_H
#define FONTEDITORDIALOG_H

#include <QDialog>
#include <QFont>
#include <QImage>
#include <QMap>

class QComboBox;
class QSpinBox;
class QPushButton;
class QLabel;
class QScrollArea;
class QCheckBox;
class QLineEdit;
class QSlider;

struct FntGlyph {
  uint32_t width;
  uint32_t height;
  uint32_t xadvance;
  uint32_t yadvance;
  uint32_t xoffset;
  uint32_t yoffset;
  uint32_t fileoffset;
  QImage image; // The pixel data for this glyph
};

class FontEditorDialog : public QDialog {
  Q_OBJECT

public:
  explicit FontEditorDialog(QWidget *parent = nullptr);
  ~FontEditorDialog();

  bool loadFont(const QString &filename);

private slots:
  void updatePreview();
  void selectColor();
  void saveFont();
  void openFont();          // Load existing FNT/FNX
  void updateTextPreview(); // Render custom text
  void updateZoom();        // New Zoom slot

private:
  void setupUi();
  void generateFontData();
  QByteArray createFntData();
  bool loadFntData(const QString &filename);
  void renderTextPreview(const QString &text);

  // UI Elements
  QComboBox *fontFamilyCombo;
  QComboBox *fontStyleCombo;
  QSpinBox *fontSizeSpin;
  QPushButton *colorBtn;
  QComboBox *bppCombo; // 8, 16, 32
  QCheckBox *antialiasCheck;
  QCheckBox *boldCheck;
  QCheckBox *italicCheck;

  // Preview Controls
  QLineEdit *testTextInput;
  QLabel *textPreviewLabel;

  QLabel *previewLabel; // Atlas preview
  QScrollArea *scrollArea;
  QSlider *zoomSlider;
  QLabel *zoomLabel;

  // Data
  QColor currentColor;
  QMap<int, FntGlyph> glyphs; // Char code -> Glyph Data
  int currentBpp;
  bool isLoadedFont; // True if we are editing/viewing a loaded file (disable
                     // generation params?)
  QImage currentAtlas;
};

#endif // FONTEDITORDIALOG_H
