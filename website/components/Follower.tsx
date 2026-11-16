"use client";

// The friend hatched at the top walks along in the corner while the page scrolls.
// It steps aside where it is already on screen: the egg scene, the care demo and the race.
import { useEffect, useState } from "react";
import { useSite } from "@/lib/state";
import Sprite from "./Sprite";

export default function Follower() {
  const { hatched, starter } = useSite();
  const [walking, setWalking] = useState(false);
  const [away, setAway] = useState(true);

  useEffect(() => {
    if (!hatched) return;
    let stop = 0;
    const onScroll = () => {
      setWalking(true);
      window.clearTimeout(stop);
      stop = window.setTimeout(() => setWalking(false), 180);
    };
    window.addEventListener("scroll", onScroll, { passive: true });
    const seen = new Set<Element>();
    const io = new IntersectionObserver(
      (entries) => {
        for (const e of entries) {
          if (e.isIntersecting) seen.add(e.target);
          else seen.delete(e.target);
        }
        setAway(seen.size > 0);
      },
      { threshold: 0.3 },
    );
    for (const id of ["top", "live", "tournament"]) {
      const el = document.getElementById(id);
      if (el) io.observe(el);
    }
    return () => {
      window.removeEventListener("scroll", onScroll);
      window.clearTimeout(stop);
      io.disconnect();
    };
  }, [hatched]);

  if (!hatched) return null;
  return (
    <div
      aria-hidden
      className={`pointer-events-none fixed bottom-2 left-2 z-20 flex h-[124px] w-[124px] origin-bottom-left scale-[.6] flex-col items-center justify-end transition-[translate,opacity] duration-500 md:bottom-4 md:left-4 md:scale-[.75] xl:scale-100 ${away ? "translate-y-[130%] opacity-0" : "opacity-100"}`}
    >
      <Sprite id={starter} anim={walking ? "walk" : "idle"} scale={3} />
      <div className="-mt-6 h-2 w-14 rounded-full bg-ink/20" />
    </div>
  );
}
