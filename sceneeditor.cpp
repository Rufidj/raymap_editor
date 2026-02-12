#include "sceneeditor.h"
#include "bennurenderer.h"
#include "fpgloader.h"
#include <QApplication>
#include <QCheckBox>
#include <QComboBox>
#include <QContextMenuEvent>
#include <QDebug>
#include <QDialogButtonBox>
#include <QDir>
#include <QDirIterator>
#include <QDragEnterEvent>
#include <QFileDialog>
#include <QFileInfo>
#include <QFontMetrics>
#include <QFormLayout>
#include <QGraphicsSceneHoverEvent>
#include <QGraphicsSceneMouseEvent>
#include <QHBoxLayout>
#include <QInputDialog>
#include <QLabel>
#include <QLineEdit>
#include <QMenu>
#include <QMessageBox>
#include <QMimeData>
#include <QPainter>
#include <QPushButton>
#include <QScrollBar>
#include <QSpinBox>
#include <QStyle>
#include <QStyleOptionGraphicsItem>
#include <QtMath>

// ============================================================================
// SceneEntityItem Implementation
// ============================================================================
// ============================================================================
// SceneEntityItem Implementation
// ============================================================================
SceneEntityItem::SceneEntityItem(SceneEntity *data) : m_data(data) {
  setFlags(QGraphicsItem::ItemIsMovable | QGraphicsItem::ItemIsSelectable |
           QGraphicsItem::ItemSendsGeometryChanges |
           QGraphicsItem::ItemUsesExtendedStyleOption); // For better selection
                                                        // rendering if needed
  setAcceptHoverEvents(true); // Crucial for resizing cursors

  setPos(data->x, data->y);
  setRotation(data->angle);

  // Apply Non-Uniform Scale via Transform using X and Y factors
  QTransform t;
  t.scale(data->scaleX, data->scaleY);
  setTransform(t);

  setZValue(-data->z); // Invert Z for Qt (Bennu: Lower=Top, Qt: Higher=Top)

  updateVisuals();
}

void SceneEntityItem::syncToData() {
  if (!m_data)
    return;
  m_data->x = pos().x();
  m_data->y = pos().y();
  m_data->angle = rotation();

  // Retrieve scale from transform
  m_data->scaleX = transform().m11();
  m_data->scaleY = transform().m22();
  // Keep legacy uniform 'scale' somewhat updated for compatibility, picking max
  // or avg? Let's check if they are uniform
  if (qAbs(m_data->scaleX - m_data->scaleY) < 0.001) {
    m_data->scale = m_data->scaleX;
  } else {
    m_data->scale = 1.0; // Mark as non-uniform default? Or just ignore.
  }

  m_data->z = -zValue(); // Invert back
}

void SceneEntityItem::updateVisuals() {
  prepareGeometryChange();

  if (m_data->type == ENTITY_SPRITE) {
    if (m_data->graphId > 0 && !m_data->sourceFile.isEmpty()) {
      // FPG loading ... (Keep existing logic or simplified for brevity in this
      // replace?) Keeping logic similar to before:
      QVector<TextureEntry> textures;
      if (FPGLoader::loadFPG(m_data->sourceFile, textures)) {
        QMap<int, QPixmap> map = FPGLoader::getTextureMap(textures);
        if (map.contains(m_data->graphId))
          m_pixmap = map[m_data->graphId];
      }
    } else if (!m_data->sourceFile.isEmpty()) {
      m_pixmap.load(m_data->sourceFile);
    }

    if (m_pixmap.isNull()) {
      m_pixmap = QPixmap(32, 32);
      m_pixmap.fill(Qt::red);
    }
    m_bounds = QRectF(-m_pixmap.width() / 2.0, -m_pixmap.height() / 2.0,
                      m_pixmap.width(), m_pixmap.height());
  } else if (m_data->type == ENTITY_WORLD3D) {
    m_bounds = QRectF(-64, -64, 128, 128);
  } else if (m_data->type == ENTITY_TEXT) {
    // (Text rendering logic same as before, assuming BennuFontManager exists)
    if (!m_data->fontFile.isEmpty()) {
      // Placeholder for existing BennuFontManager logic if needed, or
      // simplified We rely on previous implementation's header inclusion. Since
      // I am replacing the block, I should try to preserve the logic if I can
      // recall it. Actually, I can just use a simple placeholder if I don't
      // want to re-type the whole font manager call, BUT users might complain
      // if text disappears. I'll try to keep it simple or assume
      // BennuFontManager singleton is available. The previous code used
      // BennuFontManager::instance().renderText
      m_pixmap = BennuFontManager::instance().renderText(
          m_data->text.isEmpty() ? "TEXT" : m_data->text, m_data->fontFile);
    }
    if (m_pixmap.isNull()) {
      QFont font("Segoe UI", 14);
      font.setBold(true);
      QFontMetrics fm(font);
      QString txt = m_data->text.isEmpty() ? "TEXT" : m_data->text;
      QRect r = fm.boundingRect(txt);
      m_bounds = QRectF(r).adjusted(-10, -5, 10, 5);
    } else {
      m_bounds = QRectF(0, 0, m_pixmap.width(), m_pixmap.height());
    }
  }

  update();
}

QRectF SceneEntityItem::boundingRect() const {
  return m_bounds.adjusted(-10, -10, 10, 10); // Expanded for handles
}

void SceneEntityItem::paint(QPainter *painter,
                            const QStyleOptionGraphicsItem *option,
                            QWidget *widget) {
  Q_UNUSED(widget);

  if (m_data->type == ENTITY_SPRITE) {
    painter->drawPixmap(m_bounds.topLeft(), m_pixmap);
  } else if (m_data->type == ENTITY_WORLD3D) {
    painter->setBrush(QColor(0, 100, 200, 100));
    painter->setPen(QPen(Qt::cyan, 2));
    painter->drawRect(m_bounds);
    painter->drawText(m_bounds, Qt::AlignCenter, "3D WORLD");
  } else if (m_data->type == ENTITY_TEXT) {
    if (!m_pixmap.isNull()) {
      painter->drawPixmap(0, 0, m_pixmap);
    } else {
      painter->setPen(Qt::white);
      painter->setBrush(QColor(40, 40, 40, 180));
      painter->drawRoundedRect(m_bounds, 4, 4);
      painter->drawText(m_bounds, Qt::AlignCenter,
                        m_data->text.isEmpty() ? "TEXT" : m_data->text);
    }
  }

  // Draw Manual Hitbox Visualization
  if (m_data->hitW > 0 && m_data->hitH > 0) {
    painter->save();
    painter->setPen(QPen(Qt::blue, 1, Qt::DashLine));
    painter->setBrush(QColor(0, 0, 255, 50));
    painter->drawRect(m_data->hitX - m_data->hitW / 2.0,
                      m_data->hitY - m_data->hitH / 2.0, m_data->hitW,
                      m_data->hitH);
    painter->restore();
  }

  if (isSelected()) {
    painter->setPen(QPen(Qt::yellow, 1, Qt::DashLine));
    painter->setBrush(Qt::NoBrush);
    painter->drawRect(m_bounds);

    // Draw Handles
    painter->setPen(QPen(Qt::black, 1));
    painter->setBrush(Qt::white);

    QVector<Handle> handles = {TopLeft,     Top,    TopRight,   Right,
                               BottomRight, Bottom, BottomLeft, Left};
    for (Handle h : handles) {
      painter->drawRect(getHandleRect(h));
    }
  }
}

