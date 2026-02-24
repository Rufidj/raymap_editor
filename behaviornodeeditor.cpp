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
    painter->setBrush(m_data->linkedPinId != -1 ? QBrush(Qt::white)
                                                : QBrush(Qt::NoBrush));
    painter->setPen(QPen(Qt::white, 2));
    painter->drawPolygon(poly);
  } else {
    // Draw circle for data pins
    painter->setBrush(m_data->linkedPinId != -1 ? brush()
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
  QColor headerColor(70, 70, 70);
  if (m_data->type.startsWith("event"))
    headerColor = QColor(150, 50, 50);
  else if (m_data->type.startsWith("action"))
    headerColor = QColor(50, 80, 150);

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
      if (!p->isExecution && p->linkedPinId == -1 && !p->value.isEmpty()) {
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
  NodeData *data = item->data();

  // 1. Remove links
  for (const NodePinData &p : data->pins) {
    removePinLinks(p.pinId);
  }

  // 2. Remove from graph data
  for (int i = 0; i < m_graph.nodes.size(); ++i) {
    if (m_graph.nodes[i].nodeId == data->nodeId) {
      m_graph.nodes.removeAt(i);
      break;
    }
  }

  // 3. Remove from items list and scene
  m_nodeItems.removeAll(item);
  removeItem(item);
  delete item;

  updateLinks();
}

void BehaviorNodeScene::removePinLinks(int pinId) {
  for (NodeData &node : m_graph.nodes) {
    for (NodePinData &pin : node.pins) {
      if (pin.linkedPinId == pinId) {
        pin.linkedPinId = -1;
      }
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
    pFile.value = "assets/sfx/jump.wav";
    data.pins.append(pIn);
    data.pins.append(pOut);
    data.pins.append(pFile);
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
    data.pins.append(pIn);
    data.pins.append(pOut);
    data.pins.append(pFile);
    data.pins.append(pGraph);
    data.pins.append(pGraphEnd);
    data.pins.append(pSpeed);
    data.pins.append(pScale);
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
  }

  m_graph.nodes.append(data);
  refreshDataPointers();
  createNodeItem(&m_graph.nodes.last());
}

void BehaviorNodeScene::updateLinks() {
  // Clear old links
  for (auto link : m_linkItems)
    removeItem(link);
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
      if (!pinItem->data()->isInput && pinItem->data()->linkedPinId != -1) {
        if (pinMap.contains(pinItem->data()->linkedPinId)) {
          BehaviorLinkItem *link = new BehaviorLinkItem(
              pinItem, pinMap[pinItem->data()->linkedPinId]);
          addItem(link);
          m_linkItems.append(link);
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

      outputPin->data()->linkedPinId = inputPin->data()->pinId;
      inputPin->data()->linkedPinId = outputPin->data()->pinId;

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
    for (QGraphicsItem *item : selected) {
      if (item->type() == BehaviorNodeItem::Type) {
        deleteNode(static_cast<BehaviorNodeItem *>(item));
      }
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

  QMenu *actions = menu.addMenu("Acciones");
  actions->addAction("Decir (Say)", [this, event]() {
    addNode("action_say", event->scenePos());
  });
  actions->addAction("Mover A", [this, event]() {
    addNode("action_moveto", event->scenePos());
  });
  actions->addAction("Reproducir Sonido", [this, event]() {
    addNode("action_sound", event->scenePos());
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
  actions->addAction("Cambiar Texto UI", [this, event]() {
    addNode("action_set_ui_text", event->scenePos());
  });

  QMenu *logic = menu.addMenu("Lógica");
  logic->addAction("Si... (If)",
                   [this, event]() { addNode("logic_if", event->scenePos()); });
  logic->addAction("Comparar", [this, event]() {
    addNode("logic_compare", event->scenePos());
  });
  logic->addAction("Operación Matemática",
                   [this, event]() { addNode("math_op", event->scenePos()); });

  menu.addSeparator();

  QGraphicsItem *item = itemAt(event->scenePos(), QTransform());
  if (item && (item->type() == BehaviorNodeItem::Type ||
               item->parentItem() &&
                   item->parentItem()->type() == BehaviorNodeItem::Type)) {
    BehaviorNodeItem *nodeItem =
        (item->type() == BehaviorNodeItem::Type)
            ? static_cast<BehaviorNodeItem *>(item)
            : static_cast<BehaviorNodeItem *>(item->parentItem());

    menu.addAction("Eliminar Nodo",
                   [this, nodeItem]() { deleteNode(nodeItem); });
  }

  menu.exec(event->screenPos());
}

/* ============================================================================
   BEHAVIOR NODE EDITOR (Dialog)
   ============================================================================
 */
BehaviorNodeEditor::BehaviorNodeEditor(BehaviorGraph &graph,
                                       const QString &projectPath,
                                       QWidget *parent)
    : QDialog(parent), m_graph(graph), m_projectPath(projectPath) {
  setWindowTitle(tr("Editor de Nodos de Comportamiento"));
  resize(1000, 700);

  QVBoxLayout *layout = new QVBoxLayout(this);
  m_view = new QGraphicsView(this);
  m_scene = new BehaviorNodeScene(m_graph, m_projectPath, this);
  m_view->setScene(m_scene);
  m_view->setRenderHint(QPainter::Antialiasing);
  m_view->setDragMode(QGraphicsView::ScrollHandDrag);

  layout->addWidget(m_view);

  QDialogButtonBox *buttonBox = new QDialogButtonBox(
      QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
  connect(buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
  connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
  layout->addWidget(buttonBox);
}
