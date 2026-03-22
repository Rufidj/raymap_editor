#include "behaviornodeeditor.h"
#include <QDialogButtonBox>
#include <QDir>
#include <QFile>
#include <QFileDialog>
#include <QGraphicsScene>
#include <QGraphicsSceneContextMenuEvent>
#include <QGraphicsSceneMouseEvent>
#include <QGraphicsView>
#include <QInputDialog>
#include <QMap>
#include <QMenu>
#include <QPainter>
#include <QPen>
#include <QPointF>
#include <QRectF>
#include <QStyleOptionGraphicsItem>
#include <QTextStream>
#include <QVBoxLayout>
#include <QtGlobal>
#include <QMessageBox>
#include <QJsonDocument>
#include <QPushButton>

#include <QWheelEvent>

/* ============================================================================
   PIN ITEM
   ============================================================================
 */
BehaviorPinItem::BehaviorPinItem(NodePinData *data, BehaviorNodeItem *parent)
    : QGraphicsEllipseItem(-6, -6, 12, 12, parent), m_data(data),
      m_node(parent) {
  setBrush(m_data->isExecution ? Qt::white : Qt::cyan);
  setPen(QPen(Qt::black, 1));
  setAcceptHoverEvents(true);
}

QRectF BehaviorPinItem::boundingRect() const {
  return QRectF(-12, -12, 24, 24); // Larger hit area
}

QPainterPath BehaviorPinItem::shape() const {
  QPainterPath path;
  path.addEllipse(boundingRect());
  return path;
}

QPointF BehaviorPinItem::connectionPoint() const { return scenePos(); }

void BehaviorPinItem::paint(QPainter *painter,
                            const QStyleOptionGraphicsItem *option,
                            QWidget *widget) {
  painter->setRenderHint(QPainter::Antialiasing);

  if (m_data->isExecution) {
    // Draw triangle for execution pins
    QPolygonF poly;
    poly << QPointF(-5, -6) << QPointF(-5, 6) << QPointF(6, 0);
    painter->setBrush(!m_data->linkedPinIds.isEmpty() ? QBrush(Qt::white)
                                                      : QBrush(Qt::NoBrush));
    painter->setPen(QPen(Qt::white, 2));
    painter->drawPolygon(poly);
  } else {
    // Draw circle for data pins
    painter->setBrush(!m_data->linkedPinIds.isEmpty() ? brush()
                                                      : QBrush(Qt::NoBrush));
    painter->setPen(QPen(brush().color(), 2));
    painter->drawEllipse(-5, -5, 10, 10);
  }
}

void BehaviorPinItem::mouseDoubleClickEvent(QGraphicsSceneMouseEvent *event) {
  if (!m_data->isExecution) {
    bool ok = false;
    QString text;

    if (m_data->name.contains("File", Qt::CaseInsensitive) ||
        m_data->name.contains("Asset", Qt::CaseInsensitive)) {

      BehaviorNodeScene *scene =
          m_node->scene() ? qobject_cast<BehaviorNodeScene *>(m_node->scene())
                          : nullptr;
      QString startPath = scene ? scene->projectPath() : QDir::currentPath();
      if (!startPath.isEmpty())
        startPath += "/assets";

      text = QFileDialog::getOpenFileName(
          nullptr, QObject::tr("Seleccionar Archivo"), startPath,
          QObject::tr("Todos los archivos (*)"));
      if (!text.isEmpty()) {
        QString projPath = scene ? scene->projectPath() : QDir::currentPath();
        QDir projectDir(projPath);
        text = projectDir.relativeFilePath(text);
        if (text.startsWith("./"))
          text = text.mid(2);

        // Wrap in quotes to avoid "Unknown identifier .." error in BennuGD
        if (!text.startsWith("\""))
          text = "\"" + text + "\"";
        ok = true;
      }
    } else {
      text = QInputDialog::getText(
          nullptr, QObject::tr("Editar Valor"),
          QObject::tr("Introduce el valor para %1:").arg(m_data->name),
          QLineEdit::Normal, m_data->value, &ok);
    }

    if (ok) {
      m_data->value = text;
      update();
      m_node->update();
    }
  }
  QGraphicsItem::mouseDoubleClickEvent(event);
}

/* ============================================================================
   NODE ITEM
   ============================================================================
 */
BehaviorNodeItem::BehaviorNodeItem(NodeData *data)
    : m_data(data), m_width(160), m_height(100) {
  setFlag(ItemIsMovable);
  setFlag(ItemIsSelectable);
  setFlag(ItemSendsGeometryChanges);
  setPos(data->x, data->y);

  // Add pins from data
  for (int i = 0; i < m_data->pins.size(); ++i) {
    BehaviorPinItem *pinItem = new BehaviorPinItem(&m_data->pins[i], this);
    float yPos = 30 + i * 20;
    if (m_data->pins[i].isInput) {
      pinItem->setPos(0, yPos);
    } else {
      pinItem->setPos(m_width, yPos);
    }
    m_pins.append(pinItem);
  }

  m_height = qMax(60.0, 30.0 + m_data->pins.size() * 20.0);
}

QRectF BehaviorNodeItem::boundingRect() const {
  return QRectF(0, 0, m_width, m_height);
}

void BehaviorNodeItem::paint(QPainter *painter,
                             const QStyleOptionGraphicsItem *option,
                             QWidget *widget) {
  painter->setRenderHint(QPainter::Antialiasing);

  // Background
  QColor bgColor(45, 45, 45, 230);
  if (option->state & QStyle::State_Selected)
    bgColor = QColor(60, 60, 60, 230);

  painter->setBrush(bgColor);
  painter->setPen(
      QPen(option->state & QStyle::State_Selected ? Qt::yellow : Qt::black, 1));
  painter->drawRoundedRect(boundingRect(), 8, 8);

  // Header
  QColor headerColor = m_data->headerColor;
  if (headerColor == QColor(70, 70, 70)) {
    if (m_data->type.startsWith("event"))
      headerColor = QColor(150, 50, 50);
    else if (m_data->type.startsWith("action"))
      headerColor = QColor(50, 80, 150);
    else if (m_data->type.startsWith("math"))
      headerColor = QColor(100, 50, 150);
    else if (m_data->type.startsWith("logic"))
      headerColor = QColor(50, 150, 80);
  }

  painter->setBrush(headerColor);
  painter->setPen(Qt::NoPen);
  painter->drawRoundedRect(0, 0, m_width, 24, 8, 8);
  painter->drawRect(0, 12, m_width, 12); // Square bottom part of header

  // Title
  painter->setPen(Qt::white);
  QFont font = painter->font();
  font.setBold(true);
  painter->setFont(font);
  painter->drawText(QRectF(8, 0, m_width - 16, 24), Qt::AlignVCenter,
                    m_data->type.toUpper());

  // Pin Names
  font.setBold(false);
  font.setPointSize(8);
  painter->setFont(font);
  for (int i = 0; i < m_pins.size(); ++i) {
    NodePinData *p = m_pins[i]->data();
    float yPos = 30 + i * 20;
    if (p->isInput) {
      QString label = p->name;
      if (!p->isExecution && p->linkedPinIds.isEmpty() && !p->value.isEmpty()) {
        label += " [" + p->value + "]";
      }
      painter->drawText(QRectF(12, yPos - 10, m_width - 24, 20),
                        Qt::AlignVCenter | Qt::AlignLeft, label);
    } else {
      painter->drawText(QRectF(12, yPos - 10, m_width - 24, 20),
                        Qt::AlignVCenter | Qt::AlignRight, p->name);
    }
  }
}

void BehaviorNodeItem::updateDataPointer(NodeData *data) {
  m_data = data;
  for (int i = 0; i < m_pins.size(); ++i) {
    m_pins[i]->updateDataPointer(&m_data->pins[i]);
  }
}

void BehaviorNodeItem::mouseMoveEvent(QGraphicsSceneMouseEvent *event) {
  QGraphicsItem::mouseMoveEvent(event);
  m_data->x = pos().x();
  m_data->y = pos().y();
}

