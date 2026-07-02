function pathBetween(start, end) {
  return `M ${start.x} ${start.y} L ${end.x} ${end.y}`;
}

function pointAt(start, end, ratio) {
  return {
    x: start.x + (end.x - start.x) * ratio,
    y: start.y + (end.y - start.y) * ratio
  };
}

export default function PartialDashEdge({
  sourceX,
  sourceY,
  targetX,
  targetY,
  markerEnd,
  style,
  data
}) {
  const start = { x: sourceX, y: sourceY };
  const end = { x: targetX, y: targetY };

  return (
    <>
      <path className="react-flow__edge-path" d={pathBetween(start, end)} markerEnd={markerEnd} style={style} />
      {(data?.dashRanges ?? []).map((range, index) => {
        const dashStart = pointAt(start, end, range.start);
        const dashEnd = pointAt(start, end, range.end);
        return (
          <path
            key={`${range.start}-${range.end}-${index}`}
            className="react-flow__edge-path partial-dash-edge__segment"
            d={pathBetween(dashStart, dashEnd)}
            style={style}
          />
        );
      })}
    </>
  );
}
