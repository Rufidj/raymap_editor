#include "npcpathcanvas.h"
#include <QMouseEvent>
#include <QPainter>
#include <QWheelEvent>
#include <QtMath>

NPCPathCanvas::NPCPathCanvas(QWidget *parent)
    : QWidget(parent), m_mapData(nullptr), m_path(nullptr),
      m_selectedWaypoint(-1), m_zoom(1.0f), m_offset(0, 0),
      m_draggingWaypoint(-1), m_panning(false) {
  setMinimumSize(400, 400);
  setMouseTracking(true);
}

void NPCPathCanvas::setMapData(const MapData *mapData) {
  m_mapData = mapData;

  // Center view on map
  if (m_mapData && !m_mapData->sectors.isEmpty()) {
    // Calculate map bounds
    float minX = 1e9, minY = 1e9, maxX = -1e9, maxY = -1e9;
    for (const Sector &sector : m_mapData->sectors) {
      for (const Wall &wall : sector.walls) {
        minX = qMin(minX, qMin(wall.x1, wall.x2));
        minY = qMin(minY, qMin(wall.y1, wall.y2));
        maxX = qMax(maxX, qMax(wall.x1, wall.x2));
        maxY = qMax(maxY, qMax(wall.y1, wall.y2));
      }
    }

    float centerX = (minX + maxX) / 2.0f;
    float centerY = (minY + maxY) / 2.0f;
    m_offset = QPointF(-centerX, -centerY);

    // Adjust zoom to fit map
    float mapWidth = maxX - minX;
    float mapHeight = maxY - minY;
    float scaleX = width() / (mapWidth * 1.2f);
    float scaleY = height() / (mapHeight * 1.2f);
    m_zoom = qMin(scaleX, scaleY);
  }

  update();
}

void NPCPathCanvas::setPath(NPCPath *path) {
  m_path = path;
  update();
}

void NPCPathCanvas::setSelectedWaypoint(int index) {
  m_selectedWaypoint = index;
  update();
}

void NPCPathCanvas::paintEvent(QPaintEvent *event) {
  QPainter painter(this);
  painter.setRenderHint(QPainter::Antialiasing);

  // Background
  painter.fillRect(rect(), QColor(40, 40, 40));

  // Transform
  painter.translate(width() / 2, height() / 2);
  painter.scale(m_zoom, m_zoom);
  painter.translate(m_offset);

  // Draw map
  drawMap(painter);

  // Draw path
  if (m_path) {
    drawPath(painter);
    drawWaypoints(painter);
  }
}

void NPCPathCanvas::drawMap(QPainter &painter) {
  if (!m_mapData)
    return;

  painter.setPen(QPen(QColor(100, 100, 100), 1.0f / m_zoom));

  for (const Sector &sector : m_mapData->sectors) {
    for (const Wall &wall : sector.walls) {
      painter.drawLine(QPointF(wall.x1, wall.y1), QPointF(wall.x2, wall.y2));
    }
  }
}

void NPCPathCanvas::drawPath(QPainter &painter) {
  if (m_path->waypoints.size() < 2)
    return;

  // Draw lines between waypoints
  painter.setPen(QPen(QColor(255, 200, 100, 200), 2.0f / m_zoom));
  for (int i = 0; i < m_path->waypoints.size() - 1; i++) {
    const Waypoint &wp1 = m_path->waypoints[i];
    const Waypoint &wp2 = m_path->waypoints[i + 1];
    painter.drawLine(QPointF(wp1.x, wp1.y), QPointF(wp2.x, wp2.y));
  }

  // Draw loop connection if needed
  if (m_path->loop_mode != NPCPath::LOOP_NONE && m_path->waypoints.size() > 1) {
    const Waypoint &first = m_path->waypoints.first();
    const Waypoint &last = m_path->waypoints.last();
    painter.setPen(
        QPen(QColor(255, 150, 50, 150), 1.5f / m_zoom, Qt::DashLine));
    painter.drawLine(QPointF(last.x, last.y), QPointF(first.x, first.y));
  }
}

