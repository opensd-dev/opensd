export default function NodeTooltip({ lines, visible }) {
  if (!visible || !lines?.length) return null;

  return (
    <div className="node-tooltip" role="tooltip">
      {lines.map((line) => (
        <div key={line}>{line}</div>
      ))}
    </div>
  );
}