QVariant BehaviorNodeItem::itemChange(GraphicsItemChange change,
                                      const QVariant &value) {
  if (change == ItemPositionChange && scene()) {
    static_cast<BehaviorNodeScene *>(scene())->updateLinks();
  }
  return QGraphicsItem::itemChange(change, value);
}

/* ============================================================================
   LINK ITEM
   ============================================================================
 */
BehaviorLinkItem::BehaviorLinkItem(BehaviorPinItem *start, BehaviorPinItem *end)
    : m_start(start), m_end(end) {
  setPen(QPen(m_start->data()->isExecution ? Qt::white : Qt::cyan, 2));
  setZValue(-1);
  // Make cables easier to click
  setFlag(QGraphicsItem::ItemIsSelectable, true);
  updatePath();
}

void BehaviorLinkItem::updatePath() {
  QPointF p1 = m_start->connectionPoint();
  QPointF p2 = m_end->connectionPoint();

  QPainterPath path;
  path.moveTo(p1);

  float dx = qAbs(p2.x() - p1.x());
  float offset = qMin(dx / 2.0f, 100.0f);

  path.cubicTo(p1 + QPointF(offset, 0), p2 - QPointF(offset, 0), p2);
  setPath(path);
}

/* ============================================================================
   BEHAVIOR NODE SCENE
   ============================================================================
 */
BehaviorNodeScene::BehaviorNodeScene(BehaviorGraph &graph,
                                     const QString &projectPath,
                                     QObject *parent)
    : QGraphicsScene(parent), m_graph(graph), m_projectPath(projectPath),
      m_dragStartPin(nullptr), m_tempLink(nullptr) {
  setBackgroundBrush(QBrush(QColor(30, 30, 30)));

  // Grid
  for (int i = 0; i < m_graph.nodes.size(); ++i) {
    createNodeItem(&m_graph.nodes[i]);
  }
  updateLinks();
}

void BehaviorNodeScene::createNodeItem(NodeData *data) {
  BehaviorNodeItem *item = new BehaviorNodeItem(data);
  addItem(item);
  m_nodeItems.append(item);
}

void BehaviorNodeScene::refreshDataPointers() {
  for (int i = 0; i < m_nodeItems.size(); ++i) {
    m_nodeItems[i]->updateDataPointer(&m_graph.nodes[i]);
  }
}

void BehaviorNodeScene::deleteNode(BehaviorNodeItem *item) {
  if (!item)
    return;

  // 1. CRITICAL: Clear all visual links FIRST. 
  // Links hold pointers to pins. If we delete the node/pins first, links become dangling.
  qDeleteAll(m_linkItems);
  m_linkItems.clear();

  NodeData *data = item->data();

  // 2. Remove from graph data
  for (int i = 0; i < m_graph.nodes.size(); ++i) {
    if (m_graph.nodes[i].nodeId == data->nodeId) {
      m_graph.nodes.removeAt(i);
      break;
    }
  }

  // 3. Remove visual node item
  m_nodeItems.removeAll(item);
  removeItem(item);
  delete item; // Pins are deleted here too (as children)

  // 4. CRITICAL: Re-sync all remaining visual nodes with the shifted QVector
  refreshDataPointers();

  // 5. Rebuild links for remaining nodes
  updateLinks();
}

void BehaviorNodeScene::removePinLinks(int pinId) {
  for (NodeData &node : m_graph.nodes) {
    for (NodePinData &pin : node.pins) {
      pin.linkedPinIds.removeOne(pinId);
    }
  }
}

