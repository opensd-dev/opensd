import { useCallback, useEffect, useMemo, useRef, useState } from "react";
import { addEdge, useEdgesState, useNodesState } from "reactflow";

import {
  FLOW_MARKER,
  HEAT_MARKER,
} from "./edgeUtils.js";

import "reactflow/dist/style.css";
import "./App.css";
import ModelFlowCanvas from "./ModelFlowCanvas.jsx";
import SolverPanel from "./SolverPanel.jsx";
import guiRequirementsMarkdown from "../requirements/opensd-web-gui.md?raw";
import guiHelpMarkdown from "./gui-help.md?raw";
import {
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
const FLOW_NODE_SIZE = { width: 22, height: 22 };
const BC_NODE_SIZE = { width: 44, height: 44 };
const PIPE_NODE_SIZE = { width: 64, height: 28 };
const PIPE_NODE_VERTICAL_SIZE = { width: 28, height: 64 };
const FLOW_SIDES = ["left", "right", "top", "bottom"];
const FLOW_HORIZONTAL_LOCK_THRESHOLD = 32;
const COPY_PADDING = 28;
const COPY_SCALE = 2;
const PDF_SCALE = 4;
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

function attributesOf(element) {
  return Object.fromEntries(Array.from(element?.attributes ?? []).map((attribute) => [attribute.name, attribute.value]));
}

function buildFlowNode({ id, position, identifier, details = [], heatHandleMap, circuitId = "", xmlAttributes = {}, sourceIdentifier = identifier }) {
  return {
    id,
    type: "flow",
    position,
    data: { ...nodeData(identifier, details), heat: heatFor(id, heatHandleMap), rotation: 0, circuitId, xmlAttributes, sourceIdentifier }
  };
}

function buildManualComponent(type, position) {
  const componentKind = componentKindForType(type);
  const componentId = getId();
  const suffix = componentId.replace(/^manual_/, "");
  const identifier = `${componentKind === "flow" ? "node" : componentKind}_${suffix}`;
  const common = { id: componentId, position, identifier, xmlAttributes: mandatoryAttributes(type, identifier) };

  if (componentKind === "flow") {
    return buildFlowNode(common);
  }

  if (componentKind === "bc") {
    return buildBcNode(common);
  }

  return buildPipeNode({
    ...common,
    kind: componentKind
  });
}

function mandatoryAttributes(type, identifier) {
  if (type === "Node") {
    return { identifier, elevation: "0.0", tpres_old: "0.0", ttemp_old: "0.0", tenth_old: "0.0", msource: "0.0", volume: "0.0", heat_input: "0.0", fixed_var: "" };
  }
  if (type === "BC") return { identifier, node: "", var: "tpres", val: "0.0" };
  if (type === "HSlab") {
    return { identifier, ucomp: "", uvar: "node", uval: "0.0", utype: "fixed", dcomp: "", dvar: "node", dval: "0.0", dtype: "fixed", uarea: "1.0", ninc: "1" };
  }
  if (type === "Pump") {
    return { identifier, curve_speed: "100.0", curve_file: "pump_curve.csv", Nop: "100.0", unode: "", dnode: "" };
  }
  return { identifier, diameter: "1.0", length: "1.0", ncell: "1", unode: "", dnode: "", roughness: "0.0", cfarea: "1.0", heat_input: "0.0" };
}

function buildPipeNode({ id, position, identifier, details = [], kind = "pipe", xmlTag = kind === "pump" ? "vspump" : "pipe", heatHandleMap, circuitId = "", xmlAttributes = {}, sourceIdentifier = identifier, rotation = 0 }) {
  return {
    id,
    type: "pipe",
    position,
    data: {
      ...nodeData(identifier, details),
      kind,
      xmlTag,
      heat: heatFor(id, heatHandleMap),
      flowConnections: emptyFlowConnections(),
      rotation,
      circuitId,
      xmlAttributes,
      sourceIdentifier
    }
  };
}

function buildBcNode({ id, position, identifier, details = [], circuitId = "", xmlAttributes = {}, sourceIdentifier = identifier }) {
  return {
    id,
    type: "bc",
    position,
    data: { ...nodeData(identifier, details), rotation: 0, circuitId, xmlAttributes, sourceIdentifier }
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
    sourceHandle: "bc-s-out",
    targetHandle: "bc-top-in",
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
  const nodeById = new Map(nodes.map((node) => [node.id, node]));

  const nearestLongSide = (node, otherNode) => {
    if (!node || !otherNode) return "top";
    const center = nodeCenter(node);
    const otherCenter = nodeCenter(otherNode);
    const rotation = node.type === "pipe" ? ((node.data?.rotation ?? 0) % 360 + 360) % 360 : 0;
    const angle = (rotation * Math.PI) / 180;
    const topNormal = rotation === 90
      ? { x: -1, y: 0 }
      : rotation === 270
        ? { x: 1, y: 0 }
        : { x: Math.sin(angle), y: -Math.cos(angle) };
    const towardOther = { x: otherCenter.x - center.x, y: otherCenter.y - center.y };
    return towardOther.x * topNormal.x + towardOther.y * topNormal.y >= 0 ? "top" : "bottom";
  };

  return edges.map((edge) => {
    if (edge.className !== "hslab-edge") return edge;
    const source = nodeById.get(edge.source);
    const target = nodeById.get(edge.target);
    const sourceSide = nearestLongSide(source, target);
    const targetSide = nearestLongSide(target, source);

    return {
      ...edge,
      type: "straight",
      sourceHandle: `ht-${sourceSide}-out`,
      targetHandle: `ht-${targetSide}-in`
    };
  });
}

function circuitEdge(id, source, target) {
  return {
    id,
    source,
    target,
    sourceHandle: "flow-out",
    targetHandle: "flow-in",
    type: "straight",
    className: "circuit-edge"
  };
}

function nodeSize(node) {
  const measured = node.measured ?? {};
  if (node.width || node.height || measured.width || measured.height) {
    return {
      width: node.width ?? measured.width ?? FLOW_NODE_SIZE.width,
      height: node.height ?? measured.height ?? FLOW_NODE_SIZE.height
    };
  }

  if (node.type === "pipe") {
    const rotation = ((node.data?.rotation ?? 0) % 360 + 360) % 360;
    return rotation === 90 || rotation === 270 ? PIPE_NODE_VERTICAL_SIZE : PIPE_NODE_SIZE;
  }

  if (node.type === "bc") return BC_NODE_SIZE;

  return FLOW_NODE_SIZE;
}

function nodeCenter(node) {
  const size = nodeSize(node);
  return {
    x: (node.position?.x ?? 0) + size.width / 2,
    y: (node.position?.y ?? 0) + size.height / 2
  };
}

function segmentIntersectsRect(start, end, rect) {
  const dx = end.x - start.x;
  const dy = end.y - start.y;
  let enter = 0;
  let exit = 1;
  const boundaries = [
    [-dx, start.x - rect.left],
    [dx, rect.right - start.x],
    [-dy, start.y - rect.top],
    [dy, rect.bottom - start.y]
  ];

  for (const [direction, distance] of boundaries) {
    if (direction === 0) {
      if (distance < 0) return false;
      continue;
    }
    const ratio = distance / direction;
    if (direction < 0) enter = Math.max(enter, ratio);
    else exit = Math.min(exit, ratio);
    if (enter > exit) return false;
  }
  return true;
}

function revealConnectionsBehindComponents(nodes, edges) {
  const nodeById = new Map(nodes.map((node) => [node.id, node]));
  return edges.map((edge) => {
    const source = nodeById.get(edge.source);
    const target = nodeById.get(edge.target);
    if (!source || !target) return edge;
    const start = nodeCenter(source);
    const end = nodeCenter(target);
    const isOccluded = nodes.some((node) => {
      if (node.id === edge.source || node.id === edge.target) return false;
      const size = nodeSize(node);
      return segmentIntersectsRect(start, end, {
        left: node.position.x - 2,
        right: node.position.x + size.width + 2,
        top: node.position.y - 2,
        bottom: node.position.y + size.height + 2
      });
    });
    if (!isOccluded) return edge;
    return {
      ...edge,
      zIndex: 1,
      style: {
        ...edge.style,
        strokeDasharray: "5 4"
      }
    };
  });
}

function preferredSourceSide(sourceNode, targetNode, lockHorizontal = false) {
  const sourceCenter = nodeCenter(sourceNode);
  const targetCenter = nodeCenter(targetNode);
  const dx = targetCenter.x - sourceCenter.x;
  const dy = targetCenter.y - sourceCenter.y;

  if (lockHorizontal && Math.abs(dx) >= FLOW_HORIZONTAL_LOCK_THRESHOLD) return dx >= 0 ? "right" : "left";
  if (Math.abs(dx) * 1.15 >= Math.abs(dy)) return dx >= 0 ? "right" : "left";
  return dy >= 0 ? "bottom" : "top";
}

function oppositeSide(side) {
  return {
    left: "right",
    right: "left",
    top: "bottom",
    bottom: "top"
  }[side];
}

function rankedFlowSides(preferredSide) {
  const opposite = oppositeSide(preferredSide);
  const perpendicular = FLOW_SIDES.filter((side) => side !== preferredSide && side !== opposite);
  return [preferredSide, opposite, ...perpendicular];
}

function spreadFlowSide(nodeId, preferredSide, sideUseByNode) {
  if (!sideUseByNode.has(nodeId)) sideUseByNode.set(nodeId, new Map());

  const sideUse = sideUseByNode.get(nodeId);
  const ranked = rankedFlowSides(preferredSide);
  const side =
    (sideUse.get(preferredSide) ?? 0) === 0
      ? preferredSide
      : ranked.find((candidate) => (sideUse.get(candidate) ?? 0) === 0) ??
        ranked.reduce((best, candidate) => {
          const bestCount = sideUse.get(best) ?? 0;
          const candidateCount = sideUse.get(candidate) ?? 0;
          return candidateCount < bestCount ? candidate : best;
        }, ranked[0]);

  sideUse.set(side, (sideUse.get(side) ?? 0) + 1);
  return side;
}

function incrementMapCount(map, key) {
  map.set(key, (map.get(key) ?? 0) + 1);
}

function flowHandleForSide(side, direction, slot = null) {
  return slot ? `flow-${side}-${slot}-${direction}` : `flow-${side}-${direction}`;
}

function flowEndpointPreferences(edge, nodeById, nodeTypeById) {
  const source = edge.source;
  const target = edge.target;
  const sourceNode = nodeById.get(source);
  const targetNode = nodeById.get(target);
  if (!sourceNode || !targetNode) return null;

  const sourceIsPipe = nodeTypeById.get(source) === "pipe";
  const targetIsPipe = nodeTypeById.get(target) === "pipe";
  const pipeToFluid = sourceIsPipe !== targetIsPipe;
  const sourcePreferredSide = preferredSourceSide(sourceNode, targetNode, pipeToFluid);
  const targetPreferredSide = oppositeSide(sourcePreferredSide);

  return {
    sourceIsPipe,
    targetIsPipe,
    sourcePreferredSide,
    targetPreferredSide
  };
}

function rerouteFlowEdges(nodes, edges) {
  const nodeById = new Map(nodes.map((node) => [node.id, node]));
  const nodeTypeById = new Map(nodes.map((node) => [node.id, node.type]));
  const preferencesByEdgeId = new Map();
  const endpointStatsByNode = new Map();
  const sideUseByNode = new Map();
  const finalSideUseByNode = new Map();
  const finalSideCountsByNode = new Map();
  const routedFlowEdges = edges.filter((edge) => edge.className === "flow-edge");

  for (const edge of routedFlowEdges) {
    const preferences = flowEndpointPreferences(edge, nodeById, nodeTypeById);
    if (!preferences) continue;

    preferencesByEdgeId.set(edge.id, preferences);

    for (const endpoint of [
      { nodeId: edge.source, isPipe: preferences.sourceIsPipe, side: preferences.sourcePreferredSide },
      { nodeId: edge.target, isPipe: preferences.targetIsPipe, side: preferences.targetPreferredSide }
    ]) {
      if (endpoint.isPipe) continue;
      if (!endpointStatsByNode.has(endpoint.nodeId)) {
        endpointStatsByNode.set(endpoint.nodeId, { total: 0, sides: new Map() });
      }

      const stats = endpointStatsByNode.get(endpoint.nodeId);
      stats.total += 1;
      incrementMapCount(stats.sides, endpoint.side);
    }
  }

  const assignSide = (nodeId, preferredSide) => {
    const stats = endpointStatsByNode.get(nodeId);
    const shouldSpreadTwoDuplicates = stats?.total === 2 && (stats.sides.get(preferredSide) ?? 0) === 2;
    const side = shouldSpreadTwoDuplicates ? spreadFlowSide(nodeId, preferredSide, sideUseByNode) : preferredSide;

    if (!finalSideCountsByNode.has(nodeId)) finalSideCountsByNode.set(nodeId, new Map());
    incrementMapCount(finalSideCountsByNode.get(nodeId), side);

    return side;
  };

  const sideByEndpointKey = new Map();
  for (const edge of routedFlowEdges) {
    const preferences = preferencesByEdgeId.get(edge.id);
    if (!preferences) continue;

    if (!preferences.sourceIsPipe) sideByEndpointKey.set(`${edge.id}:source`, assignSide(edge.source, preferences.sourcePreferredSide));
    if (!preferences.targetIsPipe) sideByEndpointKey.set(`${edge.id}:target`, assignSide(edge.target, preferences.targetPreferredSide));
  }

  const slotForSide = (nodeId, side) => {
    const sideCount = finalSideCountsByNode.get(nodeId)?.get(side) ?? 0;
    if (sideCount <= 1) return null;

    const key = `${nodeId}:${side}`;
    const index = finalSideUseByNode.get(key) ?? 0;
    finalSideUseByNode.set(key, index + 1);
    return index % 2 === 0 ? "a" : "b";
  };

  return edges.map((edge) => {
    if (edge.className !== "flow-edge") return edge;
    const preferences = preferencesByEdgeId.get(edge.id);
    if (!preferences) return edge;
    const sourceSide = sideByEndpointKey.get(`${edge.id}:source`);
    const targetSide = sideByEndpointKey.get(`${edge.id}:target`);

    return {
      ...edge,
      sourceHandle: preferences.sourceIsPipe ? "flow-out" : flowHandleForSide(sourceSide, "out", slotForSide(edge.source, sourceSide)),
      targetHandle: preferences.targetIsPipe ? "flow-in" : flowHandleForSide(targetSide, "in", slotForSide(edge.target, targetSide))
    };
  });
}

function rerouteBcEdges(nodes, edges) {
  const nodeById = new Map(nodes.map((node) => [node.id, node]));

  return edges.map((edge) => {
    if (edge.className !== "bc-edge") return edge;

    const sourceNode = nodeById.get(edge.source);
    const targetNode = nodeById.get(edge.target);
    if (!sourceNode || !targetNode) return edge;

    const bcNode = sourceNode.type === "bc" ? sourceNode : targetNode.type === "bc" ? targetNode : null;
    const otherNode = bcNode === sourceNode ? targetNode : sourceNode;
    if (!bcNode) return edge;
    const bcCenter = nodeCenter(bcNode);
    const otherCenter = nodeCenter(otherNode);
    const angle = Math.atan2(otherCenter.y - bcCenter.y, otherCenter.x - bcCenter.x);
    const portIndex = (Math.round(angle / (Math.PI / 4)) + 8) % 8;
    const port = ["e", "se", "s", "sw", "w", "nw", "n", "ne"][portIndex];
    const targetSide = preferredSourceSide(otherNode, bcNode);

    return {
      ...edge,
      source: bcNode.id,
      target: otherNode.id,
      sourceHandle: `bc-${port}-out`,
      targetHandle: `bc-${targetSide}-in`
    };
  });
}

function attr(element, name, fallback = "") {
  return element?.getAttribute(name) ?? fallback;
}

function layoutKeyForFile(file) {
  return file?.name ? `geometry:${file.name}` : MANUAL_LAYOUT_KEY;
}

function layoutFilename(layoutKey) {
  const baseName = layoutKey.startsWith("geometry:") ? layoutKey.slice("geometry:".length) : "opensd_geometry";
  return `${baseName.replace(/\.[^.]+$/, "").replace(/[^a-z0-9._-]+/gi, "_")}.layout.json`;
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

  const layout = serializeLayout(nodes);
  const layouts = readStoredLayouts();
  layouts[layoutKey] = layout;

  try {
    window.localStorage.setItem(LAYOUT_STORAGE_KEY, JSON.stringify(layouts));
  } catch {
    // Layout persistence is helpful, but storage limits should not block graph editing.
  }
}

function serializeLayout(nodes, source = "opensd-web") {
  const layout = {};
  for (const node of nodes) {
    layout[node.id] = {
      position: node.position,
      rotation: node.data?.rotation ?? 0
    };
  }

  return {
    schema: "opensd-web-layout/v1",
    source,
    savedAt: new Date().toISOString(),
    nodes: layout
  };
}

function normalizeRotation(value, fallback = 0) {
  const numeric = Number(value);
  if (!Number.isFinite(numeric)) return fallback;
  return ((Math.round(numeric / 45) * 45) % 360 + 360) % 360;
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
        rotation: normalizeRotation(saved.rotation, node.data?.rotation ?? 0)
      }
    };
  });
}

