import { memo, useState } from "react";
import { Handle, Position } from "reactflow";

import { shortLabel } from "./labelUtils.js";
import NodeTooltip from "./NodeTooltip.jsx";

const bcPorts = [
  ["n", Position.Top, { left: "50%", top: "0%" }],
  ["ne", Position.Top, { left: "85%", top: "15%" }],
  ["e", Position.Right, { left: "100%", top: "50%" }],
  ["se", Position.Bottom, { left: "85%", top: "85%" }],
  ["s", Position.Bottom, { left: "50%", top: "100%" }],
  ["sw", Position.Bottom, { left: "15%", top: "85%" }],
  ["w", Position.Left, { left: "0%", top: "50%" }],
  ["nw", Position.Top, { left: "15%", top: "15%" }]
];

function BcNode({ data }) {
  const [hovered, setHovered] = useState(false);
  const tooltipLines = data.tooltipLines ?? [data.identifier].filter(Boolean);

  return (
    <div
      className="bc-node component-shell"
      onMouseEnter={() => setHovered(true)}
      onMouseLeave={() => setHovered(false)}
    >
      {bcPorts.flatMap(([port, position, style]) => [
        <Handle key={`${port}-out`} type="source" id={`bc-${port}-out`} position={position} style={style} className="bc-node__handle" />,
        <Handle key={`${port}-in`} type="target" id={`bc-${port}-in`} position={position} style={style} className="bc-node__handle" />
      ])}
      <span className="bc-node__mark">BC</span>
      <span className="bc-node__label">{shortLabel(data.identifier, 8)}</span>
      <NodeTooltip lines={tooltipLines} visible={hovered} />
    </div>
  );
}

export default memo(BcNode);