void BehaviorNodeScene::addNode(const QString &type, const QPointF &pos) {
  NodeData data;
  data.nodeId = m_graph.nextNodeId++;
  data.type = type;
  data.x = pos.x();
  data.y = pos.y();

  // Template pins based on type
  if (type == "event_start") {
    NodePinData p;
    p.pinId = m_graph.nextPinId++;
    p.name = "Out";
    p.isInput = false;
    p.isExecution = true;
    data.pins.append(p);
  } else if (type == "event_collision") {
    NodePinData pOut, pType;
    pOut.pinId = m_graph.nextPinId++;
    pOut.name = "Out";
    pOut.isInput = false;
    pOut.isExecution = true;
    pType.pinId = m_graph.nextPinId++;
    pType.name = "Target";
    pType.isInput = true;
    pType.isExecution = false;
    pType.value = "TYPE_PLAYER";
    data.pins.append(pOut);
    data.pins.append(pType);
  } else if (type == "event_death") {
    NodePinData p;
    p.pinId = m_graph.nextPinId++;
    p.name = "Out";
    p.isInput = false;
    p.isExecution = true;
    data.pins.append(p);
  } else if (type == "event_damage") {
    NodePinData p;
    p.pinId = m_graph.nextPinId++;
    p.name = "Out";
    p.isInput = false;
    p.isExecution = true;
    data.pins.append(p);
  } else if (type == "event_player_death") {
    NodePinData p;
    p.pinId = m_graph.nextPinId++;
    p.name = "Out";
    p.isInput = false;
    p.isExecution = true;
    data.pins.append(p);
  } else if (type == "logic_if") {
    NodePinData pIn, pTrue, pFalse, pCond;
    pIn.pinId = m_graph.nextPinId++;
    pIn.name = "In";
    pIn.isInput = true;
    pIn.isExecution = true;
    pTrue.pinId = m_graph.nextPinId++;
    pTrue.name = "True";
    pTrue.isInput = false;
    pTrue.isExecution = true;
    pFalse.pinId = m_graph.nextPinId++;
    pFalse.name = "False";
    pFalse.isInput = false;
    pFalse.isExecution = true;
    pCond.pinId = m_graph.nextPinId++;
    pCond.name = "Cond";
    pCond.isInput = true;
    pCond.isExecution = false;
    data.pins.append(pIn);
    data.pins.append(pTrue);
    data.pins.append(pFalse);
    data.pins.append(pCond);
  } else if (type == "action_say") {
    NodePinData pIn, pOut, pText;
    pIn.pinId = m_graph.nextPinId++;
    pIn.name = "In";
    pIn.isInput = true;
    pIn.isExecution = true;
    pOut.pinId = m_graph.nextPinId++;
    pOut.name = "Out";
    pOut.isInput = false;
    pOut.isExecution = true;
    pText.pinId = m_graph.nextPinId++;
    pText.name = "Text";
    pText.isInput = true;
    pText.isExecution = false;
    pText.value = "¡Hola!";
    data.pins.append(pIn);
    data.pins.append(pOut);
    data.pins.append(pText);
  } else if (type == "action_moveto") {
    NodePinData pIn, pOut, pX, pY;
    pIn.pinId = m_graph.nextPinId++;
    pIn.name = "In";
    pIn.isInput = true;
    pIn.isExecution = true;
    pOut.pinId = m_graph.nextPinId++;
    pOut.name = "Out";
    pOut.isInput = false;
    pOut.isExecution = true;
    pX.pinId = m_graph.nextPinId++;
    pX.name = "X";
    pX.isInput = true;
    pX.isExecution = false;
    pX.value = "0";
    pY.pinId = m_graph.nextPinId++;
    pY.name = "Y";
    pY.isInput = true;
    pY.isExecution = false;
    pY.value = "0";
    data.pins.append(pIn);
    data.pins.append(pOut);
    data.pins.append(pX);
    data.pins.append(pY);
  } else if (type == "action_campath") {
    NodePinData pIn, pOut, pFile;
    pIn.pinId = m_graph.nextPinId++;
    pIn.name = "In";
    pIn.isInput = true;
    pIn.isExecution = true;
    pOut.pinId = m_graph.nextPinId++;
    pOut.name = "Out";
    pOut.isInput = false;
    pOut.isExecution = true;
    pFile.pinId = m_graph.nextPinId++;
    pFile.name = "File";
    pFile.isInput = true;
    pFile.isExecution = false;
    pFile.value = "assets/cam/intro.cam";
    data.pins.append(pIn);
    data.pins.append(pOut);
    data.pins.append(pFile);
  } else if (type == "action_sound") {
    NodePinData pIn, pOut, pFile, pVolume, pLoops;
    pIn.pinId = m_graph.nextPinId++;
    pIn.name = "In";
    pIn.isInput = true;
    pIn.isExecution = true;
    pOut.pinId = m_graph.nextPinId++;
    pOut.name = "Out";
    pOut.isInput = false;
    pOut.isExecution = true;
    pFile.pinId = m_graph.nextPinId++;
    pFile.name = "File";
    pFile.isInput = true;
    pFile.isExecution = false;
    pFile.value = "assets/sfx/engine.wav";
    pVolume.pinId = m_graph.nextPinId++;
    pVolume.name = "Volume";
    pVolume.isInput = true;
    pVolume.isExecution = false;
    pVolume.value = "128";
    pLoops.pinId = m_graph.nextPinId++;
    pLoops.name = "Loops";
    pLoops.isInput = true;
    pLoops.isExecution = false;
    pLoops.value = "0"; // 0 = once, -1 = infinite
    data.pins.append(pIn);
    data.pins.append(pOut);
    data.pins.append(pFile);
    data.pins.append(pVolume);
    data.pins.append(pLoops);
  } else if (type == "action_shake_camera") {
    NodePinData pIn, pOut, pIntensity, pDuration;
    pIn.pinId = m_graph.nextPinId++;
    pIn.name = "In";
    pIn.isInput = true;
    pIn.isExecution = true;
    pOut.pinId = m_graph.nextPinId++;
    pOut.name = "Out";
    pOut.isInput = false;
    pOut.isExecution = true;
    pIntensity.pinId = m_graph.nextPinId++;
    pIntensity.name = "Intensity";
    pIntensity.isInput = true;
    pIntensity.isExecution = false;
    pIntensity.value = "5.0";
    pDuration.pinId = m_graph.nextPinId++;
    pDuration.name = "Duration";
    pDuration.isInput = true;
    pDuration.isExecution = false;
    pDuration.value = "0.5";
    data.pins.append(pIn);
    data.pins.append(pOut);
    data.pins.append(pIntensity);
    data.pins.append(pDuration);
  } else if (type == "action_kill") {
    NodePinData pIn, pTarget;
    pIn.pinId = m_graph.nextPinId++;
    pIn.name = "In";
    pIn.isInput = true;
    pIn.isExecution = true;
    pTarget.pinId = m_graph.nextPinId++;
    pTarget.name = "Target";
    pTarget.isInput = true;
    pTarget.isExecution = false;
    pTarget.value = "id"; // Default to current process
    data.pins.append(pIn);
    data.pins.append(pTarget);
  } else if (type == "action_spawn_billboard") {
    NodePinData pIn, pOut, pFile, pGraph, pGraphEnd, pSpeed;
    pIn.pinId = m_graph.nextPinId++;
    pIn.name = "In";
    pIn.isInput = true;
    pIn.isExecution = true;
    pOut.pinId = m_graph.nextPinId++;
    pOut.name = "Out";
    pOut.isInput = false;
    pOut.isExecution = true;
    pFile.pinId = m_graph.nextPinId++;
    pFile.name = "File";
    pFile.isInput = true;
    pFile.isExecution = false;
    pFile.value = "0";
    pGraph.pinId = m_graph.nextPinId++;
    pGraph.name = "Graph";
    pGraph.isInput = true;
    pGraph.isExecution = false;
    pGraph.value = "1";
    pGraphEnd.pinId = m_graph.nextPinId++;
    pGraphEnd.name = "GraphEnd";
    pGraphEnd.isInput = true;
    pGraphEnd.isExecution = false;
    pGraphEnd.value = "1";
    pSpeed.pinId = m_graph.nextPinId++;
    pSpeed.name = "Speed";
    pSpeed.isInput = true;
    pSpeed.isExecution = false;
    pSpeed.value = "0.2";
    NodePinData pScale;
    pScale.pinId = m_graph.nextPinId++;
    pScale.name = "Scale";
    pScale.isInput = true;
    pScale.isExecution = false;
    pScale.value = "8.0";
    NodePinData pRepeat;
    pRepeat.pinId = m_graph.nextPinId++;
    pRepeat.name = "Loop (1/0)";
    pRepeat.isInput = true;
    pRepeat.isExecution = false;
    pRepeat.value = "0";
    data.pins.append(pIn);
    data.pins.append(pOut);
    data.pins.append(pFile);
    data.pins.append(pGraph);
    data.pins.append(pGraphEnd);
    data.pins.append(pSpeed);
    data.pins.append(pScale);
    data.pins.append(pRepeat);
  } else if (type == "action_damage") {
    NodePinData pIn, pOut, pVal;
    pIn.pinId = m_graph.nextPinId++;
    pIn.name = "In";
    pIn.isInput = true;
    pIn.isExecution = true;
    pOut.pinId = m_graph.nextPinId++;
    pOut.name = "Out";
    pOut.isInput = false;
    pOut.isExecution = true;
    pVal.pinId = m_graph.nextPinId++;
    pVal.name = "Damage";
    pVal.isInput = true;
    pVal.isExecution = false;
    pVal.value = "25";
    NodePinData pTarget;
    pTarget.pinId = m_graph.nextPinId++;
    pTarget.name = "Target";
    pTarget.isInput = true;
    pTarget.isExecution = false;
    pTarget.value = "TYPE_PLAYER";
    NodePinData pHitFrame;
    pHitFrame.pinId = m_graph.nextPinId++;
    pHitFrame.name = "Hit Frame";
    pHitFrame.isInput = true;
    pHitFrame.isExecution = false;
    pHitFrame.value = "0"; // 0 = last frame of attack anim (current_anim_end)
    data.pins.append(pIn);
    data.pins.append(pOut);
    data.pins.append(pVal);
    data.pins.append(pTarget);
    data.pins.append(pHitFrame);
  } else if (type == "logic_key") {
    NodePinData pKey, pOut;
    pKey.pinId = m_graph.nextPinId++;
    pKey.name = "Key";
    pKey.isInput = true;
    pKey.isExecution = false;
    pKey.value = "SPACE";
    pOut.pinId = m_graph.nextPinId++;
    pOut.name = "Pressed";
    pOut.isInput = false;
    pOut.isExecution = false;
    data.pins.append(pKey);
    data.pins.append(pOut);
  } else if (type == "action_die") {
    NodePinData pIn, pStart, pEnd, pBillboard;
    pIn.pinId = m_graph.nextPinId++;
    pIn.name = "In";
    pIn.isInput = true;
    pIn.isExecution = true;
    pStart.pinId = m_graph.nextPinId++;
    pStart.name = "Start Frame";
    pStart.isInput = true;
    pStart.isExecution = false;
    pStart.value = "30";
    pEnd.pinId = m_graph.nextPinId++;
    pEnd.name = "End Frame";
    pEnd.isInput = true;
    pEnd.isExecution = false;
    pEnd.value = "45";
    pBillboard.pinId = m_graph.nextPinId++;
    pBillboard.name = "Billboard Type";
    pBillboard.isInput = true;
    pBillboard.isExecution = false;
    pBillboard.value = "2";
    data.pins.append(pIn);
    data.pins.append(pStart);
    data.pins.append(pEnd);
    data.pins.append(pBillboard);
  } else if (type == "action_set_health") {
    NodePinData pIn, pOut, pVal;
    pIn.pinId = m_graph.nextPinId++;
    pIn.name = "In";
    pIn.isInput = true;
    pIn.isExecution = true;
    pOut.pinId = m_graph.nextPinId++;
    pOut.name = "Out";
    pOut.isInput = false;
    pOut.isExecution = true;
    pVal.pinId = m_graph.nextPinId++;
    pVal.name = "Health";
    pVal.isInput = true;
    pVal.isExecution = false;
    pVal.value = "100";
    NodePinData pTarget;
    pTarget.pinId = m_graph.nextPinId++;
    pTarget.name = "Target";
    pTarget.isInput = true;
    pTarget.isExecution = false;
    pTarget.value = "Self";
    data.pins.append(pIn);
    data.pins.append(pOut);
    data.pins.append(pVal);
    data.pins.append(pTarget);
  } else if (type == "action_setvar") {
    NodePinData pIn, pOut, pVar, pVal;
    pIn.pinId = m_graph.nextPinId++;
    pIn.name = "In";
    pIn.isInput = true;
    pIn.isExecution = true;
    pOut.pinId = m_graph.nextPinId++;
    pOut.name = "Out";
    pOut.isInput = false;
    pOut.isExecution = true;
    pVar.pinId = m_graph.nextPinId++;
    pVar.name = "Var";
    pVar.isInput = true;
    pVar.isExecution = false;
    pVar.value = "vida";
    pVal.pinId = m_graph.nextPinId++;
    pVal.name = "Value";
    pVal.isInput = true;
    pVal.isExecution = false;
    pVal.value = "vida - 10";
    data.pins.append(pIn);
    data.pins.append(pOut);
    data.pins.append(pVar);
    data.pins.append(pVal);
  } else if (type == "logic_compare") {
    NodePinData pA, pOp, pB, pOut;
    pA.pinId = m_graph.nextPinId++;
    pA.name = "A";
    pA.isInput = true;
    pA.isExecution = false;
    pA.value = "vida";
    pOp.pinId = m_graph.nextPinId++;
    pOp.name = "Op";
    pOp.isInput = true;
    pOp.isExecution = false;
    pOp.value = "<=";
    pB.pinId = m_graph.nextPinId++;
    pB.name = "B";
    pB.isInput = true;
    pB.isExecution = false;
    pB.value = "0";
    pOut.pinId = m_graph.nextPinId++;
    pOut.name = "Result";
    pOut.isInput = false;
    pOut.isExecution = false;
    data.pins.append(pA);
    data.pins.append(pOp);
    data.pins.append(pB);
    data.pins.append(pOut);
  } else if (type == "math_op") {
    NodePinData pA, pOp, pB, pOut;
    pA.pinId = m_graph.nextPinId++;
    pA.name = "A";
    pA.isInput = true;
    pA.isExecution = false;
    pA.value = "vida";
    pOp.pinId = m_graph.nextPinId++;
    pOp.name = "Op";
    pOp.isInput = true;
    pOp.isExecution = false;
    pOp.value = "-";
    pB.pinId = m_graph.nextPinId++;
    pB.name = "B";
    pB.isInput = true;
    pB.isExecution = false;
    pB.value = "10";
    pOut.pinId = m_graph.nextPinId++;
    pOut.name = "Result";
    pOut.isInput = false;
    pOut.isExecution = false;
    data.pins.append(pA);
    data.pins.append(pOp);
    data.pins.append(pB);
    data.pins.append(pOut);
  } else if (type == "math_dist") {
    NodePinData pA, pB, pOut;
    pA.pinId = m_graph.nextPinId++;
    pA.name = "Sprite A";
    pA.isInput = true;
    pA.isExecution = false;
    pA.value = "id";
    pB.pinId = m_graph.nextPinId++;
    pB.name = "Sprite B";
    pB.isInput = true;
    pB.isExecution = false;
    pB.value = "get_id(type player)";
    pOut.pinId = m_graph.nextPinId++;
    pOut.name = "Dist";
    pOut.isInput = false;
    pOut.isExecution = false;
    data.pins.append(pA);
    data.pins.append(pB);
    data.pins.append(pOut);
  } else if (type == "math_camera_dist") {
    NodePinData pA, pOut;
    pA.pinId = m_graph.nextPinId++;
    pA.name = "Sprite";
    pA.isInput = true;
    pA.isExecution = false;
    pA.value = "id";
    pOut.pinId = m_graph.nextPinId++;
    pOut.name = "Dist";
    pOut.isInput = false;
    pOut.isExecution = false;
    data.pins.append(pA);
    data.pins.append(pOut);
  } else if (type == "math_point_dist") {
    NodePinData pX1, pY1, pZ1, pX2, pY2, pZ2, pOut;
    pX1.pinId = m_graph.nextPinId++;
    pX1.name = "X1";
    pX1.isInput = true;
    pX1.value = "0";
    pY1.pinId = m_graph.nextPinId++;
    pY1.name = "Y1";
    pY1.isInput = true;
    pY1.value = "0";
    pZ1.pinId = m_graph.nextPinId++;
    pZ1.name = "Z1";
    pZ1.isInput = true;
    pZ1.value = "0";
    pX2.pinId = m_graph.nextPinId++;
    pX2.name = "X2";
    pX2.isInput = true;
    pX2.value = "world_x";
    pY2.pinId = m_graph.nextPinId++;
    pY2.name = "Y2";
    pY2.isInput = true;
    pY2.value = "world_y";
    pZ2.pinId = m_graph.nextPinId++;
    pZ2.name = "Z2";
    pZ2.isInput = true;
    pZ2.value = "world_z";
    pOut.pinId = m_graph.nextPinId++;
    pOut.name = "Dist";
    pOut.isInput = false;
    data.pins.append(pX1);
    data.pins.append(pY1);
    data.pins.append(pZ1);
    data.pins.append(pX2);
    data.pins.append(pY2);
    data.pins.append(pZ2);
    data.pins.append(pOut);
  } else if (type == "math_angle") {
    NodePinData pA, pB, pOut;
    pA.pinId = m_graph.nextPinId++;
    pA.name = "Origin";
    pA.isInput = true;
    pA.isExecution = false;
    pA.value = "id";
    pB.pinId = m_graph.nextPinId++;
    pB.name = "Target";
    pB.isInput = true;
    pB.isExecution = false;
    pB.value = "get_id(type player)";
    pOut.pinId = m_graph.nextPinId++;
    pOut.name = "Angle";
    pOut.isInput = false;
    pOut.isExecution = false;
    data.pins.append(pA);
    data.pins.append(pB);
    data.pins.append(pOut);
  } else if (type == "math_camera_angle") {
    NodePinData pA, pOut;
    pA.pinId = m_graph.nextPinId++;
    pA.name = "Sprite";
    pA.isInput = true;
    pA.isExecution = false;
    pA.value = "id";
    pOut.pinId = m_graph.nextPinId++;
    pOut.name = "Angle";
    pOut.isInput = false;
    pOut.isExecution = false;
    data.pins.append(pA);
    data.pins.append(pOut);
  } else if (type == "action_set_ui_text") {
    NodePinData pIn, pOut, pTarget, pText;
    pIn.pinId = m_graph.nextPinId++;
    pIn.name = "In";
    pIn.isInput = true;
    pIn.isExecution = true;
    pOut.pinId = m_graph.nextPinId++;
    pOut.name = "Out";
    pOut.isInput = false;
    pOut.isExecution = true;
    pTarget.pinId = m_graph.nextPinId++;
    pTarget.name = "Entity";
    pTarget.isInput = true;
    pTarget.isExecution = false;
    pTarget.value = "HUD_Message";
    pText.pinId = m_graph.nextPinId++;
    pText.name = "Text";
    pText.isInput = true;
    pText.isExecution = false;
    pText.value = "Hello World";
    data.pins.append(pIn);
    data.pins.append(pOut);
    data.pins.append(pTarget);
    data.pins.append(pText);
  } else if (type == "event_update") {
    NodePinData pOut;
    pOut.pinId = m_graph.nextPinId++;
    pOut.name = "Each Frame";
    pOut.isInput = false;
    pOut.isExecution = true;
    data.pins.append(pOut);
    data.headerColor = QColor(200, 50, 50); // Red for events
  } else if (type == "action_car_engine") {
    NodePinData pIn, pOut, pFile, pMinVol, pMaxVol;
    pIn.pinId = m_graph.nextPinId++;
    pIn.name = "In";
    pIn.isInput = true;
    pIn.isExecution = true;
    pOut.pinId = m_graph.nextPinId++;
    pOut.name = "Out";
    pOut.isInput = false;
    pOut.isExecution = true;
    pFile.pinId = m_graph.nextPinId++;
    pFile.name = "File";
    pFile.isInput = true;
    pFile.value = "assets/sfx/engine.wav";
    pMinVol.pinId = m_graph.nextPinId++;
    pMinVol.name = "Idle Volume";
    pMinVol.isInput = true;
    pMinVol.value = "40";
    pMaxVol.pinId = m_graph.nextPinId++;
    pMaxVol.name = "Max Volume";
    pMaxVol.isInput = true;
    pMaxVol.value = "128";
    data.pins.append(pIn);
    data.pins.append(pOut);
    data.pins.append(pFile);
    data.pins.append(pMinVol);
    data.pins.append(pMaxVol);
  } else if (type == "action_set_animation") {
    NodePinData pIn, pOut, pStart, pEnd, pSpeed;
    pIn.pinId = m_graph.nextPinId++;
    pIn.name = "In";
    pIn.isInput = true;
    pIn.isExecution = true;
    pOut.pinId = m_graph.nextPinId++;
    pOut.name = "Out";
    pOut.isInput = false;
    pOut.isExecution = true;
    pStart.pinId = m_graph.nextPinId++;
    pStart.name = "Start Frame";
    pStart.isInput = true;
    pStart.value = "0";
    pEnd.pinId = m_graph.nextPinId++;
    pEnd.name = "End Frame";
    pEnd.isInput = true;
    pEnd.value = "10";
    pSpeed.pinId = m_graph.nextPinId++;
    pSpeed.name = "Speed";
    pSpeed.isInput = true;
    pSpeed.value = "1.0";
    data.pins.append(pIn);
    data.pins.append(pOut);
    data.pins.append(pStart);
    data.pins.append(pEnd);
    data.pins.append(pSpeed);
  } else if (type == "action_set_glb_animation") {
    NodePinData pIn, pOut, pIdx, pSpeed;
    pIn.pinId = m_graph.nextPinId++;
    pIn.name = "In";
    pIn.isInput = true;
    pIn.isExecution = true;
    pOut.pinId = m_graph.nextPinId++;
    pOut.name = "Out";
    pOut.isInput = false;
    pOut.isExecution = true;
    pIdx.pinId = m_graph.nextPinId++;
    pIdx.name = "Anim Index";
    pIdx.isInput = true;
    pIdx.value = "0";
    pSpeed.pinId = m_graph.nextPinId++;
    pSpeed.name = "Speed";
    pSpeed.isInput = true;
    pSpeed.value = "1.0";
    data.pins.append(pIn);
    data.pins.append(pOut);
    data.pins.append(pIdx);
    data.pins.append(pSpeed);
  } else if (type == "action_set_path_active") {
    NodePinData pIn, pOut, pActive;
    pIn.pinId = m_graph.nextPinId++;
    pIn.name = "In";
    pIn.isInput = true;
    pIn.isExecution = true;
    pOut.pinId = m_graph.nextPinId++;
    pOut.name = "Out";
    pOut.isInput = false;
    pOut.isExecution = true;
    pActive.pinId = m_graph.nextPinId++;
    pActive.name = "Active (0/1)";
    pActive.isInput = true;
    pActive.value = "0";
    data.pins.append(pIn);
    data.pins.append(pOut);
    data.pins.append(pActive);
  } else if (type == "action_npc_chase") {
    NodePinData pIn, pOut, pTarget, pSpeed;
    pIn.pinId = m_graph.nextPinId++;
    pIn.name = "In";
    pIn.isInput = true;
    pIn.isExecution = true;
    pOut.pinId = m_graph.nextPinId++;
    pOut.name = "Out";
    pOut.isInput = false;
    pOut.isExecution = true;
    pTarget.pinId = m_graph.nextPinId++;
    pTarget.name = "Target ID";
    pTarget.isInput = true;
    pTarget.value = "get_id(type player)";
    pSpeed.pinId = m_graph.nextPinId++;
    pSpeed.name = "Speed";
    pSpeed.isInput = true;
    pSpeed.value = "5.0";
    data.pins.append(pIn);
    data.pins.append(pOut);
    data.pins.append(pTarget);
    data.pins.append(pSpeed);
  } else if (type == "action_wait") {
    NodePinData pIn, pOut, pSeconds;
    pIn.pinId = m_graph.nextPinId++;
    pIn.name = "In";
    pIn.isInput = true;
    pIn.isExecution = true;
    pOut.pinId = m_graph.nextPinId++;
    pOut.name = "Out";
    pOut.isInput = false;
    pOut.isExecution = true;
    pSeconds.pinId = m_graph.nextPinId++;
    pSeconds.name = "Segundos";
    pSeconds.isInput = true;
    pSeconds.value = "1.0";
    data.pins.append(pIn);
    data.pins.append(pOut);
    data.pins.append(pSeconds);
  } else if (type == "action_music") {
    NodePinData pIn, pOut, pFile, pVolume;
    pIn.pinId = m_graph.nextPinId++;
    pIn.name = "In";
    pIn.isInput = true;
    pIn.isExecution = true;
    pOut.pinId = m_graph.nextPinId++;
    pOut.name = "Out";
    pOut.isInput = false;
    pOut.isExecution = true;
    pFile.pinId = m_graph.nextPinId++;
    pFile.name = "File";
    pFile.isInput = true;
    pFile.value = "assets/music/bgm.ogg";
    pVolume.pinId = m_graph.nextPinId++;
    pVolume.name = "Volume";
    pVolume.isInput = true;
    pVolume.value = "128";
    data.pins.append(pIn);
    data.pins.append(pOut);
    data.pins.append(pFile);
    data.pins.append(pVolume);
  } else if (type == "action_scene") {
    NodePinData pIn, pScene;
    pIn.pinId = m_graph.nextPinId++;
    pIn.name = "In";
    pIn.isInput = true;
    pIn.isExecution = true;
    pScene.pinId = m_graph.nextPinId++;
    pScene.name = "Nombre Escena";
    pScene.isInput = true;
    pScene.value = "Scene1";
    data.pins.append(pIn);
    data.pins.append(pScene);
  } else if (type == "action_set_resolution") {
    NodePinData pIn, pOut, pW, pH;
    pIn.pinId = m_graph.nextPinId++;
    pIn.name = "In";
    pIn.isInput = true;
    pIn.isExecution = true;
    pOut.pinId = m_graph.nextPinId++;
    pOut.name = "Out";
    pOut.isInput = false;
    pOut.isExecution = true;
    pW.pinId = m_graph.nextPinId++;
    pW.name = "Ancho";
    pW.isInput = true;
    pW.value = "1920";
    pH.pinId = m_graph.nextPinId++;
    pH.name = "Alto";
    pH.isInput = true;
    pH.value = "1080";
    data.pins.append(pIn);
    data.pins.append(pOut);
    data.pins.append(pW);
    data.pins.append(pH);
  } else if (type == "action_set_fullscreen") {
    NodePinData pIn, pOut, pActive;
    pIn.pinId = m_graph.nextPinId++;
    pIn.name = "In";
    pIn.isInput = true;
    pIn.isExecution = true;
    pOut.pinId = m_graph.nextPinId++;
    pOut.name = "Out";
    pOut.isInput = false;
    pOut.isExecution = true;
    pActive.pinId = m_graph.nextPinId++;
    pActive.name = "Activo (0/1)";
    pActive.isInput = true;
    pActive.value = "1";
    data.pins.append(pIn);
    data.pins.append(pOut);
    data.pins.append(pActive);
  } else if (type == "action_set_music_volume") {
    NodePinData pIn, pOut, pVol;
    pIn.pinId = m_graph.nextPinId++;
    pIn.name = "In";
    pIn.isInput = true;
    pIn.isExecution = true;
    pOut.pinId = m_graph.nextPinId++;
    pOut.name = "Out";
    pOut.isInput = false;
    pOut.isExecution = true;
    pVol.pinId = m_graph.nextPinId++;
    pVol.name = "Volumen (0-128)";
    pVol.isInput = true;
    pVol.value = "128";
    data.pins.append(pIn);
    data.pins.append(pOut);
    data.pins.append(pVol);
  } else if (type == "action_set_sound_volume") {
    NodePinData pIn, pOut, pVol;
    pIn.pinId = m_graph.nextPinId++;
    pIn.name = "In";
    pIn.isInput = true;
    pIn.isExecution = true;
    pOut.pinId = m_graph.nextPinId++;
    pOut.name = "Out";
    pOut.isInput = false;
    pOut.isExecution = true;
    pVol.pinId = m_graph.nextPinId++;
    pVol.name = "Volumen (0-128)";
    pVol.isInput = true;
    pVol.value = "128";
    data.pins.append(pIn);
    data.pins.append(pOut);
    data.pins.append(pVol);
  } else if (type == "action_npc_flee") {
    NodePinData pIn, pOut, pTargetId, pSpeed;
    pIn.pinId = m_graph.nextPinId++;
    pIn.name = "Ejecutar";
    pIn.isInput = true;
    pIn.isExecution = true;
    pOut.pinId = m_graph.nextPinId++;
    pOut.name = "Siguiente";
    pOut.isInput = false;
    pOut.isExecution = true;
    pTargetId.pinId = m_graph.nextPinId++;
    pTargetId.name = "ID Sprite a evitar";
    pTargetId.isInput = true;
    pTargetId.value = "0";
    pSpeed.pinId = m_graph.nextPinId++;
    pSpeed.name = "Velocidad";
    pSpeed.isInput = true;
    pSpeed.value = "100";
    data.pins.append(pIn);
    data.pins.append(pOut);
    data.pins.append(pTargetId);
    data.pins.append(pSpeed);
  } else if (type == "action_npc_attack") {
    NodePinData pIn, pOut, pRange, pDamage, pCooldown, pAnimStart, pAnimEnd;
    pIn.pinId = m_graph.nextPinId++;
    pIn.name = "Ejecutar";
    pIn.isInput = true;
    pIn.isExecution = true;
    pOut.pinId = m_graph.nextPinId++;
    pOut.name = "Siguiente";
    pOut.isInput = false;
    pOut.isExecution = true;
    pRange.pinId = m_graph.nextPinId++;
    pRange.name = "Rango Ataque";
    pRange.isInput = true;
    pRange.value = "80";
    pDamage.pinId = m_graph.nextPinId++;
    pDamage.name = "Daño";
    pDamage.isInput = true;
    pDamage.value = "25";
    pCooldown.pinId = m_graph.nextPinId++;
    pCooldown.name = "Cooldown (seg)";
    pCooldown.isInput = true;
    pCooldown.value = "1.0";
    pAnimStart.pinId = m_graph.nextPinId++;
    pAnimStart.name = "Frame Inicio Ataque";
    pAnimStart.isInput = true;
    pAnimStart.value = "15";
    pAnimEnd.pinId = m_graph.nextPinId++;
    pAnimEnd.name = "Frame Fin Ataque";
    pAnimEnd.isInput = true;
    pAnimEnd.value = "30";
    data.pins.append(pIn);
    data.pins.append(pOut);
    data.pins.append(pRange);
    data.pins.append(pDamage);
    data.pins.append(pCooldown);
    data.pins.append(pAnimStart);
    data.pins.append(pAnimEnd);
  } else if (type == "action_set_alpha") {
    NodePinData pIn, pOut, pAlpha;
    pIn.pinId = m_graph.nextPinId++;
    pIn.name = "In";
    pIn.isInput = true;
    pIn.isExecution = true;
    pOut.pinId = m_graph.nextPinId++;
    pOut.name = "Out";
    pOut.isInput = false;
    pOut.isExecution = true;
    pAlpha.pinId = m_graph.nextPinId++;
    pAlpha.name = "Alpha (0-255)";
    pAlpha.isInput = true;
    pAlpha.value = "255";
    data.pins.append(pIn);
    data.pins.append(pOut);
    data.pins.append(pAlpha);
  } else if (type == "action_set_scale") {
    NodePinData pIn, pOut, pScale;
    pIn.pinId = m_graph.nextPinId++;
    pIn.name = "In";
    pIn.isInput = true;
    pIn.isExecution = true;
    pOut.pinId = m_graph.nextPinId++;
    pOut.name = "Out";
    pOut.isInput = false;
    pOut.isExecution = true;
    pScale.pinId = m_graph.nextPinId++;
    pScale.name = "Escala (%)";
    pScale.isInput = true;
    pScale.value = "100";
    data.pins.append(pIn);
    data.pins.append(pOut);
    data.pins.append(pScale);
  } else if (type == "action_stop_music") {
    NodePinData pIn, pOut;
    pIn.pinId = m_graph.nextPinId++;
    pIn.name = "In";
    pIn.isInput = true;
    pIn.isExecution = true;
    pOut.pinId = m_graph.nextPinId++;
    pOut.name = "Out";
    pOut.isInput = false;
    pOut.isExecution = true;
    data.pins.append(pIn);
    data.pins.append(pOut);
  } else if (type == "action_stop_sound") {
    NodePinData pIn, pOut;
    pIn.pinId = m_graph.nextPinId++;
    pIn.name = "In";
    pIn.isInput = true;
    pIn.isExecution = true;
    pOut.pinId = m_graph.nextPinId++;
    pOut.name = "Out";
    pOut.isInput = false;
    pOut.isExecution = true;
    data.pins.append(pIn);
    data.pins.append(pOut);
  }

  m_graph.nodes.append(data);
  refreshDataPointers();
  createNodeItem(&m_graph.nodes.last());
}

