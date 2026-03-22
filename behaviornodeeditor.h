#ifndef BEHAVIORNODEEDITOR_H
#define BEHAVIORNODEEDITOR_H

#include "mapdata.h"
#include <QDialog>
#include <QGraphicsItem>
#include <QGraphicsPathItem>
#include <QGraphicsScene>
#include <QGraphicsView>
#include <QKeyEvent>
#include <QMenu>
#include <QMouseEvent>
#include <QPainter>
#include <QPointF>
#include <QRectF>
#include <QString>
#include <QStyleOptionGraphicsItem>
#include <QVariant>
#include <QVector>
#include <QJsonObject>
#include <QJsonArray>

class BehaviorNodeItem;
class BehaviorPinItem;
class BehaviorLinkItem;

/* ============================================================================
   PIN ITEM - The connection point on a node
   ============================================================================
 */
class BehaviorPinItem : public QGraphicsEllipseItem {
public:
  enum { Type = UserType + 2 };
  BehaviorPinItem(NodePinData *data, BehaviorNodeItem *parent);

  int type() const override { return Type; }
  NodePinData *data() { return m_data; }
  BehaviorNodeItem *node() { return m_node; }
  void updateDataPointer(NodePinData *data) { m_data = data; }

  QPointF connectionPoint() const;

protected:
  QRectF boundingRect() const override;
  QPainterPath shape() const override;
  void paint(QPainter *painter, const QStyleOptionGraphicsItem *option,
             QWidget *widget) override;
  void mouseDoubleClickEvent(QGraphicsSceneMouseEvent *event) override;

private:
  NodePinData *m_data;
  BehaviorNodeItem *m_node;
};

/* ============================================================================
   NODE ITEM - Visual representation of a behavior node
   ============================================================================
 */
class BehaviorNodeItem : public QGraphicsItem {
public:
  enum { Type = UserType + 1 };
  BehaviorNodeItem(NodeData *data);

  int type() const override { return Type; }
  QRectF boundingRect() const override;
  void paint(QPainter *painter, const QStyleOptionGraphicsItem *option,
             QWidget *widget) override;

  NodeData *data() { return m_data; }
  void updateDataPointer(NodeData *data);
  void addPin(BehaviorPinItem *pin);
  QVector<BehaviorPinItem *> pins() { return m_pins; }

protected:
  void mouseMoveEvent(QGraphicsSceneMouseEvent *event) override;
  QVariant itemChange(GraphicsItemChange change,
                      const QVariant &value) override;

private:
  NodeData *m_data;
  QVector<BehaviorPinItem *> m_pins;
  float m_width, m_height;
};

/* ============================================================================
   LINK ITEM - Line connecting two pins
   ============================================================================
 */
class BehaviorLinkItem : public QGraphicsPathItem {
public:
  enum { Type = UserType + 3 };
  BehaviorLinkItem(BehaviorPinItem *start, BehaviorPinItem *end);
  void updatePath();
  int type() const override { return Type; }
  BehaviorPinItem *startPin() const { return m_start; }
  BehaviorPinItem *endPin()   const { return m_end; }

private:
  BehaviorPinItem *m_start;
  BehaviorPinItem *m_end;
};

/* ============================================================================
   NODE EDITOR SCENE - The canvas
   ============================================================================
 */
class BehaviorNodeScene : public QGraphicsScene {
  Q_OBJECT
public:
  BehaviorNodeScene(BehaviorGraph &graph, const QString &projectPath,
                    QObject *parent = nullptr);

  QString projectPath() const { return m_projectPath; }

  void addNode(const QString &type, const QPointF &pos);
  void deleteNode(BehaviorNodeItem *item);
  void removePinLinks(int pinId);
  void updateLinks();
  void refreshDataPointers();

protected:
  void contextMenuEvent(QGraphicsSceneContextMenuEvent *event) override;
  void mousePressEvent(QGraphicsSceneMouseEvent *event) override;
  void mouseMoveEvent(QGraphicsSceneMouseEvent *event) override;
  void mouseReleaseEvent(QGraphicsSceneMouseEvent *event) override;
  void keyPressEvent(QKeyEvent *event) override;

private:
  BehaviorGraph &m_graph;
  QString m_projectPath;
  QVector<BehaviorNodeItem *> m_nodeItems;
  QVector<BehaviorLinkItem *> m_linkItems;

  // Interaction
  BehaviorPinItem *m_dragStartPin;
  QGraphicsPathItem *m_tempLink;

  void createNodeItem(NodeData *data);
  BehaviorPinItem *pinAt(const QPointF &pos);
};

class BehaviorNodeView : public QGraphicsView {
  Q_OBJECT
public:
  explicit BehaviorNodeView(QWidget *parent = nullptr);

protected:
  void wheelEvent(QWheelEvent *event) override;
};

/* ============================================================================
   NODE EDITOR DIALOG - The container
   ============================================================================
 */
class BehaviorNodeEditor : public QDialog {
  Q_OBJECT
public:
  explicit BehaviorNodeEditor(BehaviorGraph &graph, const QString &projectPath,
                              QWidget *parent = nullptr);

private slots:
  void onSaveGraph();
  void onLoadGraph();

private:
  QJsonObject serializeGraph() const;
  void deserializeGraph(const QJsonObject &obj);

  BehaviorNodeView *m_view;
  BehaviorNodeScene *m_scene;
  BehaviorGraph &m_graph;
  QString m_projectPath;
};

#endif // BEHAVIORNODEEDITOR_H
