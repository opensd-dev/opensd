const CIRCUIT_ROW_GAP = 130;
const NODE_STEP = 64;
const PIPE_STEP = 96;
const BC_STACK_GAP = 46;
const BC_LIFT = 58;
const CIRCUIT_LABEL_WIDTH = 88;

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

export function layoutCircuitRow({ circuitIndex, circuitId, sequence, bcRecords }) {
  const circuitY = 50 + circuitIndex * CIRCUIT_ROW_GAP;
  let x = 24;
  const positions = new Map();

  positions.set(`circuit:${circuitId}`, { x, y: circuitY });
  x += CIRCUIT_LABEL_WIDTH;

  for (const item of sequence) {
    if (item.kind === "node") {
      positions.set(`node:${item.name}`, { x, y: circuitY });
      x += NODE_STEP;
    } else {
      positions.set(`pipe:${item.name}`, { x, y: circuitY });
      x += PIPE_STEP;
    }
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

  return { circuitY, positions };
}

export { CIRCUIT_ROW_GAP, PIPE_STEP };