void BehaviorNodeScene::updateLinks() {
  // Clear old links and DELETE them to prevent dangling pointers and leaks
  for (auto link : m_linkItems) {
    removeItem(link);
    delete link;
  }
  m_linkItems.clear();

  // Find all connected pins
  QMap<int, BehaviorPinItem *> pinMap;
  for (auto nodeItem : m_nodeItems) {
    for (auto pinItem : nodeItem->pins()) {
      pinMap[pinItem->data()->pinId] = pinItem;
    }
  }

  for (auto nodeItem : m_nodeItems) {
    for (auto pinItem : nodeItem->pins()) {
      if (!pinItem->data()->isInput) {
        for (int linkedId : pinItem->data()->linkedPinIds) {
          if (pinMap.contains(linkedId)) {
            BehaviorLinkItem *link =
                new BehaviorLinkItem(pinItem, pinMap[linkedId]);
            addItem(link);
            m_linkItems.append(link);
          }
        }
      }
    }
  }
}

void BehaviorNodeScene::mousePressEvent(QGraphicsSceneMouseEvent *event) {
  BehaviorPinItem *pin = pinAt(event->scenePos());
  if (pin) {
    m_dragStartPin = pin;
    m_tempLink = new QGraphicsPathItem();
    m_tempLink->setPen(
        QPen(m_dragStartPin->data()->isExecution ? Qt::white : Qt::cyan, 2,
             Qt::DashLine));
    addItem(m_tempLink);
    return;
  }
  QGraphicsScene::mousePressEvent(event);
}