QRectF SceneEntityItem::getHandleRect(Handle h) const {
  double scaleX = transform().m11();
  double scaleY = transform().m22();
  if (scaleX == 0)
    scaleX = 1;
  if (scaleY == 0)
    scaleY = 1;

  double hw = 8.0 / qAbs(scaleX); // Handle Width adjusted for scale
  double hh = 8.0 / qAbs(scaleY);

  if (h == TopLeft)
    return QRectF(m_bounds.left() - hw / 2, m_bounds.top() - hh / 2, hw, hh);
  if (h == Top)
    return QRectF(m_bounds.center().x() - hw / 2, m_bounds.top() - hh / 2, hw,
                  hh);
  if (h == TopRight)
    return QRectF(m_bounds.right() - hw / 2, m_bounds.top() - hh / 2, hw, hh);
  if (h == Right)
    return QRectF(m_bounds.right() - hw / 2, m_bounds.center().y() - hh / 2, hw,
                  hh);
  if (h == BottomRight)
    return QRectF(m_bounds.right() - hw / 2, m_bounds.bottom() - hh / 2, hw,
                  hh);
  if (h == Bottom)
    return QRectF(m_bounds.center().x() - hw / 2, m_bounds.bottom() - hh / 2,
                  hw, hh);
  if (h == BottomLeft)
    return QRectF(m_bounds.left() - hw / 2, m_bounds.bottom() - hh / 2, hw, hh);
  if (h == Left)
    return QRectF(m_bounds.left() - hw / 2, m_bounds.center().y() - hh / 2, hw,
                  hh);

  return QRectF();
}

SceneEntityItem::Handle SceneEntityItem::getHandleAt(const QPointF &pos) const {
  QVector<Handle> handles = {TopLeft,     Top,    TopRight,   Right,
                             BottomRight, Bottom, BottomLeft, Left};
  for (Handle h : handles) {
    if (getHandleRect(h).contains(pos))
      return h;
  }
  return None;
}

void SceneEntityItem::hoverMoveEvent(QGraphicsSceneHoverEvent *event) {
  if (isSelected()) {
    Handle h = getHandleAt(event->pos());
    if (h == TopLeft || h == BottomRight)
      setCursor(Qt::SizeFDiagCursor);
    else if (h == TopRight || h == BottomLeft)
      setCursor(Qt::SizeBDiagCursor);
    else if (h == Top || h == Bottom)
      setCursor(Qt::SizeVerCursor);
    else if (h == Left || h == Right)
      setCursor(Qt::SizeHorCursor);
    else
      setCursor(Qt::ArrowCursor);
  } else {
    setCursor(Qt::ArrowCursor);
  }
  QGraphicsItem::hoverMoveEvent(event);
}

void SceneEntityItem::mousePressEvent(QGraphicsSceneMouseEvent *event) {
  if (event->button() == Qt::LeftButton && isSelected()) {
    m_resizingHandle = getHandleAt(event->pos());
    if (m_resizingHandle != None) {
      m_resizeStartPos = event->pos();  // Local pos
      m_initialTransform = transform(); // Store start transform
      return; // Consume event, don't propagate to move
    }
  }
  QGraphicsItem::mousePressEvent(event);
}

void SceneEntityItem::mouseMoveEvent(QGraphicsSceneMouseEvent *event) {
  if (m_resizingHandle != None) {
    // Calculate new scale based on drag
    // This is tricky because we are dragging in Local coordinates which are
    // transformed by the scale we are changing! Actually, dragging in local
    // coordinates of the item changes if we change the item's transform? Yes.
    // It's better to use Parent coordinates or Scene coordinates for drag
    // calculation to avoid feedback loop.

    // Let's rely on the fact that we stored m_initialTransform.
    // But we want to calculate the new scale.

    // Simplified Logic:
    // 1. Calculate distance from Center to Mouse.
    // 2. Compare with initial distance (or bounds edge).

    // Center in Local coords is (0,0) usually (because m_bounds is centered).
    // Let's assume (0,0) is pivot.

    QPointF curPos = event->pos();
    double w = m_bounds.width() / 2.0;
    double h = m_bounds.height() / 2.0;
    if (w < 1)
      w = 1;
    if (h < 1)
      h = 1;

    double dx = curPos.x();
    double dy = curPos.y();

    // If we are dragging Right handle, we want dx to define the new width
    // scaling. The original right edge is at X = w. The current mouse is at X =
    // dx. But dx is in CURRENT local coords. If we change scale, local coords
    // mapping changes? No, local coords are fixed relative to the image
    // content. Wait, if I scale the item (2x), the image draws 2x bigger, but
    // local coord (10,10) is still (10,10) on the original image? YES.
    // QTransform affects how Local maps to Parent. So `event->pos()` is in
    // Local Coords. If I drag the handle, I am moving the mouse in
    // Screen/Scene, which maps to Local. If I change the scale, the mapping
    // from Scene to Local changes. So `event->pos()` will "drift" if I update
    // scale live? Yes.

    // SOLUTION: Use SCENE coordinates for calculation.

    QPointF centerScene = mapToScene(0, 0);
    QPointF mouseScene = event->scenePos();

    // Unrotate mouse position relative to center to align with local axes
    double angleRad = -rotation() * M_PI / 180.0;
    double dxS = mouseScene.x() - centerScene.x();
    double dyS = mouseScene.y() - centerScene.y();

    double localX = dxS * cos(angleRad) - dyS * sin(angleRad);
    double localY = dxS * sin(angleRad) + dyS * cos(angleRad);

    // Now localX/localY are distance from center in unrotated space (scaling
    // axes). Target scale X = localX / (original_width/2) ?

    double newSx = m_initialTransform.m11();
    double newSy = m_initialTransform.m22();

    // If dragging Right, new ScaleX should make the right edge (w) map to
    // localX. Wait, if I change scale, 'w' (local width) stays 'w'. The screen
    // width becomes w * scale. So I want w * newSx = localX (which is the
    // screen distance from center).

    if (m_resizingHandle == Right || m_resizingHandle == TopRight ||
        m_resizingHandle == BottomRight) {
      newSx = localX / w;
    } else if (m_resizingHandle == Left || m_resizingHandle == TopLeft ||
               m_resizingHandle == BottomLeft) {
      newSx = -localX / w;
    }

    if (m_resizingHandle == Bottom || m_resizingHandle == BottomRight ||
        m_resizingHandle == BottomLeft) {
      newSy = localY / h;
    } else if (m_resizingHandle == Top || m_resizingHandle == TopRight ||
               m_resizingHandle == TopLeft) {
      newSy = -localY / h;
    }

    // Prevent 0 or negative scale issues (though negative flips, which is valid
    // in Bennu, but confusing in editor)
    if (qAbs(newSx) < 0.01)
      newSx = 0.01;
    if (qAbs(newSy) < 0.01)
      newSy = 0.01;

    QTransform t;
    t.scale(newSx, newSy);
    setTransform(t);

    // Update data immediately? Or on release?
    // Better on release to avoid spamming data sync if it triggers other
    // things. But visuals need update immediately. Let's do partial sync.

    return;
  }
  QGraphicsItem::mouseMoveEvent(event);
}

void SceneEntityItem::mouseReleaseEvent(QGraphicsSceneMouseEvent *event) {
  if (m_resizingHandle != None) {
    m_resizingHandle = None;
    syncToData(); // Commit changes
    return;
  }
  QGraphicsItem::mouseReleaseEvent(event);
}

QVariant SceneEntityItem::itemChange(GraphicsItemChange change,
                                     const QVariant &value) {
  if (change == ItemSelectedHasChanged) {
    update(); // Redraw to show/hide handles
  }
  return QGraphicsItem::itemChange(change, value);
}

