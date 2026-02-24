#include "fonteditordialog.h"

#include <QCheckBox>
#include <QColorDialog>
#include <QComboBox>
#include <QDataStream>
#include <QDebug>
#include <QFileDialog>
#include <QFontDatabase>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QMessageBox>
#include <QPainter>
#include <QPushButton>
#include <QScrollArea>
#include <QSpinBox>
#include <QVBoxLayout>

#include <QBuffer>
#include <QGroupBox>
#include <QLineEdit>
#include <QSlider>
#include <cstring>
#include <zlib.h>

// Little Endian Helper
static void writeLE32(QByteArray &data, uint32_t value) {
  data.append((char)(value & 0xFF));
  data.append((char)((value >> 8) & 0xFF));
  data.append((char)((value >> 16) & 0xFF));
  data.append((char)((value >> 24) & 0xFF));
}

static void writeLE16(QByteArray &data, uint16_t value) {
  data.append((char)(value & 0xFF));
  data.append((char)((value >> 8) & 0xFF));
}

// Reading Helpers
static uint32_t readLE32(const char *data, int &offset) {
  uint32_t val = (uint8_t)data[offset] | ((uint8_t)data[offset + 1] << 8) |
                 ((uint8_t)data[offset + 2] << 16) |
                 ((uint8_t)data[offset + 3] << 24);
  offset += 4;
  return val;
}

FontEditorDialog::FontEditorDialog(QWidget *parent)
    : QDialog(parent), currentColor(Qt::white), currentBpp(32),
      isLoadedFont(false) {
  setupUi();
  setWindowTitle("Generador FNT"); // User requested change
  resize(900, 700);

  // Initial update
  updatePreview();
}

FontEditorDialog::~FontEditorDialog() {}

bool FontEditorDialog::loadFont(const QString &filename) {
  if (loadFntData(filename)) {
    updatePreview();
    return true;
  }
  return false;
}

