import { useCallback, useState } from "react";
import ReactFlow, {
  Background,
  Controls,
  MiniMap,
  addEdge,
  useNodesState,
  useEdgesState
} from "reactflow";

import "reactflow/dist/style.css";

const initialNodes = [];
const initialEdges = [];

let id = 0;
const getId = () => `node_${id++}`;

export default function App() {

  const [nodes, setNodes, onNodesChange] =
    useNodesState(initialNodes);

  const [edges, setEdges, onEdgesChange] =
    useEdgesState(initialEdges);

  const onConnect = useCallback(
    (params) => setEdges((eds) => addEdge(params, eds)),
    []
  );

  const onDragStart = (event, nodeType) => {
    event.dataTransfer.setData(
      "application/reactflow",
      nodeType
    );

    event.dataTransfer.effectAllowed = "move";
  };

  const onDrop = useCallback(
    (event) => {

      event.preventDefault();

      const type = event.dataTransfer.getData(
        "application/reactflow"
      );

      const position = {
        x: event.clientX - 250,
        y: event.clientY - 40
      };

      const newNode = {
        id: getId(),
        type: "default",
        position,
        data: {
          label: `${type}`
        }
      };

      setNodes((nds) => nds.concat(newNode));
    },
    [setNodes]
  );

  const onDragOver = useCallback((event) => {
    event.preventDefault();
    event.dataTransfer.dropEffect = "move";
  }, []);

  const exportJSON = () => {

    const data = {
      nodes,
      edges
    };

    const blob = new Blob(
      [JSON.stringify(data, null, 2)],
      { type: "application/json" }
    );

    const url = URL.createObjectURL(blob);

    const a = document.createElement("a");

    a.href = url;
    a.download = "opensd_flow.json";

    a.click();

    URL.revokeObjectURL(url);
  };

  return (
    <div style={{ width: "100vw", height: "100vh", display: "flex" }}>

      {/* Sidebar */}
      <div
        style={{
          width: "200px",
          borderRight: "1px solid gray",
          padding: "10px"
        }}
      >

        <h3>Components</h3>

        {["Pump", "Valve", "Pipe"].map((comp) => (
          <div
            key={comp}
            draggable
            onDragStart={(event) =>
              onDragStart(event, comp)
            }
            style={{
              padding: "10px",
              marginBottom: "10px",
              border: "1px solid black",
              cursor: "grab"
            }}
          >
            {comp}
          </div>
        ))}

        <button onClick={exportJSON}>
          Export JSON
        </button>

      </div>

      {/* Flow Canvas */}
      <div style={{ flexGrow: 1 }}>

        <ReactFlow
          nodes={nodes}
          edges={edges}
          onNodesChange={onNodesChange}
          onEdgesChange={onEdgesChange}
          onConnect={onConnect}
          onDrop={onDrop}
          onDragOver={onDragOver}
          fitView
        >

          <Background />
          <Controls />
          <MiniMap />

        </ReactFlow>

      </div>

    </div>
  );
}