// ============================================================================
// SceneEditor Implementation
// ============================================================================
SceneEditor::SceneEditor(QWidget *parent)
    : QGraphicsView(parent), m_scene(new QGraphicsScene(this)) {
  setupScene();

  setRenderHint(QPainter::Antialiasing, false);
  setRenderHint(QPainter::SmoothPixmapTransform,
                false); // true if we want better scaling
  setDragMode(QGraphicsView::RubberBandDrag);
  setTransformationAnchor(QGraphicsView::AnchorUnderMouse);
  setResizeAnchor(QGraphicsView::AnchorViewCenter);
  setViewportUpdateMode(QGraphicsView::FullViewportUpdate);
  setAcceptDrops(true);
  setBackgroundBrush(QColor(40, 40, 40));
}

SceneEditor::~SceneEditor() {}

void SceneEditor::setupScene() {
  setScene(m_scene);

  QRectF screenRect(0, 0, m_data.width, m_data.height);
  m_scene->setSceneRect(screenRect.adjusted(-500, -500, 500, 500));

  // Interaction Map Layer (Initially transparent)
  m_interactionMap = QImage(m_data.width, m_data.height, QImage::Format_ARGB32);
  m_interactionMap.fill(Qt::transparent);
  m_interactionPixmapItem = new QGraphicsPixmapItem();
  m_interactionPixmapItem->setPixmap(QPixmap::fromImage(m_interactionMap));
  m_interactionPixmapItem->setZValue(999);  // Below border, above entities
  m_interactionPixmapItem->setOpacity(0.5); // Semi-transparent for editing
  m_scene->addItem(m_interactionPixmapItem);

  m_screenBorder = new QGraphicsRectItem(0, 0, m_data.width, m_data.height);
  m_screenBorder->setPen(QPen(Qt::yellow, 2, Qt::DashLine));
  m_screenBorder->setZValue(1000);
  m_scene->addItem(m_screenBorder);

  connect(m_scene, &QGraphicsScene::selectionChanged, this, [this]() {
    QList<QGraphicsItem *> selected = m_scene->selectedItems();
    if (selected.isEmpty()) {
      emit entitySelected(nullptr);
    } else {
      SceneEntityItem *item =
          qgraphicsitem_cast<SceneEntityItem *>(selected.first());
      if (item) {
        emit entitySelected(item->entityData());
      } else {
        emit entitySelected(nullptr);
      }
    }
  });

  resetZoom();
}

void SceneEditor::setEditorMode(EditorMode mode) {
  m_mode = mode;
  if (m_mode == ModePaintInteraction) {
    setDragMode(QGraphicsView::NoDrag);
    m_interactionPixmapItem->setAcceptedMouseButtons(Qt::LeftButton);
  } else {
    setDragMode(QGraphicsView::RubberBandDrag);
    m_interactionPixmapItem->setAcceptedMouseButtons(Qt::NoButton);
  }
}

void SceneEditor::clearInteractionMap() {
  m_interactionMap.fill(Qt::transparent);
  m_interactionPixmapItem->setPixmap(QPixmap::fromImage(m_interactionMap));
  emit sceneChanged();
}

void SceneEditor::paintAt(const QPointF &pos) {
  if (m_interactionMap.isNull())
    return;

  QPainter p(&m_interactionMap);
  p.setCompositionMode(
      QPainter::CompositionMode_Source); // Overwrite (including alpha)
  p.setBrush(m_brushColor);
  p.setPen(Qt::NoPen);
  p.drawEllipse(pos, m_brushSize, m_brushSize);
  p.end();

  m_interactionPixmapItem->setPixmap(QPixmap::fromImage(m_interactionMap));
  emit sceneChanged();
}

void SceneEditor::mousePressEvent(QMouseEvent *event) {
  if (m_mode == ModePaintInteraction && event->button() == Qt::LeftButton) {
    paintAt(mapToScene(event->pos()));
    return;
  }
  QGraphicsView::mousePressEvent(event);
}

void SceneEditor::mouseMoveEvent(QMouseEvent *event) {
  if (m_mode == ModePaintInteraction && (event->buttons() & Qt::LeftButton)) {
    paintAt(mapToScene(event->pos()));
    return;
  }
  QGraphicsView::mouseMoveEvent(event);
}

bool SceneEditor::loadScene(const QString &fileName) {
  m_currentFile = fileName;
  QFile file(fileName);
  if (!file.open(QIODevice::ReadOnly)) {
    return false;
  }

  m_scene->clear();

  // Re-create permanent support items after clear()
  m_interactionPixmapItem = new QGraphicsPixmapItem();
  m_interactionPixmapItem->setZValue(999);
  m_interactionPixmapItem->setOpacity(0.5);
  m_interactionPixmapItem->setAcceptedMouseButtons(
      (m_mode == ModePaintInteraction) ? Qt::LeftButton : Qt::NoButton);
  m_scene->addItem(m_interactionPixmapItem);

  m_screenBorder = new QGraphicsRectItem(0, 0, m_data.width, m_data.height);

  m_screenBorder->setPen(QPen(Qt::yellow, 2, Qt::DashLine));
  m_screenBorder->setZValue(1000);
  m_screenBorder->setAcceptedMouseButtons(Qt::NoButton);
  m_scene->addItem(m_screenBorder);
  qDeleteAll(m_data.entities);
  m_data.entities.clear();

  QByteArray jsonData = file.readAll();
  file.close();

  QJsonDocument doc = QJsonDocument::fromJson(jsonData);
  if (doc.isNull())
    return false;

  QJsonObject root = doc.object();

  int w = root["width"].toInt(320);
  int h = root["height"].toInt(240);
  setResolution(w, h);

  m_data.inputMode = root["inputMode"].toInt(INPUT_MOUSE);
  m_data.exitOnEsc = root["exitOnEsc"].toBool(true);
  m_data.cursorFile = root["cursorFile"].toString();
  m_data.cursorGraph = root["cursorGraph"].toInt(0);
  m_data.musicFile = root["musicFile"].toString();
  m_data.musicLoop = root["musicLoop"].toBool(true);

  m_data.timeout = root["timeout"].toInt(0);
  m_data.nextScene = root["nextScene"].toString();

  QJsonArray entitiesArray = root["entities"].toArray();
  for (const QJsonValue &val : entitiesArray) {
    QJsonObject obj = val.toObject();

    SceneEntity *ent = new SceneEntity();
    ent->type = (SceneEntityType)obj["type"].toInt(ENTITY_SPRITE);
    ent->name = obj["name"].toString();
    ent->x = obj["x"].toDouble();
    ent->y = obj["y"].toDouble();
    ent->z = obj["z"].toDouble();
    ent->angle = obj["angle"].toDouble();
    ent->scale = obj["scale"].toDouble(1.0);
    ent->scaleX = obj["scaleX"].toDouble(ent->scale);
    ent->scaleY = obj["scaleY"].toDouble(ent->scale);
    ent->script = obj["script"].toString();
    ent->onClick = obj["onClick"].toString();
    ent->hitW = obj["hitW"].toInt(0);
    ent->hitH = obj["hitH"].toInt(0);
    ent->hitX = obj["hitX"].toInt(0);
    ent->hitY = obj["hitY"].toInt(0);

    // Sprite
    if (ent->type == ENTITY_SPRITE) {
      QString srcRel = obj["sourceFile"].toString();
      QFileInfo sceneInfo(fileName);
      ent->sourceFile =
          QDir(sceneInfo.absolutePath())
              .cleanPath(QDir(sceneInfo.absolutePath()).filePath(srcRel));
      ent->graphId = obj["graphId"].toInt();
    }
    // Text
    else if (ent->type == ENTITY_TEXT) {
      ent->text = obj["text"].toString();
      ent->fontId = obj["fontId"].toInt();
      ent->alignment = obj["alignment"].toInt(0);

      if (obj.contains("fontFile")) {
        QString fontRel = obj["fontFile"].toString();
        QFileInfo sceneInfo(fileName);
        ent->fontFile =
            QDir(sceneInfo.absolutePath())
                .cleanPath(QDir(sceneInfo.absolutePath()).filePath(fontRel));
      }
    }
    // World 3D
    else if (ent->type == ENTITY_WORLD3D) {
      QString srcRel = obj["sourceFile"].toString();
      QFileInfo sceneInfo(fileName);
      ent->sourceFile =
          QDir(sceneInfo.absolutePath())
              .cleanPath(QDir(sceneInfo.absolutePath()).filePath(srcRel));
    }

    // Visual Item (Common)
    SceneEntityItem *item = new SceneEntityItem(ent);
    m_scene->addItem(item);
    ent->item = item;

    m_data.entities.append(ent);
  }

  // Load interaction map if exists
  QFileInfo scnInfo(fileName);
  QString interactionPath =
      scnInfo.absolutePath() + "/" + scnInfo.baseName() + "_interaction.png";
  if (QFileInfo::exists(interactionPath)) {
    m_interactionMap.load(interactionPath);
    m_interactionPixmapItem->setPixmap(QPixmap::fromImage(m_interactionMap));
  } else {
    m_interactionMap =
        QImage(m_data.width, m_data.height, QImage::Format_ARGB32);
    m_interactionMap.fill(Qt::transparent);
    m_interactionPixmapItem->setPixmap(QPixmap::fromImage(m_interactionMap));
  }

  return true;
}

