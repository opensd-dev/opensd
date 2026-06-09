import { useCallback, useEffect, useMemo, useRef, useState } from "react";
import { addEdge, useEdgesState, useNodesState } from "reactflow";

import {
  FLOW_MARKER,
  HEAT_MARKER,
} from "./edgeUtils.js";

import "reactflow/dist/style.css";
import "./App.css";
import ModelFlowCanvas from "./ModelFlowCanvas.jsx";
import guiRequirementsMarkdown from "../requirements/opensd-web-gui.md?raw";
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
const LAYOUT_STORAGE_KEY = "opensd-web:component-layouts:v1";
const MANUAL_LAYOUT_KEY = "manual";
const MAX_UNDO_STEPS = 80;
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

function componentKindForType(type) {
  if (type === "Node") return "flow";
  if (type === "BC") return "bc";
  if (type === "HSlab") return "hslab";
  if (type === "Pump" || type === "Valve") return type.toLowerCase();
  return "pipe";
}

let id = 0;
const getId = () => `manual_${id++}`;

function emptyHeat() {
  return {
    htTopIn: false,
    htTopOut: false,
    htBottomIn: false,
    htBottomOut: false,
    htSideIn: false,
    htSideOut: false
  };
}

function emptyFlowConnections() {
  return { upstream: false, downstream: false };
}

function heatFor(id, heatHandleMap) {
  return heatHandleMap?.get(id) ?? emptyHeat();
}

function buildFlowNode({ id, position, identifier, details = [], heatHandleMap }) {
  return {
    id,
    type: "flow",
    position,
    data: { ...nodeData(identifier, details), heat: heatFor(id, heatHandleMap), rotation: 0 }
  };
}

function buildManualComponent(type, position) {
  const componentKind = componentKindForType(type);

  if (componentKind === "flow") {
    return buildFlowNode({
      id: getId(),
      position,
      identifier: type
    });
  }

  if (componentKind === "bc") {
    return buildBcNode({
      id: getId(),
      position,
      identifier: type
    });
  }

  return buildPipeNode({
    id: getId(),
    position,
    identifier: type,
    kind: componentKind
  });
}

function buildPipeNode({ id, position, identifier, details = [], kind = "pipe", heatHandleMap }) {
  return {
    id,
    type: "pipe",
    position,
    data: {
      ...nodeData(identifier, details),
      kind,
      heat: heatFor(id, heatHandleMap),
      flowConnections: emptyFlowConnections(),
      rotation: 0
    }
  };
}