void FontEditorDialog::setupUi() {
  QVBoxLayout *mainLayout = new QVBoxLayout(this);

  // Controls Area
  QHBoxLayout *controlsLayout = new QHBoxLayout();

  // Col 1: Font Selection
  QVBoxLayout *col1 = new QVBoxLayout();
  fontFamilyCombo = new QComboBox();
  QFontDatabase db;
  fontFamilyCombo->addItems(db.families());
  // Select standard font if available
  int idx = fontFamilyCombo->findText("Arial");
  if (idx >= 0)
    fontFamilyCombo->setCurrentIndex(idx);

  col1->addWidget(new QLabel("Fuente Sistema:"));
  col1->addWidget(fontFamilyCombo);

  // Col 2: Size & Style
  QVBoxLayout *col2 = new QVBoxLayout();
  fontSizeSpin = new QSpinBox();
  fontSizeSpin->setRange(4, 256);
  fontSizeSpin->setValue(24);

  boldCheck = new QCheckBox("Bold");
  italicCheck = new QCheckBox("Italic");
  antialiasCheck = new QCheckBox("Anti-aliasing");
  antialiasCheck->setChecked(true);

  col2->addWidget(new QLabel("Tamaño:"));
  col2->addWidget(fontSizeSpin);
  col2->addWidget(boldCheck);
  col2->addWidget(italicCheck);
  col2->addWidget(antialiasCheck);

  // Col 3: Color & Format
  QVBoxLayout *col3 = new QVBoxLayout();
  colorBtn = new QPushButton("Color Fuente");
  QPalette pal = colorBtn->palette();
  pal.setColor(QPalette::Button, currentColor);
  colorBtn->setAutoFillBackground(true);
  colorBtn->setPalette(pal);

  bppCombo = new QComboBox();
  bppCombo->addItem("32-bit (RGBA, FNX)", 32);
  bppCombo->addItem("16-bit (RGB565, FNX)", 16);
  bppCombo->addItem("8-bit (Palette, FNT)", 8);

  col3->addWidget(colorBtn);
  col3->addWidget(new QLabel("Formato Salida:"));
  col3->addWidget(bppCombo);

  controlsLayout->addLayout(col1);
  controlsLayout->addLayout(col2);
  controlsLayout->addLayout(col3);

  // Buttons
  QVBoxLayout *btnLayout = new QVBoxLayout();
  QPushButton *loadBtn = new QPushButton("Abrir FNT/FNX...");
  QPushButton *previewBtn = new QPushButton("Actualizar Vista");
  QPushButton *saveBtn = new QPushButton("Guardar FNT/FNX...");

  btnLayout->addWidget(loadBtn);
  btnLayout->addWidget(previewBtn);
  btnLayout->addWidget(saveBtn);
  btnLayout->addStretch();
  controlsLayout->addLayout(btnLayout);

  mainLayout->addLayout(controlsLayout);

  // --- Text Preview Area ---
  QGroupBox *testGroup = new QGroupBox("Prueba de Texto");
  QVBoxLayout *testLayout =
      new QVBoxLayout(); // Removed const QObject* parent in constructor? No,
                         // QVBoxLayout(QWidget*)

  testTextInput = new QLineEdit("Hola Mundo 123");
  testTextInput->setPlaceholderText("Escribe aquí para probar la fuente...");

  textPreviewLabel = new QLabel();
  textPreviewLabel->setMinimumHeight(64);
  textPreviewLabel->setAlignment(Qt::AlignCenter);
  textPreviewLabel->setStyleSheet(
      "background-color: #202020; border: 1px solid #404040;");

  testLayout->addWidget(testTextInput);
  testLayout->addWidget(textPreviewLabel);
  testGroup->setLayout(testLayout);

  mainLayout->addWidget(testGroup);

  // --- Atlas Preview ---
  QVBoxLayout *atlasLayout = new QVBoxLayout();

  QHBoxLayout *zoomLayout = new QHBoxLayout();
  zoomLayout->addWidget(new QLabel("Zoom:"));
  zoomSlider = new QSlider(Qt::Horizontal);
  zoomSlider->setRange(10, 800);
  zoomSlider->setValue(100);
  zoomLabel = new QLabel("100%");
  zoomLayout->addWidget(zoomSlider);
  zoomLayout->addWidget(zoomLabel);

  atlasLayout->addLayout(zoomLayout);
  atlasLayout->addWidget(new QLabel("Mapa de Caracteres (Atlas):"));

  scrollArea = new QScrollArea();
  scrollArea->setBackgroundRole(QPalette::Dark);
  scrollArea->setStyleSheet(
      "QScrollArea { background-color: #202020; border: 1px solid #404040; }");
  // Prevent ScrollArea from growing infinitely with content
  scrollArea->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
  scrollArea->setMinimumSize(400, 300);

  previewLabel = new QLabel();
  previewLabel->setAlignment(Qt::AlignTop | Qt::AlignLeft);
  previewLabel->setStyleSheet(
      "background-color: transparent;"); // Transparent so it sits on ScrollArea
                                         // dark bg

  scrollArea->setWidget(previewLabel);
  scrollArea->setWidgetResizable(true);

  atlasLayout->addWidget(scrollArea);
  mainLayout->addLayout(atlasLayout);

  // Connections
  connect(previewBtn, &QPushButton::clicked, this,
          &FontEditorDialog::updatePreview);
  connect(saveBtn, &QPushButton::clicked, this, &FontEditorDialog::saveFont);
  connect(loadBtn, &QPushButton::clicked, this, &FontEditorDialog::openFont);
  connect(colorBtn, &QPushButton::clicked, this,
          &FontEditorDialog::selectColor);
  connect(zoomSlider, &QSlider::valueChanged, this,
          &FontEditorDialog::updateZoom);

  // Auto-update connections
  connect(fontFamilyCombo, SIGNAL(currentIndexChanged(int)), this,
          SLOT(updatePreview()));
  connect(fontSizeSpin, SIGNAL(valueChanged(int)), this, SLOT(updatePreview()));
  connect(boldCheck, &QCheckBox::toggled, this,
          &FontEditorDialog::updatePreview);
  connect(italicCheck, &QCheckBox::toggled, this,
          &FontEditorDialog::updatePreview);
  connect(antialiasCheck, &QCheckBox::toggled, this,
          &FontEditorDialog::updatePreview);
  connect(bppCombo, SIGNAL(currentIndexChanged(int)), this,
          SLOT(updatePreview()));

  // Text preview update
  connect(testTextInput, &QLineEdit::textChanged, this,
          &FontEditorDialog::updateTextPreview);
}

