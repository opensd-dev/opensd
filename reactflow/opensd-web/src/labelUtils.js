export function shortLabel(identifier, maxLen = 12) {
  const text = String(identifier ?? "");
  if (text.length <= maxLen) return text;
  return `${text.slice(0, Math.max(1, maxLen - 3))}...`;
}
