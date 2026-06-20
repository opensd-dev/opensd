import { useEffect, useRef } from "react";
import ReactFlow, {
  Background,
  ConnectionMode,
  Controls,
  MiniMap,
  ReactFlowProvider,
  useReactFlow
} from "reactflow";

import BcNode from "./BcNode.jsx";
import FlowNode from "./FlowNode.jsx";
import PipeNode from "./PipeNode.jsx";

const nodeTypes = { flow: FlowNode, pipe: PipeNode, bc: BcNode };

function ModelFlowInner({
  nodes,
  edges,
  onNodesChange,
  onEdgesChange,
  onConnect,
  isValidConnection,
  onDrop,
  onDragOver,
  onPaneClick,
  onCanvasContextMenu,
  onNodeContextMenu,
  onNodeDoubleClick,
  fitViewTrigger
}) {
  const { fitView, screenToFlowPosition, zoomIn, zoomOut } = useReactFlow();
  const selectionStart = useRef(null);

  useEffect(() => {
    if (fitViewTrigger <= 0) return undefined;

    const frame = requestAnimationFrame(() => {
      fitView({ padding: 0.14, duration: 250 });
    });

    return () => cancelAnimationFrame(frame);
  }, [fitViewTrigger, fitView]);

  const handleDrop = (event) => {
    event.preventDefault();
    const position = screenToFlowPosition({ x: event.clientX, y: event.clientY });
    onDrop(event, position);
  };

  const handlePaneClick = (event) => {
    const position = screenToFlowPosition({ x: event.clientX, y: event.clientY });
    onPaneClick(event, position);
  };

  const handleSelectionStart = (event) => {
    selectionStart.current = {
      x: event.clientX,
      y: event.clientY,
      additive: event.ctrlKey || event.metaKey,
      selectedIds: new Set(nodes.filter((node) => node.selected).map((node) => node.id))
    };
  };

  const handleSelectionEnd = (event) => {
    const start = selectionStart.current;
    selectionStart.current = null;
    if (!start || event.clientX >= start.x) return;

    const selectionRect = {
      left: event.clientX,
      right: start.x,
      top: Math.min(start.y, event.clientY),
      bottom: Math.max(start.y, event.clientY)
    };

    requestAnimationFrame(() => {
      const nodeElements = new Map(
        Array.from(document.querySelectorAll(".canvas-wrap .react-flow__node[data-id]"))
          .map((element) => [element.getAttribute("data-id"), element])
      );
      const changes = nodes.map((node) => {
        const rect = nodeElements.get(node.id)?.getBoundingClientRect();
        const intersects = Boolean(rect
          && rect.left < selectionRect.right
          && rect.right > selectionRect.left
          && rect.top < selectionRect.bottom
          && rect.bottom > selectionRect.top);
        return {
          id: node.id,
          type: "select",
          selected: intersects || (start.additive && start.selectedIds.has(node.id))
        };
      });
      onNodesChange(changes);
    });
  };

  return (
    <>
      <div className="flow-toolbar">
        <button
          type="button"
          className="flow-tool-button"
          title="Fit entire graph in view"
          onClick={() => fitView({ padding: 0.14, duration: 200 })}
        >
          Fit view
        </button>
        <button
          type="button"
          className="flow-tool-button"
          title="Zoom in"
          onClick={() => zoomIn({ duration: 150 })}
        >
          +
        </button>
        <button
          type="button"
          className="flow-tool-button"
          title="Zoom out"
          onClick={() => zoomOut({ duration: 150 })}
        >
          −
        </button>
      </div>
      <ReactFlow
        nodeTypes={nodeTypes}
        nodes={nodes}
        edges={edges}
        onNodesChange={onNodesChange}
        onEdgesChange={onEdgesChange}
        onConnect={onConnect}
        isValidConnection={isValidConnection}
        onDrop={handleDrop}
        onDragOver={onDragOver}
        onPaneClick={handlePaneClick}
        onPaneContextMenu={onCanvasContextMenu}
        onNodeContextMenu={onNodeContextMenu}
        onNodeDoubleClick={onNodeDoubleClick}
        onSelectionStart={handleSelectionStart}
        onSelectionEnd={handleSelectionEnd}
        connectionMode={ConnectionMode.Loose}
        deleteKeyCode={["Backspace", "Delete"]}
        multiSelectionKeyCode={["Control", "Meta"]}
        elementsSelectable
        edgesFocusable
        panOnDrag={[1, 2]}
        selectionOnDrag
        fitView
        minZoom={0.05}
        maxZoom={2.5}
      >
        <Background color="#c6ccd6" gap={22} />
        <Controls />
        <MiniMap nodeStrokeWidth={2} zoomable pannable />
      </ReactFlow>
    </>
  );
}

export default function ModelFlowCanvas(props) {
  return (
    <section
      className="workspace-pane canvas-wrap"
      onContextMenu={(event) => {
        if (event.defaultPrevented) return;
        props.onCanvasContextMenu?.(event);
      }}
    >
      <ReactFlowProvider>
        <ModelFlowInner {...props} />
      </ReactFlowProvider>
    </section>
  );
}
