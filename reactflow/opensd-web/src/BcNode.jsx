import { memo, useState } from "react";
import { Handle, Position } from "reactflow";

import { shortLabel } from "./labelUtils.js";
import NodeTooltip from "./NodeTooltip.jsx";

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

function BcNode({ id, data, selected }) {
  const [hovered, setHovered] = useState(false);
  const tooltipLines = data.tooltipLines ?? [data.identifier].filter(Boolean);

  return (
    <div
      className="bc-node component-shell"
      onMouseEnter={() => setHovered(true)}
      onMouseLeave={() => setHovered(false)}
    >
      <RotateButton id={id} data={data} selected={selected} />
      <Handle type="source" id="bc-top-out" position={Position.Top} />
      <Handle type="source" id="bc-bottom-out" position={Position.Bottom} />
      <span className="bc-node__label">{shortLabel(data.identifier, 10)}</span>
      <NodeTooltip lines={tooltipLines} visible={hovered} />
    </div>
  );
}

export default memo(BcNode);
