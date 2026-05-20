import { memo, useState } from "react";
import { Handle, Position } from "reactflow";

import NodeTooltip, { shortLabel } from "./NodeTooltip.jsx";

function PipeNode({ data }) {
  const [hovered, setHovered] = useState(false);
  const tooltipLines = data.tooltipLines ?? [data.identifier].filter(Boolean);
  const kind = data.kind ?? "pipe";
  const heat = data.heat ?? {};

  return (
    <div
      className={`pipe-node pipe-node--${kind}`}
      onMouseEnter={() => setHovered(true)}
      onMouseLeave={() => setHovered(false)}
    >
      <Handle type="target" id="flow-in" position={Position.Left} style={{ top: "50%" }} />
      <Handle type="source" id="flow-out" position={Position.Right} style={{ top: "50%" }} />
      {heat.htIn && (
        <Handle type="target" id="ht-in" position={Position.Top} className="handle-heat" />
      )}
      {heat.htOut && (
        <Handle type="source" id="ht-out" position={Position.Bottom} className="handle-heat" />
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