void BehaviorNodeScene::mouseMoveEvent(QGraphicsSceneMouseEvent *event) {
  if (m_dragStartPin && m_tempLink) {
    QPointF p1 = m_dragStartPin->connectionPoint();
    QPointF p2 = event->scenePos();

    QPainterPath path;
    path.moveTo(p1);
    float dx = qAbs(p2.x() - p1.x());
    float offset = qMin(dx / 2.0f, 100.0f);
    path.cubicTo(p1 + QPointF(offset, 0), p2 - QPointF(offset, 0), p2);
    m_tempLink->setPath(path);
    return;
  }
  QGraphicsScene::mouseMoveEvent(event);
}

void BehaviorNodeScene::mouseReleaseEvent(QGraphicsSceneMouseEvent *event) {
  if (m_dragStartPin && m_tempLink) {
    BehaviorPinItem *endPin = pinAt(event->scenePos());

    if (endPin && endPin != m_dragStartPin &&
        endPin->data()->isInput != m_dragStartPin->data()->isInput &&
        endPin->data()->isExecution == m_dragStartPin->data()->isExecution) {

      // Valid connection!
      BehaviorPinItem *outputPin =
          m_dragStartPin->data()->isInput ? endPin : m_dragStartPin;
      BehaviorPinItem *inputPin =
          m_dragStartPin->data()->isInput ? m_dragStartPin : endPin;

      // Map to find pins by ID
      QMap<int, BehaviorPinItem *> pinMap;
      for (auto nodeItem : m_nodeItems) {
        for (auto pinItem : nodeItem->pins()) {
          pinMap[pinItem->data()->pinId] = pinItem;
        }
      }

      // Input pins can only have ONE incoming link - clear previous if exists
      if (!inputPin->data()->linkedPinIds.isEmpty()) {
        int oldOutId = inputPin->data()->linkedPinIds.first();
        if (pinMap.contains(oldOutId)) {
          pinMap[oldOutId]->data()->linkedPinIds.removeOne(
              inputPin->data()->pinId);
        }
        inputPin->data()->linkedPinIds.clear();
      }

      outputPin->data()->linkedPinIds.append(inputPin->data()->pinId);
      inputPin->data()->linkedPinIds.append(outputPin->data()->pinId);

      updateLinks();
    }

    removeItem(m_tempLink);
    delete m_tempLink;
    m_tempLink = nullptr;
    m_dragStartPin = nullptr;
    return;
  }
  QGraphicsScene::mouseReleaseEvent(event);
}

