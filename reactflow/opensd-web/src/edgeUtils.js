import { MarkerType } from "reactflow";

export const FLOW_MARKER = {
  type: MarkerType.Arrow,
  width: 10,
  height: 10,
  color: "#3468b5"
};

export const HEAT_MARKER = {
  type: MarkerType.Arrow,
  width: 9,
  height: 9,
  color: "#e67e22"
};

/** Heat links use top/bottom on pipes; side handles for hslab → layer. */
export function resolveHeatHandles(source, target) {
  const handles = {};

  if (source.startsWith("pipe:") || source.startsWith("node:")) {
    handles.sourceHandle = "ht-out";
  } else if (source.startsWith("hslab:")) {
    handles.sourceHandle = target.startsWith("layer:") ? "ht-side-out" : "ht-out";
  } else if (source.startsWith("layer:")) {
    handles.sourceHandle = "ht-side-out";
  }

  if (target.startsWith("pipe:") || target.startsWith("node:")) {
    handles.targetHandle = "ht-in";
  } else if (target.startsWith("layer:") && source.startsWith("hslab:")) {
    handles.targetHandle = "ht-side-in";
  } else if (target.startsWith("hslab:") || target.startsWith("layer:")) {
    handles.targetHandle = "ht-in";
  }

  return handles;
}

function emptyHeatFlags() {
  return { htIn: false, htOut: false, htSideIn: false, htSideOut: false };
}

function markHeatEndpoint(flags, nodeId, handleId) {
  if (!nodeId || !handleId) return;
  if (!flags.has(nodeId)) flags.set(nodeId, emptyHeatFlags());
  const entry = flags.get(nodeId);

  if (handleId === "ht-in") entry.htIn = true;
  if (handleId === "ht-out") entry.htOut = true;
  if (handleId === "ht-side-in") entry.htSideIn = true;
  if (handleId === "ht-side-out") entry.htSideOut = true;
}

/** Which handles each node needs for hslab-related edges only. */
export function buildHeatHandleMap(geometry, attr) {
  const flags = new Map();
  const hslabs = Array.from(geometry.querySelectorAll(":scope > hslab"));

  for (const hslab of hslabs) {
    const hslabName = attr(hslab, "identifier", "hslab");
    const hslabId = `hslab:${hslabName}`;
    const ucomp = attr(hslab, "ucomp");
    const dcomp = attr(hslab, "dcomp");
    const upstreamId = attr(hslab, "uvar") === "pipe" ? `pipe:${ucomp}` : `node:${ucomp}`;
    const downstreamId = attr(hslab, "dvar") === "pipe" ? `pipe:${dcomp}` : `node:${dcomp}`;

    const upstreamEdge = resolveHeatHandles(upstreamId, hslabId);
    markHeatEndpoint(flags, upstreamId, upstreamEdge.sourceHandle);
    markHeatEndpoint(flags, hslabId, upstreamEdge.targetHandle);

    const downstreamEdge = resolveHeatHandles(hslabId, downstreamId);
    markHeatEndpoint(flags, hslabId, downstreamEdge.sourceHandle);
    markHeatEndpoint(flags, downstreamId, downstreamEdge.targetHandle);
  }

  return flags;
}
