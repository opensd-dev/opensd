const PIPE_STEP = 96;
const BC_STACK_GAP = 46;
const BC_LIFT = 58;
const CIRCUIT_LABEL_WIDTH = 88;
const LAYER_STEP_X = 176;
const LANE_GAP = 20;
const ROW_TOP_PADDING = 12;
const ROW_BOTTOM_EXTENT = 32;
const ROW_GAP = 24;

export function nodeData(identifier, details = []) {
  const tooltipLines = [identifier, ...details.filter(Boolean)];
  return { identifier, tooltipLines };
}

export function buildHorizontalSequence(nodeNames, pipes, compare) {
  const outByNode = new Map();
  const inDegree = new Map();

  nodeNames.forEach((name) => inDegree.set(name, 0));

  for (const pipe of pipes) {
    if (!outByNode.has(pipe.unode)) outByNode.set(pipe.unode, []);
    outByNode.get(pipe.unode).push(pipe);
    inDegree.set(pipe.dnode, (inDegree.get(pipe.dnode) ?? 0) + 1);
  }

  for (const list of outByNode.values()) {
    list.sort((a, b) => compare(a.name, b.name));
  }

  const sequence = [];
  const usedPipes = new Set();
  const usedNodes = new Set();

  function walkFrom(nodeName) {
    if (usedNodes.has(nodeName)) return;
    usedNodes.add(nodeName);
    sequence.push({ kind: "node", name: nodeName });

    for (const pipe of outByNode.get(nodeName) ?? []) {
      if (usedPipes.has(pipe.name)) continue;
      usedPipes.add(pipe.name);
      sequence.push({ kind: "pipe", name: pipe.name, pipe });
      walkFrom(pipe.dnode);
    }
  }

  let starts = [...nodeNames].filter((name) => (inDegree.get(name) ?? 0) === 0);
  starts.sort(compare);
  if (starts.length === 0) starts = [...nodeNames].sort(compare);

  for (const start of starts) walkFrom(start);

  const sortedPipes = [...pipes].sort((a, b) => compare(a.name, b.name));
  for (const pipe of sortedPipes) {
    if (usedPipes.has(pipe.name)) continue;
    walkFrom(pipe.unode);
  }

  for (const name of [...nodeNames].sort(compare)) {
    if (!usedNodes.has(name)) sequence.push({ kind: "node", name });
  }

  return sequence;
}