function buildBcNode({ id, position, identifier, details = [] }) {
  return {
    id,
    type: "bc",
    position,
    data: { ...nodeData(identifier, details), rotation: 0 }
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
    className: "flow-edge",
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

function heatHandlesFromPositions(source, target, positionById) {
  const sourceY = positionById.get(source)?.y ?? 0;
  const targetY = positionById.get(target)?.y ?? sourceY;
  const targetIsBelow = targetY >= sourceY;

  return targetIsBelow
    ? { sourceHandle: "ht-bottom-out", targetHandle: "ht-top-in" }
    : { sourceHandle: "ht-top-out", targetHandle: "ht-bottom-in" };
}

function hslabEdge(id, source, target, positionById) {
  return {
    id,
    source,
    target,
    ...heatHandlesFromPositions(source, target, positionById),
    type: "straight",
    animated: false,
    className: "hslab-edge",
    markerEnd: HEAT_MARKER
  };
}

function markHeatEndpoint(flags, nodeId, handleId) {
  if (!nodeId || !handleId) return;
  if (!flags.has(nodeId)) flags.set(nodeId, emptyHeat());
  const entry = flags.get(nodeId);

  if (handleId.startsWith("ht-top-") || handleId.startsWith("ht-bottom-")) {
    entry.htTopIn = true;
    entry.htTopOut = true;
    entry.htBottomIn = true;
    entry.htBottomOut = true;
    return;
  }

  if (handleId === "ht-top-in") entry.htTopIn = true;
  if (handleId === "ht-top-out") entry.htTopOut = true;
  if (handleId === "ht-bottom-in") entry.htBottomIn = true;
  if (handleId === "ht-bottom-out") entry.htBottomOut = true;
  if (handleId === "ht-side-in") entry.htSideIn = true;
  if (handleId === "ht-side-out") entry.htSideOut = true;
}

function buildHeatHandleMapFromEdges(edges) {
  const flags = new Map();

  for (const edge of edges) {
    if (edge.className !== "hslab-edge") continue;
    markHeatEndpoint(flags, edge.source, edge.sourceHandle);
    markHeatEndpoint(flags, edge.target, edge.targetHandle);
  }

  return flags;
}

function applyHeatHandleMap(nodes, heatHandleMap) {
  return nodes.map((node) => ({
    ...node,
    data: {
      ...node.data,
      heat: heatFor(node.id, heatHandleMap)
    }
  }));
}

function rerouteHeatEdges(nodes, edges) {
  const positionById = new Map(nodes.map((node) => [node.id, node.position]));

  return edges.map((edge) => {
    if (edge.className !== "hslab-edge") return edge;

    return {
      ...edge,
      type: "straight",
      ...heatHandlesFromPositions(edge.source, edge.target, positionById)
    };
  });
}

function flowHandlesFromPositions(source, target, positionById) {
  const sourceX = positionById.get(source)?.x ?? 0;
  const targetX = positionById.get(target)?.x ?? sourceX;
  const targetIsRight = targetX >= sourceX;

  return targetIsRight
    ? { sourceHandle: "flow-out", targetHandle: "flow-in" }
    : { sourceHandle: "flow-in", targetHandle: "flow-out" };
}

function rerouteFlowEdges(nodes, edges) {
  const positionById = new Map(nodes.map((node) => [node.id, node.position]));

  return edges.map((edge) => {
    if (edge.className !== "flow-edge") return edge;

    return {
      ...edge,
      ...flowHandlesFromPositions(edge.source, edge.target, positionById)
    };
  });
}

function attr(element, name, fallback = "") {
  return element?.getAttribute(name) ?? fallback;
}

function layoutKeyForFile(file) {
  return file?.name ? `geometry:${file.name}` : MANUAL_LAYOUT_KEY;
}

function readStoredLayouts() {
  try {
    const stored = window.localStorage.getItem(LAYOUT_STORAGE_KEY);
    return stored ? JSON.parse(stored) : {};
  } catch {
    return {};
  }
}

function readStoredLayout(layoutKey) {
  return readStoredLayouts()[layoutKey] ?? null;
}

function saveStoredLayout(layoutKey, nodes) {
  if (!layoutKey || !nodes.length) return;

  const layout = {};
  for (const node of nodes) {
    layout[node.id] = {
      position: node.position,
      rotation: node.data?.rotation ?? 0
    };
  }

  const layouts = readStoredLayouts();
  layouts[layoutKey] = {
    savedAt: new Date().toISOString(),
    nodes: layout
  };

  try {
    window.localStorage.setItem(LAYOUT_STORAGE_KEY, JSON.stringify(layouts));
  } catch {
    // Layout persistence is helpful, but storage limits should not block graph editing.
  }
}

function applyStoredLayout(nodes, storedLayout) {
  if (!storedLayout?.nodes) return nodes;

  return nodes.map((node) => {
    const saved = storedLayout.nodes[node.id];
    if (!saved) return node;

    return {
      ...node,
      position: saved.position ?? node.position,
      data: {
        ...node.data,
        rotation: saved.rotation ?? node.data?.rotation ?? 0
      }
    };
  });
}

function clearStoredLayout(layoutKey) {
  const layouts = readStoredLayouts();
  delete layouts[layoutKey];

  try {
    window.localStorage.setItem(LAYOUT_STORAGE_KEY, JSON.stringify(layouts));
  } catch {
    // A failed cleanup should not block returning to the default generated layout.
  }
}

function isFlowNodeId(nodeId) {
  return nodeId?.startsWith("node:");
}

function buildFlowConnectionMap(nodes, edges) {
  const nodeTypeById = new Map(nodes.map((node) => [node.id, node.type]));
  const connections = new Map();

  for (const node of nodes) {
    if (node.type === "pipe") connections.set(node.id, emptyFlowConnections());
  }

  for (const edge of edges) {
    const sourceIsFluidNode = nodeTypeById.get(edge.source) === "flow" || isFlowNodeId(edge.source);
    const targetIsFluidNode = nodeTypeById.get(edge.target) === "flow" || isFlowNodeId(edge.target);

    if (connections.has(edge.target) && edge.targetHandle === "flow-in" && sourceIsFluidNode) {
      connections.get(edge.target).upstream = true;
    }

    if (connections.has(edge.source) && edge.sourceHandle === "flow-out" && targetIsFluidNode) {
      connections.get(edge.source).downstream = true;
    }
  }

  return connections;
}

function normalizedFlowConnection(params, nodes) {
  const nodeTypeById = new Map(nodes.map((node) => [node.id, node.type]));
  const sourceIsPipe = nodeTypeById.get(params.source) === "pipe";
  const targetIsPipe = nodeTypeById.get(params.target) === "pipe";

  return {
    ...params,
    sourceHandle: sourceIsPipe ? "flow-out" : params.sourceHandle,
    targetHandle: targetIsPipe ? "flow-in" : params.targetHandle,
    type: "straight",
    className: "flow-edge",
    markerEnd: FLOW_MARKER
  };
}

function cloneGraphState(nodes, edges) {
  return {
    nodes: nodes.map((node) => ({
      ...node,
      position: { ...node.position },
      data: { ...node.data }
    })),
    edges: edges.map((edge) => ({ ...edge }))
  };
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

function parseRequirementSections(markdown) {
  const sectionPattern = /^## (?!Appendix\b)(.+?)\s*$/gm;
  const anySectionPattern = /^## .+?\s*$/gm;
  const headingPattern = /^### (GUI-\d{3})\s+[—-]\s+(.+?)\s+\((Must|Should)\)\s*$/gm;
  const sectionMatches = Array.from(markdown.matchAll(sectionPattern));
  const anySectionMatches = Array.from(markdown.matchAll(anySectionPattern));
  const matches = Array.from(markdown.matchAll(headingPattern));

  return matches.map((match, index) => {
    const nextMatch = matches[index + 1];
    const parentSection = sectionMatches.findLast((sectionMatch) => sectionMatch.index < match.index);
    const nextSection = anySectionMatches.find((sectionMatch) => sectionMatch.index > match.index);
    const sectionStart = match.index;
    const bodyStart = match.index + match[0].length;
    const sectionEnd = Math.min(
      ...[nextMatch?.index, nextSection?.index, markdown.length].filter((position) => position != null)
    );
    const body = markdown.slice(bodyStart, sectionEnd).replace(/^\r?\n/, "").replace(/\r?\n---\s*$/m, "").trim();

    return {
      id: match[1],
      summary: match[2],
      priority: match[3],
      sectionTitle: parentSection?.[1] ?? "Requirements",
      body,
      sectionStart,
      sectionEnd
    };
  });
}

function renderMarkdownInline(text) {
  const parts = [];
  const pattern = /(\*\*[^*]+\*\*|`[^`]+`|\[[^\]]+\]\([^)]+\))/g;
  let cursor = 0;

  for (const match of text.matchAll(pattern)) {
    if (match.index > cursor) {
      parts.push(text.slice(cursor, match.index));
    }

    const token = match[0];
    const key = `${match.index}-${token}`;

    if (token.startsWith("**")) {
      parts.push(<strong key={key}>{token.slice(2, -2)}</strong>);
    } else if (token.startsWith("`")) {
      parts.push(<code key={key}>{token.slice(1, -1)}</code>);
    } else {
      const linkMatch = token.match(/^\[([^\]]+)\]\(([^)]+)\)$/);
      parts.push(
        <a key={key} href={linkMatch[2]}>
          {linkMatch[1]}
        </a>
      );
    }

    cursor = match.index + token.length;
  }

  if (cursor < text.length) {
    parts.push(text.slice(cursor));
  }

  return parts;
}

