// A stepped pixel edge between two sections, instead of a hard cut from one colour to the next.
import { useId } from "react";

export default function PixelEdge({ from, to }: { from: string; to: string }) {
  const id = `edge${useId().replace(/[^a-zA-Z0-9]/g, "")}`;
  return (
    <div aria-hidden style={{ background: from }}>
      <svg width="100%" height="24" className="block" shapeRendering="crispEdges">
        <defs>
          <pattern id={id} width="96" height="24" patternUnits="userSpaceOnUse" style={{ fill: to }}>
            <rect x="0" y="16" width="96" height="8" />
            <rect x="0" y="8" width="24" height="8" />
            <rect x="48" y="8" width="32" height="8" />
            <rect x="8" y="0" width="8" height="8" />
            <rect x="56" y="0" width="16" height="8" />
          </pattern>
        </defs>
        <rect width="100%" height="24" fill={`url(#${id})`} />
      </svg>
    </div>
  );
}