void FontEditorDialog::selectColor() {
  QColor c = QColorDialog::getColor(currentColor, this, "Select Font Color");
  if (c.isValid()) {
    currentColor = c;
    // Update button style
    QString qss = QString("background-color: %1").arg(c.name());
    colorBtn->setStyleSheet(qss);
    updatePreview();
  }
}

void FontEditorDialog::updatePreview() {
  if (!isLoadedFont) {
    generateFontData();
  }

  // update label text based on mode
  if (isLoadedFont) {
    // Maybe change title to indicate loaded file?
  }

  // Show an atlas of generated glyphs in the preview label
  // We'll organize them in a grid
  if (glyphs.isEmpty()) {
    previewLabel->clear();
    return;
  }

  // Intelligent Packing (Flow Layout)
  int atlasTargetW = 1024; // Reasonable width

  // We need to calculate positions first to determine total height
  struct PackNode {
    int x, y;
    int id;
  };
  QMap<int, PackNode> layout;

  int cx = 2;
  int cy = 2;
  int rowH = 0;

  for (int i = 0; i < 256; ++i) {
    if (!glyphs.contains(i))
      continue;
    const FntGlyph &g = glyphs[i];

    int gw = g.width + 4; // Padding
    int gh = g.height + 4;

    if (cx + gw > atlasTargetW) {
      // New Row
      cy += rowH;
      cx = 2;
      rowH = 0;
    }

    layout[i] = {cx, cy, i};

    cx += gw;
    if (gh > rowH)
      rowH = gh;
  }

  int atlasTotalW = atlasTargetW;
  int atlasTotalH = cy + rowH + 2;

  // Create Atlas Image
  QImage atlas(atlasTotalW, atlasTotalH, QImage::Format_ARGB32);
  atlas.fill(Qt::black);

  QPainter p(&atlas);

  for (auto it = layout.begin(); it != layout.end(); ++it) {
    int id = it.key();
    PackNode node = it.value();
    const FntGlyph &g = glyphs[id];

    // Draw image
    // Node x,y includes padding/2 ?
    // Logic above: cx starts at 2. gw includes 4 padding.
    // So let's draw at x+2, y+2 to center in padding?
    // Or just x,y.
    int x = node.x;
    int y = node.y;

    p.drawImage(x, y, g.image);
    p.setPen(Qt::darkGray);
    p.drawRect(x - 1, y - 1, g.width + 1, g.height + 1);

    // Optional: Draw Char Code?
    if (g.height > 10) {
      p.setPen(Qt::yellow);
      // p.drawText(x, y, QString::number(id));
    }
  }
  p.end();

  currentAtlas = atlas;
  updateZoom();

  // Update Text Preview as well
  updateTextPreview();
}

void FontEditorDialog::generateFontData() {
  isLoadedFont = false; // We are generating new data
  glyphs.clear();
  currentBpp = bppCombo->currentData().toInt();

  QString family = fontFamilyCombo->currentText();
  int size = fontSizeSpin->value();

  QFont font(family, size);
  font.setBold(boldCheck->isChecked());
  font.setItalic(italicCheck->isChecked());

  // For proper rendering we might need font metrics
  // But QPainter draws text fine.

  // We render range 0-255 (ISO-8859-1 coverage usually)
  // Note: char 0-31 are control chars, usually empty, but Bennu might use them.
  // We'll generate rendering for 32-255, and maybe placeholders for others.

  QFontMetrics fm(font);

  for (int i = 32; i < 256; ++i) { // Start at 32 (Space)

    QChar c(i);
    QString text = QString(c);

    // Calculate Bounding Rect
    QRect boundingRect = fm.boundingRect(text);
    int w = boundingRect.width();
    int h = boundingRect.height();

    if (i == 32) { // Space
      w = fm.horizontalAdvance(text);
      h = fm.height();
      if (w <= 0)
        w = size / 3;
    }

    if (w <= 0)
      w = size / 2; // Fallback
    if (h <= 0)
      h = fm.height();

    // Ensure size
    if (w < 1)
      w = 1;
    if (h < 1)
      h = 1;

    // Create GLYPH Image
    // Use Format_ARGB32_Premultiplied for best rendering quality then convert
    QImage img(w, h, QImage::Format_ARGB32_Premultiplied);
    img.fill(Qt::transparent);

    QPainter p(&img);
    p.setFont(font);
    p.setPen(currentColor);
    p.setRenderHint(QPainter::TextAntialiasing, antialiasCheck->isChecked());

    // Draw centered and aligned?
    // boundingRect.left() can be negative (bearing).
    // We need to draw at -boundingRect.left() to fit in [0, w]
    int drawX = -boundingRect.left();
    int drawY = -boundingRect.top();

    if (i == 32) { // Space, empty draw
    } else {
      p.drawText(drawX, drawY, text);
    }
    p.end();

    FntGlyph g;
    g.width = w;
    g.height = h;
    g.xadvance = fm.horizontalAdvance(text);
    g.yadvance = fm.height();
    g.xoffset = 0; // Usually 0 for basic generation
    g.yoffset = 0;
    g.image = img;

    glyphs.insert(i, g);
  }
}

