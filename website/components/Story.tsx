"use client";

// The scroll story, "a day with TamaPoke": the chapters scroll past while the 3D board
// stays in view (CSS sticky, the page scroll is never taken over). The board turns to
// each chapter's pose and shows its screenshots, and the sky behind it goes from dawn
// to night.
import dynamic from "next/dynamic";
import { useCallback, useEffect, useRef, useState } from "react";
import { asset } from "@/lib/asset";
import { SHOT_CAPS, STORY } from "@/lib/dict";
import { useSite, useT } from "@/lib/state";
import type { Pose } from "./Board3D";

const Board3D = dynamic(() => import("./Board3D"), { ssr: false });

const N = STORY.length;
const TURN = Math.PI * 2;
// where the board looks in each chapter (radians): first the whole ball from the side,
// then one full turn and the screen to the reader
const POSES: Pose[] = [
  { x: -0.2, y: 0.85 },
  { x: -0.26, y: TURN },
  { x: -0.24, y: TURN - 0.3 },
  { x: -0.24, y: TURN + 0.3 },
  { x: -0.26, y: TURN - 0.22 },
  { x: -0.4, y: TURN + 0.12 },
];

const rgb = (c: string) => [1, 3, 5].map((i) => parseInt(c.slice(i, i + 2), 16));
const mix = (a: string, b: string, t: number) => {
  const A = rgb(a);
  const B = rgb(b);
  return `rgb(${A.map((v, i) => Math.round(v + (B[i] - v) * t)).join(",")})`;
};
const clamp01 = (v: number) => Math.max(0, Math.min(1, v));

