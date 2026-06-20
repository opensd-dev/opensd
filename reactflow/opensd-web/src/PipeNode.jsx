import { memo, useState } from "react";
import { Handle, Position } from "reactflow";

import { shortLabel } from "./labelUtils.js";
import NodeTooltip from "./NodeTooltip.jsx";

const flowPositions = {
  0: { in: Position.Left, out: Position.Right },
  45: { in: Position.Left, out: Position.Right },
  90: { in: Position.Top, out: Position.Bottom },
  135: { in: Position.Left, out: Position.Right },
  180: { in: Position.Right, out: Position.Left },
  225: { in: Position.Left, out: Position.Right },
  270: { in: Position.Bottom, out: Position.Top },
  315: { in: Position.Left, out: Position.Right }
};

const heatLongSidePositions = {
  0: { top: Position.Top, bottom: Position.Bottom },
  45: { top: Position.Top, bottom: Position.Bottom },
  90: { top: Position.Left, bottom: Position.Right },
  135: { top: Position.Top, bottom: Position.Bottom },
  180: { top: Position.Bottom, bottom: Position.Top },
  225: { top: Position.Top, bottom: Position.Bottom },
  270: { top: Position.Right, bottom: Position.Left },
  315: { top: Position.Top, bottom: Position.Bottom }
};

const DIAGONAL_ORIENTATIONS = new Set([45, 135, 225, 315]);
const INLET_ARROW_CLEARANCE = 4;
const PIPE_DIMENSIONS = {
  default: { width: 84, height: 32 },
  circuit: { width: 92, height: 32 }
};

function rotatedHandle(orientation, kind, localX, localY) {
  if (!DIAGONAL_ORIENTATIONS.has(orientation)) return null;

  const dimensions = kind === "circuit" ? PIPE_DIMENSIONS.circuit : PIPE_DIMENSIONS.default;
  const angle = (orientation * Math.PI) / 180;
  const left = dimensions.width / 2 + localX * Math.cos(angle) - localY * Math.sin(angle);
  const top = dimensions.height / 2 + localX * Math.sin(angle) + localY * Math.cos(angle);

  return {
    position: Position.Top,
    style: { left, top, transform: "translate(-50%, -50%)" }
  };
}

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
  const heatHandles = heatLongSidePositions[orientation];
  const dimensions = kind === "circuit" ? PIPE_DIMENSIONS.circuit : PIPE_DIMENSIONS.default;
  const flowIn = rotatedHandle(orientation, kind, -dimensions.width / 2, 0);
  const flowInTarget = rotatedHandle(
    orientation,
    kind,
    -dimensions.width / 2 - INLET_ARROW_CLEARANCE,
    0
  );
  const flowOut = rotatedHandle(orientation, kind, dimensions.width / 2, 0);
  const heatTop = rotatedHandle(orientation, kind, 0, -dimensions.height / 2);
  const heatBottom = rotatedHandle(orientation, kind, 0, dimensions.height / 2);
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
          <Handle type="target" id="flow-in" position={flowInTarget?.position ?? handles.in} style={flowInTarget?.style} className="handle-flow" />
          <Handle type="source" id="flow-out" position={flowOut?.position ?? handles.out} style={flowOut?.style} className="handle-flow" />
          <Handle type="source" id="flow-in" position={flowIn?.position ?? handles.in} style={flowIn?.style} className="handle-flow" />
          <Handle type="target" id="flow-out" position={flowOut?.position ?? handles.out} style={flowOut?.style} className="handle-flow" />
        </>
      )}
      {heat.htIn && (
        <Handle type="target" id="ht-in" position={heatTop?.position ?? heatHandles.top} style={heatTop?.style} className="handle-heat" />
      )}
      {heat.htOut && (
        <Handle type="source" id="ht-out" position={heatBottom?.position ?? heatHandles.bottom} style={heatBottom?.style} className="handle-heat" />
      )}
      {heat.htTopIn && (
        <Handle type="target" id="ht-top-in" position={heatTop?.position ?? heatHandles.top} style={heatTop?.style} className="handle-heat" />
      )}
      {heat.htTopOut && (
        <Handle type="source" id="ht-top-out" position={heatTop?.position ?? heatHandles.top} style={heatTop?.style} className="handle-heat" />
      )}
      {heat.htBottomIn && (
        <Handle type="target" id="ht-bottom-in" position={heatBottom?.position ?? heatHandles.bottom} style={heatBottom?.style} className="handle-heat" />
      )}
      {heat.htBottomOut && (
        <Handle type="source" id="ht-bottom-out" position={heatBottom?.position ?? heatHandles.bottom} style={heatBottom?.style} className="handle-heat" />
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
      <div className="pipe-node__body">
        {kind === "pump" && <span className="pump-node__symbol" aria-hidden="true" />}
        <span className="pipe-node__label">{shortLabel(data.identifier, 14)}</span>
      </div>
      <NodeTooltip lines={tooltipLines} visible={hovered} />
    </div>
  );
}

export default memo(PipeNode);
