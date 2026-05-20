import { useCallback, useMemo, useRef, useState } from "react";
import { addEdge, useEdgesState, useNodesState } from "reactflow";

import {
  FLOW_MARKER,
  HEAT_MARKER,
  buildHeatHandleMap,
  resolveHeatHandles
} from "./edgeUtils.js";

import "reactflow/dist/style.css";
import "./App.css";
import ModelFlowCanvas from "./ModelFlowCanvas.jsx";
import {
  CIRCUIT_ROW_GAP,
  PIPE_STEP,
  buildHorizontalSequence,
  layoutCircuitRow,
  nodeData
} from "./circuitLayout.js";

const componentTypes = ["Node", "Pipe", "Pump", "Valve", "HSlab", "BC"];
const initialNodes = [];
const initialEdges = [];
const nodeVariables = [
  { value: "temperature_from_tenth", label: "Temperature from total enthalpy" },
  { value: "ttemp_gues", label: "Total temperature" },
  { value: "stemp_gues", label: "Static temperature" },
  { value: "tpres_gues", label: "Total pressure" },
  { value: "spres_gues", label: "Static pressure" },
  { value: "tenth_gues", label: "Total enthalpy" },
  { value: "senth_gues", label: "Static enthalpy" },
  { value: "velocity", label: "Velocity" },
  { value: "msource", label: "Mass source" }
];

let id = 0;
const getId = () => `manual_${id++}`;

function emptyHeat() {
  return { htIn: false, htOut: false, htSideIn: false, htSideOut: false };
}

function heatFor(id, heatHandleMap) {
  return heatHandleMap.get(id) ?? emptyHeat();
}

function buildFlowNode({ id, position, identifier, details = [], heatHandleMap }) {
  return {
    id,
    type: "flow",
    position,
    data: { ...nodeData(identifier, details), heat: heatFor(id, heatHandleMap) }
  };
}

function buildPipeNode({ id, position, identifier, details = [], kind = "pipe", heatHandleMap }) {
  return {
    id,
    type: "pipe",
    position,
    data: { ...nodeData(identifier, details), kind, heat: heatFor(id, heatHandleMap) }
  };
}

function buildBcNode({ id, position, identifier, details = [] }) {
  return {
    id,
    type: "bc",
    position,
    data: nodeData(identifier, details)
  };
}

function flowEdge(id, source, target) {
  return {
    id,
    source,
    target,
    sourceHandle: "flow-out",
    targetHandle: "flow-in",
    type: "straight",
    markerEnd: FLOW_MARKER
  };
}

function bcEdge(id, source, target) {
  return {
    id,
    source,
    target,
    sourceHandle: "bc-out",
    targetHandle: "bc-in",
    type: "straight",
    className: "bc-edge"
  };
}

function hslabEdge(id, source, target) {
  return {
    id,
    source,
    target,
    ...resolveHeatHandles(source, target),
    type: "smoothstep",
    animated: false,
    className: "hslab-edge",
    markerEnd: HEAT_MARKER
  };
}

function attr(element, name, fallback = "") {
  return element?.getAttribute(name) ?? fallback;
}

function layerTooltipLines(layers) {
  if (!layers.length) return [];

  const lines = [`${layers.length} layer(s):`];
  layers.forEach((layer, layerIndex) => {
    const layerNo = attr(layer, "layerno", String(layerIndex));
    const summary = [
      `L${layerNo}`,
      attr(layer, "solname"),
      attr(layer, "nnodes") && `${attr(layer, "nnodes")} radial nodes`
    ]
      .filter(Boolean)
      .join(" · ");
    lines.push(`  ${summary}`);
  });

  return lines;
}

function naturalCompare(a, b) {
  return a.localeCompare(b, undefined, { numeric: true, sensitivity: "base" });
}

function unwrapHdf5Value(value) {
  if (value == null) return null;
  if (typeof value === "string" || typeof value === "number") return value;
  if (ArrayBuffer.isView(value)) return value.length === 1 ? value[0] : Array.from(value);
  if (Array.isArray(value)) return value.length === 1 ? value[0] : value;
  return value;
}