void FontEditorDialog::saveFont() {
  QString ext = (currentBpp == 8) ? ".fnt" : ".fnx";
  QString filter = QString("BennuGD Font (*%1)").arg(ext);

  QString filename =
      QFileDialog::getSaveFileName(this, "Save Font", "", filter);
  if (filename.isEmpty())
    return;

  // Fix extension logic
  QFileInfo fi(filename);
  QString currentExt = "." + fi.suffix();

  if (currentExt.compare(ext, Qt::CaseInsensitive) != 0) {
    // If it has the WRONG font extension, replace it
    if (currentExt.compare(".fnt", Qt::CaseInsensitive) == 0 ||
        currentExt.compare(".fnx", Qt::CaseInsensitive) == 0) {
      filename = filename.left(filename.length() - 4) + ext;
    } else {
      // No extension or unrelated -> Append correct one
      filename += ext;
    }
  }

  QByteArray data = createFntData();

  QFile file(filename);
  if (!file.open(QIODevice::WriteOnly)) {
    QMessageBox::critical(this, "Error",
                          "No se pudo abrir el archivo para escritura.");
    return;
  }

  file.write(data);
  file.close();

  QMessageBox::information(this, "Éxito", "¡Fuente guardada correctamente!");
}

QByteArray FontEditorDialog::createFntData() {
  QByteArray data;

  // 1. Header
  if (currentBpp == 8) {
    // "fnt\x1A\x0D\x0A\x00\x00" (8 bytes)
    const char magic[] = {'f', 'n', 't', 0x1A, 0x0D, 0x0A, 0x00, 0x00};
    data.append(magic, 8);
  } else {
    // "fnx\x1A\x0D\x0A\x00" + bpp
    const char magic[] = {'f', 'n', 'x', 0x1A, 0x0D, 0x0A, 0x00};
    data.append(magic, 7);
    data.append((char)currentBpp);
  }

  // 2. Palette (Only for 8bpp)
  if (currentBpp == 8) {
    // Generate Palette (Application dependent)
    // Usually index 0 is transparent.
    // We will make a gradient palette from index 1 to 255 based on chosen
    // color. This allows AA.

    QByteArray palData;

    // Entry 0: Black/Transparent
    palData.append((char)0);
    palData.append((char)0);
    palData.append((char)0);

    int r = currentColor.red();
    int g = currentColor.green();
    int b = currentColor.blue();

    for (int i = 1; i < 256; i++) {
      // Simple opacity mapping: i/255 -> alpha
      // Bennu 8bit usually doesn't have per-pixel alpha mixing with background
      // unless using special blit flags. But if we want simple anti-aliasing
      // against black background: Color = BaseColor * (i/255).

      palData.append((char)((r * i) / 255));
      palData.append((char)((g * i) / 255));
      palData.append((char)((b * i) / 255));
    }

    data.append(palData);

    // Gamma (576 zeros)
    QByteArray gamma(576, 0);
    data.append(gamma);
  }

  // 3. Charset (4 bytes)
  // 0 = ISO8859
  writeLE32(data, 0);

  // 4. Character Table (256 * 28 bytes)
  // We need to calculate file offsets first.
  // The pixel data starts immediately after this table.

  int tableStart = data.size();
  int tableSize = 256 * 28;
  int pixelDataStart = tableStart + tableSize;

  int currentPixelOffset = pixelDataStart;

  // Temporary buffer for Table
  QByteArray tableData;

  // We also need to build Pixel Data Blob simultaneously to know offsets
  QByteArray pixelDataBlob;

  for (int i = 0; i < 256; i++) {
    if (!glyphs.contains(i)) {
      // Empty char
      for (int k = 0; k < 7; k++)
        writeLE32(tableData, 0);
      continue;
    }

    const FntGlyph &g = glyphs[i];

    writeLE32(tableData, g.width);
    writeLE32(tableData, g.height);
    writeLE32(tableData, g.xadvance);
    writeLE32(tableData, g.yadvance);
    writeLE32(tableData, g.xoffset); // Usually 0
    writeLE32(tableData, g.yoffset);
    writeLE32(tableData, currentPixelOffset);

    // Process Image Pixels
    int bytesPerPixel = currentBpp / 8;
    int glyphSize = g.width * g.height * bytesPerPixel;

    // Scanlines
    for (uint32_t y = 0; y < g.height; y++) {
      for (uint32_t x = 0; x < g.width; x++) {
        QRgb p = g.image.pixel(x, y);
        int alpha = qAlpha(p);

        if (currentBpp == 8) {
          if (alpha < 10) {
            pixelDataBlob.append((char)0); // Transparent
          } else {
            // Map alpha to palette index (1-255)
            // Or map luminance? Assuming single color font.
            // We use alpha mapping as defined in palette generation.
            int idx = (alpha * 255) / 255;
            if (idx < 1)
              idx = 1;
            if (idx > 255)
              idx = 255;
            pixelDataBlob.append((char)idx);
          }
        } else if (currentBpp == 16) {
          // RGB565
          // Bennu 16bit transparency is keycolor 0 (Black).
          // If pixel is transparent, write 0.
          // If pixel is NOT transparent but black... conflict.
          // We assume font color is not black.

          uint16_t color565 = 0;
          if (alpha > 10) {
            int R = qRed(p);
            int G = qGreen(p);
            int B = qBlue(p);

            // RGB565: R5, G6, B5
            color565 = ((R >> 3) << 11) | ((G >> 2) << 5) | (B >> 3);

            if (color565 == 0)
              color565 = 1; // Avoid transparent key
          }
          writeLE16(pixelDataBlob, color565);
        } else if (currentBpp == 32) {
          // RGBA (Little Endian -> A B G R ?)
          // Standard RGBA or ARGB?
          // Bennu usually uses RGBA8888 or ARGB8888 depending on platform.
          // Assuming RGBA (R low, A high).

          uint8_t R = qRed(p);
          uint8_t G = qGreen(p);
          uint8_t B = qBlue(p);
          uint8_t A = qAlpha(p);

          pixelDataBlob.append((char)R);
          pixelDataBlob.append((char)G);
          pixelDataBlob.append((char)B);
          pixelDataBlob.append((char)A);
        }
      }
    }

    currentPixelOffset += glyphSize;
  }

  data.append(tableData);
  data.append(pixelDataBlob);

  return data;
}