function applyStoredLayoutGraph(nodes, edges, storedLayout) {
  const layoutNodes = applyStoredLayout(nodes, storedLayout);
  const routedEdges = rerouteHeatEdges(layoutNodes, edges);
  const heatHandleMap = buildHeatHandleMapFromEdges(routedEdges);

  return {
    nodes: applyHeatHandleMap(layoutNodes, heatHandleMap),
    edges: routedEdges
  };
}

function isFlowNodeId(nodeId) {
  return nodeId?.startsWith("node:");
}

function isFlowInHandle(handleId) {
  return handleId === "flow-in" || /^flow-(left|right|top|bottom)(-[ab])?-in$/.test(handleId ?? "");
}

function isFlowOutHandle(handleId) {
  return handleId === "flow-out" || /^flow-(left|right|top|bottom)(-[ab])?-out$/.test(handleId ?? "");
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

    if (connections.has(edge.target) && isFlowInHandle(edge.targetHandle) && sourceIsFluidNode) {
      connections.get(edge.target).upstream = true;
    }

    if (connections.has(edge.source) && isFlowOutHandle(edge.sourceHandle) && targetIsFluidNode) {
      connections.get(edge.source).downstream = true;
    }
  }

  return connections;
}

function normalizedFlowConnection(params, nodes) {
  if (!params.source || !params.target || params.source === params.target) return null;

  const nodeTypeById = new Map(nodes.map((node) => [node.id, node.type]));
  let source = params.source;
  let target = params.target;
  let sourceHandle = params.sourceHandle;
  let targetHandle = params.targetHandle;
  const sourceIsPipe = nodeTypeById.get(source) === "pipe";
  const targetIsPipe = nodeTypeById.get(target) === "pipe";

  if ((sourceIsPipe && isFlowInHandle(sourceHandle)) || (targetIsPipe && isFlowOutHandle(targetHandle))) {
    [source, target] = [target, source];
    [sourceHandle, targetHandle] = [targetHandle, sourceHandle];
  }

  const canonicalSourceIsPipe = nodeTypeById.get(source) === "pipe";
  const canonicalTargetIsPipe = nodeTypeById.get(target) === "pipe";

  return {
    ...params,
    source,
    target,
    sourceHandle: canonicalSourceIsPipe ? "flow-out" : sourceHandle,
    targetHandle: canonicalTargetIsPipe ? "flow-in" : targetHandle,
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
      data: { ...node.data, xmlAttributes: { ...node.data.xmlAttributes } }
    })),
    edges: edges.map((edge) => ({ ...edge }))
  };
}

function geometryFingerprint(nodes, edges) {
  return JSON.stringify({
    nodes: nodes.map((node) => ({
      id: node.id,
      type: node.type,
      kind: node.data.kind ?? "",
      xmlTag: node.data.xmlTag ?? "",
      identifier: node.data.identifier,
      circuitId: node.data.circuitId ?? "",
      attributes: node.data.xmlAttributes ?? {}
    })),
    edges: edges.map(({ source, target, className }) => ({ source, target, className }))
  });
}

function attributesWithConnectivity(node, nodes, edges) {
  const attributes = { ...(node.data.xmlAttributes ?? {}) };
  const nodeById = new Map(nodes.map((item) => [item.id, item]));
  const identifierFor = (nodeId) => nodeById.get(nodeId)?.data.identifier ?? "";
  const incoming = edges.find((edge) => edge.target === node.id && nodeById.get(edge.source)?.type === "flow");
  const outgoing = edges.find((edge) => edge.source === node.id && nodeById.get(edge.target)?.type === "flow");

  if (node.type === "pipe" && !["circuit", "hslab"].includes(node.data.kind)) {
    attributes.unode = incoming ? identifierFor(incoming.source) : "";
    attributes.dnode = outgoing ? identifierFor(outgoing.target) : "";
  } else if (node.type === "bc") {
    const edge = edges.find((item) => item.source === node.id && nodeById.get(item.target)?.type === "flow")
      ?? edges.find((item) => item.target === node.id && nodeById.get(item.source)?.type === "flow");
    attributes.node = edge ? identifierFor(edge.source === node.id ? edge.target : edge.source) : "";
  } else if (node.data.kind === "hslab") {
    const upstream = edges.find((edge) => edge.target === node.id && edge.className === "hslab-edge");
    const downstream = edges.find((edge) => edge.source === node.id && edge.className === "hslab-edge");
    const setHeatEndpoint = (prefix, edge, endpoint) => {
      const component = edge ? nodeById.get(edge[endpoint]) : null;
      attributes[`${prefix}comp`] = component?.data.identifier ?? "";
      if (component) attributes[`${prefix}var`] = component.type === "pipe" ? "pipe" : "node";
    };
    setHeatEndpoint("u", upstream, "source");
    setHeatEndpoint("d", downstream, "target");
  }

  return attributes;
}

function isEditableTarget(target) {
  if (!(target instanceof HTMLElement)) return false;
  const tagName = target.tagName.toLowerCase();
  return target.isContentEditable || ["input", "textarea", "select"].includes(tagName);
}

function selectedNodes(nodes) {
  return nodes.filter((node) => node.selected);
}

function selectedNodeBounds(nodes) {
  return nodes.map((node) => {
    const size = nodeSize(node);
    return {
      node,
      width: size.width,
      height: size.height,
      left: node.position.x,
      top: node.position.y,
      right: node.position.x + size.width,
      bottom: node.position.y + size.height,
      centerX: node.position.x + size.width / 2,
      centerY: node.position.y + size.height / 2
    };
  });
}

function alignNodes(nodes, mode) {
  const bounds = selectedNodeBounds(selectedNodes(nodes));
  if (bounds.length < 2) return nodes;

  const group = {
    left: Math.min(...bounds.map((bound) => bound.left)),
    right: Math.max(...bounds.map((bound) => bound.right)),
    top: Math.min(...bounds.map((bound) => bound.top)),
    bottom: Math.max(...bounds.map((bound) => bound.bottom))
  };
  const groupCenterX = (group.left + group.right) / 2;
  const groupCenterY = (group.top + group.bottom) / 2;
  const nextPositionById = new Map();

  for (const bound of bounds) {
    const position = { ...bound.node.position };

    if (mode === "left") position.x = group.left;
    if (mode === "right") position.x = group.right - bound.width;
    if (mode === "center") position.x = groupCenterX - bound.width / 2;
    if (mode === "top") position.y = group.top;
    if (mode === "bottom") position.y = group.bottom - bound.height;
    if (mode === "middle") position.y = groupCenterY - bound.height / 2;

    nextPositionById.set(bound.node.id, position);
  }

  return nodes.map((node) => (nextPositionById.has(node.id) ? { ...node, position: nextPositionById.get(node.id) } : node));
}

