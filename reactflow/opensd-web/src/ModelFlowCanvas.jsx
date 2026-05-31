import { useEffect } from "react";
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
  onDrop,
  onDragOver,
  onPaneClick,
  fitViewTrigger
}) {
  const { fitView, screenToFlowPosition, zoomIn, zoomOut } = useReactFlow();

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
        onDrop={handleDrop}
        onDragOver={onDragOver}
        onPaneClick={handlePaneClick}
        connectionMode={ConnectionMode.Loose}
        deleteKeyCode={["Backspace", "Delete"]}
        multiSelectionKeyCode={null}
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
    <section className="workspace-pane canvas-wrap">
      <ReactFlowProvider>
        <ModelFlowInner {...props} />
      </ReactFlowProvider>
    </section>
  );
}