function MarkdownViewer({ text }) {
  return (
    <div className="requirements-body">
      {text.split(/\r?\n+/).filter(Boolean).map((block, index) => (
        <p key={`${index}-${block.slice(0, 16)}`}>{renderMarkdownInline(block)}</p>
      ))}
    </div>
  );
}

function RequirementManager({ requirements, selectedId, onSelect }) {
  const selectedRequirement = requirements.find((requirement) => requirement.id === selectedId);
  const sections = requirements.reduce((groups, requirement) => {
    const section = groups.find((group) => group.title === requirement.sectionTitle);
    if (section) {
      section.requirements.push(requirement);
    } else {
      groups.push({ title: requirement.sectionTitle, requirements: [requirement] });
    }
    return groups;
  }, []);

  return (
    <section className="workspace-pane requirements-view">
      <div className="requirements-toolbar">
        <div className="panel-title">Project Requirements</div>
        <div className="status-strip">Read-only view of bundled GUI requirements</div>
      </div>

      <div className="requirements-shell">
        <nav className="requirements-list" aria-label="Requirements">
          {sections.map((section) => (
            <div key={section.title} className="requirements-nav-section">
              <h3>{section.title}</h3>
              {section.requirements.map((requirement) => (
                <button
                  key={requirement.id}
                  className={requirement.id === selectedId ? "active" : ""}
                  onClick={() => onSelect(requirement.id)}
                >
                  <strong>{requirement.id}</strong>
                  <span>{requirement.summary}</span>
                </button>
              ))}
            </div>
          ))}
        </nav>

        <div className="requirements-editor">
          {selectedRequirement ? (
            <>
              <div className="requirements-editor-header">
                <div>
                  <em>{selectedRequirement.sectionTitle}</em>
                  <span>{selectedRequirement.id}</span>
                  <h2>{selectedRequirement.summary}</h2>
                </div>
                <strong>{selectedRequirement.priority}</strong>
              </div>

              <MarkdownViewer text={selectedRequirement.body} />
            </>
          ) : (
            <div className="empty-plot">No requirements found in this Markdown document.</div>
          )}
        </div>
      </div>
    </section>
  );
}