void FontEditorDialog::openFont() {
  QString filename = QFileDialog::getOpenFileName(this, "Abrir Fuente", "",
                                                  "BennuGD Font (*.fnt *.fnx)");
  if (filename.isEmpty())
    return;

  if (loadFntData(filename)) {
    updatePreview();
    QMessageBox::information(this, "Éxito", "Fuente cargada correctamente.");
  } else {
    QMessageBox::critical(
        this, "Error",
        "No se pudo cargar la fuente. Formato desconocido o archivo dañado.");
  }
}

static uint32_t peekLE32(const char *data, int offset) {
  // Check bounds? Caller must ensure
  return (uint8_t)data[offset] | ((uint8_t)data[offset + 1] << 8) |
         ((uint8_t)data[offset + 2] << 16) | ((uint8_t)data[offset + 3] << 24);
}

// Returns score: -1 (Invalid), >= 0 (Number of valid chars)
static int checkFntIntegrity(const char *ptr, int offset, int dataSize) {
  if (offset + 4 + (256 * 28) > dataSize)
    return -1;

  uint32_t charset = peekLE32(ptr, offset);
  if (charset > 100)
    return -1; // Charset is usually 0.

  int tableStart = offset + 4;
  int tableEnd = tableStart + (256 * 28);
  int currentPos = tableStart;
  int validChars = 0;

  for (int i = 0; i < 256; i++) {
    uint32_t w = peekLE32(ptr, currentPos);
    uint32_t h = peekLE32(ptr, currentPos + 4);
    uint32_t fo = peekLE32(ptr, currentPos + 24);

    currentPos += 28;

    if (w == 0 && h == 0)
      continue;

    // Sanity checks
    if (w > 512 || h > 512)
      return -1; // Limit for integrity check
    if (fo < (uint32_t)tableEnd || fo >= (uint32_t)dataSize) {
      return -1;
    }
    validChars++;
  }
  return validChars;
}