bool SceneEditor::saveScene(const QString &fileName) {
  if (fileName.isEmpty())
    return false;

  QJsonObject root;
  root["type"] = "scene2d";
  root["version"] = 2; // Bump version
  root["width"] = m_data.width;
  root["height"] = m_data.height;
  root["inputMode"] = m_data.inputMode;
  root["exitOnEsc"] = m_data.exitOnEsc;

  QDir sceneDir = QFileInfo(fileName).absoluteDir();

  if (!m_data.cursorFile.isEmpty())
    root["cursorFile"] = sceneDir.relativeFilePath(m_data.cursorFile);
  if (m_data.cursorGraph > 0)
    root["cursorGraph"] = m_data.cursorGraph;
  if (!m_data.musicFile.isEmpty())
    root["musicFile"] = sceneDir.relativeFilePath(m_data.musicFile);
  root["musicLoop"] = m_data.musicLoop;

  root["timeout"] = m_data.timeout;
  if (!m_data.nextScene.isEmpty())
    root["nextScene"] = m_data.nextScene;
  QJsonArray entitiesArray;

  for (SceneEntity *ent : m_data.entities) {
    if (ent->item)
      ent->item->syncToData();

    QJsonObject obj;
    obj["type"] = (int)ent->type;
    obj["name"] = ent->name;
    obj["x"] = ent->x;
    obj["y"] = ent->y;
    obj["z"] = ent->z;
    obj["angle"] = ent->angle;
    obj["scale"] = ent->scale;
    obj["scaleX"] = ent->scaleX;
    obj["scaleY"] = ent->scaleY;

    if (!ent->script.isEmpty()) {
      obj["script"] = sceneDir.relativeFilePath(ent->script);
    }
    if (!ent->onClick.isEmpty()) {
      obj["onClick"] = ent->onClick;
    }
    if (ent->hitW != 0)
      obj["hitW"] = ent->hitW;
    if (ent->hitH != 0)
      obj["hitH"] = ent->hitH;
    if (ent->hitX != 0)
      obj["hitX"] = ent->hitX;
    if (ent->hitY != 0)
      obj["hitY"] = ent->hitY;

    if (ent->type == ENTITY_SPRITE) {
      obj["sourceFile"] = sceneDir.relativeFilePath(ent->sourceFile);
      obj["graphId"] = ent->graphId;
    } else if (ent->type == ENTITY_TEXT) {
      obj["text"] = ent->text;
      obj["fontId"] = ent->fontId;
      if (!ent->fontFile.isEmpty()) {
        obj["fontFile"] =
            QDir(QFileInfo(fileName).path()).relativeFilePath(ent->fontFile);
      }
      obj["alignment"] = ent->alignment;
    } else if (ent->type == ENTITY_WORLD3D) {
      obj["sourceFile"] =
          QDir(QFileInfo(fileName).path()).relativeFilePath(ent->sourceFile);
    }

    entitiesArray.append(obj);
  }

  root["entities"] = entitiesArray;

  QJsonDocument doc(root);
  QByteArray data = doc.toJson();

  if (data.isEmpty()) {
    qDebug() << "Error: Intentando guardar una escena vacía o corrupta.";
    return false;
  }

  QFile file(fileName);
  if (!file.open(QIODevice::WriteOnly)) {
    return false;
  }
  file.write(data);
  file.close();

  // Save interaction map
  QFileInfo scnInfo(fileName);
  QString interactionPath =
      scnInfo.absolutePath() + "/" + scnInfo.baseName() + "_interaction.png";
  if (!m_interactionMap.isNull()) {

    m_interactionMap.save(interactionPath);
  }

  m_currentFile = fileName;

  // Emit signal so MainWindow can regenerate scripts
  emit sceneSaved(fileName);

  return true;
}

// ... SetResolution, Zoom methods same as before ...
void SceneEditor::setResolution(int w, int h) {
  m_data.width = w;
  m_data.height = h;
  if (m_screenBorder)
    m_screenBorder->setRect(0, 0, w, h);
  m_scene->update();
}
void SceneEditor::zoomIn() {
  scale(1.2, 1.2);
  m_zoomLevel *= 1.2;
}
void SceneEditor::zoomOut() {
  scale(1.0 / 1.2, 1.0 / 1.2);
  m_zoomLevel /= 1.2;
}
void SceneEditor::resetZoom() {
  resetTransform();
  m_zoomLevel = 1.0;
  centerOn(m_data.width / 2.0, m_data.height / 2.0);
}
void SceneEditor::showGrid(bool show) {
  m_showGrid = show;
  m_scene->update();
}
void SceneEditor::drawBackground(QPainter *painter, const QRectF &rect) {
  QGraphicsView::drawBackground(painter, rect);
  QRectF screenRect(0, 0, m_data.width, m_data.height);
  if (rect.intersects(screenRect))
    painter->fillRect(screenRect, m_data.backgroundColor);
  if (m_showGrid)
    drawGrid(painter, screenRect);
}
void SceneEditor::drawGrid(QPainter *painter, const QRectF &rect) {
  QPen pen(QColor(255, 255, 255, 40));
  pen.setWidth(0);
  painter->setPen(pen);
  qreal left = int(rect.left()) - (int(rect.left()) % m_gridSize);
  qreal top = int(rect.top()) - (int(rect.top()) % m_gridSize);
  QVarLengthArray<QLineF, 100> lines;
  for (qreal x = left; x < rect.right(); x += m_gridSize)
    lines.append(QLineF(x, rect.top(), x, rect.bottom()));
  for (qreal y = top; y < rect.bottom(); y += m_gridSize)
    lines.append(QLineF(rect.left(), y, rect.right(), y));
  painter->drawLines(lines.data(), lines.size());
}
void SceneEditor::wheelEvent(QWheelEvent *event) {
  if (event->modifiers() & Qt::ControlModifier) {
    if (event->angleDelta().y() > 0)
      zoomIn();
    else
      zoomOut();
    event->accept();
  } else {
    QGraphicsView::wheelEvent(event);
  }
}