function ComponentGlyph({ type }) {
  const kind = componentKindForType(type);

  if (kind === "flow") {
    return <span className="component-glyph component-glyph--node" aria-hidden="true" />;
  }

  if (kind === "bc") {
    return <span className="component-glyph component-glyph--bc" aria-hidden="true" />;
  }

  return <span className={`component-glyph component-glyph--pipe component-glyph--${kind}`} aria-hidden="true" />;
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
            ]
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
          ]
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
            ]
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
        ]
      })
    );

    const ucomp = attr(hslab, "ucomp");
    const dcomp = attr(hslab, "dcomp");
    const upstreamId = attr(hslab, "uvar") === "pipe" ? `pipe:${ucomp}` : `node:${ucomp}`;
    const downstreamId = attr(hslab, "dvar") === "pipe" ? `pipe:${dcomp}` : `node:${dcomp}`;

    const positionById = new Map(flowNodes.map((node) => [node.id, node.position]));

    pushEdge(hslabEdge(`${upstreamId}->${hslabId}`, upstreamId, hslabId, positionById));
    pushEdge(hslabEdge(`${hslabId}->${downstreamId}`, hslabId, downstreamId, positionById));
  });

  return { nodes: applyHeatHandleMap(flowNodes, buildHeatHandleMapFromEdges(flowEdges)), edges: flowEdges, stats };
}