bool FontEditorDialog::loadFntData(const QString &filename) {
  QFile file(filename);
  if (!file.open(QIODevice::ReadOnly)) {
    qDebug() << "FontEditor: Error opening file" << filename;
    return false;
  }

  QByteArray data = file.readAll();
  file.close();

  if (data.size() < 16) {
    qDebug() << "FontEditor: File too small" << data.size();
    return false;
  }

  // Check for GZIP signature (1F 8B)
  if ((uint8_t)data[0] == 0x1F && (uint8_t)data[1] == 0x8B) {
    qDebug() << "FontEditor: Detected GZIP compression. Inflating...";

    QByteArray decompressed;
    char buffer[32768];

    z_stream strm;
    strm.zalloc = Z_NULL;
    strm.zfree = Z_NULL;
    strm.opaque = Z_NULL;
    strm.avail_in = data.size();
    strm.next_in = (Bytef *)data.data();

    // 32 + 15 = 47 to enable gzip decoding + automatic header detection
    if (inflateInit2(&strm, 16 + 15) != Z_OK) {
      qDebug() << "FontEditor: inflateInit2 failed";
      return false;
    }

    int ret;
    do {
      strm.avail_out = sizeof(buffer);
      strm.next_out = (Bytef *)buffer;
      ret = inflate(&strm, Z_NO_FLUSH);

      if (ret == Z_STREAM_ERROR) {
        qDebug() << "FontEditor: Z_STREAM_ERROR";
        inflateEnd(&strm);
        return false;
      }

      int have = sizeof(buffer) - strm.avail_out;
      if (have > 0)
        decompressed.append(buffer, have);

      switch (ret) {
      case Z_NEED_DICT:
        ret = Z_DATA_ERROR; /* and fall through */
      case Z_DATA_ERROR:
      case Z_MEM_ERROR:
        qDebug() << "FontEditor: inflate error" << ret;
        inflateEnd(&strm);
        return false;
      }

    } while (strm.avail_out == 0);

    inflateEnd(&strm);

    data = decompressed; // Swap content
    qDebug() << "FontEditor: Decompressed size:" << data.size();

    if (data.size() < 16) {
      qDebug() << "FontEditor: Decompressed data too small";
      return false;
    }
  }

  const char *ptr = data.constData();
  int offset = 0;

  // Header
  // "fnt..." or "fnx..."
  int loadedBpp = 0;

  // Debug header
  QByteArray headerSnippet = data.left(8);
  qDebug() << "FontEditor: Header hex:" << headerSnippet.toHex();

  if (memcmp(ptr, "fnt", 3) == 0) {
    loadedBpp = 8;
    offset = 8; // magic+00
    qDebug() << "FontEditor: Detected FNT (8bpp)";
  } else if (memcmp(ptr, "fnx", 3) == 0) {
    offset = 7;
    loadedBpp = (uint8_t)ptr[offset];
    offset++;
    qDebug() << "FontEditor: Detected FNX with BPP:" << loadedBpp;
  } else {
    qDebug() << "FontEditor: Unknown magic header";
    return false;
  }

  // Palette
  QVector<QRgb> palette;
  if (loadedBpp == 8) {
    // 768 bytes palette + 576 gamma
    if (data.size() < offset + 768 + 576) {
      qDebug() << "FontEditor: File too small for palette. Size:" << data.size()
               << "Needed:" << (offset + 768 + 576);
      return false;
    }

    // Skip entry 0 (transparent) - Bennu logic might vary, but usually
    // palette[0] is transparent.
    palette.append(qRgba(0, 0, 0, 0));

    // Read 1..255
    for (int i = 1; i < 256; i++) {
      uint8_t r = (uint8_t)ptr[offset++];
      uint8_t g = (uint8_t)ptr[offset++];
      uint8_t b = (uint8_t)ptr[offset++];
      // Force Alpha 255 for non-0
      palette.append(qRgba(r, g, b, 255));
    }
    // Wait, standard palette is 768 bytes = 256 * 3.
    // My previous logic had rewind which was confusing.
    // Let's stick to simple sequential read.
    // The block is 768 bytes.
    // 256 colors * 3 bytes (RGB).
    // I read 255 * 3 = 765 bytes above.
    // I missed the last color (255) or the first (0)?

    // Let's re-do palette reading simply.
    // Reset offset to palette start
    offset -= (255 * 3);

    palette.clear();
    for (int i = 0; i < 256; i++) {
      uint8_t r = (uint8_t)ptr[offset++];
      uint8_t g = (uint8_t)ptr[offset++];
      uint8_t b = (uint8_t)ptr[offset++];
      // Index 0 transparent
      int a = (i == 0) ? 0 : 255;
      palette.append(qRgba(r, g, b, a));
    }

    // AUTO-DETECT GAMMA INTELLIGENTLY
    int offsetNoGamma = offset;
    int offsetGamma = offset + 576;

    qDebug() << "FontEditor: Testing integrity at NoGamma:" << offsetNoGamma
             << " vs Gamma:" << offsetGamma;

    int scoreNoGamma = checkFntIntegrity(ptr, offsetNoGamma, data.size());
    int scoreGamma = checkFntIntegrity(ptr, offsetGamma, data.size());

    qDebug() << "FontEditor: Score NoGamma:" << scoreNoGamma
             << " Score Gamma:" << scoreGamma;

    if (scoreNoGamma == -1 && scoreGamma == -1) {
      qDebug() << "FontEditor: Both formats fail integrity check! Trying "
                  "fallback (Gamma)...";
      offset += 576;
    } else if (scoreNoGamma > scoreGamma) {
      // No Gamma wins
      qDebug() << "FontEditor: WINNER -> NO GAMMA";
      // offset is already correct
    } else {
      // Gamma wins (or both equal, prefer standard)
      qDebug() << "FontEditor: WINNER -> GAMMA";
      offset += 576;
    }
  }

  // Charset/Types
  readLE32(ptr, offset); // Skip Types/Charset
  qDebug() << "FontEditor: Charset read. Table starts at:" << offset;

  // Chardata Table
  // FNT (Old): 16 bytes (w, h, yo, fo)
  // FNX (New): 28 bytes (w, h, xa, ya, xo, yo, fo)

  bool isFnx = (memcmp(data.constData(), "fnx", 3) == 0);

  struct DiskChar {
    uint32_t w, h, xa, ya, xo, yo, fo;
  };
  QVector<DiskChar> chars;

  for (int i = 0; i < 256; i++) {
    DiskChar d;
    if (isFnx) {
      d.w = readLE32(ptr, offset);
      d.h = readLE32(ptr, offset);
      d.xa = readLE32(ptr, offset);
      d.ya = readLE32(ptr, offset);
      d.xo = readLE32(ptr, offset);
      d.yo = readLE32(ptr, offset);
      d.fo = readLE32(ptr, offset);
    } else {
      // FNT Format
      d.w = readLE32(ptr, offset);
      d.h = readLE32(ptr, offset);
      uint32_t yo = readLE32(ptr, offset);
      d.fo = readLE32(ptr, offset);

      // Map to new
      d.xo = 0;
      d.yo = yo;
      d.xa = d.w;
      d.ya = d.h + d.yo; // Approximation usually
    }
    chars.append(d);
  }

  qDebug() << "FontEditor: Char table read. Last char fo:" << chars.last().fo;

  // Pixel Data
  glyphs.clear();

  for (int i = 0; i < 256; i++) {
    DiskChar d = chars[i];
    if (d.w == 0 || d.h == 0)
      continue;

    if (d.w > 512 || d.h > 512) {
      qDebug() << "FontEditor: Char" << i << "too large" << d.w << "x" << d.h;
      continue; // Skip invalid char
    }

    if ((int)d.fo < 0 || (int)d.fo >= data.size())
      continue;

    QImage img(d.w, d.h, QImage::Format_ARGB32);
    img.fill(Qt::transparent);

    int pOffset = d.fo;

    for (uint32_t y = 0; y < d.h; y++) {
      for (uint32_t x = 0; x < d.w; x++) {
        if (pOffset >= data.size())
          break;

        if (loadedBpp == 8) {
          uint8_t idx = (uint8_t)ptr[pOffset++];
          if (idx < palette.size())
            img.setPixel(x, y, palette[idx]);
        } else if (loadedBpp == 16) {
          uint16_t col =
              (uint8_t)ptr[pOffset] | ((uint8_t)ptr[pOffset + 1] << 8);
          pOffset += 2;

          if (col == 0) {
            img.setPixel(x, y, Qt::transparent);
          } else {
            // R5 G6 B5
            int r = ((col >> 11) & 0x1F) * 255 / 31;
            int g = ((col >> 5) & 0x3F) * 255 / 63;
            int b = (col & 0x1F) * 255 / 31;
            img.setPixel(x, y, qRgb(r, g, b));
          }
        } else if (loadedBpp == 32) {
          // RGBA
          uint8_t r = (uint8_t)ptr[pOffset++];
          uint8_t g = (uint8_t)ptr[pOffset++];
          uint8_t b = (uint8_t)ptr[pOffset++];
          uint8_t a = (uint8_t)ptr[pOffset++];
          img.setPixel(x, y, qRgba(r, g, b, a));
        }
      }
    }

    FntGlyph g;
    g.width = d.w;
    g.height = d.h;
    g.xadvance = d.xa;
    g.yadvance = d.ya;
    g.image = img;
    glyphs.insert(i, g);
  }

  bppCombo->blockSignals(true);
  int idx = bppCombo->findData(loadedBpp);
  if (idx >= 0)
    bppCombo->setCurrentIndex(idx);
  bppCombo->blockSignals(false);

  currentBpp = loadedBpp;
  isLoadedFont = true;

  return true;
}