void BehaviorNodeScene::keyPressEvent(QKeyEvent *event) {
  if (event->key() == Qt::Key_Delete || event->key() == Qt::Key_Backspace) {
    QList<QGraphicsItem *> selected = selectedItems();
    QList<BehaviorNodeItem *> toDelete;
    for (QGraphicsItem *item : selected) {
      if (item->type() == BehaviorNodeItem::Type) {
        toDelete.append(static_cast<BehaviorNodeItem *>(item));
      }
    }
    
    // Process deletions one by one
    // deleteNode now handles updateLinks and refreshDataPointers internally for safety
    for (BehaviorNodeItem *node : toDelete) {
      deleteNode(node);
    }
  }
  QGraphicsScene::keyPressEvent(event);
}

BehaviorPinItem *BehaviorNodeScene::pinAt(const QPointF &pos) {
  QList<QGraphicsItem *> items = this->items(pos);
  for (auto item : items) {
    if (item->type() == BehaviorPinItem::Type) {
      return static_cast<BehaviorPinItem *>(item);
    }
  }
  return nullptr;
}

void BehaviorNodeScene::contextMenuEvent(
    QGraphicsSceneContextMenuEvent *event) {
  QMenu menu;
  QMenu *events = menu.addMenu("Eventos");
  events->addAction("Al Inicio", [this, event]() {
    addNode("event_start", event->scenePos());
  });
  events->addAction("Al Colisionar", [this, event]() {
    addNode("event_collision", event->scenePos());
  });
  events->addAction("Al Actualizar", [this, event]() {
    addNode("event_update", event->scenePos());
  });
  events->addAction("Al Morir", [this, event]() {
    addNode("event_death", event->scenePos());
  });
  events->addAction("Al Morir el Jugador", [this, event]() {
    addNode("event_player_death", event->scenePos());
  });
  events->addAction("Al Recibir Daño", [this, event]() {
    addNode("event_damage", event->scenePos());
  });

  QMenu *actions = menu.addMenu("Acciones");
  actions->addAction("Sonido Motor Coche", [this, event]() {
    addNode("action_car_engine", event->scenePos());
  });
  actions->addAction("Decir (Say)", [this, event]() {
    addNode("action_say", event->scenePos());
  });
  actions->addAction("Mover A", [this, event]() {
    addNode("action_moveto", event->scenePos());
  });
  actions->addAction("Cambiar Animación (MD3)", [this, event]() {
    addNode("action_set_animation", event->scenePos());
  });
  actions->addAction("Cambiar Animación (GLB/GLTF)", [this, event]() {
    addNode("action_set_glb_animation", event->scenePos());
  });
  actions->addAction("Reproducir Sonido", [this, event]() {
    addNode("action_sound", event->scenePos());
  });
  actions->addAction("Temblor Cámara", [this, event]() {
    addNode("action_shake_camera", event->scenePos());
  });
  actions->addAction("Lanzar Cámara (CamPath)", [this, event]() {
    addNode("action_campath", event->scenePos());
  });
  actions->addAction("Eliminar Proceso", [this, event]() {
    addNode("action_kill", event->scenePos());
  });
  actions->addAction("Crear Billboard (Efecto)", [this, event]() {
    addNode("action_spawn_billboard", event->scenePos());
  });
  actions->addAction("Asignar Variable", [this, event]() {
    addNode("action_setvar", event->scenePos());
  });
  actions->addSeparator();
  actions->addAction("Hacer Daño", [this, event]() {
    addNode("action_damage", event->scenePos());
  });
  actions->addAction("Morir (Die)", [this, event]() {
    addNode("action_die", event->scenePos());
  });
  actions->addAction("Set Vida (HP)", [this, event]() {
    addNode("action_set_health", event->scenePos());
  });
  actions->addSeparator();
  actions->addAction("Cambiar Texto UI", [this, event]() {
    addNode("action_set_ui_text", event->scenePos());
  });
  actions->addAction("Esperar (Wait)", [this, event]() {
    addNode("action_wait", event->scenePos());
  });
  actions->addAction("Reproducir Música", [this, event]() {
    addNode("action_music", event->scenePos());
  });
  actions->addAction("Detener Música", [this, event]() {
    addNode("action_stop_music", event->scenePos());
  });
  actions->addAction("Detener Sonidos/Sfx", [this, event]() {
    addNode("action_stop_sound", event->scenePos());
  });
  actions->addAction("Cambiar de Escena", [this, event]() {
    addNode("action_scene", event->scenePos());
  });
  actions->addAction("Cambiar Transparencia (Alpha)", [this, event]() {
    addNode("action_set_alpha", event->scenePos());
  });
  actions->addAction("Cambiar Escala (Size)", [this, event]() {
    addNode("action_set_scale", event->scenePos());
  });
  actions->addSeparator();
  actions->addAction("Activar/Desactivar Ruta NPC", [this, event]() {
    addNode("action_set_path_active", event->scenePos());
  });
  actions->addAction("Perseguir Objetivo (NPC)", [this, event]() {
    addNode("action_npc_chase", event->scenePos());
  });
  actions->addAction("Huir de Objetivo (NPC)", [this, event]() {
    addNode("action_npc_flee", event->scenePos());
  });
  actions->addAction("Atacar Objetivo (NPC)", [this, event]() {
    addNode("action_npc_attack", event->scenePos());
  });

  QMenu *logic = menu.addMenu("Lógica");
  logic->addAction("Si... (If)",
                   [this, event]() { addNode("logic_if", event->scenePos()); });
  logic->addAction("Comparar", [this, event]() {
    addNode("logic_compare", event->scenePos());
  });
  logic->addAction("Operación Matemática",
                   [this, event]() { addNode("math_op", event->scenePos()); });
  logic->addAction("Distancia entre Sprites", [this, event]() {
    addNode("math_dist", event->scenePos());
  });
  logic->addAction("Distancia a Cámara", [this, event]() {
    addNode("math_camera_dist", event->scenePos());
  });
  logic->addAction("Distancia entre Puntos", [this, event]() {
    addNode("math_point_dist", event->scenePos());
  });
  logic->addAction("Ángulo entre Sprites", [this, event]() {
    addNode("math_angle", event->scenePos());
  });

  QMenu *system = menu.addMenu("Sistema");
  system->addAction("Cambiar Resolución", [this, event]() {
    addNode("action_set_resolution", event->scenePos());
  });
  system->addAction("Pantalla Completa", [this, event]() {
    addNode("action_set_fullscreen", event->scenePos());
  });
  system->addAction("Volumen Música", [this, event]() {
    addNode("action_set_music_volume", event->scenePos());
  });
  system->addAction("Volumen Sonidos", [this, event]() {
    addNode("action_set_sound_volume", event->scenePos());
  });
  logic->addAction("Ángulo a Cámara", [this, event]() {
    addNode("math_camera_angle", event->scenePos());
  });
  logic->addAction("Tecla Pulsada (Key)", [this, event]() {
    addNode("logic_key", event->scenePos());
  });
  logic->addAction("Variable: Colisionando? (colliding)", [this, event]() {
    addNode("logic_compare", event->scenePos());
    // Note: I could create a specific node, but logic_compare with "colliding"
    // as A is fine.
  });

  menu.addSeparator();

  QGraphicsItem *item = itemAt(event->scenePos(), QTransform());

  // Right-click on a CABLE → offer to delete it
  if (item && item->type() == BehaviorLinkItem::Type) {
    BehaviorLinkItem *linkItem = static_cast<BehaviorLinkItem *>(item);
    menu.addAction("❌ Eliminar Cable", [this, linkItem]() {
      // Remove the connection from graph data (both directions)
      int startPinId = linkItem->startPin()->data()->pinId;
      int endPinId   = linkItem->endPin()->data()->pinId;
      for (NodeData &node : m_graph.nodes) {
        for (NodePinData &pin : node.pins) {
          pin.linkedPinIds.removeOne(endPinId);
          pin.linkedPinIds.removeOne(startPinId);
        }
      }
      updateLinks(); // Rebuild visuals
    });
  }
  // Right-click on a NODE → offer to delete it
  else if (item && (item->type() == BehaviorNodeItem::Type ||
               item->parentItem() &&
                   item->parentItem()->type() == BehaviorNodeItem::Type)) {
    BehaviorNodeItem *nodeItem =
        (item->type() == BehaviorNodeItem::Type)
            ? static_cast<BehaviorNodeItem *>(item)
            : static_cast<BehaviorNodeItem *>(item->parentItem());

    menu.addAction("🗑 Eliminar Nodo",
                   [this, nodeItem]() {
                     deleteNode(nodeItem);
                     updateLinks();
                   });
  }

  menu.exec(event->screenPos());
}