function readHdf5Scalar(container, key) {
  if (!container) return null;

  if (container.attrs?.[key]) {
    return unwrapHdf5Value(container.attrs[key].value);
  }

  if (container.keys?.().includes(key)) {
    const child = container.get(key);
    if (!child?.keys) return unwrapHdf5Value(child.value);
  }

  return null;
}

function readHdf5Record(group, key) {
  const item = group.get(key);
  const record = {
    key,
    identifier: readHdf5Scalar(item, "identifier") ?? key
  };

  for (const [attrName, attribute] of Object.entries(item.attrs ?? {})) {
    record[attrName] = unwrapHdf5Value(attribute.value);
  }

  for (const datasetName of item.keys?.() ?? []) {
    const dataset = item.get(datasetName);
    if (!dataset?.keys) {
      record[datasetName] = unwrapHdf5Value(dataset.value);
    }
  }

  return record;
}

function readHdf5GroupRecords(parent, groupName) {
  if (!parent.keys?.().includes(groupName)) return [];

  const group = parent.get(groupName);
  return group.keys().sort(naturalCompare).map((key) => readHdf5Record(group, key));
}

async function parseHdf5Results(file) {
  const { default: h5wasm } = await import("h5wasm");
  const { FS } = await h5wasm.ready;
  const path = `/opensd-${Date.now()}-${file.name}`;
  const bytes = new Uint8Array(await file.arrayBuffer());

  FS.writeFile(path, bytes);

  const h5file = new h5wasm.File(path, "r");
  try {
    const circuitsGroup = h5file.get("circuits");
    const circuits = circuitsGroup.keys().sort(naturalCompare).map((key) => {
      const circuit = circuitsGroup.get(key);

      return {
        key,
        identifier: readHdf5Scalar(circuit, "identifier") ?? key,
        flname: readHdf5Scalar(circuit, "flname") ?? "",
        fltype: readHdf5Scalar(circuit, "fltype") ?? "",
        nodes: readHdf5GroupRecords(circuit, "nodes"),
        faces: readHdf5GroupRecords(circuit, "faces"),
        pipes: readHdf5GroupRecords(circuit, "pipes")
      };
    });

    return { circuits };
  } finally {
    h5file.close();
    try {
      FS.unlink(path);
    } catch {
      // The virtual file is temporary; a failed unlink should not block plotting.
    }
  }
}

function getNodeValue(record, variable, cp) {
  if (variable === "temperature_from_tenth") {
    const enthalpy = Number(record.tenth_gues);
    const heatCapacity = Number(cp);
    if (!Number.isFinite(enthalpy) || !Number.isFinite(heatCapacity) || heatCapacity === 0) {
      return null;
    }

    return enthalpy / heatCapacity;
  }

  const value = Number(record[variable]);
  return Number.isFinite(value) ? value : null;
}

function getPipeNodeSeries(circuit, variable, pipeFilter, cp) {
  if (!circuit) return [];

  return circuit.nodes
    .filter((node) => {
      const id = String(node.identifier ?? "");
      if (pipeFilter === "__all__") return true;
      return id === pipeFilter || id.startsWith(`${pipeFilter}_node`);
    })
    .map((node) => ({
      label: String(node.identifier ?? node.key),
      value: getNodeValue(node, variable, cp)
    }))
    .filter((point) => point.value != null)
    .sort((a, b) => naturalCompare(a.label, b.label));
}

