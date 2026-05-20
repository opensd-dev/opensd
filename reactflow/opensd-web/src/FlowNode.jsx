import { memo, useState } from "react";
import { Handle, Position } from "reactflow";

import NodeTooltip, { shortLabel } from "./NodeTooltip.jsx";

function FlowNode({ data }) {
  const [hovered, setHovered] = useState(false);
  const tooltipLines = data.tooltipLines ?? [data.identifier].filter(Boolean);
  const heat = data.heat ?? {};

  return (
    <div
      className="flow-node"
      onMouseEnter={() => setHovered(true)}
      onMouseLeave={() => setHovered(false)}
    >
      <Handle type="target" id="flow-in" position={Position.Left} />
      <Handle type="source" id="flow-out" position={Position.Right} />
      <Handle type="target" id="bc-in" position={Position.Top} />
      {heat.htIn && (
        <Handle
          type="target"
          id="ht-in"
          position={Position.Bottom}
          className="handle-heat"
          style={{ left: "38%" }}
        />
      )}
      {heat.htOut && (
        <Handle
          type="source"
          id="ht-out"
          position={Position.Bottom}
          className="handle-heat"
          style={{ left: "62%" }}
        />
      )}
      <span className="flow-node__label">{shortLabel(data.identifier, 8)}</span>
      <NodeTooltip lines={tooltipLines} visible={hovered} />
    </div>
  );
}

export default memo(FlowNode);