function distributeNodes(nodes, axis) {
  const bounds = selectedNodeBounds(selectedNodes(nodes));
  if (bounds.length < 3) return nodes;

  const centerKey = axis === "horizontal" ? "centerX" : "centerY";
  const sizeKey = axis === "horizontal" ? "width" : "height";
  const positionKey = axis === "horizontal" ? "x" : "y";
  const sorted = [...bounds].sort((a, b) => a[centerKey] - b[centerKey]);
  const first = sorted[0][centerKey];
  const last = sorted.at(-1)[centerKey];
  const step = (last - first) / (sorted.length - 1);
  const nextPositionById = new Map();

  sorted.forEach((bound, index) => {
    if (index === 0 || index === sorted.length - 1) return;
    nextPositionById.set(bound.node.id, {
      ...bound.node.position,
      [positionKey]: first + step * index - bound[sizeKey] / 2
    });
  });

  return nodes.map((node) => (nextPositionById.has(node.id) ? { ...node, position: nextPositionById.get(node.id) } : node));
}

function moveSelectedNodes(nodes, direction, distance = 12) {
  const delta = {
    left: { x: -distance, y: 0 },
    right: { x: distance, y: 0 },
    up: { x: 0, y: -distance },
    down: { x: 0, y: distance }
  }[direction];
  if (!delta) return nodes;

  return nodes.map((node) => node.selected
    ? { ...node, position: { x: node.position.x + delta.x, y: node.position.y + delta.y } }
    : node);
}

function adjustSelectedSpacing(nodes, axis, direction) {
  const bounds = selectedNodeBounds(selectedNodes(nodes));
  if (bounds.length < 2) return nodes;

  const centerKey = axis === "horizontal" ? "centerX" : "centerY";
  const positionKey = axis === "horizontal" ? "x" : "y";
  const sizeKey = axis === "horizontal" ? "width" : "height";
  const centers = bounds.map((bound) => bound[centerKey]);
  const groupCenter = (Math.min(...centers) + Math.max(...centers)) / 2;
  const currentSpan = Math.max(...centers) - Math.min(...centers);
  const desiredSpan = direction === "farther" ? currentSpan + 24 : Math.max(24, currentSpan - 24);
  const scale = currentSpan > 0 ? desiredSpan / currentSpan : 1;
  const nextPositionById = new Map();

  for (const bound of bounds) {
    const nextCenter = groupCenter + (bound[centerKey] - groupCenter) * scale;
    nextPositionById.set(bound.node.id, {
      ...bound.node.position,
      [positionKey]: nextCenter - bound[sizeKey] / 2
    });
  }

  return nodes.map((node) => nextPositionById.has(node.id)
    ? { ...node, position: nextPositionById.get(node.id) }
    : node);
}

function transposeSelectedLayout(nodes) {
  const bounds = selectedNodeBounds(selectedNodes(nodes));
  if (bounds.length < 2) return nodes;

  const centerX = (Math.min(...bounds.map((bound) => bound.left)) + Math.max(...bounds.map((bound) => bound.right))) / 2;
  const centerY = (Math.min(...bounds.map((bound) => bound.top)) + Math.max(...bounds.map((bound) => bound.bottom))) / 2;
  const nextNodeById = new Map();

  for (const bound of bounds) {
    const rotatedNode = bound.node.type === "pipe"
      ? { ...bound.node, data: { ...bound.node.data, rotation: ((bound.node.data?.rotation ?? 0) + 90) % 360 } }
      : bound.node;
    const rotatedSize = bound.node.type === "pipe"
      ? { width: bound.height, height: bound.width }
      : { width: bound.width, height: bound.height };
    const offsetX = bound.centerX - centerX;
    const offsetY = bound.centerY - centerY;
    const rotatedCenterX = centerX - offsetY;
    const rotatedCenterY = centerY + offsetX;
    nextNodeById.set(bound.node.id, {
      ...rotatedNode,
      position: {
        x: rotatedCenterX - rotatedSize.width / 2,
        y: rotatedCenterY - rotatedSize.height / 2
      }
    });
  }

  return nodes.map((node) => nextNodeById.get(node.id) ?? node);
}

function isAllowedConnection(connection, nodes) {
  if (!connection.source || !connection.target || connection.source === connection.target) return false;
  const source = nodes.find((node) => node.id === connection.source);
  const target = nodes.find((node) => node.id === connection.target);
  const isDirectNodeConnection = source?.type === "flow" && target?.type === "flow";
  const isDirectHslabConnection = source?.data.kind === "hslab" && target?.data.kind === "hslab";
  return !isDirectNodeConnection && !isDirectHslabConnection;
}

function graphClipboardFromSelection(nodes, edges) {
  const pickedNodes = selectedNodes(nodes);
  if (!pickedNodes.length) return null;

  const selectedIds = new Set(pickedNodes.map((node) => node.id));
  return {
    nodes: pickedNodes.map((node) => ({
      ...node,
      position: { ...node.position },
      data: { ...node.data },
      selected: true
    })),
    edges: edges.filter((edge) => selectedIds.has(edge.source) && selectedIds.has(edge.target)).map((edge) => ({ ...edge }))
  };
}

function pasteGraphClipboard(nodes, edges, clipboard, offset = 36) {
  if (!clipboard?.nodes?.length) return { nodes, edges };

  const idByOriginal = new Map();
  const clonedNodes = clipboard.nodes.map((node) => {
    const nextId = `${node.id}:copy:${getId()}`;
    idByOriginal.set(node.id, nextId);

    return {
      ...node,
      id: nextId,
      position: {
        x: (node.position?.x ?? 0) + offset,
        y: (node.position?.y ?? 0) + offset
      },
      data: {
        ...node.data,
        identifier: `${node.data?.identifier ?? node.id} copy`
      },
      selected: true
    };
  });

  const clonedEdges = clipboard.edges
    .filter((edge) => idByOriginal.has(edge.source) && idByOriginal.has(edge.target))
    .map((edge) => {
      const source = idByOriginal.get(edge.source);
      const target = idByOriginal.get(edge.target);

      return {
        ...edge,
        id: `${source}->${target}:${getId()}`,
        source,
        target,
        selected: false
      };
    });

  return {
    nodes: [...nodes.map((node) => ({ ...node, selected: false })), ...clonedNodes],
    edges: [...edges.map((edge) => ({ ...edge, selected: false })), ...clonedEdges]
  };
}

function documentStyles() {
  const rules = [];
  Array.from(document.styleSheets).forEach((sheet) => {
    try {
      Array.from(sheet.cssRules ?? []).forEach((rule) => rules.push(rule.cssText));
    } catch {
      // Cross-origin stylesheets cannot be read; the app's bundled styles remain available.
    }
  });
  return rules.join("\n");
}

function elementId(element) {
  const directId = element.getAttribute("data-id") ?? element.getAttribute("data-nodeid");
  if (directId) return directId;
  const testId = element.getAttribute("data-testid") ?? "";
  if (testId.startsWith("rf__edge-")) return testId.slice("rf__edge-".length);
  const domId = element.getAttribute("id") ?? "";
  return domId.startsWith("reactflow__edge-") ? domId.slice("reactflow__edge-".length) : "";
}

function copiedEdgeIds(edges, selectedIds) {
  return new Set(
    edges
      .filter((edge) => selectedIds.has(edge.source) && selectedIds.has(edge.target))
      .map((edge) => edge.id)
  );
}

function pruneUnselectedGraphParts(sourceRenderer, cloneRenderer, selectedIds, selectedEdgeIds) {
  const sourceNodes = Array.from(sourceRenderer.querySelectorAll(".react-flow__node"));
  const cloneNodes = Array.from(cloneRenderer.querySelectorAll(".react-flow__node"));
  sourceNodes.forEach((node, index) => {
    if (!selectedIds.has(elementId(node))) cloneNodes[index]?.remove();
  });

  const sourceEdges = Array.from(sourceRenderer.querySelectorAll(".react-flow__edge"));
  const cloneEdges = Array.from(cloneRenderer.querySelectorAll(".react-flow__edge"));
  sourceEdges.forEach((edge, index) => {
    const id = elementId(edge);
    if (!id || !selectedEdgeIds.has(id)) cloneEdges[index]?.remove();
  });
}

function cleanGraphCloneForImage(cloneRenderer) {
  cloneRenderer.querySelectorAll([
    ".node-rotate-button",
    ".node-tooltip",
    ".react-flow__selection",
    ".react-flow__nodesselection",
    ".react-flow__nodesselection-rect",
    ".react-flow__selectionpane",
    ".react-flow__connectionline",
    ".react-flow__handle",
    ".react-flow__edge-interaction"
  ].join(", "))
    .forEach((element) => element.remove());

  cloneRenderer.querySelectorAll(".react-flow__node.selected").forEach((node) => {
    node.classList.remove("selected");
    node.style.outline = "none";
    const shell = node.querySelector(".component-shell");
    if (!shell) return;
    shell.style.outline = "none";
    shell.style.outlineOffset = "0";
    shell.style.boxShadow = shell.classList.contains("pipe-node--pump")
      ? "none"
      : shell.classList.contains("pipe-node")
        ? "0 4px 12px rgba(16, 24, 40, 0.1)"
        : "0 4px 10px rgba(16, 24, 40, 0.1)";
  });

  cloneRenderer.querySelectorAll(".react-flow__edge").forEach((edge) => {
    edge.classList.remove("selected");
    edge.style.display = "block";
    edge.style.visibility = "visible";
    edge.style.opacity = "1";
    const path = edge.querySelector(".react-flow__edge-path");
    if (!path) return;
    if (edge.classList.contains("hslab-edge")) {
      path.style.stroke = "#e67e22";
      path.style.strokeWidth = "2px";
    } else if (edge.classList.contains("bc-edge")) {
      path.style.stroke = "#e57373";
      path.style.strokeWidth = "2px";
    } else if (edge.classList.contains("circuit-edge")) {
      path.style.stroke = "#667085";
      path.style.strokeWidth = "1.5px";
    } else {
      path.style.stroke = "#3468b5";
      path.style.strokeWidth = "2px";
    }
  });

  cloneRenderer.querySelectorAll(".react-flow__edges").forEach((edgeLayer) => {
    edgeLayer.style.overflow = "visible";
    edgeLayer.style.pointerEvents = "none";
  });
}

function blobFromCanvas(canvas, type = "image/png", quality) {
  return new Promise((resolve, reject) => {
    canvas.toBlob((blob) => {
      if (blob) {
        resolve(blob);
      } else {
        reject(new Error("Unable to create image."));
      }
    }, type, quality);
  });
}