void NPCPathCanvas::drawWaypoints(QPainter &painter) {
  for (int i = 0; i < m_path->waypoints.size(); i++) {
    const Waypoint &wp = m_path->waypoints[i];
    QPointF pos(wp.x, wp.y);

    float radius = 8.0f / m_zoom;

    // Color based on selection
    QColor wpColor;
    if (i == m_selectedWaypoint) {
      wpColor = QColor(255, 100, 100); // Selected = red
      painter.setPen(QPen(QColor(255, 255, 255), 2.0f / m_zoom));
    } else {
      wpColor = QColor(100, 200, 255); // Normal = blue
      painter.setPen(QPen(QColor(255, 255, 255), 1.0f / m_zoom));
    }

    painter.setBrush(wpColor);
    painter.drawEllipse(pos, radius, radius);

    // Draw waypoint number
    painter.setPen(Qt::white);
    QFont font = painter.font();
    font.setPointSizeF(10.0f / m_zoom);
    painter.setFont(font);
    painter.drawText(
        QRectF(pos.x() - radius, pos.y() - radius, radius * 2, radius * 2),
        Qt::AlignCenter, QString::number(i + 1));
  }
}

void NPCPathCanvas::mousePressEvent(QMouseEvent *event) {
  if (event->button() == Qt::LeftButton) {
    int wpIndex = findWaypointAt(event->pos());

    if (wpIndex >= 0) {
      // Start dragging waypoint
      m_draggingWaypoint = wpIndex;
      emit waypointSelected(wpIndex);
    } else {
      // Add new waypoint
      QPointF worldPos = screenToWorld(event->pos());
      emit waypointAdded(worldPos.x(), worldPos.y());
    }
  } else if (event->button() == Qt::MiddleButton) {
    m_panning = true;
    m_lastMousePos = event->pos();
    setCursor(Qt::ClosedHandCursor);
  }
}

void NPCPathCanvas::mouseMoveEvent(QMouseEvent *event) {
  if (m_draggingWaypoint >= 0) {
    QPointF worldPos = screenToWorld(event->pos());
    emit waypointMoved(m_draggingWaypoint, worldPos.x(), worldPos.y());
    update();
  } else if (m_panning) {
    QPoint delta = event->pos() - m_lastMousePos;
    m_offset += QPointF(delta.x() / m_zoom, delta.y() / m_zoom);
    m_lastMousePos = event->pos();
    update();
  }
}

void NPCPathCanvas::mouseReleaseEvent(QMouseEvent *event) {
  if (event->button() == Qt::LeftButton) {
    m_draggingWaypoint = -1;
  } else if (event->button() == Qt::MiddleButton) {
    m_panning = false;
    setCursor(Qt::ArrowCursor);
  }
}

void NPCPathCanvas::wheelEvent(QWheelEvent *event) {
  float zoomFactor = event->angleDelta().y() > 0 ? 1.1f : 0.9f;
  m_zoom *= zoomFactor;
  m_zoom = qBound(0.1f, m_zoom, 10.0f);
  update();
}

QPointF NPCPathCanvas::worldToScreen(float x, float y) const {
  QPointF world(x, y);
  world += m_offset;
  world *= m_zoom;
  world += QPointF(width() / 2, height() / 2);
  return world;
}

QPointF NPCPathCanvas::screenToWorld(const QPoint &screen) const {
  QPointF world = screen - QPointF(width() / 2, height() / 2);
  world /= m_zoom;
  world -= m_offset;
  return world;
}

int NPCPathCanvas::findWaypointAt(const QPoint &pos) const {
  if (!m_path)
    return -1;

  QPointF worldPos = screenToWorld(pos);
  float threshold = 15.0f / m_zoom;

  for (int i = 0; i < m_path->waypoints.size(); i++) {
    const Waypoint &wp = m_path->waypoints[i];
    float dx = wp.x - worldPos.x();
    float dy = wp.y - worldPos.y();
    float dist = qSqrt(dx * dx + dy * dy);

    if (dist < threshold) {
      return i;
    }
  }

  return -1;
}