/* ============================================================================
   BEHAVIOR NODE VIEW (For Zooming)
   ============================================================================
 */
BehaviorNodeView::BehaviorNodeView(QWidget *parent) : QGraphicsView(parent) {
  setRenderHint(QPainter::Antialiasing);
  setDragMode(QGraphicsView::ScrollHandDrag);
  setTransformationAnchor(QGraphicsView::AnchorUnderMouse);
}

void BehaviorNodeView::wheelEvent(QWheelEvent *event) {
  if (event->modifiers() & Qt::ControlModifier) {
    const double scaleFactor = 1.15;
    if (event->angleDelta().y() > 0) {
      scale(scaleFactor, scaleFactor);
    } else {
      scale(1.0 / scaleFactor, 1.0 / scaleFactor);
    }
  } else {
    QGraphicsView::wheelEvent(event);
  }
}

/* ============================================================================
   BEHAVIOR NODE EDITOR (Dialog)
   ============================================================================
 */
BehaviorNodeEditor::BehaviorNodeEditor(BehaviorGraph &graph,
                                       const QString &projectPath,
                                       QWidget *parent)
    : QDialog(parent), m_graph(graph), m_projectPath(projectPath) {
  setWindowFlags(Qt::Window | Qt::WindowMinMaxButtonsHint |
                 Qt::WindowCloseButtonHint);
  setWindowTitle(tr("Editor de Nodos de Comportamiento"));
  resize(1000, 700);

  QVBoxLayout *layout = new QVBoxLayout(this);
  m_view = new BehaviorNodeView(this);
  m_scene = new BehaviorNodeScene(m_graph, m_projectPath, this);
  m_scene->setSceneRect(-5000, -5000, 10000, 10000); // Massive space for nodes
  m_view->setScene(m_scene);
  m_view->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
  m_view->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);

  layout->addWidget(m_view);

  QDialogButtonBox *buttonBox = new QDialogButtonBox(
      QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);

  QPushButton *saveBtn = buttonBox->addButton("Guardar Grafo", QDialogButtonBox::ActionRole);
  QPushButton *loadBtn = buttonBox->addButton("Cargar Grafo", QDialogButtonBox::ActionRole);
  connect(saveBtn, &QPushButton::clicked, this, &BehaviorNodeEditor::onSaveGraph);
  connect(loadBtn, &QPushButton::clicked, this, &BehaviorNodeEditor::onLoadGraph);

  connect(buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
  connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
  layout->addWidget(buttonBox);
}

