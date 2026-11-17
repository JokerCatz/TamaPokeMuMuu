"use client";

// The nine regions as a road. The visitor's friend walks from one stop to the next and a
// region opens when it gets there: on the board they open one after another too.
import { useEffect, useRef, useState } from "react";
import { asset } from "@/lib/asset";
import { REGIONS } from "@/lib/dict";
import { useSite, useT } from "@/lib/state";
import Sprite from "./Sprite";

const WALK_MS = 1300;
const LAST = REGIONS.length - 1;
const SPOTS = [10, 27, 87]; // % across a stop: where its three Pokémon stand (the middle is the friend's)

function Padlock() {
  return (
    <svg width="44" height="52" viewBox="0 0 11 13" shapeRendering="crispEdges" aria-hidden>
      <rect x="3" y="0" width="5" height="1" fill="#2a2a36" />
      <rect x="2" y="1" width="1" height="5" fill="#2a2a36" />
      <rect x="8" y="1" width="1" height="5" fill="#2a2a36" />
      <rect x="1" y="6" width="9" height="7" fill="#2a2a36" />
      <rect x="2" y="7" width="7" height="5" fill="#f2c84b" />
      <rect x="5" y="8" width="1" height="3" fill="#2a2a36" />
    </svg>
  );
}

export default function Journey() {
  const t = useT();
  const { lang, starter } = useSite();
  const [at, setAt] = useState(0);
  const [open, setOpen] = useState(0); // the last region opened so far
  const [walking, setWalking] = useState<0 | 1 | -1>(0); // 1 forwards, -1 back
  const [fresh, setFresh] = useState(-1); // the region that has just opened
  const timer = useRef(0);
  const swipeX = useRef<number | null>(null);

  useEffect(() => () => window.clearTimeout(timer.current), []);

  // walks to a stop: any region already open, or the next one (which opens on arrival)
  const go = (to: number) => {
    if (walking || to < 0 || to > LAST || to > open + 1 || to === at) return;
    const ms = window.matchMedia("(prefers-reduced-motion: reduce)").matches ? 0 : WALK_MS;
    setWalking(to > at ? 1 : -1);
    setAt(to);
    timer.current = window.setTimeout(() => {
      setWalking(0);
      if (to <= open) return;
      setOpen(to);
      setFresh(to);
      timer.current = window.setTimeout(() => setFresh(-1), 1200);
    }, ms);
  };

  const cur = REGIONS[at];
  const next = at < LAST ? REGIONS[at + 1] : null;
  const slide = { transitionDuration: `${WALK_MS}ms`, transitionTimingFunction: "cubic-bezier(.4, 0, .3, 1)" };

  return (
    <section id="regions" className="py-24" style={{ background: "var(--color-cblue)" }}>
      <div className="mx-auto max-w-6xl px-4" data-reveal>
        <p className="kicker mb-3 text-ink2">{t("regions_kicker")}</p>
        <h2 className="text-balance text-4xl font-extrabold tracking-tight md:text-5xl">{t("regions_title")}</h2>
        <p className="mt-5 max-w-2xl text-lg text-ink2">{t("regions_lead")}</p>
      </div>

      {/* the road: the stops slide past while the friend walks in the middle */}
      <div
        className="relative mt-10 touch-pan-y overflow-hidden [--g:28px] [--w:min(76vw,440px)]"
        onPointerDown={(e) => (swipeX.current = e.clientX)}
        onPointerUp={(e) => {
          const dx = swipeX.current === null ? 0 : e.clientX - swipeX.current;
          swipeX.current = null;
          if (Math.abs(dx) > 40) go(at + (dx < 0 ? 1 : -1));
        }}
        onPointerCancel={() => (swipeX.current = null)}
      >
        <div
          className="absolute inset-x-0 bottom-0 h-[34px] border-t-4 border-[#6fb85c] transition-[background-position]"
          style={{
            ...slide,
            background: "repeating-linear-gradient(90deg, #b98d5d 0 40px, #ab8053 40px 52px)",
            backgroundPositionX: `calc((var(--w) + var(--g)) * ${-at})`,
          }}
        />
        <ol
          className="relative ml-[50%] flex w-max gap-[var(--g)] pt-2 transition-transform"
          style={{ ...slide, transform: `translateX(calc((var(--w) + var(--g)) * ${-at} - var(--w) / 2))` }}
        >
          {REGIONS.map((r, i) => {
            const locked = i > open;
            return (
              <li key={r.id} className="w-[var(--w)] shrink-0" aria-current={i === at}>
                <div className={`block-card relative overflow-hidden transition-[filter,opacity] duration-700 ${locked ? "opacity-60 grayscale" : ""}`}>
                  {/* eslint-disable-next-line @next/next/no-img-element */}
                  <img src={asset(`/regions/${r.id}.jpg`)} alt="" className="aspect-[495/159] w-full object-cover" draggable={false} loading="lazy" />
                  <div className="flex items-baseline justify-between gap-3 px-4 py-2.5">
                    <span className="text-lg font-bold">
                      <span className="font-pixel mr-2 text-sm text-hint">{String(i + 1).padStart(2, "0")}</span>
                      {r[lang]}
                    </span>
                    <span className="text-sm text-ink2">{t("regions_species", { n: r.b - r.a + 1 })}</span>
                  </div>
                  {(locked || fresh === i) && (
                    <span className={`absolute left-1/2 top-[36%] -translate-x-1/2 -translate-y-1/2 ${fresh === i ? "anim-crack" : ""}`}>
                      <Padlock />
                    </span>
                  )}
                </div>
                {/* who lives here, standing on the road */}
                <div className="relative h-[96px]">
                  {!locked &&
                    r.mons.map((m, k) => (
                      <div
                        key={m}
                        className={`absolute bottom-[14px] -translate-x-1/2 ${fresh === i ? "anim-pop" : ""}`}
                        style={{ left: `${SPOTS[k]}%`, animationDelay: `${0.15 + k * 0.15}s`, animationFillMode: "backwards" }}
                      >
                        <Sprite id={m} anim="idle" scale={2} grounded />
                      </div>
                    ))}
                </div>
              </li>
            );
          })}
        </ol>
        <div className="absolute bottom-[14px] left-1/2 z-10 -translate-x-1/2" style={{ scale: walking < 0 ? "-1 1" : undefined }}>
          <Sprite id={starter} anim={walking ? "walk_r" : "idle"} scale={2} grounded />
        </div>
      </div>

      <div className="mx-auto max-w-6xl px-4">
        <ol className="mx-auto mt-8 flex max-w-xl items-center">
          {REGIONS.map((r, i) => (
            <li key={r.id} className={`flex items-center ${i < LAST ? "flex-1" : ""}`}>
              <button
                type="button"
                onClick={() => go(i)}
                disabled={i > open || !!walking}
                aria-label={r[lang]}
                aria-current={i === at}
                className={`font-pixel h-8 w-8 shrink-0 rounded-full border-2 border-ink text-xs ${i === at ? "bg-ink text-cream" : i <= open ? "bg-paper" : "bg-paper/40 text-ink/40"}`}
              >
                {i + 1}
              </button>
              {i < LAST && <span className={`h-[3px] flex-1 ${i < open ? "bg-ink" : "bg-ink/25"}`} />}
            </li>
          ))}
        </ol>

        <div className="mt-6 flex flex-wrap items-center justify-center gap-3">
          <button type="button" className="btn btn-sm" disabled={at === 0 || !!walking} onClick={() => go(at - 1)}>
            {t("regions_back")}
          </button>
          {next ? (
            <button type="button" className="btn btn-green" disabled={!!walking} onClick={() => go(at + 1)}>
              {t("regions_go", { r: next[lang] })}
            </button>
          ) : (
            <span className="font-bold">{t("regions_done")}</span>
          )}
        </div>

        <div className="mx-auto mt-5 min-h-[4.5rem] max-w-2xl text-center" aria-live="polite">
          <p className="text-ink2">
            <b className="text-ink">{cur[lang]}</b> · {t("regions_range", { a: cur.a, b: cur.b })} · {t("regions_species", { n: cur.b - cur.a + 1 })} · {t("regions_gyms")}
          </p>
          {next && at === open && <p className="mt-1 text-sm text-ink2">{t("regions_rule", { cur: cur[lang], next: next[lang] })}</p>}
        </div>
      </div>
    </section>
  );
}
