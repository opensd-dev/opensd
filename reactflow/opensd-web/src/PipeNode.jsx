import { memo, useState } from "react";
import { Handle, Position } from "reactflow";

import { shortLabel } from "./labelUtils.js";
import NodeTooltip from "./NodeTooltip.jsx";

const flowPositions = {
  0: { in: Position.Left, out: Position.Right },
  90: { in: Position.Top, out: Position.Bottom },
  180: { in: Position.Right, out: Position.Left },
  270: { in: Position.Bottom, out: Position.Top }
};

function RotateButton({ id, data, selected }) {
  if (!selected || !data.onRotate) return null;

  return (
    <button
      type="button"
      className="node-rotate-button nodrag nopan"
      title="Rotate component"
      onClick={(event) => {
        event.stopPropagation();
        data.onRotate(id);
      }}
    >
      ↻
    </button>
  );
}

function PipeNode({ id, data, selected }) {
  const [hovered, setHovered] = useState(false);
  const tooltipLines = data.tooltipLines ?? [data.identifier].filter(Boolean);
  const kind = data.kind ?? "pipe";
  const heat = data.heat ?? {};
  const rotation = ((data.rotation ?? 0) % 360 + 360) % 360;
  const orientation = flowPositions[rotation] ? rotation : 0;
  const handles = flowPositions[orientation];
  const showFlowHandles = !["hslab", "layer"].includes(kind);

  return (
    <div
      className={`pipe-node pipe-node--${kind} pipe-node--rot-${orientation} component-shell`}
      onMouseEnter={() => setHovered(true)}
      onMouseLeave={() => setHovered(false)}
    >
      <RotateButton id={id} data={data} selected={selected} />
      {showFlowHandles && (
        <>
          <Handle type="target" id="flow-in" position={handles.in} className="handle-flow" />
          <Handle type="source" id="flow-out" position={handles.out} className="handle-flow" />
          <Handle type="source" id="flow-in" position={handles.in} className="handle-flow" />
          <Handle type="target" id="flow-out" position={handles.out} className="handle-flow" />
        </>
      )}
      {heat.htIn && (
        <Handle type="target" id="ht-in" position={Position.Top} className="handle-heat" />
      )}
      {heat.htOut && (
        <Handle type="source" id="ht-out" position={Position.Bottom} className="handle-heat" />
      )}
      {heat.htTopIn && (
        <Handle type="target" id="ht-top-in" position={Position.Top} className="handle-heat" />
      )}
      {heat.htTopOut && (
        <Handle type="source" id="ht-top-out" position={Position.Top} className="handle-heat" />
      )}
      {heat.htBottomIn && (
        <Handle type="target" id="ht-bottom-in" position={Position.Bottom} className="handle-heat" />
      )}
      {heat.htBottomOut && (
        <Handle type="source" id="ht-bottom-out" position={Position.Bottom} className="handle-heat" />
      )}
      {heat.htSideOut && (
        <Handle
          type="source"
          id="ht-side-out"
          position={Position.Right}
          className="handle-heat"
          style={{ top: "78%" }}
        />
      )}
      {heat.htSideIn && (
        <Handle
          type="target"
          id="ht-side-in"
          position={Position.Left}
          className="handle-heat"
          style={{ top: "78%" }}
        />
      )}
      <span className="pipe-node__label">{shortLabel(data.identifier, 14)}</span>
      <NodeTooltip lines={tooltipLines} visible={hovered} />
    </div>
  );
}

export default memo(PipeNode);