export function layoutCircuitRow({ circuitId, sequence, bcRecords, previousBottom }) {
  const nodeNames = sequence.filter((item) => item.kind === "node").map((item) => item.name);
  const pipes = sequence.filter((item) => item.kind === "pipe").map((item) => item.pipe);
  const nodeOrder = new Map(nodeNames.map((name, index) => [name, index]));
  const outgoing = new Map(nodeNames.map((name) => [name, []]));
  const inDegree = new Map(nodeNames.map((name) => [name, 0]));

  for (const pipe of pipes) {
    if (!outgoing.has(pipe.unode)) outgoing.set(pipe.unode, []);
    outgoing.get(pipe.unode).push(pipe.dnode);
    inDegree.set(pipe.dnode, (inDegree.get(pipe.dnode) ?? 0) + 1);
  }

  const depthByNode = new Map();
  const starts = nodeNames.filter((name) => (inDegree.get(name) ?? 0) === 0);
  const seeds = starts.length > 0 ? starts : nodeNames.slice(0, 1);

  function spreadFrom(seed) {
    if (depthByNode.has(seed)) return;
    depthByNode.set(seed, 0);
    const queue = [seed];

    for (let cursor = 0; cursor < queue.length; cursor += 1) {
      const name = queue[cursor];
      const nextDepth = depthByNode.get(name) + 1;
      for (const target of outgoing.get(name) ?? []) {
        if (depthByNode.has(target)) continue;
        depthByNode.set(target, nextDepth);
        queue.push(target);
      }
    }
  }

  seeds.forEach(spreadFrom);
  nodeNames.forEach(spreadFrom);

  const nodesByDepth = new Map();
  for (const name of nodeNames) {
    const depth = depthByNode.get(name) ?? 0;
    if (!nodesByDepth.has(depth)) nodesByDepth.set(depth, []);
    nodesByDepth.get(depth).push(name);
  }

  for (const names of nodesByDepth.values()) {
    names.sort((a, b) => (nodeOrder.get(a) ?? 0) - (nodeOrder.get(b) ?? 0));
  }

  const bcCountByTarget = bcRecords.reduce(
    (counts, bc) => counts.set(bc.targetNode, (counts.get(bc.targetNode) ?? 0) + 1),
    new Map()
  );
  const relativeNodePositions = new Map();
  for (const [depth, names] of nodesByDepth) {
    let laneCursor = 0;
    const lanePositions = names.map((name) => {
      const bcCount = bcCountByTarget.get(name) ?? 0;
      const topExtent = bcCount > 0 ? BC_LIFT + (bcCount - 1) * BC_STACK_GAP + ROW_TOP_PADDING : ROW_TOP_PADDING;
      const y = laneCursor + topExtent;
      laneCursor = y + ROW_BOTTOM_EXTENT + LANE_GAP;
      return { name, y };
    });
    const layerHeight = Math.max(0, laneCursor - LANE_GAP);
    lanePositions.forEach(({ name, y }) => {
      relativeNodePositions.set(name, {
        x: 24 + CIRCUIT_LABEL_WIDTH + depth * LAYER_STEP_X,
        y: y - layerHeight / 2
      });
    });
  }

  let contentTop = -ROW_TOP_PADDING;
  let contentBottom = ROW_BOTTOM_EXTENT;
  for (const [name, position] of relativeNodePositions) {
    const bcCount = bcCountByTarget.get(name) ?? 0;
    const bcExtent = bcCount > 0 ? BC_LIFT + (bcCount - 1) * BC_STACK_GAP : 0;
    contentTop = Math.min(contentTop, position.y - bcExtent - ROW_TOP_PADDING);
    contentBottom = Math.max(contentBottom, position.y + ROW_BOTTOM_EXTENT);
  }

  const layoutTop = previousBottom == null ? 38 : previousBottom + ROW_GAP;
  const yOffset = layoutTop - contentTop;
  const firstNodeY = relativeNodePositions.get(nodeNames[0])?.y ?? 0;
  const circuitY = yOffset + firstNodeY;
  const positions = new Map();

  positions.set(`circuit:${circuitId}`, { x: 24, y: circuitY });
  for (const [name, position] of relativeNodePositions) {
    positions.set(`node:${name}`, { x: position.x, y: yOffset + position.y });
  }

  const occupied = Array.from(relativeNodePositions.values(), (position) => ({
    x: position.x,
    y: yOffset + position.y,
    width: 22,
    height: 22
  }));
  for (const [targetNode, count] of bcCountByTarget) {
    const anchor = relativeNodePositions.get(targetNode);
    if (!anchor) continue;
    for (let index = 0; index < count; index += 1) {
      occupied.push({
        x: anchor.x,
        y: yOffset + anchor.y - BC_LIFT - index * BC_STACK_GAP,
        width: 52,
        height: 24
      });
    }
  }

  function overlapsOccupied(box) {
    const clearance = 10;
    return occupied.some((other) => (
      box.x < other.x + other.width + clearance
      && box.x + box.width + clearance > other.x
      && box.y < other.y + other.height + clearance
      && box.y + box.height + clearance > other.y
    ));
  }

  const pipeCountByPair = new Map();
  const pipeIndexByPair = new Map();
  for (const pipe of pipes) {
    const key = `${pipe.unode}->${pipe.dnode}`;
    pipeCountByPair.set(key, (pipeCountByPair.get(key) ?? 0) + 1);
  }
  for (const pipe of pipes) {
    const source = relativeNodePositions.get(pipe.unode) ?? { x: 24 + CIRCUIT_LABEL_WIDTH, y: 0 };
    const target = relativeNodePositions.get(pipe.dnode) ?? { x: source.x + LAYER_STEP_X, y: source.y };
    const key = `${pipe.unode}->${pipe.dnode}`;
    const index = pipeIndexByPair.get(key) ?? 0;
    const count = pipeCountByPair.get(key) ?? 1;
    const horizontalDelta = target.x - source.x;
    const pipeX = source.x + (horizontalDelta === 0 ? LAYER_STEP_X / 2 : horizontalDelta / 2);
    const preferredY = yOffset + source.y + (target.y - source.y) / 2 + (index - (count - 1) / 2) * 36;
    let pipeY = preferredY;
    for (let attempt = 1; overlapsOccupied({ x: pipeX, y: pipeY, width: 64, height: 28 }); attempt += 1) {
      const direction = attempt % 2 === 1 ? 1 : -1;
      const distance = Math.ceil(attempt / 2) * 42;
      pipeY = Math.max(layoutTop, preferredY + direction * distance);
    }
    pipeIndexByPair.set(key, index + 1);
    positions.set(`pipe:${pipe.name}`, {
      x: pipeX,
      y: pipeY
    });
    occupied.push({ x: pipeX, y: pipeY, width: 64, height: 28 });
  }

  const bcsByTarget = new Map();
  for (const bc of bcRecords) {
    if (!bcsByTarget.has(bc.targetNode)) bcsByTarget.set(bc.targetNode, []);
    bcsByTarget.get(bc.targetNode).push(bc);
  }

  for (const [targetNode, bcs] of bcsByTarget) {
    const anchor = positions.get(`node:${targetNode}`);
    if (!anchor) continue;

    bcs.forEach((bc, index) => {
      positions.set(bc.id, {
        x: anchor.x,
        y: anchor.y - BC_LIFT - index * BC_STACK_GAP
      });
    });
  }

  return {
    circuitY,
    positions,
    bounds: {
      top: layoutTop,
      right: Math.max(24 + CIRCUIT_LABEL_WIDTH, ...Array.from(positions.values(), (position) => position.x + PIPE_STEP)),
      bottom: Math.max(yOffset + contentBottom, ...Array.from(positions.values(), (position) => position.y + ROW_BOTTOM_EXTENT))
    }
  };
}

export { PIPE_STEP };