function LinePlot({ data, variableLabel }) {
  const width = 640;
  const height = 300;
  const pad = { top: 18, right: 18, bottom: 52, left: 64 };

  if (!data.length) {
    return <div className="empty-plot">No numeric values found for this selection.</div>;
  }

  const minY = Math.min(...data.map((point) => point.value));
  const maxY = Math.max(...data.map((point) => point.value));
  const spanY = maxY - minY || 1;
  const innerWidth = width - pad.left - pad.right;
  const innerHeight = height - pad.top - pad.bottom;

  const points = data.map((point, index) => {
    const x = pad.left + (data.length === 1 ? innerWidth / 2 : (index / (data.length - 1)) * innerWidth);
    const y = pad.top + innerHeight - ((point.value - minY) / spanY) * innerHeight;
    return { ...point, x, y };
  });

  const pathData = points.map((point, index) => `${index === 0 ? "M" : "L"} ${point.x} ${point.y}`).join(" ");
  const labelStep = Math.ceil(points.length / 8);

  return (
    <svg className="line-plot" viewBox={`0 0 ${width} ${height}`} role="img">
      <title>{variableLabel}</title>
      <line x1={pad.left} y1={pad.top} x2={pad.left} y2={pad.top + innerHeight} />
      <line x1={pad.left} y1={pad.top + innerHeight} x2={pad.left + innerWidth} y2={pad.top + innerHeight} />
      <text x={pad.left - 10} y={pad.top + 4} textAnchor="end">
        {maxY.toPrecision(5)}
      </text>
      <text x={pad.left - 10} y={pad.top + innerHeight} textAnchor="end">
        {minY.toPrecision(5)}
      </text>
      <path d={pathData} />
      {points.map((point, index) => (
        <g key={point.label}>
          <circle cx={point.x} cy={point.y} r="4" />
          {index % labelStep === 0 && (
            <text className="x-label" x={point.x} y={height - 24} textAnchor="middle">
              {point.label}
            </text>
          )}
        </g>
      ))}
    </svg>
  );
}