void FontEditorDialog::updateZoom() {
  if (currentAtlas.isNull()) {
    previewLabel->clear();
    return;
  }

  int val = zoomSlider->value();
  zoomLabel->setText(QString("%1%").arg(val));

  double scale = val / 100.0;
  QSize sz = currentAtlas.size() * scale;
  if (sz.isEmpty())
    sz = QSize(1, 1);

  Qt::TransformationMode mode =
      (scale >= 1.0) ? Qt::FastTransformation : Qt::SmoothTransformation;

  previewLabel->setPixmap(
      QPixmap::fromImage(currentAtlas.scaled(sz, Qt::IgnoreAspectRatio, mode)));
}

void FontEditorDialog::updateTextPreview() {
  QString text = testTextInput->text();
  renderTextPreview(text);
}

void FontEditorDialog::renderTextPreview(const QString &text) {
  if (glyphs.isEmpty()) {
    textPreviewLabel->clear();
    return;
  }

  // Calculate size
  int totalW = 0;
  int maxH = 0;

  // First pass measure
  for (QChar c : text) {
    int code = c.toLatin1();
    if (glyphs.contains(code)) {
      const FntGlyph &g = glyphs[code];
      totalW += g.xadvance;
      if ((int)g.height > maxH)
        maxH = g.height;
    } else {
      totalW += 10; // Space for missing
    }
  }

  if (totalW == 0)
    totalW = 10;
  if (maxH == 0)
    maxH = 20;

  totalW += 20; // Padding
  maxH += 10;

  QImage preview(totalW, maxH, QImage::Format_ARGB32);
  preview.fill(Qt::transparent); // Transparent logic? Or dark bg?
  // User requested dark bg in setupUi label style, so transparent icon on top
  // is fine. Or we can fill with transparent and let QLabel style show through?
  // QLabel setPixmap replaces content.
  // Let's fill with transparent.

  QPainter p(&preview);

  int currentX = 10;
  int baselineY = maxH - 5; // Bottom align? Or top align?
  // Fonts are usually top align or baseline.
  // Bennu fonts don't have baseline info in FNT struct (only yoffset).
  // Let's align top for now.

  int drawY = 5;

  for (QChar c : text) {
    int code = c.toLatin1();
    // Handle extended ascii? qString is unicode.
    // map unicode to latin1 if possible?
    // simple cast for now.

    if (glyphs.contains(code)) {
      const FntGlyph &g = glyphs[code];
      p.drawImage(currentX, drawY, g.image);
      currentX += g.xadvance;
    } else {
      currentX += 10;
    }
  }

  textPreviewLabel->setPixmap(QPixmap::fromImage(preview));
}
