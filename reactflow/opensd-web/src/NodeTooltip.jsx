export function shortLabel(identifier, maxLen = 12) {
  const text = String(identifier ?? "");
  if (text.length <= maxLen) return text;
  return `${text.slice(0, maxLen - 1)}…`;
}

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