function parseGeometryXml(xmlText) {
  const parser = new DOMParser();
  const document = parser.parseFromString(xmlText, "application/xml");
  const parseError = document.querySelector("parsererror");

  if (parseError) {
    throw new Error("The selected file is not valid XML.");
  }

  const geometry = document.querySelector("geometry");
  if (!geometry) {
    throw new Error("Could not find a <geometry> root element.");
  }

  const heatHandleMap = buildHeatHandleMap(geometry, attr);

  const flowNodes = [];
  const flowEdges = [];
  const nodeIds = new Set();
  const stats = {
    circuits: 0,
    nodes: 0,
    pipes: 0,
    hslabs: 0,
    bcs: 0
  };

  const pushNode = (node) => {
    if (!nodeIds.has(node.id)) {
      nodeIds.add(node.id);
      flowNodes.push(node);
    }
  };

  const pushEdge = (edge) => {
    if (nodeIds.has(edge.source) && nodeIds.has(edge.target)) {
      flowEdges.push(edge);
    }
  };

  const circuits = Array.from(geometry.querySelectorAll(":scope > circuit"));

  circuits.forEach((circuit, circuitIndex) => {
    const circuitId = attr(circuit, "identifier", `circuit_${circuitIndex + 1}`);
    const circuitNodeId = `circuit:${circuitId}`;

    stats.circuits += 1;

    const nodeElements = Array.from(circuit.querySelectorAll(":scope > node"));
    const pipeElements = Array.from(circuit.querySelectorAll(":scope > pipe"));
    const bcElements = Array.from(circuit.querySelectorAll(":scope > bc"));

    const nodeByName = new Map();
    const nodeNames = [];

    nodeElements.forEach((nodeEl, nodeIndex) => {
      const nodeName = attr(nodeEl, "identifier", `node_${nodeIndex + 1}`);
      nodeNames.push(nodeName);
      nodeByName.set(nodeName, nodeEl);
      stats.nodes += 1;
    });

    const pipes = pipeElements.map((pipeEl, pipeIndex) => {
      const pipeName = attr(pipeEl, "identifier", `pipe_${pipeIndex + 1}`);
      stats.pipes += 1;
      return {
        name: pipeName,
        unode: attr(pipeEl, "unode"),
        dnode: attr(pipeEl, "dnode"),
        element: pipeEl
      };
    });

    const bcRecords = bcElements.map((bcEl, bcIndex) => {
      const bcName = attr(bcEl, "identifier", `bc_${bcIndex + 1}`);
      stats.bcs += 1;
      return {
        id: `bc:${circuitId}:${bcName}`,
        name: bcName,
        targetNode: attr(bcEl, "node"),
        element: bcEl
      };
    });

    const sequence = buildHorizontalSequence(nodeNames, pipes, naturalCompare);
    const { positions } = layoutCircuitRow({
      circuitIndex,
      circuitId,
      sequence,
      bcRecords
    });

    pushNode(
      buildPipeNode({
        id: circuitNodeId,
        position: positions.get(circuitNodeId),
        identifier: circuitId,
        kind: "circuit",
        details: [attr(circuit, "solveSS") && `solveSS ${attr(circuit, "solveSS")}`],
        heatHandleMap
      })
    );

    const pipeByName = new Map(pipes.map((pipe) => [pipe.name, pipe]));

    for (const item of sequence) {
      if (item.kind === "node") {
        const nodeEl = nodeByName.get(item.name);
        pushNode(
          buildFlowNode({
            id: `node:${item.name}`,
            position: positions.get(`node:${item.name}`),
            identifier: item.name,
            details: [
              attr(nodeEl, "fixed_var") && `fixed ${attr(nodeEl, "fixed_var")}`,
              attr(nodeEl, "volume") && `vol ${Number(attr(nodeEl, "volume")).toExponential(2)}`
            ],
            heatHandleMap
          })
        );
        continue;
      }

      const pipe = pipeByName.get(item.name);
      pushNode(
        buildPipeNode({
          id: `pipe:${item.name}`,
          position: positions.get(`pipe:${item.name}`),
          identifier: item.name,
          details: [
            attr(pipe.element, "ncell") && `${attr(pipe.element, "ncell")} cells`,
            attr(pipe.element, "diameter") && `D ${attr(pipe.element, "diameter")} m`,
            pipe.unode && pipe.dnode && `${pipe.unode} → ${pipe.dnode}`
          ],
          heatHandleMap
        })
      );
    }

    for (const pipe of pipes) {
      for (const nodeName of [pipe.unode, pipe.dnode]) {
        if (!nodeName || !nodeByName.has(nodeName)) continue;
        const nodeId = `node:${nodeName}`;
        if (nodeIds.has(nodeId)) continue;

        const nodeEl = nodeByName.get(nodeName);
        const fallbackX = (positions.get(`pipe:${pipe.name}`)?.x ?? 0) + PIPE_STEP;
        pushNode(
          buildFlowNode({
            id: nodeId,
            position: positions.get(nodeId) ?? { x: fallbackX, y: positions.get(circuitNodeId)?.y ?? 50 },
            identifier: nodeName,
            details: [
              attr(nodeEl, "fixed_var") && `fixed ${attr(nodeEl, "fixed_var")}`,
              attr(nodeEl, "volume") && `vol ${Number(attr(nodeEl, "volume")).toExponential(2)}`
            ],
            heatHandleMap
          })
        );
      }
    }

    for (const bc of bcRecords) {
      const bcPosition = positions.get(bc.id);
      if (!bcPosition) continue;

      const bcEl = bc.element;
      pushNode(
        buildBcNode({
          id: bc.id,
          position: bcPosition,
          identifier: bc.name,
          details: [
            attr(bcEl, "var") && `${attr(bcEl, "var")} = ${attr(bcEl, "val")}`,
            bc.targetNode && `node ${bc.targetNode}`
          ]
        })
      );
    }

    if (sequence.length > 0) {
      const first = sequence[0];
      const firstId = first.kind === "node" ? `node:${first.name}` : `pipe:${first.name}`;
      pushEdge(flowEdge(`${circuitNodeId}->${firstId}`, circuitNodeId, firstId));
    }

    for (const pipe of pipes) {
      const pipeId = `pipe:${pipe.name}`;
      pushEdge(flowEdge(`node:${pipe.unode}->${pipeId}`, `node:${pipe.unode}`, pipeId));
      pushEdge(flowEdge(`${pipeId}->node:${pipe.dnode}`, pipeId, `node:${pipe.dnode}`));
    }

    for (const bc of bcRecords) {
      if (!positions.get(bc.id)) continue;
      pushEdge(bcEdge(`${bc.id}->node:${bc.targetNode}`, bc.id, `node:${bc.targetNode}`));
    }
  });

  const hslabs = Array.from(geometry.querySelectorAll(":scope > hslab"));
  const hslabBaseY = 50 + circuits.length * CIRCUIT_ROW_GAP + 60;

  hslabs.forEach((hslab, hslabIndex) => {
    const hslabName = attr(hslab, "identifier", `hslab_${hslabIndex + 1}`);
    const hslabId = `hslab:${hslabName}`;
    const layers = Array.from(hslab.querySelectorAll(":scope > layer"));
    const hslabOffset = hslabIndex * 220;

    stats.hslabs += 1;

    pushNode(
      buildPipeNode({
        id: hslabId,
        position: { x: 220 + hslabOffset, y: hslabBaseY },
        identifier: hslabName,
        kind: "hslab",
        details: [
          attr(hslab, "uvar") && `u ${attr(hslab, "uvar")}`,
          attr(hslab, "dvar") && `d ${attr(hslab, "dvar")}`,
          ...layerTooltipLines(layers)
        ],
        heatHandleMap
      })
    );

    const ucomp = attr(hslab, "ucomp");
    const dcomp = attr(hslab, "dcomp");
    const upstreamId = attr(hslab, "uvar") === "pipe" ? `pipe:${ucomp}` : `node:${ucomp}`;
    const downstreamId = attr(hslab, "dvar") === "pipe" ? `pipe:${dcomp}` : `node:${dcomp}`;

    pushEdge(hslabEdge(`${upstreamId}->${hslabId}`, upstreamId, hslabId));
    pushEdge(hslabEdge(`${hslabId}->${downstreamId}`, hslabId, downstreamId));
  });

  return { nodes: flowNodes, edges: flowEdges, stats };
}

