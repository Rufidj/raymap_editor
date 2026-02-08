#ifndef SCENEEDITOR_H
#define SCENEEDITOR_H

#include <QGraphicsView>
#include <QGraphicsScene>
#include <QGraphicsItem>
#include <QGraphicsPixmapItem>
#include <QJsonObject>
#include <QJsonArray>
#include <QJsonDocument>
#include <QFile>
#include <QMap>
#include <QList>
#include <QTreeWidget>

// Forward declaration
class SceneEntityItem;

enum SceneEntityType {
    ENTITY_SPRITE = 0,
    ENTITY_TEXT = 1,
    ENTITY_PRIMITIVE = 2, // Future: Rectangles/Lines
    ENTITY_WORLD3D = 3     // 3D Raycasting World
};

// Estructura para describir una entidad en la escena
struct SceneEntity {
    SceneEntityType type = ENTITY_SPRITE;
    QString name;       // Unique ID/Name
    
    // Transform
    double x = 0;
    double y = 0;
    int z = 0;
    double angle = 0;
    double scale = 1.0;
    int alpha = 255;
    
    // Sprite Data
    QString sourceFile; // path relative to project (assets/lib.fpg)
    int graphId = 0;    // 0 for direct image files
    
    // Text Data
    QString text;       // Text content
    QString fontFile;   // .fnt path
    int fontId = 0;     // runtime ID (0 is system font often)
    int alignment = 0;  // 0=Left, 1=Center, 2=Right
    
    // Logic
    QString script;     // Attached .prg behavior
    QString onClick;    // Simple One-Liner Action (e.g. change_scene("game"))

    // Runtime
    SceneEntityItem *item = nullptr; 
};

// Scene Input Modes
enum SceneInputMode {
    INPUT_MOUSE = 0,
    INPUT_KEYBOARD = 1,
    INPUT_HYBRID = 2
};

// Estructura para datos globales de la escena
struct SceneData {
    int width = 320;
    int height = 240;
    QColor backgroundColor = Qt::black;
    QString backgroundFile;
    int inputMode = INPUT_MOUSE; // Default
    bool exitOnEsc = true;       // New: Auto-generate ESC exit code
    
    // Music support
    QString musicFile;
    bool musicLoop = true;
    
    // Mouse Cursor
    QString cursorFile; // Optional: Custom Cursor FPG/PNG
    int cursorGraph = 0;
    
    QList<SceneEntity*> entities; // List of all entities
    
    ~SceneData() {
        qDeleteAll(entities);
        entities.clear();
    }
};

// Item gráfico personalizado que enlaza con los datos
class SceneEntityItem : public QGraphicsItem
{
public:
    SceneEntityItem(SceneEntity *data);
    
    SceneEntity *entityData() const { return m_data; }
    void syncToData();
    void updateVisuals(); // Re-read data and update graphics
    
    // QGraphicsItem interface
    QRectF boundingRect() const override;
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget) override;
    
protected:
    QVariant itemChange(GraphicsItemChange change, const QVariant &value) override;
    
private:
    SceneEntity *m_data;
    
    // Visual Caches
    QPixmap m_pixmap;
    QRectF m_bounds;
};

// El Editor Principal
class SceneEditor : public QGraphicsView
{
    Q_OBJECT

public:
    explicit SceneEditor(QWidget *parent = nullptr);
    ~SceneEditor();
    
    bool loadScene(const QString &fileName);
    bool saveScene(const QString &fileName);
    QString currentFile() const { return m_currentFile; }
    
    SceneData& sceneData() { return m_data; }
    QGraphicsScene* scene() { return m_scene; }
    void setEntityTree(QTreeWidget *tree) { m_entityTree = tree; }
    
public slots:
    void setResolution(int w, int h);
    void zoomIn();
    void zoomOut();
    void resetZoom();
    void showGrid(bool show);

signals:
    void editLogicRequested();
    void runSceneRequested(const QString &sceneName);
    void startupSceneRequested(const QString &sceneName);
    void sceneSaved(const QString &scenePath); // NEW signal

protected:
    void drawBackground(QPainter *painter, const QRectF &rect) override;
    void dragEnterEvent(QDragEnterEvent *event) override;
    void dragMoveEvent(QDragMoveEvent *event) override;
    void dropEvent(QDropEvent *event) override;
    void wheelEvent(QWheelEvent *event) override;
    void contextMenuEvent(QContextMenuEvent *event) override;

private:
    void setupScene();
    void drawGrid(QPainter *painter, const QRectF &rect);
    void refreshEntityTree(); // NEW: Update entity list
    void addEntity(const QString &filePath, const QPointF &pos, int graphId = 0, const QString &text = QString());
    
    QGraphicsScene *m_scene;
    QGraphicsRectItem *m_screenBorder = nullptr;
    
    SceneData m_data;
    QString m_currentFile;
    
    bool m_showGrid = true;
    int m_gridSize = 32;
    double m_zoomLevel = 1.0;
    
    QTreeWidget *m_entityTree = nullptr; // Entity list panel
};

#endif // SCENEEDITOR_H
