"use client";

// A PMD SpriteCollab animation strip (front-facing row), played with the
// frame durations from AnimData.xml (in 1/60 s game frames).
import { useEffect, useState } from "react";
import { asset } from "@/lib/asset";

type Anim = { w: number; h: number; durations: number[]; foot?: number }; // foot: empty rows under the feet
type Meta = Record<string, Anim>;

const metaCache: Record<string, Promise<Meta>> = {};
function loadMeta(id: string) {
  metaCache[id] ??= fetch(asset(`/sprites/${id}/meta.json`)).then((r) => r.json() as Promise<Meta>);
  return metaCache[id];
}

export default function Sprite({
  id,
  anim = "idle",
  scale = 4,
  label,
  grounded = false,
  className = "",
}: {
  id: string;
  anim?: string;
  scale?: number;
  label?: string;
  grounded?: boolean; // feet on the bottom edge of the box, whatever the frame's padding
  className?: string;
}) {
  const [meta, setMeta] = useState<Meta | null>(null);
  const [frame, setFrame] = useState(0);

  useEffect(() => {
    let alive = true;
    loadMeta(id).then((m) => alive && setMeta(m));
    return () => {
      alive = false;
    };
  }, [id]);

  const name = meta && meta[anim] ? anim : "idle";
  const a = meta?.[name];

  useEffect(() => {
    if (!a) return;
    let i = 0;
    setFrame(0);
    let t = window.setTimeout(function tick() {
      i = (i + 1) % a.durations.length;
      setFrame(i);
      t = window.setTimeout(tick, (a.durations[i] * 1000) / 60);
    }, (a.durations[0] * 1000) / 60);
    return () => window.clearTimeout(t);
  }, [a]);

  if (!a) return <div className={className} style={{ width: 32 * scale, height: 40 * scale }} />;
  return (
    <div
      role="img"
      aria-label={label}
      className={`pixelated ${className}`}
      style={{
        width: a.w * scale,
        height: a.h * scale,
        backgroundImage: `url(${asset(`/sprites/${id}/${name}.png`)})`,
        backgroundSize: `${a.w * a.durations.length * scale}px ${a.h * scale}px`,
        backgroundPosition: `${-frame * a.w * scale}px 0`,
        backgroundRepeat: "no-repeat",
        marginBottom: grounded ? -(a.foot ?? 0) * scale : undefined,
      }}
    />
  );
}