export default function App() {
  const fileInput = useRef(null);
  const hdf5Input = useRef(null);
  const [nodes, setNodes, onNodesChange] = useNodesState(initialNodes);
  const [edges, setEdges, onEdgesChange] = useEdgesState(initialEdges);
  const [importStatus, setImportStatus] = useState("No geometry loaded");
  const [modelStats, setModelStats] = useState(null);
  const [activeWorkspace, setActiveWorkspace] = useState("model");
  const [resultsStatus, setResultsStatus] = useState("No results loaded");
  const [results, setResults] = useState(null);
  const [selectedCircuit, setSelectedCircuit] = useState("");
  const [selectedPipe, setSelectedPipe] = useState("__all__");
  const [selectedVariable, setSelectedVariable] = useState("ttemp_gues");
  const [cpValue, setCpValue] = useState("1267");
  const [fitViewTrigger, setFitViewTrigger] = useState(0);

  const onConnect = useCallback(
    (params) => setEdges((eds) => addEdge({ ...params, type: "smoothstep" }, eds)),
    [setEdges]
  );

  const onDragStart = (event, nodeType) => {
    event.dataTransfer.setData("application/reactflow", nodeType);
    event.dataTransfer.effectAllowed = "move";
  };

  const onDrop = useCallback(
    (event) => {
      event.preventDefault();

      const type = event.dataTransfer.getData("application/reactflow");
      if (!type) return;

      const isPipe = type === "Pipe" || type === "Pump" || type === "Valve";
      const newNode = isPipe
        ? buildPipeNode({
            id: getId(),
            position: { x: event.clientX - 260, y: event.clientY - 60 },
            identifier: type,
            kind: "manual"
          })
        : buildFlowNode({
            id: getId(),
            position: { x: event.clientX - 260, y: event.clientY - 60 },
            identifier: type
          });

      setNodes((nds) => nds.concat(newNode));
    },
    [setNodes]
  );

  const onDragOver = useCallback((event) => {
    event.preventDefault();
    event.dataTransfer.dropEffect = "move";
  }, []);

  const importGeometry = useCallback(
    async (file) => {
      if (!file) return;

      try {
        const xmlText = await file.text();
        const parsed = parseGeometryXml(xmlText);

        setNodes(parsed.nodes);
        setEdges(parsed.edges);
        setModelStats(parsed.stats);
        setFitViewTrigger((count) => count + 1);
        setImportStatus(`Loaded ${file.name}`);
        setActiveWorkspace("model");
      } catch (error) {
        setImportStatus(error.message);
      }
    },
    [setEdges, setNodes]
  );

  const importResults = useCallback(async (file) => {
    if (!file) return;

    try {
      setResultsStatus("Reading HDF5...");
      const parsed = await parseHdf5Results(file);
      const firstCircuit = parsed.circuits[0]?.key ?? "";

      setResults(parsed);
      setSelectedCircuit(firstCircuit);
      setSelectedPipe("__all__");
      setSelectedVariable(
        parsed.circuits[0]?.nodes.some((node) => Object.hasOwn(node, "ttemp_gues"))
          ? "ttemp_gues"
          : "temperature_from_tenth"
      );
      setResultsStatus(`Loaded ${file.name}`);
      setActiveWorkspace("postprocess");
    } catch (error) {
      setResults(null);
      setResultsStatus(error.message);
    }
  }, []);

  const exportJSON = () => {
    const data = { nodes, edges };
    const blob = new Blob([JSON.stringify(data, null, 2)], {
      type: "application/json"
    });
    const url = URL.createObjectURL(blob);
    const anchor = document.createElement("a");

    anchor.href = url;
    anchor.download = "opensd_flow.json";
    anchor.click();

    URL.revokeObjectURL(url);
  };

  const summaryItems = useMemo(() => {
    if (!modelStats) return [];

    return [
      ["Circuits", modelStats.circuits],
      ["Flow Nodes", modelStats.nodes],
      ["Pipes", modelStats.pipes],
      ["Heat Slabs", modelStats.hslabs],
      ["BCs", modelStats.bcs]
    ];
  }, [modelStats]);

  const activeCircuit = useMemo(() => {
    return results?.circuits.find((circuit) => circuit.key === selectedCircuit) ?? null;
  }, [results, selectedCircuit]);

  const pipeOptions = useMemo(() => {
    if (!activeCircuit) return [];

    const pipeNames = new Set();
    activeCircuit.nodes.forEach((node) => {
      const id = String(node.identifier ?? "");
      const match = id.match(/^(.+)_node\d+$/);
      if (match) pipeNames.add(match[1]);
    });

    activeCircuit.pipes.forEach((pipe) => {
      if (pipe.identifier) pipeNames.add(String(pipe.identifier));
    });

    return Array.from(pipeNames).sort(naturalCompare);
  }, [activeCircuit]);

  const availableVariables = useMemo(() => {
    if (!activeCircuit) return nodeVariables;

    const keys = new Set(activeCircuit.nodes.flatMap((node) => Object.keys(node)));
    return nodeVariables.filter((variable) => {
      return variable.value === "temperature_from_tenth" || keys.has(variable.value);
    });
  }, [activeCircuit]);

  const plotData = useMemo(() => {
    return getPipeNodeSeries(activeCircuit, selectedVariable, selectedPipe, cpValue);
  }, [activeCircuit, cpValue, selectedPipe, selectedVariable]);

  const selectedVariableLabel = useMemo(() => {
    return nodeVariables.find((variable) => variable.value === selectedVariable)?.label ?? selectedVariable;
  }, [selectedVariable]);

  return (
    <div className="app-shell">
      <aside className="sidebar">
        <div className="brand-block">
          <h1>OpenSD</h1>
          <p>Model graph</p>
        </div>

        <section className="panel">
          <div className="panel-title">Geometry</div>
          <input
            ref={fileInput}
            className="file-input"
            type="file"
            accept=".xml,text/xml,application/xml"
            onChange={(event) => importGeometry(event.target.files?.[0])}
          />
          <button className="primary-button" onClick={() => fileInput.current?.click()}>
            Import XML
          </button>
          <div className="status-text">{importStatus}</div>
        </section>

        {summaryItems.length > 0 && (
          <section className="panel metrics">
            {summaryItems.map(([label, value]) => (
              <div key={label} className="metric-row">
                <span>{label}</span>
                <strong>{value}</strong>
              </div>
            ))}
          </section>
        )}

        <section className="panel">
          <div className="panel-title">Components</div>
          <div className="component-list">
            {componentTypes.map((component) => (
              <button
                key={component}
                className="component-chip"
                draggable
                onDragStart={(event) => onDragStart(event, component)}
              >
                {component}
              </button>
            ))}
          </div>
        </section>

        <button className="secondary-button" onClick={exportJSON}>
          Export JSON
        </button>
      </aside>

      <main className="workspace">
        <div className="workspace-tabs">
          <button
            className={activeWorkspace === "model" ? "active" : ""}
            onClick={() => setActiveWorkspace("model")}
          >
            Model
          </button>
          <button
            className={activeWorkspace === "postprocess" ? "active" : ""}
            onClick={() => setActiveWorkspace("postprocess")}
          >
            Postprocess
          </button>
        </div>

        {activeWorkspace === "model" && (
          <ModelFlowCanvas
            nodes={nodes}
            edges={edges}
            onNodesChange={onNodesChange}
            onEdgesChange={onEdgesChange}
            onConnect={onConnect}
            onDrop={onDrop}
            onDragOver={onDragOver}
            fitViewTrigger={fitViewTrigger}
          />
        )}

        {activeWorkspace === "postprocess" && (
          <section className="workspace-pane postprocess-view">
            <input
              ref={hdf5Input}
              className="file-input"
              type="file"
              accept=".h5,.hdf5,application/x-hdf5"
              onChange={(event) => importResults(event.target.files?.[0])}
            />

            <div className="post-toolbar">
              <button className="primary-button" onClick={() => hdf5Input.current?.click()}>
                Import HDF5
              </button>

              {results && (
                <div className="post-controls">
                  <label>
                    Circuit
                    <select value={selectedCircuit} onChange={(event) => setSelectedCircuit(event.target.value)}>
                      {results.circuits.map((circuit) => (
                        <option key={circuit.key} value={circuit.key}>
                          {circuit.identifier}
                        </option>
                      ))}
                    </select>
                  </label>

                  <label>
                    Pipe
                    <select value={selectedPipe} onChange={(event) => setSelectedPipe(event.target.value)}>
                      <option value="__all__">All nodes</option>
                      {pipeOptions.map((pipe) => (
                        <option key={pipe} value={pipe}>
                          {pipe}
                        </option>
                      ))}
                    </select>
                  </label>

                  <label>
                    Variable
                    <select value={selectedVariable} onChange={(event) => setSelectedVariable(event.target.value)}>
                      {availableVariables.map((variable) => (
                        <option key={variable.value} value={variable.value}>
                          {variable.label}
                        </option>
                      ))}
                    </select>
                  </label>

                  {selectedVariable === "temperature_from_tenth" && (
                    <label>
                      Cp
                      <input
                        value={cpValue}
                        inputMode="decimal"
                        onChange={(event) => setCpValue(event.target.value)}
                      />
                    </label>
                  )}
                </div>
              )}
            </div>

            <div className="status-strip">{resultsStatus}</div>

            <div className="plot-shell">
              <div className="results-header">
                <h2>{selectedVariableLabel}</h2>
                <span>{plotData.length} points</span>
              </div>
              <LinePlot data={plotData} variableLabel={selectedVariableLabel} />
              {plotData.length > 0 && (
                <div className="data-table">
                  {plotData.map((point) => (
                    <div key={point.label}>
                      <span>{point.label}</span>
                      <strong>{point.value.toPrecision(7)}</strong>
                    </div>
                  ))}
                </div>
              )}
            </div>
          </section>
        )}
      </main>
    </div>
  );
}
