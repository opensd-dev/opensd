import { memo, useState } from "react";
import { Handle, Position } from "reactflow";

import { shortLabel } from "./labelUtils.js";
import NodeTooltip from "./NodeTooltip.jsx";

function FlowNode({ data }) {
  const [hovered, setHovered] = useState(false);
  const tooltipLines = data.tooltipLines ?? [data.identifier].filter(Boolean);
  const heat = data.heat ?? {};
  const flowHandles = [
    ["left", Position.Left],
    ["right", Position.Right],
    ["top", Position.Top],
    ["bottom", Position.Bottom]
  ];
  const offsetFlowHandles = [
    ["left", "a", Position.Left, { top: "35%" }],
    ["left", "b", Position.Left, { top: "65%" }],
    ["right", "a", Position.Right, { top: "35%" }],
    ["right", "b", Position.Right, { top: "65%" }],
    ["top", "a", Position.Top, { left: "35%" }],
    ["top", "b", Position.Top, { left: "65%" }],
    ["bottom", "a", Position.Bottom, { left: "35%" }],
    ["bottom", "b", Position.Bottom, { left: "65%" }]
  ];

  return (
    <div
      className="flow-node component-shell"
      onMouseEnter={() => setHovered(true)}
      onMouseLeave={() => setHovered(false)}
    >
      <Handle type="target" id="flow-in" position={Position.Left} className="handle-flow" />
      <Handle type="source" id="flow-out" position={Position.Right} className="handle-flow" />
      <Handle type="source" id="flow-in" position={Position.Left} className="handle-flow" />
      <Handle type="target" id="flow-out" position={Position.Right} className="handle-flow" />
      {flowHandles.map(([side, position]) => (
        <Handle
          key={`flow-${side}-in`}
          type="target"
          id={`flow-${side}-in`}
          position={position}
          className="handle-flow"
        />
      ))}
      {flowHandles.map(([side, position]) => (
        <Handle
          key={`flow-${side}-out`}
          type="source"
          id={`flow-${side}-out`}
          position={position}
          className="handle-flow"
        />
      ))}
      {offsetFlowHandles.map(([side, slot, position, style]) => (
        <Handle
          key={`flow-${side}-${slot}-in`}
          type="target"
          id={`flow-${side}-${slot}-in`}
          position={position}
          className="handle-flow"
          style={style}
        />
      ))}
      {offsetFlowHandles.map(([side, slot, position, style]) => (
        <Handle
          key={`flow-${side}-${slot}-out`}
          type="source"
          id={`flow-${side}-${slot}-out`}
          position={position}
          className="handle-flow"
          style={style}
        />
      ))}
      <Handle type="target" id="bc-in" position={Position.Top} className="handle-flow" />
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
      <span className="flow-node__label">{shortLabel(data.identifier, 10)}</span>
      <NodeTooltip lines={tooltipLines} visible={hovered} />
    </div>
  );
}

export default memo(FlowNode);
