#include "bennurenderer.h"
#include <QFile>
#include <QFileInfo>
#include <QDebug>
#include <zlib.h>
#include <memory>
#include <cstring>

uint32_t BennuFont::readLE32(const char* data, int &offset) {
    uint32_t val = (uint8_t)data[offset] | ((uint8_t)data[offset+1] << 8) | ((uint8_t)data[offset+2] << 16) | ((uint8_t)data[offset+3] << 24);
    offset += 4;
    return val;
}

uint16_t BennuFont::readLE16(const char* data, int &offset) {
    uint16_t val = (uint8_t)data[offset] | ((uint8_t)data[offset+1] << 8);
    offset += 2;
    return val;
}

BennuFont::BennuFont() : bpp(0) {}

bool BennuFont::load(const QString &path) {
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) return false;
    QByteArray data = file.readAll();
    file.close();

    if (data.size() < 16) return false;

    // Decompress if GZIP
    if ((uint8_t)data[0] == 0x1F && (uint8_t)data[1] == 0x8B) {
        QByteArray decompressed;
        char buffer[32768];
        z_stream strm;
        memset(&strm, 0, sizeof(strm));
        if (inflateInit2(&strm, 16 + 15) != Z_OK) return false;
        strm.avail_in = data.size();
        strm.next_in = (Bytef*)data.data();
        int ret;
        do {
            strm.avail_out = sizeof(buffer);
            strm.next_out = (Bytef*)buffer;
            ret = inflate(&strm, Z_NO_FLUSH);
            if (ret < 0 && ret != Z_BUF_ERROR) { inflateEnd(&strm); return false; }
            int have = sizeof(buffer) - strm.avail_out;
            if (have > 0) decompressed.append(buffer, have);
        } while (strm.avail_out == 0);
        inflateEnd(&strm);
        data = decompressed;
    }

    const char* ptr = data.constData();
    int offset = 0;
    if (memcmp(ptr, "fnt", 3) == 0) { bpp = 8; offset = 8; }
    else if (memcmp(ptr, "fnx", 3) == 0) { offset = 7; bpp = (uint8_t)ptr[offset++]; }
    else return false;

    QVector<QRgb> palette;
    if (bpp == 8) {
        if (data.size() < offset + 768) return false;
        for (int i = 0; i < 256; i++) {
            uint8_t r = (uint8_t)ptr[offset++];
            uint8_t g = (uint8_t)ptr[offset++];
            uint8_t b = (uint8_t)ptr[offset++];
            palette.append(qRgba(r, g, b, (i == 0 ? 0 : 255)));
        }
        // Integrity check for Gamma
        auto checkIntegrity = [&](int off) {
            if (off + 2 > data.size()) return false;
            uint32_t cs = (uint8_t)ptr[off] | (uint8_t)ptr[off+1]<<8;
            if (cs > 100) return false;
            return true;
        };
        if (!checkIntegrity(offset)) {
            if (data.size() >= offset + 576) offset += 576;
        }
    }

    offset += 4; // Skip charset
    int tableStart = offset;

    for (int i = 0; i < 256; i++) {
        BennuGlyph g;
        bool isFNX = (bpp != 8 || data.size() >= tableStart + 256 * 28);
        int entryOffset = tableStart + i * (isFNX ? 28 : 16);
        int off = entryOffset;

        if (isFNX) {
            if (off + 28 > data.size()) break;
            g.width = readLE32(ptr, off);
            g.height = readLE32(ptr, off);
            g.xadvance = readLE32(ptr, off);
            g.yadvance = readLE32(ptr, off);
            g.xoffset = readLE32(ptr, off);
            g.yoffset = readLE32(ptr, off);
            uint32_t pixelOffset = readLE32(ptr, off);
            
            if (g.width > 0 && g.height > 0 && pixelOffset < (uint32_t)data.size()) {
                g.image = QImage(g.width, g.height, QImage::Format_ARGB32);
                int po = pixelOffset;
                int bytesPerPixel = (bpp == 8 ? 1 : (bpp == 16 ? 2 : 4));
                if (po + (int)(g.width * g.height * bytesPerPixel) > data.size()) continue;

                for (uint32_t y = 0; y < g.height; y++) {
                    for (uint32_t x = 0; x < g.width; x++) {
                        if (bpp == 8) {
                            uint8_t idx = (uint8_t)data[po++];
                            g.image.setPixel(x, y, palette[idx]);
                        } else if (bpp == 16) {
                            uint16_t c16 = readLE16(ptr, po);
                            if (c16 == 0) g.image.setPixel(x, y, 0);
                            else {
                                int r = ((c16 >> 11) & 0x1F) << 3;
                                int g1 = ((c16 >> 5) & 0x3F) << 2;
                                int b = (c16 & 0x1F) << 3;
                                g.image.setPixel(x, y, qRgba(r, g1, b, 255));
                            }
                        } else if (bpp == 32) {
                            uint8_t r = (uint8_t)ptr[po++];
                            uint8_t g1 = (uint8_t)ptr[po++];
                            uint8_t b = (uint8_t)ptr[po++];
                            uint8_t a = (uint8_t)ptr[po++];
                            g.image.setPixel(x, y, qRgba(r, g1, b, a));
                        }
                    }
                }
                glyphs[i] = g;
            }
        } else {
            // Old FNT (16 bytes)
            if (tableStart + i * 16 + 16 > data.size()) break;
            int localOff = tableStart + i * 16;
            g.width = readLE32(ptr, localOff);
            g.height = readLE32(ptr, localOff);
            g.yoffset = readLE32(ptr, localOff);
            uint32_t pixelOffset = readLE32(ptr, localOff);
            g.xadvance = g.width;
            g.yadvance = g.height;
            g.xoffset = 0;
            if (g.width > 0 && g.height > 0 && pixelOffset + (g.width * g.height) <= (uint32_t)data.size()) {
                 g.image = QImage(g.width, g.height, QImage::Format_ARGB32);
                 int po = pixelOffset;
                 for (uint32_t y = 0; y < g.height; y++) {
                     for (uint32_t x = 0; x < g.width; x++) {
                         uint8_t idx = (uint8_t)data[po++];
                         g.image.setPixel(x, y, palette[idx]);
                     }
                 }
                 glyphs[i] = g;
            }
        }
    }
    return true;
}