function selectedGraphSvg(edges) {
  const sourceRenderer = document.querySelector(".canvas-wrap .react-flow__renderer");
  if (!sourceRenderer) throw new Error("Canvas is not ready.");

  const selectedElements = Array.from(sourceRenderer.querySelectorAll(".react-flow__node.selected"));
  if (!selectedElements.length) throw new Error("Select one or more components before copying.");

  const selectedIds = new Set(selectedElements.map(elementId).filter(Boolean));
  const selectedEdgeIds = copiedEdgeIds(edges, selectedIds);
  const sourceRect = sourceRenderer.getBoundingClientRect();
  const selectedRects = [
    ...selectedElements,
    ...Array.from(sourceRenderer.querySelectorAll(".react-flow__edge")).filter((edge) => selectedEdgeIds.has(elementId(edge)))
  ].map((element) => element.getBoundingClientRect());
  const crop = {
    left: Math.min(...selectedRects.map((rect) => rect.left)),
    right: Math.max(...selectedRects.map((rect) => rect.right)),
    top: Math.min(...selectedRects.map((rect) => rect.top)),
    bottom: Math.max(...selectedRects.map((rect) => rect.bottom))
  };
  const width = Math.ceil(crop.right - crop.left + COPY_PADDING * 2);
  const height = Math.ceil(crop.bottom - crop.top + COPY_PADDING * 2);
  const cloneRenderer = sourceRenderer.cloneNode(true);

  pruneUnselectedGraphParts(sourceRenderer, cloneRenderer, selectedIds, selectedEdgeIds);
  cleanGraphCloneForImage(cloneRenderer);
  cloneRenderer.style.position = "absolute";
  cloneRenderer.style.inset = "0";
  cloneRenderer.style.width = `${sourceRect.width}px`;
  cloneRenderer.style.height = `${sourceRect.height}px`;
  cloneRenderer.querySelectorAll(".react-flow__edges").forEach((edgeLayer) => {
    edgeLayer.setAttribute("width", String(sourceRect.width));
    edgeLayer.setAttribute("height", String(sourceRect.height));
  });

  const wrapper = document.createElement("div");
  wrapper.setAttribute("xmlns", "http://www.w3.org/1999/xhtml");
  wrapper.style.position = "relative";
  wrapper.style.width = `${width}px`;
  wrapper.style.height = `${height}px`;
  wrapper.style.overflow = "hidden";
  wrapper.style.background = "transparent";

  const styleElement = document.createElement("style");
  styleElement.textContent = documentStyles();
  wrapper.appendChild(styleElement);

  const shifted = document.createElement("div");
  shifted.style.position = "absolute";
  shifted.style.left = `${COPY_PADDING - (crop.left - sourceRect.left)}px`;
  shifted.style.top = `${COPY_PADDING - (crop.top - sourceRect.top)}px`;
  shifted.style.width = `${sourceRect.width}px`;
  shifted.style.height = `${sourceRect.height}px`;
  shifted.appendChild(cloneRenderer);
  wrapper.appendChild(shifted);

  const serialized = new XMLSerializer().serializeToString(wrapper);
  const svg = [
    `<svg xmlns="http://www.w3.org/2000/svg" width="${width}" height="${height}" viewBox="0 0 ${width} ${height}">`,
    `<foreignObject width="100%" height="100%">${serialized}</foreignObject>`,
    "</svg>"
  ].join("");
  return { svg, count: selectedIds.size, width, height };
}

async function selectedGraphCanvas(edges, scale = COPY_SCALE) {
  const { svg, count, width, height } = selectedGraphSvg(edges);
  const url = URL.createObjectURL(new Blob([svg], { type: "image/svg+xml;charset=utf-8" }));

  try {
    const image = new Image();
    image.decoding = "async";
    image.src = url;
    await image.decode();

    const canvas = document.createElement("canvas");
    canvas.width = width * scale;
    canvas.height = height * scale;
    const context = canvas.getContext("2d");
    context.scale(scale, scale);
    context.drawImage(image, 0, 0);

    return { canvas, count, width, height };
  } finally {
    URL.revokeObjectURL(url);
  }
}

async function selectedGraphImageBlob(edges) {
  const { canvas, count } = await selectedGraphCanvas(edges);
  return { blob: await blobFromCanvas(canvas), count };
}

async function copySelectedGraphImage(edges) {
  if (!navigator.clipboard || typeof ClipboardItem === "undefined") {
    throw new Error("Image clipboard is not available in this browser.");
  }

  const { blob, count } = await selectedGraphImageBlob(edges);
  await navigator.clipboard.write([new ClipboardItem({ "image/png": blob })]);
  return count;
}

async function exportSelectedGraphPng(edges) {
  const { blob, count } = await selectedGraphImageBlob(edges);
  const url = URL.createObjectURL(blob);
  const anchor = document.createElement("a");
  anchor.href = url;
  anchor.download = `opensd-selection-${new Date().toISOString().replace(/[:.]/g, "-")}.png`;
  anchor.click();
  URL.revokeObjectURL(url);
  return count;
}

function exportSelectedGraphSvg(edges) {
  const { svg, count } = selectedGraphSvg(edges);
  const blob = new Blob([svg], { type: "image/svg+xml;charset=utf-8" });
  const url = URL.createObjectURL(blob);
  const anchor = document.createElement("a");
  anchor.href = url;
  anchor.download = `opensd-selection-${new Date().toISOString().replace(/[:.]/g, "-")}.svg`;
  anchor.click();
  URL.revokeObjectURL(url);
  return count;
}

function pdfBlobFromJpeg(jpegBytes, pixelWidth, pixelHeight, logicalWidth, logicalHeight) {
  const margin = 18;
  const pointScale = Math.min(0.75, 960 / Math.max(logicalWidth, logicalHeight));
  const imageWidth = logicalWidth * pointScale;
  const imageHeight = logicalHeight * pointScale;
  const pageWidth = imageWidth + margin * 2;
  const pageHeight = imageHeight + margin * 2;
  const content = `q ${imageWidth.toFixed(2)} 0 0 ${imageHeight.toFixed(2)} ${margin} ${margin} cm /Im0 Do Q`;
  const chunks = [];
  const offsets = [0];
  let byteLength = 0;
  const append = (chunk) => {
    chunks.push(chunk);
    byteLength += typeof chunk === "string" ? chunk.length : chunk.byteLength;
  };
  const object = (number, body) => {
    offsets[number] = byteLength;
    append(`${number} 0 obj\n`);
    body();
    append("\nendobj\n");
  };

  append("%PDF-1.4\n%OpenSD\n");
  object(1, () => append("<< /Type /Catalog /Pages 2 0 R >>"));
  object(2, () => append("<< /Type /Pages /Kids [3 0 R] /Count 1 >>"));
  object(3, () => append(`<< /Type /Page /Parent 2 0 R /MediaBox [0 0 ${pageWidth.toFixed(2)} ${pageHeight.toFixed(2)}] /Resources << /XObject << /Im0 4 0 R >> >> /Contents 5 0 R >>`));
  object(4, () => {
    append(`<< /Type /XObject /Subtype /Image /Width ${pixelWidth} /Height ${pixelHeight} /ColorSpace /DeviceRGB /BitsPerComponent 8 /Filter /DCTDecode /Length ${jpegBytes.byteLength} >>\nstream\n`);
    append(jpegBytes);
    append("\nendstream");
  });
  object(5, () => append(`<< /Length ${content.length} >>\nstream\n${content}\nendstream`));
  const xrefOffset = byteLength;
  append("xref\n0 6\n0000000000 65535 f \n");
  for (let index = 1; index <= 5; index += 1) append(`${String(offsets[index]).padStart(10, "0")} 00000 n \n`);
  append(`trailer\n<< /Size 6 /Root 1 0 R >>\nstartxref\n${xrefOffset}\n%%EOF`);
  return new Blob(chunks, { type: "application/pdf" });
}

