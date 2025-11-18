# refined_flow_editor.py
import sys
from PyQt6.QtWidgets import (
    QApplication, QMainWindow, QWidget, QHBoxLayout,
    QListWidget, QListWidgetItem, QGraphicsScene,
    QGraphicsView, QGraphicsRectItem, QGraphicsEllipseItem,
    QGraphicsLineItem
)
from PyQt6.QtGui import QBrush, QColor, QPen, QDrag, QPainter
from PyQt6.QtCore import Qt, QMimeData
from PyQt6.QtWidgets import QGraphicsItem

class PortItem(QGraphicsEllipseItem):
    def __init__(self, parent, port_id, x, y):
        super().__init__(-5, -5, 10, 10, parent)
        self.setBrush(QBrush(Qt.GlobalColor.red))
        self.setPos(x, y)
        self.port_id = port_id
        self.edges = []
        self.setFlag(QGraphicsItem.GraphicsItemFlag.ItemSendsGeometryChanges)

    def itemChange(self, change, value):
        if change == QGraphicsItem.GraphicsItemChange.ItemScenePositionHasChanged:
            for edge in self.edges:
                edge.update_position()
        return super().itemChange(change, value)

# -----------------------
# Component Item
# -----------------------
class ComponentItem(QGraphicsRectItem):
    COLORS = {"Pump": Qt.GlobalColor.red, "Valve": Qt.GlobalColor.green, "Pipe": Qt.GlobalColor.blue}

    def __init__(self, comp_type):
        super().__init__(0, 0, 80, 50)
        self.comp_type = comp_type
        self.setBrush(QBrush(self.COLORS.get(comp_type, Qt.GlobalColor.lightGray)))
        self.setFlags(QGraphicsRectItem.GraphicsItemFlag.ItemIsMovable |
                      QGraphicsRectItem.GraphicsItemFlag.ItemIsSelectable)
        # Add two ports: input and output
        self.ports = [PortItem(self, "in", 0, 25), PortItem(self, "out", 80, 25)]

# -----------------------
# Edge Item
# -----------------------
class EdgeItem(QGraphicsLineItem):
    def __init__(self, source_port, dest_port):
        super().__init__()
        self.source_port = source_port
        self.dest_port = dest_port
        self.setFlags(QGraphicsItem.GraphicsItemFlag.ItemIsSelectable)

        # Register edge in both ports
        self.source_port.edges.append(self)
        self.dest_port.edges.append(self)

        pen = QPen(Qt.GlobalColor.darkBlue)
        pen.setWidth(2)
        self.setPen(pen)

        self.update_position()

    def update_position(self):
        p1 = self.source_port.scenePos()
        p2 = self.dest_port.scenePos()
        self.setLine(p1.x(), p1.y(), p2.x(), p2.y())

# -----------------------
# Custom Scene
# -----------------------
class FlowScene(QGraphicsScene):
    def __init__(self):
        super().__init__()
        self.setBackgroundBrush(Qt.GlobalColor.lightGray)
        self.temp_line = None
        self.start_port = None

    def keyPressEvent(self, event):
        if event.key() == Qt.Key.Key_Delete:
            for item in self.selectedItems():
                # If it’s an edge, remove it from ports
                if isinstance(item, EdgeItem):
                    if item.source_port and item in item.source_port.edges:
                        item.source_port.edges.remove(item)
                    if item.dest_port and item in item.dest_port.edges:
                        item.dest_port.edges.remove(item)
                # Remove the item from scene
                self.removeItem(item)
        else:
            super().keyPressEvent(event)

    def port_at(self, pos):
        """Return the first PortItem under the given scene position."""
        items_at_pos = self.items(pos)
        for item in items_at_pos:
            if isinstance(item, PortItem):
                return item
        return None

    def mousePressEvent(self, event):
        item = self.itemAt(event.scenePos(), self.views()[0].transform())
        if isinstance(item, PortItem):
            self.start_port = item
            self.temp_line = QGraphicsLineItem()
            pen = QPen(Qt.GlobalColor.darkGreen)
            pen.setWidth(2)
            self.temp_line.setPen(pen)
            self.addItem(self.temp_line)
        else:
            super().mousePressEvent(event)

    def mouseMoveEvent(self, event):
        if self.temp_line and self.start_port:
            p1 = self.start_port.scenePos()
            p2 = event.scenePos()
            self.temp_line.setLine(p1.x(), p1.y(), p2.x(), p2.y())
        else:
            super().mouseMoveEvent(event)

    def mouseReleaseEvent(self, event):
        if self.temp_line and self.start_port:
            # Use helper to detect port
            target_port = self.port_at(event.scenePos())
            if target_port and target_port != self.start_port:
                # Create permanent edge
                edge = EdgeItem(self.start_port, target_port)
                self.addItem(edge)
            # Remove temporary line
            self.removeItem(self.temp_line)
            self.temp_line = None
            self.start_port = None
        else:
            super().mouseReleaseEvent(event)

# -----------------------
# Custom GraphicsView for Drag-and-Drop
# -----------------------
class FlowView(QGraphicsView):
    def __init__(self, scene):
        super().__init__(scene)
        self.setAcceptDrops(True)
        self.setRenderHints(self.renderHints() | QPainter.RenderHint.Antialiasing)

    def dragEnterEvent(self, event):
        event.acceptProposedAction()

    def dragMoveEvent(self, event):
        event.acceptProposedAction()

    def dropEvent(self, event):
        comp_type = event.mimeData().text()
        pos = self.mapToScene(event.position().toPoint())
        comp = ComponentItem(comp_type)
        comp.setPos(pos)
        self.scene().addItem(comp)
        event.acceptProposedAction()

# -----------------------
# Main Window
# -----------------------
class MainWindow(QMainWindow):
    def __init__(self):
        super().__init__()
        self.setWindowTitle("Refined Flow Circuit Editor")
        self.setGeometry(100, 100, 1200, 800)

        # Central widget and layout
        central_widget = QWidget()
        self.setCentralWidget(central_widget)
        layout = QHBoxLayout(central_widget)

        # Component library
        self.library = QListWidget()
        for comp in ["Pump", "Valve", "Pipe"]:
            item = QListWidgetItem(comp)
            item.setData(Qt.ItemDataRole.UserRole, comp)
            self.library.addItem(item)
        self.library.setDragEnabled(True)
        layout.addWidget(self.library, 0)  # library minimal width

        # Scene and view
        self.scene = FlowScene()
        self.view = FlowView(self.scene)
        layout.addWidget(self.view, 1)  # canvas expands

# -----------------------
# Run application
# -----------------------
if __name__ == "__main__":
    app = QApplication(sys.argv)
    window = MainWindow()
    window.show()
    sys.exit(app.exec())
