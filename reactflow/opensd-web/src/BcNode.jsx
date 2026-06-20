import { memo, useState } from "react";
import { Handle, Position } from "reactflow";

import { shortLabel } from "./labelUtils.js";
import NodeTooltip from "./NodeTooltip.jsx";

function BcNode({ data }) {
  const [hovered, setHovered] = useState(false);
  const tooltipLines = data.tooltipLines ?? [data.identifier].filter(Boolean);

  return (
    <div
      className="bc-node component-shell"
      onMouseEnter={() => setHovered(true)}
      onMouseLeave={() => setHovered(false)}
    >
      <Handle type="source" id="bc-top-out" position={Position.Top} className="bc-node__handle" />
      <Handle type="source" id="bc-bottom-out" position={Position.Bottom} className="bc-node__handle" />
      <span className="bc-node__mark">BC</span>
      <span className="bc-node__label">{shortLabel(data.identifier, 8)}</span>
      <NodeTooltip lines={tooltipLines} visible={hovered} />
    </div>
  );
}

export default memo(BcNode);
