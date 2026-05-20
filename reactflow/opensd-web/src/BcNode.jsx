import { memo, useState } from "react";
import { Handle, Position } from "reactflow";

import NodeTooltip, { shortLabel } from "./NodeTooltip.jsx";

function BcNode({ data }) {
  const [hovered, setHovered] = useState(false);
  const tooltipLines = data.tooltipLines ?? [data.identifier].filter(Boolean);

  return (
    <div
      className="bc-node"
      onMouseEnter={() => setHovered(true)}
      onMouseLeave={() => setHovered(false)}
    >
      <Handle type="source" id="bc-out" position={Position.Bottom} />
      <span className="bc-node__label">{shortLabel(data.identifier, 10)}</span>
      <NodeTooltip lines={tooltipLines} visible={hovered} />
    </div>
  );
}

export default memo(BcNode);