QPixmap BennuFont::render(const QString &text) {
    if (glyphs.isEmpty()) return QPixmap();
    int totalW = 0; int maxH = 0;
    for (QChar c : text) {
        int id = c.unicode() % 256;
        if (glyphs.contains(id)) {
            totalW += glyphs[id].xadvance;
            maxH = qMax((int)glyphs[id].yadvance, maxH);
        } else totalW += 8;
    }
    if (totalW == 0) totalW = 1; if (maxH == 0) maxH = 1;
    QImage img(totalW, maxH, QImage::Format_ARGB32);
    img.fill(Qt::transparent);
    QPainter p(&img);
    int cx = 0;
    for (QChar c : text) {
        int id = c.unicode() % 256;
        if (glyphs.contains(id)) {
            p.drawImage(cx + glyphs[id].xoffset, glyphs[id].yoffset, glyphs[id].image);
            cx += glyphs[id].xadvance;
        } else cx += 8;
    }
    p.end();
    return QPixmap::fromImage(img);
}

BennuFontManager& BennuFontManager::instance() { static BennuFontManager inst; return inst; }
BennuFontManager::BennuFontManager() {}
QPixmap BennuFontManager::renderText(const QString &text, const QString &fontPath) {
    if (fontPath.isEmpty()) return QPixmap();
    if (!m_cache.contains(fontPath)) {
        BennuFont *f = new BennuFont();
        if (f->load(fontPath)) m_cache[fontPath] = f;
        else { delete f; return QPixmap(); }
    }
    return m_cache[fontPath]->render(text);
}
void BennuFontManager::clearCache() { qDeleteAll(m_cache); m_cache.clear(); }