void BehaviorNodeEditor::onSaveGraph() {
  QString file = QFileDialog::getSaveFileName(this, "Guardar Grafo", m_projectPath, "Grapho Files (*.grapho *.json)");
  if (file.isEmpty()) return;
  
  if (!file.endsWith(".grapho") && !file.endsWith(".json")) {
      file += ".grapho";
  }

  QJsonObject obj = serializeGraph();
  QJsonDocument doc(obj);
  QFile f(file);
  if (f.open(QIODevice::WriteOnly)) {
    f.write(doc.toJson());
    f.close();
    QMessageBox::information(this, "Guardado", "Grafo guardado exitosamente.");
  } else {
    QMessageBox::warning(this, "Error", "No se pudo guardar el archivo.");
  }
}

void BehaviorNodeEditor::onLoadGraph() {
  QString file = QFileDialog::getOpenFileName(this, "Cargar Grafo", m_projectPath, "Grapho Files (*.grapho *.json)");
  if (file.isEmpty()) return;
  QFile f(file);
  if (f.open(QIODevice::ReadOnly)) {
    QByteArray data = f.readAll();
    f.close();
    QJsonDocument doc = QJsonDocument::fromJson(data);
    if (!doc.isNull() && doc.isObject()) {
      deserializeGraph(doc.object());
      QMessageBox::information(this, "Cargado", "Grafo cargado exitosamente.");
    } else {
      QMessageBox::warning(this, "Error", "El archivo de grafo no es válido o está corrupto.");
    }
  } else {
    QMessageBox::warning(this, "Error", "No se pudo leer el archivo.");
  }
}

QJsonObject BehaviorNodeEditor::serializeGraph() const {
  QJsonObject graphObj;
  graphObj["nextNodeId"] = m_graph.nextNodeId;
  graphObj["nextPinId"] = m_graph.nextPinId;
  QJsonArray nodesArr;
  for (const auto &node : m_graph.nodes) {
    QJsonObject nodeObj;
    nodeObj["nodeId"] = node.nodeId;
    nodeObj["type"] = node.type;
    nodeObj["x"] = (double)node.x;
    nodeObj["y"] = (double)node.y;
    QJsonArray pinsArr;
    for (const auto &pin : node.pins) {
      QJsonObject pinObj;
      pinObj["pinId"] = pin.pinId;
      pinObj["name"] = pin.name;
      pinObj["isInput"] = pin.isInput;
      pinObj["isExecution"] = pin.isExecution;
      pinObj["value"] = pin.value;
      QJsonArray linksArr;
      for (int lid : pin.linkedPinIds)
        linksArr.append(lid);
      pinObj["links"] = linksArr;
      pinsArr.append(pinObj);
    }
    nodeObj["pins"] = pinsArr;
    nodesArr.append(nodeObj);
  }
  graphObj["nodes"] = nodesArr;
  return graphObj;
}

void BehaviorNodeEditor::deserializeGraph(const QJsonObject &graphObj) {
  BehaviorGraph newGraph;
  newGraph.nextNodeId = graphObj["nextNodeId"].toInt(1);
  newGraph.nextPinId = graphObj["nextPinId"].toInt(1);
  QJsonArray nodesArr = graphObj["nodes"].toArray();
  for (const QJsonValue &nodeVal : nodesArr) {
    QJsonObject nodeObj = nodeVal.toObject();
    NodeData node;
    node.nodeId = nodeObj["nodeId"].toInt();
    node.type = nodeObj["type"].toString();
    node.x = nodeObj["x"].toDouble();
    node.y = nodeObj["y"].toDouble();
    QJsonArray pinsArr = nodeObj["pins"].toArray();
    for (const QJsonValue &pinVal : pinsArr) {
      QJsonObject pinObj = pinVal.toObject();
      NodePinData pin;
      pin.pinId = pinObj["pinId"].toInt();
      pin.name = pinObj["name"].toString();
      pin.isInput = pinObj["isInput"].toBool();
      pin.isExecution = pinObj["isExecution"].toBool();
      pin.value = pinObj["value"].toString();
      QJsonArray linksArr = pinObj["links"].toArray();
      for (const QJsonValue &linkVal : linksArr) {
        pin.linkedPinIds.append(linkVal.toInt());
      }
      node.pins.append(pin);
    }
    newGraph.nodes.append(node);
  }
  m_graph = newGraph;
  
  if (m_view && m_scene) {
    m_view->setScene(nullptr);
    m_scene->deleteLater();
    m_scene = new BehaviorNodeScene(m_graph, m_projectPath, this);
    m_scene->setSceneRect(-5000, -5000, 10000, 10000);
    m_view->setScene(m_scene);
  }
}