void SceneEditor::contextMenuEvent(QContextMenuEvent *event) {
  // Use items() to get ALL items at the click position, sorted by Z-order
  QList<QGraphicsItem *> clickedItems = items(event->pos());

  QGraphicsItem *item = nullptr;
  SceneEntityItem *entItem = nullptr;

  // Find the first valid SceneEntityItem in the stack
  for (QGraphicsItem *candidate : clickedItems) {
    entItem = dynamic_cast<SceneEntityItem *>(candidate);
    if (entItem) {
      item = candidate;
      break;
    }
  }

  qDebug() << "ContextMenuEvent at" << event->pos()
           << "Found entity:" << (entItem ? "YES" : "NO");

  // Scene Menu (Background) - Only if no entity found
  if (!entItem) {
    QPointF clickPos = mapToScene(event->pos());
    QMenu menu(this);

    menu.addAction(
        QIcon::fromTheme("system-run"), "Añadir Botón / Enlace...", this,
        [this, clickPos]() {
          QDir sceneDir(QFileInfo(m_currentFile).path());
          QStringList scenes =
              sceneDir.entryList(QStringList() << "*.scn", QDir::Files);

          QDialog dlg(this);
          dlg.setWindowTitle("Crear Botón de Navegación");
          QVBoxLayout *lay = new QVBoxLayout(&dlg);

          lay->addWidget(new QLabel("Texto del Botón:"));
          QLineEdit *txtEdit = new QLineEdit();
          lay->addWidget(txtEdit);

          lay->addWidget(new QLabel("Acción al hacer Click:"));
          QComboBox *combo = new QComboBox();
          for (const QString &s : scenes) {
            if (s != QFileInfo(m_currentFile).fileName())
              combo->addItem("Ir a Escena: " + QFileInfo(s).baseName(),
                             QFileInfo(s).baseName());
          }
          combo->addItem("NINGUNA (Solo Texto)", "NONE");
          combo->addItem("Salir del Juego", "EXIT");
          lay->addWidget(combo);

          lay->addWidget(new QLabel("Fuente (.fnt):"));
          QComboBox *fontCombo = new QComboBox();
          fontCombo->addItem("System (0)", "");

          // Scan for fonts in common project directories
          QDir rootDir(QFileInfo(m_currentFile).path());
          rootDir.cdUp();
          rootDir.cdUp(); // src/scenes -> root

          QStringList fontDirs = {"assets", "fnt", "fonts", "src/assets"};
          for (const QString &dirName : fontDirs) {
            QDir d = rootDir;
            if (d.exists(dirName) && d.cd(dirName)) {
              QDirIterator it(d.path(), QStringList() << "*.fnt" << "*.fnx",
                              QDir::Files, QDirIterator::Subdirectories);
              while (it.hasNext()) {
                QString f = it.next();
                QString rel =
                    QDir(QFileInfo(m_currentFile).path()).relativeFilePath(f);
                // Avoid duplicates in combo
                if (fontCombo->findData(rel) == -1) {
                  fontCombo->addItem(
                      QFileInfo(f).fileName() + " [" + dirName + "]", rel);
                }
              }
            }
          }
          lay->addWidget(fontCombo);

          QDialogButtonBox *btns = new QDialogButtonBox(
              QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
          lay->addWidget(btns);
          connect(btns, &QDialogButtonBox::accepted, &dlg, &QDialog::accept);
          connect(btns, &QDialogButtonBox::rejected, &dlg, &QDialog::reject);

          if (dlg.exec() == QDialog::Accepted) {
            QString action = combo->currentData().toString();
            QString code;
            QString prefix = "btn_";

            if (action == "EXIT") {
              code = "exit(\"\",0);";
              prefix = "btn_exit";
            } else if (action == "NONE") {
              code = ""; // No action
              prefix = "txt_";
            } else {
              code = QString("goto_scene(\"%1\");").arg(action);
              prefix = "btn_" + action;
            }

            QString labelText = txtEdit->text();
            if (labelText.isEmpty())
              labelText = (action == "EXIT")
                              ? "SALIR"
                              : (action == "NONE" ? "TEXTO" : action.toUpper());

            SceneEntity *newEnt = new SceneEntity();
            newEnt->type = ENTITY_TEXT;
            newEnt->name =
                prefix +
                (action == "NONE"
                     ? "label"
                     : ""); // Ensure unique name logic elsewhere if needed
            newEnt->text = labelText;
            // Fix: Store ABSOLUTE path for immediate rendering
            QString relFontPath = fontCombo->currentData().toString();
            if (!relFontPath.isEmpty()) {
              newEnt->fontFile =
                  QDir::cleanPath(sceneDir.absoluteFilePath(relFontPath));
            } else {
              newEnt->fontFile = "";
            }
            newEnt->x = clickPos.x();
            newEnt->y = clickPos.y();
            newEnt->onClick = code;

            m_data.entities.append(newEnt);

            // Create visual item immediately
            SceneEntityItem *item = new SceneEntityItem(newEnt);
            m_scene->addItem(item);
            newEnt->item = item;

            // Update entity tree if exists
            if (m_entityTree) {
              QTreeWidgetItem *treeItem = new QTreeWidgetItem();
              treeItem->setText(0, newEnt->name);
              treeItem->setText(1, "Texto");
              treeItem->setData(0, Qt::UserRole,
                                QVariant::fromValue((void *)newEnt));
              m_entityTree->addTopLevelItem(treeItem);
            }
          }
        });
    menu.addSeparator();

    menu.addAction(QIcon::fromTheme("text-x-script"),
                   "Editar Código de Escena (.prg)", this,
                   [this]() { emit editLogicRequested(); });

    menu.addAction(
        QIcon::fromTheme("preferences-system"), "Propiedades de Escena...",
        this, [this]() {
          QDialog dlg(this);
          dlg.setWindowTitle("Propiedades de Escena");
          QFormLayout *layout = new QFormLayout(&dlg);

          QSpinBox *wSpin = new QSpinBox();
          wSpin->setRange(1, 4096);
          wSpin->setValue(m_data.width);
          QSpinBox *hSpin = new QSpinBox();
          hSpin->setRange(1, 4096);
          hSpin->setValue(m_data.height);

          QComboBox *inputCombo = new QComboBox();
          inputCombo->addItem("Ratón (Mouse)", INPUT_MOUSE);
          inputCombo->addItem("Teclado / Gamepad", INPUT_KEYBOARD);
          inputCombo->addItem("Híbrido", INPUT_HYBRID);
          inputCombo->setCurrentIndex(m_data.inputMode);

          // Cursor Config
          QLineEdit *cursorPathEdit = new QLineEdit(m_data.cursorFile);
          QPushButton *btnBrowseCursor = new QPushButton("...");
          connect(btnBrowseCursor, &QPushButton::clicked, [&]() {
            QString f = QFileDialog::getOpenFileName(
                &dlg, "Cursor (FPG / PNG)", QFileInfo(m_currentFile).path(),
                "Images/FPG (*.png *.fpg *.jpg)");
            if (!f.isEmpty()) {
              QDir d(QFileInfo(m_currentFile).path());
              cursorPathEdit->setText(d.relativeFilePath(f));
            }
          });
          QHBoxLayout *cursorLay = new QHBoxLayout();
          cursorLay->addWidget(cursorPathEdit);
          cursorLay->addWidget(btnBrowseCursor);

          QSpinBox *cursorGraphSpin = new QSpinBox();
          cursorGraphSpin->setRange(0, 999);
          cursorGraphSpin->setValue(m_data.cursorGraph);

          QCheckBox *exitCheck = new QCheckBox("Salir con ESC");
          exitCheck->setChecked(m_data.exitOnEsc);

          // Timeout
          QSpinBox *timeoutSpin = new QSpinBox();
          timeoutSpin->setRange(0, 9999);
          timeoutSpin->setSuffix(" seg");
          timeoutSpin->setValue(m_data.timeout);

          QLineEdit *nextSceneEdit = new QLineEdit(m_data.nextScene);
          nextSceneEdit->setPlaceholderText("Nombre de la escena (ej: menu2)");

          layout->addRow("Ancho:", wSpin);
          layout->addRow("Alto:", hSpin);
          layout->addRow("Input:", inputCombo);
          layout->addRow("Opciones:", exitCheck);
          layout->addRow("Timeout:", timeoutSpin);
          layout->addRow("Siguiente Escena:", nextSceneEdit);
          layout->addRow("---------------", new QWidget());
          layout->addRow("Cursor File:", cursorLay);
          layout->addRow("Cursor GraphID:", cursorGraphSpin);

          QDialogButtonBox *btns = new QDialogButtonBox(
              QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
          layout->addRow(btns);
          connect(btns, &QDialogButtonBox::accepted, &dlg, &QDialog::accept);
          connect(btns, &QDialogButtonBox::rejected, &dlg, &QDialog::reject);

          if (dlg.exec() == QDialog::Accepted) {
            setResolution(wSpin->value(), hSpin->value());
            m_data.inputMode = inputCombo->currentData().toInt();
            m_data.exitOnEsc = exitCheck->isChecked();
            m_data.timeout = timeoutSpin->value();
            m_data.nextScene = nextSceneEdit->text();
            m_data.cursorFile = cursorPathEdit->text();
            m_data.cursorGraph = cursorGraphSpin->value();
            emit sceneChanged(); // Realtime update
          }
        });

    menu.addSeparator();

    menu.addAction(
        QIcon::fromTheme("media-playback-start"), "Ejecutar Escena (Debug)",
        this, [this]() {
          if (!m_currentFile.isEmpty()) {
            saveScene(m_currentFile);
            emit runSceneRequested(QFileInfo(m_currentFile).baseName());
          } else {
            QMessageBox::warning(this, "Aviso", "Guarda la escena primero.");
          }
        });

    menu.addAction(
        QIcon::fromTheme("flag"), "Establecer como Escena Inicial", this,
        [this]() {
          if (!m_currentFile.isEmpty()) {
            if (QMessageBox::question(
                    this, "Escena Inicial",
                    "¿Establecer esta escena como la predeterminada del "
                    "proyecto?\n"
                    "Esto actualizará el archivo main.prg.") ==
                QMessageBox::Yes) {
              emit startupSceneRequested(QFileInfo(m_currentFile).baseName());
            }
          } else {
            QMessageBox::warning(this, "Aviso", "Guarda la escena primero.");
          }
        });

    menu.addAction("Guardar Escena", this, [this]() {
      if (!m_currentFile.isEmpty())
        saveScene(m_currentFile);
    });

    menu.exec(event->globalPos());
    return;
  }

  if (!entItem)
    return;
  SceneEntity *ent = entItem->entityData();
  if (!ent)
    return;

  QMenu menu(this);

  // Edit Hitbox (NEW: Integrated here)
  menu.addAction(
      QIcon::fromTheme("edit-guides"), "Editar Hitbox Manual...", this,
      [this, ent, entItem]() {
        QDialog dlg(this);
        dlg.setWindowTitle("Editar Hitbox Manual");
        QFormLayout layout(&dlg);

        QSpinBox wSpin, hSpin, xSpin, ySpin;
        wSpin.setRange(0, 9999);
        wSpin.setValue(ent->hitW);
        hSpin.setRange(0, 9999);
        hSpin.setValue(ent->hitH);
        xSpin.setRange(-9999, 9999);
        xSpin.setValue(ent->hitX);
        ySpin.setRange(-9999, 9999);
        ySpin.setValue(ent->hitY);

        layout.addRow("Ancho (0=Auto):", &wSpin);
        layout.addRow("Alto (0=Auto):", &hSpin);
        layout.addRow("Offset X:", &xSpin);
        layout.addRow("Offset Y:", &ySpin);

        QDialogButtonBox btns(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
        layout.addRow(&btns);
        connect(&btns, &QDialogButtonBox::accepted, &dlg, &QDialog::accept);
        connect(&btns, &QDialogButtonBox::rejected, &dlg, &QDialog::reject);

        if (dlg.exec() == QDialog::Accepted) {
          ent->hitW = wSpin.value();
          ent->hitH = hSpin.value();
          ent->hitX = xSpin.value();
          ent->hitY = ySpin.value();
          if (entItem)
            entItem->update(); // Redraw debug rect
          emit sceneChanged();
        }
      });

  menu.addSeparator();

  // Script Action

  QString scriptActionText =
      ent->script.isEmpty()
          ? "Asignar Script (.prg)..."
          : "Cambiar Script (" + QFileInfo(ent->script).fileName() + ")...";
  QAction *scriptAction =
      menu.addAction(QIcon::fromTheme("text-x-script"), scriptActionText);
  connect(scriptAction, &QAction::triggered, this, [this, ent]() {
    QString path = QFileDialog::getOpenFileName(
        this, "Seleccionar Script", QFileInfo(m_currentFile).absolutePath(),
        "BennuGD Scripts (*.prg)");
    if (!path.isEmpty()) {
      ent->script = path;
      QMessageBox::information(this, "Script Asignado",
                               "Script asignado: " +
                                   QFileInfo(path).fileName());
      emit sceneChanged(); // Realtime update
    }
  });

  // Click Action
  QAction *clickAction = menu.addAction(QIcon::fromTheme("input-mouse"),
                                        "Definir Acción Click (Código)...");
  connect(clickAction, &QAction::triggered, this, [this, ent]() {
    bool ok;
    QString text = QInputDialog::getText(
        this, "Acción al hacer Click",
        "Código (ej: change_scene(\"game\");):", QLineEdit::Normal,
        ent->onClick, &ok);
    if (ok) {
      ent->onClick = text;
      emit sceneChanged(); // Realtime update
    }
  });

  // Special Edit for Navigation Buttons (Text + onClick)
  if (ent->type == ENTITY_TEXT) {
    QString label = ent->onClick.isEmpty()
                        ? "Convertir en Botón de Navegación..."
                        : "Editar Enlace / Botón...";
    QAction *editLinkAction =
        menu.addAction(QIcon::fromTheme("applications-internet"), label);
    connect(editLinkAction, &QAction::triggered, this, [this, ent]() {
      QDialog dlg(this);
      dlg.setWindowTitle("Editar Botón");
      QVBoxLayout *lay = new QVBoxLayout(&dlg);

      QDir sceneDir(QFileInfo(m_currentFile).path());
      QStringList scenes =
          sceneDir.entryList(QStringList() << "*.scn", QDir::Files);

      lay->addWidget(new QLabel("Destino del Click:"));
      QComboBox *combo = new QComboBox();

      // Try to parse current action
      QString currentDest = "";
      if (ent->onClick.contains("goto_scene")) {
        int start = ent->onClick.indexOf("\"") + 1;
        int end = ent->onClick.lastIndexOf("\"");
        if (start > 0 && end > start) {
          currentDest = ent->onClick.mid(start, end - start);
        }
      } else if (ent->onClick.contains("exit")) {
        currentDest = "EXIT";
      }

      for (const QString &s : scenes) {
        QString base = QFileInfo(s).baseName();
        combo->addItem("Ir a Escena: " + base, base);
      }
      combo->addItem("Salir del Juego", "EXIT");

      int comboIdx = combo->findData(currentDest);
      if (comboIdx >= 0)
        combo->setCurrentIndex(comboIdx);

      lay->addWidget(combo);

      QDialogButtonBox *btns =
          new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
      lay->addWidget(btns);

      connect(btns, &QDialogButtonBox::accepted, &dlg, &QDialog::accept);
      connect(btns, &QDialogButtonBox::rejected, &dlg, &QDialog::reject);

      if (dlg.exec() == QDialog::Accepted) {
        QString action = combo->currentData().toString();
        ent->onClick = (action == "EXIT")
                           ? "exit(\"\",0);"
                           : QString("goto_scene(\"%1\");").arg(action);
      }
    });
  }

  if (!ent->script.isEmpty()) {
    QAction *removeScript = menu.addAction("Quitar Script");
    connect(removeScript, &QAction::triggered, this,
            [this, ent]() { ent->script.clear(); });
  }

  menu.addSeparator();

  // Delete Entity Action
  QAction *deleteAction =
      menu.addAction(QIcon::fromTheme("edit-delete"), "Eliminar");
  connect(deleteAction, &QAction::triggered, this, [this, entItem, ent]() {
    if (QMessageBox::question(this, "Eliminar",
                              "¿Eliminar entidad " + ent->name + "?") ==
        QMessageBox::Yes) {
      m_scene->removeItem(entItem);
      m_data.entities.removeOne(ent);
      refreshEntityTree();
      emit sceneChanged();
      // Note: We don't delete entItem/ent immediately to avoid crashes if
      // they are used later in this event cycle, although here it should be
      // safe.
    }
  });

  menu.addSeparator();
  QAction *propsAction = menu.addAction(QIcon::fromTheme("preferences-system"),
                                        "Propiedades de Entidad...");

  connect(propsAction, &QAction::triggered, this, [this, ent]() {
    QDialog dlg(this);
    dlg.setWindowTitle("Propiedades de Entidad");
    QFormLayout *layout = new QFormLayout(&dlg);

    QLineEdit *nameEdit = new QLineEdit(ent->name);
    QCheckBox *introCheck = new QCheckBox("Es Intro (Bloquea Jugador)");
    introCheck->setChecked(ent->isIntro);

    layout->addRow("Nombre:", nameEdit);
    layout->addRow("", introCheck);

    QLineEdit *textEdit = nullptr;
    QComboBox *fontCombo = nullptr;
    QSpinBox *graphIdSpin = nullptr;

    if (ent->type == ENTITY_TEXT) {
      textEdit = new QLineEdit(ent->text);
      layout->addRow("Texto:", textEdit);

      fontCombo = new QComboBox();
      fontCombo->addItem("System (0)", "");

      QDir rootDir(QFileInfo(m_currentFile).path());
      rootDir.cdUp();
      rootDir.cdUp();
      QStringList fontDirs = {"assets", "fnt", "fonts", "src/assets"};
      for (const QString &dirName : fontDirs) {
        QDir d = rootDir;
        if (d.exists(dirName) && d.cd(dirName)) {
          QDirIterator it(d.path(), QStringList() << "*.fnt" << "*.fnx",
                          QDir::Files, QDirIterator::Subdirectories);
          while (it.hasNext()) {
            QString f = it.next();
            QString rel =
                QDir(QFileInfo(m_currentFile).path()).relativeFilePath(f);
            if (fontCombo->findData(rel) == -1) {
              fontCombo->addItem(QFileInfo(f).fileName() + " [" + dirName + "]",
                                 rel);
            }
          }
        }
      }
      // Fix: convert absolute path from entity to relative path expected by
      // combo
      QString currentRel =
          QDir(QFileInfo(m_currentFile).path()).relativeFilePath(ent->fontFile);
      int idx = fontCombo->findData(currentRel);

      // If relative match fails, maybe it matches absolute? (just in case
      // combo has absolute data)
      if (idx == -1)
        idx = fontCombo->findData(ent->fontFile);

      if (idx >= 0)
        fontCombo->setCurrentIndex(idx);
      layout->addRow("Fuente:", fontCombo);
    } else if (ent->type == ENTITY_SPRITE) {
      graphIdSpin = new QSpinBox();
      graphIdSpin->setRange(0, 9999);
      graphIdSpin->setValue(ent->graphId);
      layout->addRow("Graph ID:", graphIdSpin);
    }

    QDoubleSpinBox *scaleSpin = nullptr;
    QSpinBox *zSpin = nullptr;
    if (ent->type != ENTITY_TEXT) {
      scaleSpin = new QDoubleSpinBox();
      scaleSpin->setRange(0.1, 10.0);
      scaleSpin->setSingleStep(0.1);
      scaleSpin->setValue(ent->scale);
      layout->addRow("Escala:", scaleSpin);

      zSpin = new QSpinBox();
      zSpin->setRange(-9999, 9999);
      zSpin->setValue(ent->z);
      layout->addRow("Z (Orden):", zSpin);
    }

    QDialogButtonBox *btns =
        new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    layout->addRow(btns);
    connect(btns, &QDialogButtonBox::accepted, &dlg, &QDialog::accept);
    connect(btns, &QDialogButtonBox::rejected, &dlg, &QDialog::reject);

    if (dlg.exec() == QDialog::Accepted) {
      ent->name = nameEdit->text();
      ent->isIntro = introCheck->isChecked();
      if (textEdit)
        ent->text = textEdit->text();
      if (fontCombo) {
        QString relPath = fontCombo->currentData().toString();
        if (!relPath.isEmpty()) {
          ent->fontFile =
              QDir(QFileInfo(m_currentFile).path())
                  .cleanPath(
                      QDir(QFileInfo(m_currentFile).path()).filePath(relPath));
        } else {
          ent->fontFile.clear();
        }
      }
      if (graphIdSpin)
        ent->graphId = graphIdSpin->value();
      if (scaleSpin)
        ent->scale = scaleSpin->value();
      if (zSpin)
        ent->z = zSpin->value();

      if (ent->item) {
        ent->item->updateVisuals();
        // Do NOT call syncToData() here, it overwrites our changes with old
        // item values. Instead, push changes TO the item.
        ent->item->setScale(ent->scale);
        ent->item->setZValue(-ent->z);
      }
      refreshEntityTree();
      m_scene->update(); // Force refresh to show Z-order changes immediately
    }
  });

  menu.exec(event->globalPos());
}

void SceneEditor::refreshEntityTree() {
  if (!m_entityTree)
    return;
  m_entityTree->clear();

  // Add Global scene properties as items if set
  if (!m_data.musicFile.isEmpty()) {
    QTreeWidgetItem *musicItem = new QTreeWidgetItem();
    musicItem->setText(0, "Música: " + QFileInfo(m_data.musicFile).fileName());
    musicItem->setText(1, "Audio");
    musicItem->setIcon(0, QIcon::fromTheme("audio-x-generic"));
    musicItem->setData(0, Qt::UserRole, QVariant::fromValue((void *)nullptr));
    m_entityTree->addTopLevelItem(musicItem);
  }

  if (!m_data.backgroundFile.isEmpty()) {
    QTreeWidgetItem *bgItem = new QTreeWidgetItem();
    bgItem->setText(0, "Fondo: " + QFileInfo(m_data.backgroundFile).fileName());
    bgItem->setText(1, "Imagen");
    bgItem->setIcon(0, QIcon::fromTheme("image-x-generic"));
    bgItem->setData(0, Qt::UserRole, QVariant::fromValue((void *)nullptr));
    m_entityTree->addTopLevelItem(bgItem);
  }

  for (SceneEntity *ent : m_data.entities) {
    QTreeWidgetItem *treeItem = new QTreeWidgetItem();
    treeItem->setText(0, ent->name.isEmpty() ? "<Sin nombre>" : ent->name);

    QString typeStr = "Sprite";
    QIcon icon = QIcon::fromTheme(
        "image-x-generic",
        QApplication::style()->standardIcon(QStyle::SP_FileIcon));

    if (ent->type == ENTITY_WORLD3D) {
      typeStr = "Mundo 3D";
      icon = QIcon::fromTheme(
          "applications-games",
          QApplication::style()->standardIcon(QStyle::SP_DesktopIcon));
    } else if (ent->type == ENTITY_TEXT) {
      typeStr = "Texto";
      icon = QIcon::fromTheme(
          "text-x-generic",
          QApplication::style()->standardIcon(QStyle::SP_FileIcon));
    } else if (!ent->script.isEmpty()) {
      icon = QIcon::fromTheme(
          "text-x-script",
          QApplication::style()->standardIcon(QStyle::SP_FileIcon));
    }

    treeItem->setText(1, typeStr);
    treeItem->setIcon(0, icon);
    treeItem->setData(0, Qt::UserRole, QVariant::fromValue((void *)ent));
    m_entityTree->addTopLevelItem(treeItem);
  }
}

// ============================================================================
// Drops & Add Entity
// ============================================================================
// ============================================================================
// SceneEditor Context Menu (HANDLED ABOVE)
// ============================================================================

void SceneEditor::dragEnterEvent(QDragEnterEvent *event) {

  if (event->mimeData()->hasUrls() ||
      event->mimeData()->hasFormat("application/x-raymap-sprite"))
    event->acceptProposedAction();
}
void SceneEditor::dragMoveEvent(QDragMoveEvent *event) {
  event->acceptProposedAction();
}

void SceneEditor::dropEvent(QDropEvent *event) {
  const QMimeData *mime = event->mimeData();
  QPointF pos = mapToScene(event->pos());

  // 1. Handle Internal FPG Sprite Drop
  if (mime->hasFormat("application/x-raymap-sprite")) {
    QByteArray data = mime->data("application/x-raymap-sprite");
    QStringList parts = QString::fromUtf8(data).split("|");
    if (parts.size() == 2) {
      QString fpgPath = parts[0];
      int graphId = parts[1].toInt();
      addEntity(fpgPath, pos, graphId);
      event->acceptProposedAction();
      return;
    }
  }

  // 2. Handle File URI Drops
  if (!mime->hasUrls() || mime->urls().isEmpty())
    return;

  QString filePath = mime->urls().first().toLocalFile();
  QFileInfo info(filePath);
  QString ext = info.suffix().toLower();

  // Auto-import logic for images, FPGs and Audio
  QStringList audioExts = {"wav", "mod", "mid", "ogg", "mp3", "flac", "opus"};
  bool isAudio = audioExts.contains(ext);

  if (ext == "png" || ext == "jpg" || ext == "jpeg" || ext == "bmp" ||
      ext == "fpg" || ext == "map" || isAudio) {
    // Check if file is inside project
    QDir sceneDir(QFileInfo(m_currentFile).path());
    QDir projectDir = sceneDir;
    while (projectDir.cdUp() && !projectDir.exists("src") &&
           !projectDir.isRoot())
      ; // Try to find project root
    if (projectDir.isRoot())
      projectDir = sceneDir; // Fallback

    // If file is NOT in project hierarchy
    if (!filePath.startsWith(projectDir.absolutePath())) {
      QMessageBox::StandardButton reply =
          QMessageBox::question(this, "Importar Asset",
                                "El archivo está fuera del proyecto. ¿Quieres "
                                "copiarlo a la carpeta 'assets'?",
                                QMessageBox::Yes | QMessageBox::No);

      if (reply == QMessageBox::Yes) {
        QString destDir = projectDir.absolutePath() + "/assets";
        if (ext == "png" || ext == "jpg")
          destDir += "/sprites";
        else if (ext == "fpg" || ext == "map")
          destDir += "/fpg";
        else if (isAudio)
          destDir += "/sounds";

        QDir().mkpath(destDir);
        QString destPath = destDir + "/" + info.fileName();

        if (QFile::exists(destPath)) {
          // Check if overwrite?
          if (QMessageBox::warning(this, "Sobreescribir",
                                   "El archivo ya existe. ¿Sobreescribir?",
                                   QMessageBox::Yes | QMessageBox::No) !=
              QMessageBox::Yes) {
            return;
          }
          QFile::remove(destPath);
        }

        if (QFile::copy(filePath, destPath)) {
          filePath = destPath; // Use new path
        }
      }
    }

    if (isAudio) {
      QMessageBox::StandardButton loopReply = QMessageBox::question(
          this, "Música de Escena",
          "¿Quieres que la música se reproduzca en bucle (loop)?",
          QMessageBox::Yes | QMessageBox::No);
      m_data.musicFile = filePath;
      m_data.musicLoop = (loopReply == QMessageBox::Yes);
      QMessageBox::information(this, "Música",
                               "Música asignada: " + info.fileName());
      refreshEntityTree();
      event->acceptProposedAction();
      return;
    }
  }

  if (ext == "fnt" || ext == "fnx") {
    bool ok;
    QString txt = QInputDialog::getText(this, "Nuevo Texto",
                                        "Texto Inicial:", QLineEdit::Normal,
                                        "Hola Mundo", &ok);
    if (ok)
      addEntity(filePath, pos, 0, txt);
  } else if (ext == "fpg") {
    bool ok;
    int id = QInputDialog::getInt(this, "Seleccionar Gráfico FPG", "Código:", 1,
                                  1, 999, 1, &ok);
    if (ok)
      addEntity(filePath, pos, id);
  } else if (ext == "png" || ext == "jpg" || ext == "jpeg" || ext == "bmp") {
    addEntity(filePath, pos, 0);
  } else if (ext == "raymap") {
    addEntity(filePath, pos, 0);
  } else {
    QMessageBox::warning(this, "Error", "Formato no soportado.");
  }
  event->acceptProposedAction();
}

void SceneEditor::addEntity(const QString &filePath, const QPointF &pos,
                            int graphId, const QString &text) {
  SceneEntity *ent = new SceneEntity();
  ent->x = pos.x();
  ent->y = pos.y();

  if (!text.isEmpty()) {
    ent->type = ENTITY_TEXT;
    ent->name = "Label";
    ent->text = text;
    ent->fontFile = filePath;
    // Assume default font ID logic later
  } else if (QFileInfo(filePath).suffix().toLower() == "raymap") {
    ent->type = ENTITY_WORLD3D;
    ent->name = "World3D_" + QFileInfo(filePath).baseName();
    ent->sourceFile = filePath;
  } else {
    ent->type = ENTITY_SPRITE;
    ent->name = QFileInfo(filePath).baseName();
    ent->sourceFile = filePath;
    ent->graphId = graphId;
  }

  SceneEntityItem *item = new SceneEntityItem(ent);
  m_scene->addItem(item);
  ent->item = item;

  m_data.entities.append(ent);

  m_scene->clearSelection();
  item->setSelected(true);
  emit sceneChanged();
}