export default function Story() {
  const t = useT();
  const { lang } = useSite();
  const [active, setActive] = useState(0);
  const [shotIdx, setShotIdx] = useState(0);
  const [near, setNear] = useState(false); // close enough to load the 3D code
  const [visible, setVisible] = useState(false);
  const [ready, setReady] = useState(false); // the 3D model is on screen
  const onReady = useCallback(() => setReady(true), []);

  const root = useRef<HTMLElement>(null);
  const sticky = useRef<HTMLDivElement>(null);
  const chapters = useRef<HTMLElement[]>([]);
  const pos = useRef(0);
  const sky = useRef<HTMLDivElement>(null);
  const ground = useRef<HTMLDivElement>(null);
  const sun = useRef<HTMLDivElement>(null);
  const night = useRef<HTMLDivElement>(null);
  const clouds = useRef<HTMLDivElement>(null);

  useEffect(() => {
    const el = root.current;
    if (!el) return;
    const io = new IntersectionObserver(
      ([e]) => {
        if (e.isIntersecting) setNear(true);
      },
      { rootMargin: "900px 0px" },
    );
    const io2 = new IntersectionObserver(([e]) => setVisible(e.isIntersecting));
    io.observe(el);
    io2.observe(el);
    return () => {
      io.disconnect();
      io2.disconnect();
    };
  }, []);

  // the chapter position from the scroll, and the sky that goes with it
  useEffect(() => {
    let raf = 0;
    const update = () => {
      raf = 0;
      const els = chapters.current;
      const st = sticky.current;
      if (!st || els.length < N) return;
      // the line a chapter is read at: mid screen, or mid of what the board leaves free on a phone
      const oneColumn = window.matchMedia("(max-width: 767px)").matches;
      const focus = oneColumn ? (st.getBoundingClientRect().bottom + window.innerHeight) / 2 : window.innerHeight / 2;
      const c = els.map((e) => {
        const r = e.getBoundingClientRect();
        return r.top + r.height / 2;
      });
      let p = 0;
      if (focus >= c[N - 1]) p = N - 1;
      else
        for (let i = 0; i < N - 1; i++)
          if (focus >= c[i] && focus < c[i + 1]) {
            p = i + (focus - c[i]) / (c[i + 1] - c[i]);
            break;
          }
      pos.current = p;
      const idx = Math.round(p);
      setActive((a) => (a === idx ? a : idx));

      const i = Math.min(N - 2, Math.floor(p));
      const f = p - i;
      const a = STORY[i].sky;
      const b = STORY[i + 1].sky;
      if (sky.current) sky.current.style.background = `linear-gradient(${mix(a[0], b[0], f)}, ${mix(a[1], b[1], f)})`;
      if (ground.current) ground.current.style.background = mix(a[2], b[2], f);
      // the sun rises on the left, is highest at midday and sets with the last chapter of the day
      const s = p / (N - 2);
      const th = Math.PI * (1 - s);
      if (sun.current) {
        sun.current.style.left = `${50 + 40 * Math.cos(th)}%`;
        sun.current.style.top = `${72 - 54 * Math.sin(th)}%`;
      }
      const dark = clamp01(p - (N - 2));
      if (night.current) night.current.style.opacity = String(dark);
      if (clouds.current) clouds.current.style.opacity = String(1 - dark);
    };
    const onScroll = () => {
      if (!raf) raf = requestAnimationFrame(update);
    };
    update();
    window.addEventListener("scroll", onScroll, { passive: true });
    window.addEventListener("resize", onScroll);
    return () => {
      window.removeEventListener("scroll", onScroll);
      window.removeEventListener("resize", onScroll);
      cancelAnimationFrame(raf);
    };
  }, []);

  // a new chapter starts on its first picture; fetch this chapter's and the next one's ahead
  useEffect(() => {
    setShotIdx(0);
    for (const c of [STORY[active], STORY[Math.min(N - 1, active + 1)]])
      for (const s of c.shots) new Image().src = asset(`/shots/${lang}/${s}.png`);
  }, [active, lang]);

  const chapter = STORY[active];
  const shot = chapter.shots[shotIdx % chapter.shots.length];
  const nextShot = () => setShotIdx((n) => (n + 1) % chapter.shots.length);

  return (
    <section id="meet" ref={root}>
      <div className="mx-auto max-w-6xl px-4 md:grid md:grid-cols-2 md:gap-12">
        <div
          ref={sticky}
          className="sticky top-[68px] z-10 -mx-4 flex flex-col items-center bg-cream px-4 pb-3 pt-4 md:order-2 md:mx-0 md:h-[calc(100svh-68px)] md:justify-center md:self-start md:bg-transparent md:p-0"
        >
          <div className="relative aspect-square w-[min(62vw,34svh)] md:w-[min(100%,66svh,500px)]">
            {/* the sky and the ground behind the board */}
            <div ref={sky} className="absolute inset-0 overflow-hidden rounded-full border-2 border-ink shadow-[0_6px_0_var(--color-ink)]" style={{ background: `linear-gradient(${STORY[0].sky[0]}, ${STORY[0].sky[1]})` }}>
              <div ref={night} className="absolute inset-0 opacity-0" style={{ background: "radial-gradient(2px 2px at 18% 22%, #fff 50%, transparent 51%), radial-gradient(2px 2px at 72% 14%, #fff 50%, transparent 51%), radial-gradient(1.5px 1.5px at 44% 30%, #fff 50%, transparent 51%), radial-gradient(1.5px 1.5px at 86% 38%, #fff 50%, transparent 51%), radial-gradient(2px 2px at 30% 48%, #fff 50%, transparent 51%), radial-gradient(1.5px 1.5px at 60% 8%, #fff 50%, transparent 51%)" }}>
                <div className="absolute left-[68%] top-[12%] h-[11%] w-[11%] overflow-hidden rounded-full bg-[#f4f1d0]">
                  <div className="absolute -left-[28%] -top-[12%] h-full w-full rounded-full bg-[#1b2038]" />
                </div>
              </div>
              <div ref={clouds} className="absolute inset-0">
                <div className="absolute top-[16%] h-[5%] w-[20%] rounded-full bg-white/80" style={{ animation: "drift 30s linear infinite" }} />
                <div className="absolute top-[30%] h-[4%] w-[14%] rounded-full bg-white/70" style={{ animation: "drift 40s linear infinite", animationDelay: "-17s" }} />
              </div>
              <div ref={sun} className="absolute h-[12%] w-[12%] -translate-x-1/2 -translate-y-1/2 rounded-full border-2 border-ink bg-[#ffd65a]" style={{ left: "10%", top: "72%" }} />
              <div ref={ground} className="absolute inset-x-0 bottom-0 h-[24%] border-t-2 border-ink/25" style={{ background: STORY[0].sky[2] }} />
            </div>
            {/* until the 3D model has loaded (or where there is no WebGL): the picture in a flat case */}
            {!ready && (
              <button type="button" onClick={nextShot} aria-label={t("story_tap")} className="case absolute inset-[20%]">
                <span className="screen block">
                  {/* eslint-disable-next-line @next/next/no-img-element */}
                  <img src={asset(`/shots/${lang}/${shot}.png`)} alt="" className="h-full w-full" />
                </span>
              </button>
            )}
            <div className="absolute inset-0">
              {near && <Board3D shot={asset(`/shots/${lang}/${shot}.png`)} pos={pos} poses={POSES} onTap={nextShot} onReady={onReady} active={visible} />}
            </div>
          </div>
          <div className="mt-4 flex items-center gap-3 text-sm">
            <span className="font-semibold">{SHOT_CAPS[shot][lang]}</span>
            <span className="flex gap-1.5" aria-hidden>
              {chapter.shots.map((s, i) => (
                <span key={s} className={`h-2 w-2 rounded-full border-2 border-ink ${i === shotIdx % chapter.shots.length ? "bg-ink" : ""}`} />
              ))}
            </span>
            <button type="button" className="text-hint underline decoration-dotted underline-offset-4" onClick={nextShot}>
              {t("story_tap")}
            </button>
          </div>
          <p className="mt-3 hidden max-w-sm text-center text-xs text-hint md:block">{t("meet_model")}</p>
        </div>

        <div className="md:order-1">
          {STORY.map((c, i) => (
            <article
              key={c.id}
              ref={(el) => {
                if (el) chapters.current[i] = el;
              }}
              className={`flex min-h-[60svh] flex-col justify-center py-8 transition-opacity duration-500 md:min-h-[86svh] ${i === active ? "opacity-100" : "opacity-30"}`}
            >
              {i === 0 ? (
                <p className="kicker mb-3 text-hint">{t("story_kicker")}</p>
              ) : (
                <p className="font-pixel mb-4 self-start rounded-full border-2 border-ink bg-paper px-3 py-1 text-sm shadow-[0_3px_0_var(--color-ink)]">{c.time}</p>
              )}
              {i === 0 ? (
                <h2 className="text-balance text-4xl font-extrabold tracking-tight md:text-5xl">{c[lang].title}</h2>
              ) : (
                <h3 className="text-balance text-3xl font-extrabold tracking-tight md:text-5xl">{c[lang].title}</h3>
              )}
              <p className="mt-4 max-w-lg text-lg text-ink2">{c[lang].text}</p>
              {i === 0 && <p className="mt-4 text-sm text-hint">{t("meet_specs")}</p>}
            </article>
          ))}
        </div>
      </div>
    </section>
  );
}