async function exportSelectedGraphPdf(edges) {
  const { canvas, count, width, height } = await selectedGraphCanvas(edges, PDF_SCALE);
  const pdfCanvas = document.createElement("canvas");
  pdfCanvas.width = canvas.width;
  pdfCanvas.height = canvas.height;
  const context = pdfCanvas.getContext("2d");
  context.fillStyle = "#ffffff";
  context.fillRect(0, 0, pdfCanvas.width, pdfCanvas.height);
  context.drawImage(canvas, 0, 0);
  const jpegBlob = await blobFromCanvas(pdfCanvas, "image/jpeg", 0.99);
  const pdfBlob = pdfBlobFromJpeg(new Uint8Array(await jpegBlob.arrayBuffer()), pdfCanvas.width, pdfCanvas.height, width, height);
  const url = URL.createObjectURL(pdfBlob);
  const anchor = document.createElement("a");
  anchor.href = url;
  anchor.download = `opensd-selection-${new Date().toISOString().replace(/[:.]/g, "-")}.pdf`;
  anchor.click();
  URL.revokeObjectURL(url);
  return count;
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
    pumps: 0,
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
  const sharedElevationValues = Array.from(new Set(
    circuits.flatMap((circuit) => Array.from(circuit.querySelectorAll(":scope > node"), (node) => Number(attr(node, "elevation", "0"))))
      .filter(Number.isFinite)
  )).sort((a, b) => b - a);
  const maxBcStackByElevation = new Map();
  for (const circuit of circuits) {
    const bcCountByNode = Array.from(circuit.querySelectorAll(":scope > bc")).reduce((counts, bc) => {
      const target = attr(bc, "node");
      counts.set(target, (counts.get(target) ?? 0) + 1);
      return counts;
    }, new Map());
    for (const node of circuit.querySelectorAll(":scope > node")) {
      const elevation = Number(attr(node, "elevation", "0"));
      const stack = bcCountByNode.get(attr(node, "identifier")) ?? 0;
      maxBcStackByElevation.set(elevation, Math.max(maxBcStackByElevation.get(elevation) ?? 0, stack));
    }
  }
  const sharedElevationPositions = new Map();
  let elevationY = 80;
  sharedElevationValues.forEach((elevation, index) => {
    sharedElevationPositions.set(elevation, elevationY);
    const lowerElevation = sharedElevationValues[index + 1];
    const lowerBcStack = maxBcStackByElevation.get(lowerElevation) ?? 0;
    elevationY += Math.max(112, 102 + Math.max(0, lowerBcStack - 1) * 58);
  });
  let defaultLayoutBottom = 0;
  let defaultLayoutRight = 0;
  let nextCircuitX = 24;

  circuits.forEach((circuit, circuitIndex) => {
    const circuitId = attr(circuit, "identifier", `circuit_${circuitIndex + 1}`);
    const circuitNodeId = `circuit:${circuitId}`;

    stats.circuits += 1;

    const nodeElements = Array.from(circuit.querySelectorAll(":scope > node"));
    const pipeElements = Array.from(circuit.querySelectorAll(":scope > pipe, :scope > vspump, :scope > hpump"));
    const bcElements = Array.from(circuit.querySelectorAll(":scope > bc"));

    const nodeByName = new Map();
    const nodeElevations = new Map();
    const nodeNames = [];

    nodeElements.forEach((nodeEl, nodeIndex) => {
      const nodeName = attr(nodeEl, "identifier", `node_${nodeIndex + 1}`);
      nodeNames.push(nodeName);
      nodeByName.set(nodeName, nodeEl);
      nodeElevations.set(nodeName, Number(attr(nodeEl, "elevation", "0")));
      stats.nodes += 1;
    });

    const pipes = pipeElements.map((pipeEl, pipeIndex) => {
      const xmlTag = pipeEl.tagName.toLowerCase();
      const isPump = xmlTag === "vspump" || xmlTag === "hpump";
      const pipeName = attr(pipeEl, "identifier", `${isPump ? "pump" : "pipe"}_${pipeIndex + 1}`);
      if (isPump) stats.pumps += 1;
      else stats.pipes += 1;
      return {
        name: pipeName,
        unode: attr(pipeEl, "unode"),
        dnode: attr(pipeEl, "dnode"),
        kind: isPump ? "pump" : "pipe",
        xmlTag,
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
    const { positions, bounds } = layoutCircuitRow({
      circuitId,
      sequence,
      bcRecords,
      nodeElevations,
      elevationPositions: sharedElevationPositions,
      circuitX: nextCircuitX
    });
    defaultLayoutBottom = Math.max(defaultLayoutBottom, bounds.bottom);
    defaultLayoutRight = Math.max(defaultLayoutRight, bounds.right);
    nextCircuitX = bounds.right + 80;

    pushNode(
      buildPipeNode({
        id: circuitNodeId,
        position: positions.get(circuitNodeId),
        identifier: circuitId,
        kind: "circuit",
        circuitId,
        xmlAttributes: attributesOf(circuit),
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
            circuitId,
            xmlAttributes: attributesOf(nodeEl),
            details: [
              attr(nodeEl, "fixed_var") && `fixed ${attr(nodeEl, "fixed_var")}`,
              attr(nodeEl, "volume") && `vol ${Number(attr(nodeEl, "volume")).toExponential(2)}`
            ]
          })
        );
        continue;
      }

      const pipe = pipeByName.get(item.name);
      const upstreamPosition = positions.get(`node:${pipe.unode}`);
      const downstreamPosition = positions.get(`node:${pipe.dnode}`);
      const defaultRotation = upstreamPosition && downstreamPosition
        && Math.abs(downstreamPosition.y - upstreamPosition.y) > Math.abs(downstreamPosition.x - upstreamPosition.x)
        ? 90
        : 0;
      pushNode(
        buildPipeNode({
          id: `pipe:${item.name}`,
          position: positions.get(`pipe:${item.name}`),
          identifier: item.name,
          kind: pipe.kind,
          xmlTag: pipe.xmlTag,
          rotation: defaultRotation,
          circuitId,
          xmlAttributes: attributesOf(pipe.element),
          details: [
            pipe.kind === "pump" && attr(pipe.element, "Nop") && `speed ${attr(pipe.element, "Nop")}`,
            pipe.kind === "pipe" && attr(pipe.element, "ncell") && `${attr(pipe.element, "ncell")} cells`,
            pipe.kind === "pipe" && attr(pipe.element, "diameter") && `D ${attr(pipe.element, "diameter")} m`,
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
            circuitId,
            xmlAttributes: attributesOf(nodeEl),
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
          circuitId,
          xmlAttributes: attributesOf(bcEl),
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
      pushEdge(circuitEdge(`${circuitNodeId}->${firstId}`, circuitNodeId, firstId));
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
  const hslabStepX = 96;
  const hslabStepY = 56;
  const usableLayoutWidth = Math.max(defaultLayoutRight - 24, hslabStepX);
  const hslabColumns = Math.max(
    1,
    Math.min(hslabs.length, circuits.length > 0 ? Math.floor(usableLayoutWidth / hslabStepX) : Math.ceil(Math.sqrt(hslabs.length)))
  );
  const hslabBaseY = (circuits.length > 0 ? defaultLayoutBottom : 18) + 44;
  const occupiedLayoutBoxes = flowNodes.map((node) => {
    const size = nodeSize(node);
    return { x: node.position.x, y: node.position.y, width: size.width, height: size.height };
  });
  const hslabsBySinglePipe = new Map();

  const overlapsLayout = (position, width = 64, height = 28) => occupiedLayoutBoxes.some((box) => {
    const clearance = 12;
    return position.x < box.x + box.width + clearance
      && position.x + width + clearance > box.x
      && position.y < box.y + box.height + clearance
      && position.y + height + clearance > box.y;
  });

  const positionNearPipe = (pipeId) => {
    const anchor = flowNodes.find((node) => node.id === pipeId);
    if (!anchor) return null;
    const stackIndex = hslabsBySinglePipe.get(pipeId) ?? 0;
    hslabsBySinglePipe.set(pipeId, stackIndex + 1);
    const anchorSize = nodeSize(anchor);
    const rotation = [90, 270].includes(((anchor.data?.rotation ?? 0) % 360 + 360) % 360) ? 90 : 0;
    const hslabSize = rotation === 90 ? { width: 28, height: 64 } : { width: 64, height: 28 };
    const preferred = rotation === 90
      ? {
          x: anchor.position.x + anchorSize.width + 40 + stackIndex * hslabStepY,
          y: anchor.position.y + (anchorSize.height - hslabSize.height) / 2
        }
      : {
          x: anchor.position.x + (anchorSize.width - hslabSize.width) / 2,
          y: anchor.position.y + anchorSize.height + 40 + stackIndex * hslabStepY
        };

    for (let attempt = 0; attempt < 40; attempt += 1) {
      const candidate = rotation === 90
        ? { x: preferred.x + attempt * hslabStepY, y: preferred.y }
        : { x: preferred.x, y: preferred.y + attempt * hslabStepY };
      if (!overlapsLayout(candidate, hslabSize.width, hslabSize.height)) return { position: candidate, rotation };
    }
    return { position: preferred, rotation };
  };

  const positionBetweenPipes = (pipeIds) => {
    const anchors = pipeIds.map((pipeId) => flowNodes.find((node) => node.id === pipeId)).filter(Boolean);
    if (anchors.length !== 2) return null;
    const centers = anchors.map((anchor) => {
      const size = nodeSize(anchor);
      return { x: anchor.position.x + size.width / 2, y: anchor.position.y + size.height / 2 };
    });
    const bothVertical = anchors.every((anchor) => [90, 270].includes(((anchor.data?.rotation ?? 0) % 360 + 360) % 360));
    const rotation = bothVertical ? 90 : 0;
    const size = rotation === 90 ? { width: 28, height: 64 } : { width: 64, height: 28 };
    const preferred = {
      x: (centers[0].x + centers[1].x) / 2 - size.width / 2,
      y: (centers[0].y + centers[1].y) / 2 - size.height / 2
    };
    for (let attempt = 0; attempt < 40; attempt += 1) {
      const direction = attempt === 0 ? 0 : attempt % 2 === 1 ? 1 : -1;
      const distance = Math.ceil(attempt / 2) * hslabStepY;
      const candidate = rotation === 90
        ? { x: preferred.x + direction * distance, y: preferred.y }
        : { x: preferred.x, y: preferred.y + direction * distance };
      if (!overlapsLayout(candidate, size.width, size.height)) return { rotation, position: candidate };
    }
    return { rotation, position: preferred };
  };

  const positionInPackedArea = (preferred) => {
    for (let attempt = 0; attempt < 40; attempt += 1) {
      const candidate = { x: preferred.x, y: preferred.y + attempt * hslabStepY };
      if (!overlapsLayout(candidate)) return candidate;
    }
    return preferred;
  };

  hslabs.forEach((hslab, hslabIndex) => {
    const hslabName = attr(hslab, "identifier", `hslab_${hslabIndex + 1}`);
    const hslabId = `hslab:${hslabName}`;
    const layers = Array.from(hslab.querySelectorAll(":scope > layer"));
    const hslabColumn = hslabIndex % hslabColumns;
    const hslabRow = Math.floor(hslabIndex / hslabColumns);
    const ucomp = attr(hslab, "ucomp");
    const dcomp = attr(hslab, "dcomp");
    const pipeConnections = new Set([
      attr(hslab, "uvar") === "pipe" && nodeIds.has(`pipe:${ucomp}`) ? `pipe:${ucomp}` : "",
      attr(hslab, "dvar") === "pipe" && nodeIds.has(`pipe:${dcomp}`) ? `pipe:${dcomp}` : ""
    ].filter(Boolean));
    const fallbackPosition = positionInPackedArea({ x: 24 + hslabColumn * hslabStepX, y: hslabBaseY + hslabRow * hslabStepY });
    const connectedPipeIds = Array.from(pipeConnections);
    const connectedPlacement = pipeConnections.size === 1
      ? positionNearPipe(connectedPipeIds[0])
      : pipeConnections.size === 2
        ? positionBetweenPipes(connectedPipeIds)
        : null;
    const hslabPosition = connectedPlacement?.position ?? fallbackPosition;
    const hslabRotation = connectedPlacement?.rotation ?? 0;

    stats.hslabs += 1;

    pushNode(
      buildPipeNode({
        id: hslabId,
        position: hslabPosition,
        identifier: hslabName,
        kind: "hslab",
        rotation: hslabRotation,
        xmlAttributes: attributesOf(hslab),
        details: [
          attr(hslab, "uvar") && `u ${attr(hslab, "uvar")}`,
          attr(hslab, "dvar") && `d ${attr(hslab, "dvar")}`,
          ...layerTooltipLines(layers)
        ]
      })
    );
    occupiedLayoutBoxes.push({
      x: hslabPosition.x,
      y: hslabPosition.y,
      width: hslabRotation === 90 ? 28 : 64,
      height: hslabRotation === 90 ? 64 : 28
    });

    const upstreamId = attr(hslab, "uvar") === "pipe" ? `pipe:${ucomp}` : `node:${ucomp}`;
    const downstreamId = attr(hslab, "dvar") === "pipe" ? `pipe:${dcomp}` : `node:${dcomp}`;

    const positionById = new Map(flowNodes.map((node) => [node.id, node.position]));

    pushEdge(hslabEdge(`${upstreamId}->${hslabId}`, upstreamId, hslabId, positionById));
    pushEdge(hslabEdge(`${hslabId}->${downstreamId}`, hslabId, downstreamId, positionById));
  });

  return {
    nodes: applyHeatHandleMap(flowNodes, buildHeatHandleMapFromEdges(flowEdges)),
    edges: flowEdges,
    stats,
    templateXml: xmlText
  };
}

function HelpDocument({ text }) {
  return (
    <section className="workspace-pane help-view">
      <article className="help-document">
        {text.split(/\r?\n/).filter((line) => line.trim()).map((line, index) => {
          const key = `${index}-${line.slice(0, 20)}`;
          if (line.startsWith("# ")) return <h1 key={key}>{renderMarkdownInline(line.slice(2))}</h1>;
          if (line.startsWith("## ")) return <h2 key={key}>{renderMarkdownInline(line.slice(3))}</h2>;
          if (line.startsWith("- ")) return <ul key={key}><li>{renderMarkdownInline(line.slice(2))}</li></ul>;
          return <p key={key}>{renderMarkdownInline(line)}</p>;
        })}
      </article>
    </section>
  );
}

function xmlIdentifier(value, fallback) {
  const cleaned = String(value ?? "").trim().replace(/[^A-Za-z0-9_.-]+/g, "_");
  return cleaned || fallback;
}

function uniqueIdentifier(preferred, fallback, used) {
  const base = xmlIdentifier(preferred, fallback);
  let value = base;
  let suffix = 2;
  while (used.has(value)) value = `${base}_${suffix++}`;
  used.add(value);
  return value;
}

function directChildren(element, tagName) {
  return Array.from(element.children).filter((child) => child.tagName === tagName);
}

function formatGeometryXml(geometry) {
  const indent = (element, depth = 0) => {
    const children = Array.from(element.children);
    if (!children.length) return;

    for (const node of Array.from(element.childNodes)) {
      if (node.nodeType === Node.TEXT_NODE && !node.nodeValue.trim()) node.remove();
    }
    children.forEach((child) => {
      element.insertBefore(element.ownerDocument.createTextNode(`\n${"  ".repeat(depth + 1)}`), child);
      indent(child, depth + 1);
    });
    element.append(element.ownerDocument.createTextNode(`\n${"  ".repeat(depth)}`));
  };

  indent(geometry);
  return `<?xml version='1.0' encoding='utf-8'?>\n${new XMLSerializer().serializeToString(geometry)}\n`;
}

function serializeOpenSdGeometry(templateXml, nodes, edges) {
  const document = new DOMParser().parseFromString(templateXml || "<geometry/>", "application/xml");
  const geometry = document.querySelector("geometry");
  if (!geometry || document.querySelector("parsererror")) throw new Error("Cannot create geometry XML.");

  const graphById = new Map(nodes.map((node) => [node.id, node]));
  const circuits = nodes.filter((node) => node.data.kind === "circuit");
  const originalCircuitElements = new Map(directChildren(geometry, "circuit").map((element) => [element.getAttribute("identifier"), element]));
  const circuitElements = new Map();
  const circuitIds = new Set();

  for (const circuit of circuits) {
    const identifier = uniqueIdentifier(circuit.data.identifier, "circuit", circuitIds);
    const element = originalCircuitElements.get(circuit.data.sourceIdentifier) ?? document.createElement("circuit");
    for (const attribute of Array.from(element.attributes)) element.removeAttribute(attribute.name);
    for (const [name, value] of Object.entries(circuit.data.xmlAttributes ?? {})) element.setAttribute(name, value);
    element.setAttribute("identifier", identifier);
    if (!element.parentElement) geometry.append(element);
    circuitElements.set(identifier, element);
  }

  if (!circuitElements.size) {
    const element = document.createElement("circuit");
    element.setAttribute("identifier", "circuit_1");
    geometry.prepend(element);
    circuitElements.set("circuit_1", element);
  }

  if (circuits.length) {
    for (const element of originalCircuitElements.values()) if (![...circuitElements.values()].includes(element)) element.remove();
  }

  const firstCircuitId = circuitElements.keys().next().value;
  const renamedCircuitIds = new Map(circuits.map((node) => [node.data.sourceIdentifier, xmlIdentifier(node.data.identifier, "circuit")]));
  const componentCircuit = (node) => {
    const circuitId = renamedCircuitIds.get(node.data.circuitId) ?? node.data.circuitId;
    return circuitElements.has(circuitId) ? circuitId : firstCircuitId;
  };
  const usedByKind = { node: new Set(), pipe: new Set(), pump: new Set(), bc: new Set(), hslab: new Set() };
  const identifierById = new Map();

  for (const node of nodes) {
    let kind = node.type;
    if (node.data.kind === "circuit") continue;
    if (node.data.kind === "hslab") kind = "hslab";
    else if (node.data.kind === "pump") kind = "pump";
    else if (node.type === "flow") kind = "node";
    else if (node.type === "pipe") kind = "pipe";
    const preferred = node.data.identifier;
    identifierById.set(node.id, uniqueIdentifier(preferred, `${kind}_${usedByKind[kind].size + 1}`, usedByKind[kind]));
  }

  const existing = new Map();
  for (const circuit of circuitElements.values()) {
    for (const tag of ["node", "pipe", "vspump", "hpump", "bc"]) {
      for (const element of directChildren(circuit, tag)) existing.set(`${tag}:${element.getAttribute("identifier")}`, element);
    }
  }
  for (const element of directChildren(geometry, "hslab")) existing.set(`hslab:${element.getAttribute("identifier")}`, element);

  const liveElements = new Set();
  const warnings = [];
  const ensureElement = (tag, node) => {
    const identifier = identifierById.get(node.id);
    let element = existing.get(`${tag}:${node.data.sourceIdentifier}`) ?? existing.get(`${tag}:${identifier}`);
    if (!element) {
      element = document.createElement(tag);
      if (tag === "hslab") geometry.append(element);
      else circuitElements.get(componentCircuit(node)).append(element);
    }
    for (const attribute of Array.from(element.attributes)) element.removeAttribute(attribute.name);
    for (const [name, value] of Object.entries(node.data.xmlAttributes ?? {})) element.setAttribute(name, value);
    element.setAttribute("identifier", identifier);
    liveElements.add(element);
    return element;
  };

  const connectionTo = (componentId, direction, predicate) => {
    const edge = edges.find((item) => direction === "in"
      ? item.target === componentId && predicate(graphById.get(item.source))
      : item.source === componentId && predicate(graphById.get(item.target)));
    return edge ? graphById.get(direction === "in" ? edge.source : edge.target) : null;
  };
  const isFluidNode = (node) => node?.type === "flow";
  const isThermalComponent = (node) => node?.type === "flow" || (node?.type === "pipe" && node.data.kind !== "circuit" && node.data.kind !== "hslab");

  for (const node of nodes) {
    if (node.data.kind === "circuit") continue;
    if (node.type === "flow") {
      const element = ensureElement("node", node);
      if (!element.hasAttribute("elevation")) element.setAttribute("elevation", "0.0");
      if (!element.hasAttribute("volume")) element.setAttribute("volume", "0.0");
    } else if (node.type === "bc") {
      const element = ensureElement("bc", node);
      const target = connectionTo(node.id, "out", isFluidNode) || connectionTo(node.id, "in", isFluidNode);
      if (target) element.setAttribute("node", identifierById.get(target.id));
      else { element.removeAttribute("node"); warnings.push(`${identifierById.get(node.id)} has no node connection`); }
      if (!element.hasAttribute("var")) element.setAttribute("var", "tpres");
      if (!element.hasAttribute("val")) element.setAttribute("val", "0.0");
    } else if (node.data.kind === "hslab") {
      const element = ensureElement("hslab", node);
      const upstream = connectionTo(node.id, "in", isThermalComponent);
      const downstream = connectionTo(node.id, "out", isThermalComponent);
      for (const [prefix, component] of [["u", upstream], ["d", downstream]]) {
        if (component) {
          element.setAttribute(`${prefix}comp`, identifierById.get(component.id));
          element.setAttribute(`${prefix}var`, component.type === "pipe" ? "pipe" : "node");
        } else {
          element.removeAttribute(`${prefix}comp`);
          element.removeAttribute(`${prefix}var`);
          warnings.push(`${identifierById.get(node.id)} has no ${prefix === "u" ? "upstream" : "downstream"} heat connection`);
        }
      }
    } else if (node.type === "pipe") {
      const isPump = node.data.kind === "pump";
      const pumpTag = node.data.xmlTag === "hpump" ? "hpump" : "vspump";
      const element = ensureElement(isPump ? pumpTag : "pipe", node);
      const upstream = connectionTo(node.id, "in", isFluidNode);
      const downstream = connectionTo(node.id, "out", isFluidNode);
      if (upstream) element.setAttribute("unode", identifierById.get(upstream.id));
      else { element.removeAttribute("unode"); warnings.push(`${identifierById.get(node.id)} has no upstream node`); }
      if (downstream) element.setAttribute("dnode", identifierById.get(downstream.id));
      else { element.removeAttribute("dnode"); warnings.push(`${identifierById.get(node.id)} has no downstream node`); }
      if (isPump) {
        if (!element.hasAttribute("Nop")) element.setAttribute("Nop", "100.0");
        if (pumpTag === "vspump") {
          if (!element.hasAttribute("curve_speed")) element.setAttribute("curve_speed", "100.0");
          if (!element.hasAttribute("curve_file")) element.setAttribute("curve_file", "pump_curve.csv");
        }
      } else {
        if (!element.hasAttribute("diameter")) element.setAttribute("diameter", "1.0");
        if (!element.hasAttribute("length")) element.setAttribute("length", "1.0");
        if (!element.hasAttribute("ncell")) element.setAttribute("ncell", "1");
      }
    }
  }

  for (const element of existing.values()) if (!liveElements.has(element)) element.remove();
  return { xml: formatGeometryXml(geometry), warnings };
}

function AttributeEditor({ node, onCancel, onSave }) {
  const initialAttributes = { ...(node.data.xmlAttributes ?? {}), identifier: node.data.identifier ?? node.data.xmlAttributes?.identifier ?? "component" };
  const [rows, setRows] = useState(() => Object.entries(initialAttributes).map(([name, value]) => ({ name, value: String(value ?? "") })));
  const updateRow = (index, field, value) => setRows((current) => current.map((row, rowIndex) => rowIndex === index ? { ...row, [field]: value } : row));
  const attributeName = (row) => String(row?.name ?? "").trim();
  const identifier = String(rows.find((row) => attributeName(row) === "identifier")?.value ?? "").trim();

  useEffect(() => {
    const closeOnEscape = (event) => {
      if (event.key !== "Escape") return;
      event.preventDefault();
      onCancel();
    };
    window.addEventListener("keydown", closeOnEscape);
    return () => window.removeEventListener("keydown", closeOnEscape);
  }, [onCancel]);

  return (
    <div className="attribute-modal-backdrop" role="presentation" onMouseDown={(event) => event.target === event.currentTarget && onCancel()}>
      <section className="attribute-modal" role="dialog" aria-modal="true" aria-labelledby="attribute-modal-title">
        <div className="attribute-modal-header">
          <div>
            <span>OpenSD component</span>
            <h2 id="attribute-modal-title">{node.data.identifier} attributes</h2>
          </div>
          <button type="button" className="attribute-close" aria-label="Close" onClick={onCancel}>×</button>
        </div>
        <div className="attribute-grid">
          <strong>Attribute</strong><strong>Value</strong><span />
          {rows.map((row, index) => (
            <div className="attribute-row" key={`${index}-${row.name}`}>
              <input value={row.name} disabled={row.name === "identifier"} onChange={(event) => updateRow(index, "name", event.target.value)} aria-label={`Attribute ${index + 1} name`} />
              <input value={row.value} onChange={(event) => updateRow(index, "value", event.target.value)} aria-label={`${row.name || `Attribute ${index + 1}`} value`} autoFocus={row.name === "identifier"} />
              <button type="button" onClick={() => setRows((current) => current.filter((_, rowIndex) => rowIndex !== index))} disabled={row.name === "identifier"} aria-label={`Remove ${row.name}`}>−</button>
            </div>
          ))}
        </div>
        <button type="button" className="secondary-button" onClick={() => setRows((current) => [...current, { name: "", value: "" }])}>Add attribute</button>
        <div className="attribute-modal-actions">
          <button type="button" className="secondary-button" onClick={onCancel}>Cancel</button>
          <button
            type="button"
            className="primary-button"
            disabled={!identifier || rows.some((row) => !attributeName(row)) || new Set(rows.map(attributeName)).size !== rows.length}
            onClick={() => onSave(Object.fromEntries(rows.map((row) => [attributeName(row), row.value])))}
          >
            Apply
          </button>
        </div>
      </section>
    </div>
  );
}

function ProjectInformation({ filename, layoutKey, nodes, edges, hasUnsavedLayout, onClose }) {
  useEffect(() => {
    const closeOnEscape = (event) => {
      if (event.key !== "Escape") return;
      event.preventDefault();
      onClose();
    };
    window.addEventListener("keydown", closeOnEscape);
    return () => window.removeEventListener("keydown", closeOnEscape);
  }, [onClose]);

  const components = nodes.filter((node) => node.data.kind !== "circuit");
  const totals = [
    ["Components", components.length],
    ["Circuits", nodes.filter((node) => node.data.kind === "circuit").length],
    ["Fluid nodes", nodes.filter((node) => node.type === "flow").length],
    ["Pipes", nodes.filter((node) => node.type === "pipe" && node.data.kind === "pipe").length],
    ["Pumps", nodes.filter((node) => node.data.kind === "pump").length],
    ["Valves", nodes.filter((node) => node.data.kind === "valve").length],
    ["Heat slabs", nodes.filter((node) => node.data.kind === "hslab").length],
    ["Boundary conditions", nodes.filter((node) => node.type === "bc").length],
    ["Fluid connections", edges.filter((edge) => edge.className === "flow-edge").length],
    ["Heat connections", edges.filter((edge) => edge.className === "hslab-edge").length],
    ["BC connections", edges.filter((edge) => edge.className === "bc-edge").length]
  ];
  const circuitCounts = new Map();
  for (const node of components) {
    if (node.data.kind === "hslab") continue;
    const circuitId = node.data.circuitId || "Unassigned";
    circuitCounts.set(circuitId, (circuitCounts.get(circuitId) ?? 0) + 1);
  }

  return (
    <div className="attribute-modal-backdrop" role="presentation" onMouseDown={(event) => event.target === event.currentTarget && onClose()}>
      <section className="attribute-modal project-information" role="dialog" aria-modal="true" aria-labelledby="project-information-title">
        <div className="attribute-modal-header">
          <div>
            <span>OpenSD project</span>
            <h2 id="project-information-title">Project statistics and information</h2>
          </div>
          <button type="button" className="attribute-close" aria-label="Close" onClick={onClose}>&times;</button>
        </div>
        <dl className="project-information-summary">
          <div><dt>Geometry file</dt><dd>{filename}</dd></div>
          <div><dt>Layout key</dt><dd>{layoutKey}</dd></div>
          <div><dt>Layout status</dt><dd>{hasUnsavedLayout ? "Unsaved changes" : "No unsaved changes"}</dd></div>
        </dl>
        <h3>Totals</h3>
        <div className="project-statistics-grid">
          {totals.map(([label, value]) => <div key={label}><span>{label}</span><strong>{value}</strong></div>)}
        </div>
        <h3>Components by circuit</h3>
        <div className="project-statistics-grid">
          {circuitCounts.size
            ? Array.from(circuitCounts, ([circuit, count]) => <div key={circuit}><span>{circuit}</span><strong>{count}</strong></div>)
            : <div><span>No circuit components</span><strong>0</strong></div>}
        </div>
      </section>
    </div>
  );
}

export default function App() {
  const fileInput = useRef(null);
  const hdf5Input = useRef(null);
  const layoutInput = useRef(null);
  const graphClipboard = useRef(null);
  const dragUndoCaptured = useRef(false);
  const [nodes, setNodes, reactFlowOnNodesChange] = useNodesState(initialNodes);
  const [edges, setEdges, reactFlowOnEdgesChange] = useEdgesState(initialEdges);
  const [importStatus, setImportStatus] = useState("No geometry loaded");
  const [geometryTemplate, setGeometryTemplate] = useState("<geometry/>");
  const [geometryFilename, setGeometryFilename] = useState("geometry.xml");
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
  const [, setUndoStack] = useState([]);
  const [, setRedoStack] = useState([]);
  const [selectedRequirementId, setSelectedRequirementId] = useState("GUI-080");
  const [copyStatus, setCopyStatus] = useState("");
  const [contextMenu, setContextMenu] = useState(null);
  const [attributeEditorNodeId, setAttributeEditorNodeId] = useState(null);
  const [showProjectInformation, setShowProjectInformation] = useState(false);
  const [originalGeometryFingerprint, setOriginalGeometryFingerprint] = useState("");

  const parsedRequirements = useMemo(() => parseRequirementSections(guiRequirementsMarkdown), []);

  const selectedRequirement = useMemo(() => {
    return parsedRequirements.find((requirement) => requirement.id === selectedRequirementId) ?? parsedRequirements[0] ?? null;
  }, [parsedRequirements, selectedRequirementId]);

  const attributeEditorNode = useMemo(
    () => {
      const node = nodes.find((item) => item.id === attributeEditorNodeId);
      return node ? { ...node, data: { ...node.data, xmlAttributes: attributesWithConnectivity(node, nodes, edges) } } : null;
    },
    [attributeEditorNodeId, edges, nodes]
  );


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
    setRedoStack([]);
  }, [edges, nodes]);

  const undoLastAction = useCallback(() => {
    setUndoStack((stack) => {
      const previous = stack.at(-1);
      if (!previous) return stack;

      setRedoStack((redos) => [...redos, cloneGraphState(nodes, edges)].slice(-MAX_UNDO_STEPS));
      setNodes(previous.nodes);
      setEdges(previous.edges);
      setHasUnsavedLayout(true);
      return stack.slice(0, -1);
    });
  }, [edges, nodes, setEdges, setNodes]);

  const redoLastAction = useCallback(() => {
    setRedoStack((stack) => {
      const next = stack.at(-1);
      if (!next) return stack;

      setUndoStack((undos) => [...undos, cloneGraphState(nodes, edges)].slice(-MAX_UNDO_STEPS));
      setNodes(next.nodes);
      setEdges(next.edges);
      setHasUnsavedLayout(true);
      return stack.slice(0, -1);
    });
  }, [edges, nodes, setEdges, setNodes]);

  const copySelectionForCanvas = useCallback(() => {
    const selection = graphClipboardFromSelection(nodes, edges);
    if (!selection) return false;

    graphClipboard.current = selection;
    return true;
  }, [edges, nodes]);

  const pasteCanvasSelection = useCallback(() => {
    if (!graphClipboard.current?.nodes?.length) return;

    pushUndoSnapshot();
    const pasted = pasteGraphClipboard(nodes, edges, graphClipboard.current);
    setNodes(pasted.nodes);
    setEdges(pasted.edges);
    graphClipboard.current = graphClipboardFromSelection(pasted.nodes, pasted.edges);
    setHasUnsavedLayout(true);
    setCopyStatus(`Pasted ${pasted.nodes.filter((node) => node.selected).length} component(s)`);
  }, [edges, nodes, pushUndoSnapshot, setEdges, setNodes]);

  const alignSelectedComponents = useCallback(
    (mode) => {
      if (selectedNodes(nodes).length < 2) return;

      pushUndoSnapshot();
      setNodes((nds) => alignNodes(nds, mode));
      setHasUnsavedLayout(true);
    },
    [nodes, pushUndoSnapshot, setNodes]
  );

  const distributeSelectedComponents = useCallback(
    (axis) => {
      if (selectedNodes(nodes).length < 3) return;

      pushUndoSnapshot();
      setNodes((nds) => distributeNodes(nds, axis));
      setHasUnsavedLayout(true);
    },
    [nodes, pushUndoSnapshot, setNodes]
  );

  const moveSelectedComponents = useCallback(
    (direction) => {
      if (!selectedNodes(nodes).length) return;
      pushUndoSnapshot();
      setNodes((nds) => moveSelectedNodes(nds, direction));
      setHasUnsavedLayout(true);
    },
    [nodes, pushUndoSnapshot, setNodes]
  );

  const changeSelectedSpacing = useCallback(
    (axis, direction) => {
      if (selectedNodes(nodes).length < 2) return;
      pushUndoSnapshot();
      setNodes((nds) => adjustSelectedSpacing(nds, axis, direction));
      setHasUnsavedLayout(true);
    },
    [nodes, pushUndoSnapshot, setNodes]
  );

  const transposeSelection = useCallback(() => {
    if (selectedNodes(nodes).length < 2) return;
    pushUndoSnapshot();
    setNodes((nds) => transposeSelectedLayout(nds));
    setHasUnsavedLayout(true);
  }, [nodes, pushUndoSnapshot, setNodes]);

  useEffect(() => {
    const warnBeforeClose = (event) => {
      if (!hasUnsavedLayout) return;
      event.preventDefault();
      event.returnValue = "";
    };

    window.addEventListener("beforeunload", warnBeforeClose);
    return () => window.removeEventListener("beforeunload", warnBeforeClose);
  }, [hasUnsavedLayout]);

  const onNodesChange = useCallback(
    (changes) => {
      const positionChanges = changes.filter((change) => change.type === "position");
      const isDragging = positionChanges.some((change) => change.dragging === true);
      if (isDragging && !dragUndoCaptured.current) {
        pushUndoSnapshot();
        dragUndoCaptured.current = true;
      } else if (positionChanges.length && !isDragging) {
        dragUndoCaptured.current = false;
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
                  rotation: ((node.data.rotation ?? 0) + 45) % 360
                }
              }
            : node
        )
      );
      setHasUnsavedLayout(true);
    },
    [pushUndoSnapshot, setNodes]
  );

  const renderedEdges = useMemo(
    () => revealConnectionsBehindComponents(
      nodes,
      rerouteBcEdges(nodes, rerouteFlowEdges(nodes, rerouteHeatEdges(nodes, edges)))
    ),
    [edges, nodes]
  );

  const copySelectionToClipboard = useCallback(async () => {
    const storedForCanvas = copySelectionForCanvas();

    try {
      const count = await copySelectedGraphImage(renderedEdges);
      setCopyStatus(`Copied ${count} component${count === 1 ? "" : "s"} as PNG`);
    } catch (error) {
      setCopyStatus(storedForCanvas ? "Copied selection for canvas paste" : error.message);
    }
  }, [copySelectionForCanvas, renderedEdges]);

  const exportSelectionToPng = useCallback(async () => {
    try {
      const count = await exportSelectedGraphPng(renderedEdges);
      setCopyStatus(`Exported ${count} component${count === 1 ? "" : "s"} to PNG`);
    } catch (error) {
      setCopyStatus(error.message);
    } finally {
      setContextMenu(null);
    }
  }, [renderedEdges]);

  const exportSelectionToSvg = useCallback(() => {
    try {
      const count = exportSelectedGraphSvg(renderedEdges);
      setCopyStatus(`Exported ${count} component${count === 1 ? "" : "s"} to SVG`);
    } catch (error) {
      setCopyStatus(error.message);
    } finally {
      setContextMenu(null);
    }
  }, [renderedEdges]);

  const exportSelectionToPdf = useCallback(async () => {
    try {
      const count = await exportSelectedGraphPdf(renderedEdges);
      setCopyStatus(`Exported ${count} component${count === 1 ? "" : "s"} to PDF`);
    } catch (error) {
      setCopyStatus(error.message);
    } finally {
      setContextMenu(null);
    }
  }, [renderedEdges]);

  useEffect(() => {
    const handleKeyDown = (event) => {
      if (event.key === "F1") {
        event.preventDefault();
        setActiveWorkspace("help");
        return;
      }
      if (isEditableTarget(event.target)) return;
      if (!(event.ctrlKey || event.metaKey)) return;

      const key = event.key.toLowerCase();
      if (key === "c") {
        if (!selectedNodes(nodes).length) return;
        event.preventDefault();
        copySelectionToClipboard();
        return;
      }

      if (key === "v") {
        event.preventDefault();
        pasteCanvasSelection();
        return;
      }

      if (key === "y" || (key === "z" && event.shiftKey)) {
        event.preventDefault();
        redoLastAction();
        return;
      }

      if (key !== "z") return;
      event.preventDefault();
      undoLastAction();
    };

    window.addEventListener("keydown", handleKeyDown);
    return () => window.removeEventListener("keydown", handleKeyDown);
  }, [copySelectionToClipboard, nodes, pasteCanvasSelection, redoLastAction, undoLastAction]);

  const renderedNodes = useMemo(() => {
    const flowConnections = buildFlowConnectionMap(nodes, renderedEdges);
    const heatHandleMap = buildHeatHandleMapFromEdges(renderedEdges);

    return nodes.map((node) => {
      const tooltipDetails = (node.data.tooltipLines ?? []).slice(1).filter((line) => !String(line).startsWith("elevation "));
      const elevation = node.data.xmlAttributes?.elevation;
      const heat = heatFor(node.id, heatHandleMap);
      const connections = flowConnections.get(node.id) ?? node.data.flowConnections ?? emptyFlowConnections();
      return {
        ...node,
        data: {
          ...node.data,
          tooltipLines: node.type === "flow"
            ? [node.data.identifier, elevation != null && `elevation ${elevation}`, ...tooltipDetails].filter(Boolean)
            : node.data.tooltipLines,
          heat,
          flowConnections: connections,
          onRotate: rotateNode
        }
      };
    });
  }, [nodes, renderedEdges, rotateNode]);
  const selectedComponentCount = selectedNodes(nodes).length;
  const canAlignSelection = selectedComponentCount >= 2;
  const canDistributeSelection = selectedComponentCount >= 3;

  const onConnect = useCallback(
    (params) => {
      if (!isAllowedConnection(params, nodes)) return;

      pushUndoSnapshot();
      setEdges((eds) => {
        const source = nodes.find((node) => node.id === params.source);
        const target = nodes.find((node) => node.id === params.target);
        const isHeatConnection = source?.data.kind === "hslab" || target?.data.kind === "hslab";
        const isBcConnection = source?.type === "bc" || target?.type === "bc";
        const bcNode = source?.type === "bc" ? source : target?.type === "bc" ? target : null;
        const bcTarget = bcNode === source ? target : source;
        const edge = isHeatConnection
          ? hslabEdge(`${params.source}->${params.target}`, params.source, params.target, new Map(nodes.map((node) => [node.id, node.position])))
          : isBcConnection
            ? bcEdge(`${bcNode.id}->${bcTarget.id}`, bcNode.id, bcTarget.id)
            : normalizedFlowConnection(params, nodes);
        if (!edge) return eds;
        return addEdge(edge, eds);
      });
      setHasUnsavedLayout(true);
    },
    [nodes, pushUndoSnapshot, setEdges]
  );

  const isValidConnection = useCallback((connection) => {
    return isAllowedConnection(connection, nodes);
  }, [nodes]);

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
      setContextMenu(null);
      if (!pendingComponentType) return;
      pushUndoSnapshot();
      setNodes((nds) => nds.concat(buildManualComponent(pendingComponentType, position)));
      setPendingComponentType("");
      setHasUnsavedLayout(true);
    },
    [pendingComponentType, pushUndoSnapshot, setNodes]
  );

  const onNodeContextMenu = useCallback(
    (event, node) => {
      event.preventDefault();

      if (!node.selected) {
        setNodes((nds) =>
          nds.map((item) => ({
            ...item,
            selected: item.id === node.id
          }))
        );
      }

      setContextMenu({
        x: event.clientX,
        y: event.clientY
      });
    },
    [setNodes]
  );

  const onNodeDoubleClick = useCallback((_event, node) => {
    setContextMenu(null);
    setAttributeEditorNodeId(node.id);
  }, []);

  const saveComponentAttributes = useCallback((xmlAttributes) => {
    if (!attributeEditorNode) return;
    pushUndoSnapshot();
    const nextIdentifier = xmlAttributes.identifier.trim();
    setNodes((current) => current.map((node) => node.id === attributeEditorNode.id
      ? { ...node, data: { ...node.data, identifier: nextIdentifier, xmlAttributes } }
      : node));
    setAttributeEditorNodeId(null);
    setHasUnsavedLayout(true);
    setImportStatus(`Updated ${nextIdentifier}`);
  }, [attributeEditorNode, pushUndoSnapshot, setNodes]);

  const onCanvasContextMenu = useCallback(
    (event) => {
      if (!selectedNodes(nodes).length) return;

      event.preventDefault();
      setContextMenu({
        x: event.clientX,
        y: event.clientY
      });
    },
    [nodes]
  );

  const closeContextMenu = useCallback(() => {
    setContextMenu(null);
  }, []);

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
        const nextGraph = applyStoredLayoutGraph(parsed.nodes, parsed.edges, storedLayout);

        setLayoutKey(nextLayoutKey);
        setNodes(nextGraph.nodes);
        setEdges(nextGraph.edges);
        setGeometryTemplate(parsed.templateXml);
        setGeometryFilename(file.name.toLowerCase().endsWith(".xml") ? file.name : `${file.name}.xml`);
        setOriginalGeometryFingerprint(geometryFingerprint(parsed.nodes, parsed.edges));
        setFitViewTrigger((count) => count + 1);
        setImportStatus(`Loaded ${file.name}${storedLayout ? " with saved layout" : ""}`);
        setActiveWorkspace("model");
        setHasUnsavedLayout(false);
        setUndoStack([]);
        setRedoStack([]);
        setCopyStatus("");
        graphClipboard.current = null;
        setContextMenu(null);
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

  const newCircuit = () => {
    setLayoutKey(MANUAL_LAYOUT_KEY);
    setNodes(initialNodes);
    setEdges(initialEdges);
    setGeometryTemplate("<geometry/>");
    setGeometryFilename("geometry.xml");
    setOriginalGeometryFingerprint("");
    setImportStatus("Blank circuit workspace");
    setActiveWorkspace("model");
    setHasUnsavedLayout(false);
    setUndoStack([]);
    setRedoStack([]);
    setCopyStatus("");
    graphClipboard.current = null;
    setContextMenu(null);
  };

  const exportGeometry = () => {
    try {
      const unchanged = originalGeometryFingerprint && geometryFingerprint(nodes, edges) === originalGeometryFingerprint;
      const { xml, warnings } = unchanged
        ? { xml: geometryTemplate, warnings: [] }
        : serializeOpenSdGeometry(geometryTemplate, nodes, edges);
      const url = URL.createObjectURL(new Blob([xml], { type: "application/xml;charset=utf-8" }));
      const anchor = document.createElement("a");
      anchor.href = url;
      anchor.download = geometryFilename;
      anchor.click();
      URL.revokeObjectURL(url);
      setImportStatus(warnings.length ? `Exported ${geometryFilename} (${warnings.length} connectivity warning${warnings.length === 1 ? "" : "s"})` : `Exported ${geometryFilename}`);
    } catch (error) {
      setImportStatus(error.message);
    }
  };

  const currentGeometryXml = useCallback(() => {
    if (!nodes.length) return null;
    const unchanged = originalGeometryFingerprint && geometryFingerprint(nodes, edges) === originalGeometryFingerprint;
    return unchanged ? geometryTemplate : serializeOpenSdGeometry(geometryTemplate, nodes, edges).xml;
  }, [edges, geometryTemplate, nodes, originalGeometryFingerprint]);

  const exportLayout = () => {
    if (!nodes.length) return;
    const url = URL.createObjectURL(new Blob([JSON.stringify(serializeLayout(nodes, layoutKey), null, 2)], { type: "application/json" }));
    const anchor = document.createElement("a");
    anchor.href = url;
    anchor.download = layoutFilename(layoutKey);
    anchor.click();
    URL.revokeObjectURL(url);
    setImportStatus("Layout sidecar exported");
  };

  const importLayout = async (file) => {
    if (!file) return;

    try {
      const layout = JSON.parse(await file.text());
      if (!layout?.nodes || typeof layout.nodes !== "object") {
        throw new Error("Layout file does not contain node positions.");
      }

      pushUndoSnapshot();
      const nextGraph = applyStoredLayoutGraph(nodes, edges, layout);
      setNodes(nextGraph.nodes);
      setEdges(nextGraph.edges);
      saveStoredLayout(layoutKey, nextGraph.nodes);
      setHasUnsavedLayout(false);
      setImportStatus(`Imported layout ${file.name}`);
      setFitViewTrigger((count) => count + 1);
    } catch (error) {
      setImportStatus(error.message);
    } finally {
      if (layoutInput.current) {
        layoutInput.current.value = "";
      }
    }
  };

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
          <input
            ref={layoutInput}
            className="file-input"
            type="file"
            accept=".json,application/json"
            onChange={(event) => importLayout(event.target.files?.[0])}
          />
          <button className="primary-button" onClick={() => fileInput.current?.click()} title="Import an OpenSD geometry XML file">
            Import XML
          </button>
          <button className="secondary-button secondary-button--inline" onClick={exportGeometry} disabled={!nodes.length} title="Export current geometry and connectivity as XML">
            Export XML
          </button>
          <div className="status-text">{importStatus}</div>
        </section>

        <section className="panel">
          <div className="panel-title">Circuit</div>
          <button className="primary-button" onClick={newCircuit} title="Remove all components and start with a blank layout">
            Clear layout
          </button>
          <button
            className="secondary-button secondary-button--inline"
            onClick={() => setShowProjectInformation(true)}
            title="Show project file, component, connection, and per-circuit statistics"
          >
            Project information
          </button>
          <div className="align-tools" aria-label="Alignment tools">
            <button type="button" title="Align selected components on one vertical axis" onClick={() => alignSelectedComponents("center")} disabled={!canAlignSelection}>
              Align vertical
            </button>
            <button type="button" title="Align selected components on one horizontal axis" onClick={() => alignSelectedComponents("middle")} disabled={!canAlignSelection}>
              Align horizontal
            </button>
            <button type="button" title="Move selected components left" onClick={() => moveSelectedComponents("left")} disabled={!selectedComponentCount}>
              Move &larr;
            </button>
            <button type="button" title="Move selected components right" onClick={() => moveSelectedComponents("right")} disabled={!selectedComponentCount}>
              Move &rarr;
            </button>
            <button type="button" title="Move selected components up" onClick={() => moveSelectedComponents("up")} disabled={!selectedComponentCount}>
              Move &uarr;
            </button>
            <button type="button" title="Move selected components down" onClick={() => moveSelectedComponents("down")} disabled={!selectedComponentCount}>
              Move &darr;
            </button>
            <button type="button" title="Move horizontally aligned components closer" onClick={() => changeSelectedSpacing("horizontal", "closer")} disabled={!canAlignSelection}>
              Closer H
            </button>
            <button type="button" title="Move horizontally aligned components farther apart" onClick={() => changeSelectedSpacing("horizontal", "farther")} disabled={!canAlignSelection}>
              Farther H
            </button>
            <button type="button" title="Move vertically aligned components closer" onClick={() => changeSelectedSpacing("vertical", "closer")} disabled={!canAlignSelection}>
              Closer V
            </button>
            <button type="button" title="Move vertically aligned components farther apart" onClick={() => changeSelectedSpacing("vertical", "farther")} disabled={!canAlignSelection}>
              Farther V
            </button>
            <button
              type="button"
              title="Distribute horizontally"
              onClick={() => distributeSelectedComponents("horizontal")}
              disabled={!canDistributeSelection}
            >
              Dist H
            </button>
            <button
              type="button"
              title="Distribute vertically"
              onClick={() => distributeSelectedComponents("vertical")}
              disabled={!canDistributeSelection}
            >
              Dist V
            </button>
            <button type="button" title="Rotate selected layout and pipe orientations 90 degrees clockwise" onClick={transposeSelection} disabled={!canAlignSelection}>
              Transpose
            </button>
          </div>
          {copyStatus && <div className="status-text status-text--hint">{copyStatus}</div>}
          <button className="secondary-button secondary-button--inline" onClick={exportLayout} disabled={!nodes.length} title="Export component positions as layout JSON">
            Export layout
          </button>
          <button className="secondary-button secondary-button--inline" onClick={() => layoutInput.current?.click()} disabled={!nodes.length} title="Import component positions from layout JSON">
            Import layout
          </button>
          {hasUnsavedLayout && <div className="status-text status-text--warning">Unsaved layout changes</div>}
        </section>

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
                title={`Add or drag a ${component} component`}
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

      </aside>

      <main className="workspace">
        <div className="workspace-tabs">
          <button
            className={activeWorkspace === "model" ? "active" : ""}
            onClick={() => setActiveWorkspace("model")}
            title="Edit OpenSD geometry (F1 opens Help)"
          >
            Pre-processor
          </button>
          <button
            className={activeWorkspace === "solver" ? "active" : ""}
            onClick={() => setActiveWorkspace("solver")}
            title="Configure and run the OpenSD solver"
          >
            Solver
          </button>
          <button
            className={activeWorkspace === "postprocess" ? "active" : ""}
            onClick={() => setActiveWorkspace("postprocess")}
            title="Import and plot HDF5 results"
          >
            Postprocessor
          </button>
          <button
            className={activeWorkspace === "help" ? "active" : ""}
            onClick={() => setActiveWorkspace("help")}
            title="Open GUI help (F1)"
          >
            Help
          </button>
          <button
            className={activeWorkspace === "requirements" ? "active" : ""}
            onClick={() => setActiveWorkspace("requirements")}
            title="Browse bundled GUI requirements"
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
            isValidConnection={isValidConnection}
            onDrop={onDrop}
            onDragOver={onDragOver}
            onPaneClick={onPaneClick}
            onCanvasContextMenu={onCanvasContextMenu}
            onNodeContextMenu={onNodeContextMenu}
            onNodeDoubleClick={onNodeDoubleClick}
            fitViewTrigger={fitViewTrigger}
          />
        )}

        {contextMenu && activeWorkspace === "model" && (
          <div
            className="canvas-context-menu"
            style={{ left: contextMenu.x, top: contextMenu.y }}
            onContextMenu={(event) => event.preventDefault()}
          >
            <button type="button" onClick={exportSelectionToPng} disabled={!selectedComponentCount}>
              Export selection to PNG
            </button>
            <button type="button" onClick={exportSelectionToSvg} disabled={!selectedComponentCount}>
              Export selection to SVG
            </button>
            <button type="button" onClick={exportSelectionToPdf} disabled={!selectedComponentCount}>
              Export selection to PDF
            </button>
            <button
              type="button"
              onClick={() => {
                copySelectionForCanvas();
                pasteCanvasSelection();
                closeContextMenu();
              }}
              disabled={!selectedComponentCount}
            >
              Duplicate selection
            </button>
            <button type="button" onClick={closeContextMenu}>
              Close
            </button>
          </div>
        )}

        {attributeEditorNode && (
          <AttributeEditor
            key={attributeEditorNode.id}
            node={attributeEditorNode}
            onCancel={() => setAttributeEditorNodeId(null)}
            onSave={saveComponentAttributes}
          />
        )}

        {showProjectInformation && (
          <ProjectInformation
            filename={geometryFilename}
            layoutKey={layoutKey}
            nodes={nodes}
            edges={edges}
            hasUnsavedLayout={hasUnsavedLayout}
            onClose={() => setShowProjectInformation(false)}
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

        {activeWorkspace === "solver" && (
          <SolverPanel currentGeometryXml={currentGeometryXml} />
        )}

        {activeWorkspace === "requirements" && (
          <RequirementManager
            requirements={parsedRequirements}
            selectedId={selectedRequirement?.id ?? ""}
            onSelect={selectRequirement}
          />
        )}

        {activeWorkspace === "help" && <HelpDocument text={guiHelpMarkdown} />}
      </main>
    </div>
  );
}
