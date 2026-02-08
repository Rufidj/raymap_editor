#ifndef BENNURENDERER_H
#define BENNURENDERER_H

#include <QImage>
#include <QMap>
#include <QString>
#include <QPixmap>
#include <QPainter>

struct BennuGlyph {
    uint32_t width;
    uint32_t height;
    uint32_t xadvance;
    uint32_t yadvance;
    uint32_t xoffset;
    uint32_t yoffset;
    QImage image;
};

class BennuFont {
public:
    BennuFont();
    bool load(const QString &path);
    QPixmap render(const QString &text);
    bool isValid() const { return !glyphs.isEmpty(); }

private:
    QMap<int, BennuGlyph> glyphs;
    int bpp;
    
    // Internal helpers from FontEditorDialog logic
    static uint32_t readLE32(const char* data, int &offset);
    static uint16_t readLE16(const char* data, int &offset);
};

class BennuFontManager {
public:
    static BennuFontManager& instance();
    QPixmap renderText(const QString &text, const QString &fontPath);
    void clearCache();

private:
    BennuFontManager();
    QMap<QString, BennuFont*> m_cache;
};

#endif