export default function App() {
  const fileInput = useRef(null);
  const hdf5Input = useRef(null);
  const [nodes, setNodes, reactFlowOnNodesChange] = useNodesState(initialNodes);
  const [edges, setEdges, reactFlowOnEdgesChange] = useEdgesState(initialEdges);
  const [defaultLayoutNodes, setDefaultLayoutNodes] = useState(initialNodes);
  const [defaultLayoutEdges, setDefaultLayoutEdges] = useState(initialEdges);
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
  const [layoutKey, setLayoutKey] = useState(MANUAL_LAYOUT_KEY);
  const [hasUnsavedLayout, setHasUnsavedLayout] = useState(false);
  const [pendingComponentType, setPendingComponentType] = useState("");
  const [undoStack, setUndoStack] = useState([]);
  const [selectedRequirementId, setSelectedRequirementId] = useState("GUI-080");

  const parsedRequirements = useMemo(() => parseRequirementSections(guiRequirementsMarkdown), []);

  const selectedRequirement = useMemo(() => {
    return parsedRequirements.find((requirement) => requirement.id === selectedRequirementId) ?? parsedRequirements[0] ?? null;
  }, [parsedRequirements, selectedRequirementId]);

  const selectRequirement = useCallback(
    (requirementId) => {
      const nextRequirement = parsedRequirements.find((requirement) => requirement.id === requirementId);
      if (!nextRequirement) return;

      setSelectedRequirementId(nextRequirement.id);
    },
    [parsedRequirements]
  );

  const pushUndoSnapshot = useCallback(() => {
    setUndoStack((stack) => [...stack, cloneGraphState(nodes, edges)].slice(-MAX_UNDO_STEPS));
  }, [edges, nodes]);

  const undoLastAction = useCallback(() => {
    setUndoStack((stack) => {
      const previous = stack.at(-1);
      if (!previous) return stack;

      setNodes(previous.nodes);
      setEdges(previous.edges);
      setHasUnsavedLayout(true);
      return stack.slice(0, -1);
    });
  }, [setEdges, setNodes]);

  useEffect(() => {
    const warnBeforeClose = (event) => {
      if (!hasUnsavedLayout) return;
      event.preventDefault();
      event.returnValue = "";
    };

    window.addEventListener("beforeunload", warnBeforeClose);
    return () => window.removeEventListener("beforeunload", warnBeforeClose);
  }, [hasUnsavedLayout]);

  useEffect(() => {
    const handleKeyDown = (event) => {
      if (!(event.ctrlKey || event.metaKey) || event.key.toLowerCase() !== "z") return;
      event.preventDefault();
      undoLastAction();
    };

    window.addEventListener("keydown", handleKeyDown);
    return () => window.removeEventListener("keydown", handleKeyDown);
  }, [undoLastAction]);

  const onNodesChange = useCallback(
    (changes) => {
      if (changes.some((change) => change.type === "position" && change.dragging === true)) {
        pushUndoSnapshot();
      }
      reactFlowOnNodesChange(changes);
      if (changes.some((change) => change.type === "position" || change.type === "dimensions")) {
        setHasUnsavedLayout(true);
      }
    },
    [pushUndoSnapshot, reactFlowOnNodesChange]
  );

  const onEdgesChange = useCallback(
    (changes) => {
      if (changes.some((change) => change.type === "remove")) {
        pushUndoSnapshot();
      }
      reactFlowOnEdgesChange(changes);
      setHasUnsavedLayout(true);
    },
    [pushUndoSnapshot, reactFlowOnEdgesChange]
  );

  const rotateNode = useCallback(
    (nodeId) => {
      pushUndoSnapshot();
      setNodes((nds) =>
        nds.map((node) =>
          node.id === nodeId
            ? {
                ...node,
                data: {
                  ...node.data,
                  rotation: ((node.data.rotation ?? 0) + 90) % 360
                }
              }
            : node
        )
      );
      setHasUnsavedLayout(true);
    },
    [pushUndoSnapshot, setNodes]
  );

  const renderedEdges = useMemo(() => rerouteFlowEdges(nodes, rerouteHeatEdges(nodes, edges)), [edges, nodes]);

  const renderedNodes = useMemo(() => {
    const flowConnections = buildFlowConnectionMap(nodes, renderedEdges);
    const heatHandleMap = buildHeatHandleMapFromEdges(renderedEdges);

    return nodes.map((node) => ({
      ...node,
      data: {
        ...node.data,
        heat: heatFor(node.id, heatHandleMap),
        flowConnections: flowConnections.get(node.id) ?? node.data.flowConnections ?? emptyFlowConnections(),
        onRotate: rotateNode
      }
    }));
  }, [nodes, renderedEdges, rotateNode]);

  const onConnect = useCallback(
    (params) => {
      pushUndoSnapshot();
      setEdges((eds) => {
        const edge = normalizedFlowConnection(params, nodes);
        if (!edge) return eds;
        return addEdge(edge, eds);
      });
      setHasUnsavedLayout(true);
    },
    [nodes, pushUndoSnapshot, setEdges]
  );

  const onDragStart = (event, nodeType) => {
    setPendingComponentType(nodeType);
    event.dataTransfer.setData("application/reactflow", nodeType);
    event.dataTransfer.setData("text/plain", nodeType);
    event.dataTransfer.effectAllowed = "move";
  };

  const onDrop = useCallback(
    (event, position) => {
      event.preventDefault();

      const type = event.dataTransfer.getData("application/reactflow") || event.dataTransfer.getData("text/plain");
      if (!type) return;

      pushUndoSnapshot();
      setNodes((nds) => nds.concat(buildManualComponent(type, position)));
      setPendingComponentType("");
      setHasUnsavedLayout(true);
    },
    [pushUndoSnapshot, setNodes]
  );

  const onPaneClick = useCallback(
    (_event, position) => {
      if (!pendingComponentType) return;
      pushUndoSnapshot();
      setNodes((nds) => nds.concat(buildManualComponent(pendingComponentType, position)));
      setPendingComponentType("");
      setHasUnsavedLayout(true);
    },
    [pendingComponentType, pushUndoSnapshot, setNodes]
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
        const nextLayoutKey = layoutKeyForFile(file);
        const storedLayout = readStoredLayout(nextLayoutKey);

        setLayoutKey(nextLayoutKey);
        setDefaultLayoutNodes(parsed.nodes);
        setDefaultLayoutEdges(parsed.edges);
        setNodes(applyStoredLayout(parsed.nodes, storedLayout));
        setEdges(parsed.edges);
        setModelStats(parsed.stats);
        setFitViewTrigger((count) => count + 1);
        setImportStatus(`Loaded ${file.name}${storedLayout ? " with saved layout" : ""}`);
        setActiveWorkspace("model");
        setHasUnsavedLayout(false);
        setUndoStack([]);
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

  const newCircuit = () => {
    setLayoutKey(MANUAL_LAYOUT_KEY);
    setDefaultLayoutNodes(initialNodes);
    setDefaultLayoutEdges(initialEdges);
    setNodes(initialNodes);
    setEdges(initialEdges);
    setModelStats(null);
    setImportStatus("Blank circuit workspace");
    setActiveWorkspace("model");
    setHasUnsavedLayout(false);
    setUndoStack([]);
  };

  const saveLayout = () => {
    saveStoredLayout(layoutKey, nodes);
    setHasUnsavedLayout(false);
    setImportStatus("Layout saved");
  };

  const restoreDefaultLayout = () => {
    pushUndoSnapshot();
    clearStoredLayout(layoutKey);
    setNodes(defaultLayoutNodes);
    setEdges(defaultLayoutEdges);
    setFitViewTrigger((count) => count + 1);
    setHasUnsavedLayout(true);
    setImportStatus("Default layout restored. Save to keep it.");
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

        <section className="panel">
          <div className="panel-title">Circuit</div>
          <button className="primary-button" onClick={newCircuit}>
            New circuit
          </button>
          <button className="secondary-button secondary-button--inline" onClick={undoLastAction} disabled={!undoStack.length}>
            Undo
          </button>
          <button className="secondary-button secondary-button--inline" onClick={saveLayout} disabled={!nodes.length}>
            Save layout
          </button>
          <button
            className="secondary-button secondary-button--inline"
            onClick={restoreDefaultLayout}
            disabled={!defaultLayoutNodes.length && !nodes.length}
          >
            Default layout
          </button>
          {hasUnsavedLayout && <div className="status-text status-text--warning">Unsaved layout changes</div>}
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
              <div
                key={component}
                className={`component-chip ${pendingComponentType === component ? "component-chip--active" : ""}`}
                role="button"
                tabIndex={0}
                draggable
                onClick={() => setPendingComponentType(component)}
                onDragStart={(event) => onDragStart(event, component)}
                onKeyDown={(event) => {
                  if (event.key === "Enter" || event.key === " ") {
                    event.preventDefault();
                    setPendingComponentType(component);
                  }
                }}
              >
                <ComponentGlyph type={component} />
                <span>{component}</span>
              </div>
            ))}
          </div>
          {pendingComponentType && (
            <div className="status-text status-text--hint">Click the canvas to place {pendingComponentType}</div>
          )}
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
          <button
            className={activeWorkspace === "requirements" ? "active" : ""}
            onClick={() => setActiveWorkspace("requirements")}
          >
            Requirements
          </button>
        </div>

        {activeWorkspace === "model" && (
          <ModelFlowCanvas
            nodes={renderedNodes}
            edges={renderedEdges}
            onNodesChange={onNodesChange}
            onEdgesChange={onEdgesChange}
            onConnect={onConnect}
            onDrop={onDrop}
            onDragOver={onDragOver}
            onPaneClick={onPaneClick}
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

        {activeWorkspace === "requirements" && (
          <RequirementManager
            requirements={parsedRequirements}
            selectedId={selectedRequirement?.id ?? ""}
            onSelect={selectRequirement}
          />
        )}
      </main>
    </div>
  );
}
