#ifndef NPCPATHCANVAS_H
#define NPCPATHCANVAS_H

#include "mapdata.h"
#include <QWidget>

class NPCPathCanvas : public QWidget {
  Q_OBJECT

public:
  explicit NPCPathCanvas(QWidget *parent = nullptr);

  void setMapData(const MapData *mapData);
  void setPath(NPCPath *path);
  void setSelectedWaypoint(int index);

signals:
  void waypointAdded(float x, float y);
  void waypointSelected(int index);
  void waypointMoved(int index, float x, float y);

protected:
  void paintEvent(QPaintEvent *event) override;
  void mousePressEvent(QMouseEvent *event) override;
  void mouseMoveEvent(QMouseEvent *event) override;
  void mouseReleaseEvent(QMouseEvent *event) override;
  void wheelEvent(QWheelEvent *event) override;

private:
  void drawMap(QPainter &painter);
  void drawPath(QPainter &painter);
  void drawWaypoints(QPainter &painter);

  QPointF worldToScreen(float x, float y) const;
  QPointF screenToWorld(const QPoint &screen) const;
  int findWaypointAt(const QPoint &pos) const;

  const MapData *m_mapData;
  NPCPath *m_path;
  int m_selectedWaypoint;

  // View transform
  float m_zoom;
  QPointF m_offset;

  // Interaction
  int m_draggingWaypoint;
  QPoint m_lastMousePos;
  bool m_panning;
};

#endif // NPCPATHCANVAS_H
