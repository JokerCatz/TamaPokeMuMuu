"use client";

// A race from the board's tournament: the visitor's friend against two random rivals.
// Nothing to tap during the race, as on the board: lib/sport.ts works out the times
// with the firmware's rules, and the picture only plays them back.
import { useCallback, useEffect, useRef, useState } from "react";
import {
  EVENTS,
  MONS,
  RIVALS,
  TYPE_NAMES,
  fresh,
  formatMark,
  performance as dayPerformance,
  placeOf,
  rating,
  rival,
  typeBonus,
  type EventId,
  type Mon,
} from "@/lib/sport";
import { STARTERS } from "@/lib/dict";
import { useSite, useT } from "@/lib/state";
import Sprite from "./Sprite";

type Phase = "ready" | "racing" | "done";

const START = 16; // % across the screen: start and finish lines
const FINISH = 80;
const WINNER_SECONDS = 5.5; // how long the winner takes on screen
const LANE_BOTTOM = [51, 35, 19]; // % from the bottom of the screen: where each lane's runner stands
const MEDAL = ["#f2c84b", "#c9ced6", "#d99a6c"];

// speeds up and slows down a little along the way; still 0 at the start and 1 at the end
const wobble = (u: number, i: number) => u + 0.045 * (Math.sin(4 * Math.PI * u + i * 2.1) - Math.sin(i * 2.1));

function Venue({ id }: { id: EventId }) {
  const lanes = (color: string, line: string) => (
    <div className="absolute inset-x-0 top-[34%] h-[48%]" style={{ background: color }}>
      {[0, 1, 2, 3].map((i) => (
        <div key={i} className="absolute inset-x-0 h-[3px]" style={{ top: `${(i * 100) / 3}%`, background: line, translate: "0 -50%" }} />
      ))}
    </div>
  );
  if (id === "swim")
    return (
      <>
        <div className="absolute inset-0" style={{ background: "repeating-linear-gradient(90deg, #e8f6fb 0 22px, #d3ecf5 22px 24px)" }} />
        {lanes("#45aee6", "repeating-linear-gradient(90deg, #fff 0 10px, #e8503a 10px 20px)")}
        <div className="absolute inset-x-0 bottom-0 h-[18%] bg-[#cfe9f3]" />
      </>
    );
  if (id === "torch")
    return (
      <>
        <div className="absolute inset-0" style={{ background: "linear-gradient(#f39a63, #ffd9a0 38%)" }} />
        {[8, 20, 30, 44, 56, 68, 80].map((x, i) => (
          <div key={x} className="absolute bottom-[66%] bg-[#5b4a7a]" style={{ left: `${x}%`, width: "11%", height: `${[14, 22, 12, 18, 26, 15, 20][i]}%` }} />
        ))}
        {lanes("#73737f", "repeating-linear-gradient(90deg, #f4f1d0 0 14px, transparent 14px 28px)")}
        <div className="absolute inset-x-0 bottom-0 h-[18%] bg-[#5a5a66]" />
      </>
    );
  if (id === "trail")
    return (
      <>
        <div className="absolute inset-0" style={{ background: "linear-gradient(#bfe3c0, #8fca8a 38%)" }} />
        {[6, 22, 38, 54, 70, 86].map((x, i) => (
          <div key={x} className="absolute bottom-[66%] bg-[#3c8a4c]" style={{ left: `${x}%`, width: "16%", height: `${[20, 26, 18, 24, 22, 19][i]}%`, clipPath: "polygon(50% 0, 100% 100%, 0 100%)" }} />
        ))}
        {lanes("#b98d5d", "#9a7347")}
        {[
          [36, 41],
          [58, 57],
          [46, 73],
        ].map(([x, y]) => (
          <div key={x} className="absolute h-[3%] w-[7%] rounded-full border-2 border-[#5c4023] bg-[#8a6238]" style={{ left: `${x}%`, top: `${y}%` }} />
        ))}
        <div className="absolute inset-x-0 bottom-0 h-[18%] bg-[#6fb85c]" />
      </>
    );
  return (
    <>
      <div className="absolute inset-0" style={{ background: "linear-gradient(#8fd0ff, #dff2ff 30%)" }} />
      <div className="absolute inset-x-0 top-[20%] h-[14%]" style={{ background: "repeating-linear-gradient(90deg, #b9bfcc 0 16px, #a5acbb 16px 18px)" }} />
      {lanes("#d9654a", "#fff")}
      <div className="absolute inset-x-0 bottom-0 h-[18%] bg-[#6fb85c]" />
    </>
  );
}

export default function Tournament() {
  const t = useT();
  const { lang, starter, hatch } = useSite();
  const [evId, setEvId] = useState<EventId>("sprint");
  const [rivals, setRivals] = useState<[Mon, Mon] | null>(null);
  const [phase, setPhase] = useState<Phase>("ready");
  const [marks, setMarks] = useState<number[] | null>(null); // lane order: rival, friend, rival
  const [arrived, setArrived] = useState([false, false, false]);
  const racers = useRef<(HTMLDivElement | null)[]>([]);
  const clock = useRef<HTMLSpanElement>(null);
  const raf = useRef(0);

  const ev = EVENTS.find((e) => e.id === evId)!;
  const me = MONS[starter];
  const bonus = typeBonus(ev, me.type);

  const lineUp = useCallback(() => {
    cancelAnimationFrame(raf.current);
    const pool = [...RIVALS];
    const a = pool.splice(Math.floor(Math.random() * pool.length), 1)[0];
    const b = pool[Math.floor(Math.random() * pool.length)];
    setRivals([MONS[a], MONS[b]]);
    setPhase("ready");
    setMarks(null);
    setArrived([false, false, false]);
    for (const el of racers.current) if (el) el.style.left = `${START}%`;
    if (clock.current) clock.current.textContent = formatMark(ev, 0);
  }, [ev]);

  // two new rivals at the start, and whenever the event or the friend changes
  useEffect(lineUp, [lineUp, starter]);
  useEffect(() => () => cancelAnimationFrame(raf.current), []);

  const start = () => {
    if (!rivals || phase === "racing") return;
    const u = Math.random;
    const field = [rival(rivals[0], u(), u(), u(), u()), fresh(me), rival(rivals[1], u(), u(), u(), u())];
    const m = field.map((a) => ev.mark(dayPerformance(rating(a, ev), u())));
    setMarks(m);
    if (window.matchMedia("(prefers-reduced-motion: reduce)").matches) {
      for (const el of racers.current) if (el) el.style.left = `${FINISH}%`;
      setArrived([true, true, true]);
      setPhase("done");
      return;
    }
    setPhase("racing");
    const best = Math.min(...m);
    const slowest = Math.max(...m);
    // the winner takes WINNER_SECONDS; the gaps are shortened a little so nobody waits long
    const secs = m.map((x) => WINNER_SECONDS * Math.pow(x / best, 0.8));
    const here = [false, false, false];
    const t0 = window.performance.now();
    const tick = (now: number) => {
      const el = (now - t0) / 1000;
      let news = false;
      secs.forEach((s, i) => {
        const p = Math.min(1, el / s);
        const node = racers.current[i];
        if (node) node.style.left = `${START + (FINISH - START) * wobble(p, i)}%`;
        if (p >= 1 && !here[i]) {
          here[i] = true;
          news = true;
        }
      });
      if (clock.current) clock.current.textContent = formatMark(ev, Math.min(slowest, (el / WINNER_SECONDS) * best));
      if (news) setArrived([...here]);
      if (here.every(Boolean)) setPhase("done");
      else raf.current = requestAnimationFrame(tick);
    };
    raf.current = requestAnimationFrame(tick);
  };

  const lane = rivals ? [rivals[0], me, rivals[1]] : [];
  const place = marks ? placeOf(marks[1], marks[0], marks[2]) : 0;
  // podium order: by time, the friend first on a tie
  const order = marks ? [1, 0, 2].sort((a, b) => marks[a] - marks[b]) : [];
  const mark = (v: number) => (ev.minsec ? formatMark(ev, v) : t("tour_secs", { v: formatMark(ev, v) }));
  const pct = `${bonus > 1 ? "+" : "−"}${Math.round(Math.abs(bonus - 1) * 100)}%`;

  return (
    <section id="tournament" className="py-24" style={{ background: "var(--color-cpink)" }}>
      <div className="mx-auto grid max-w-6xl items-center gap-12 px-4 md:grid-cols-[1fr_auto]">
        <div data-reveal>
          <p className="kicker mb-3 text-ink2">{t("tour_kicker")}</p>
          <h2 className="text-balance text-4xl font-extrabold tracking-tight md:text-5xl">{t("tour_title")}</h2>
          <p className="mt-5 max-w-xl text-lg text-ink2">{t("tour_lead")}</p>

          <div className="mt-7 flex flex-wrap items-center gap-3">
            <span className="text-sm font-semibold">{t("tour_friend")}</span>
            {STARTERS.map((s) => (
              <button
                key={s.id}
                type="button"
                aria-label={s[lang]}
                aria-pressed={starter === s.id}
                title={s[lang]}
                disabled={phase === "racing"}
                onClick={() => hatch(s.id)}
                className={`flex h-16 w-16 items-end justify-center overflow-hidden rounded-2xl border-2 border-ink pb-1 ${starter === s.id ? "bg-paper shadow-[0_3px_0_var(--color-ink)]" : "bg-paper/40"}`}
              >
                <Sprite id={s.id} anim="idle" scale={1.5} />
              </button>
            ))}
          </div>

          <div className="mt-5 grid max-w-xl grid-cols-2 gap-3">
            {EVENTS.map((e) => {
              const b = typeBonus(e, me.type);
              return (
                <button
                  key={e.id}
                  type="button"
                  aria-pressed={evId === e.id}
                  disabled={phase === "racing"}
                  onClick={() => setEvId(e.id)}
                  className={`block-card relative px-4 py-3 text-left transition-transform ${evId === e.id ? "-translate-y-0.5" : "bg-paper/50 shadow-none"}`}
                >
                  <span className="block font-bold">{e[lang]}</span>
                  <span className="block text-sm text-ink2">{e.hint[lang]}</span>
                  {b !== 1 && (
                    <span className={`font-pixel absolute right-3 top-3 rounded-full px-2 py-0.5 text-xs text-white ${b > 1 ? "bg-ok" : "bg-bad"}`}>
                      {b > 1 ? "+" : "−"}
                      {Math.round(Math.abs(b - 1) * 100)}%
                    </span>
                  )}
                </button>
              );
            })}
          </div>

          <div className="mt-5 min-h-[7.5rem] max-w-xl" aria-live="polite">
            <p className="text-ink2">
              {bonus === 1 ? t("tour_neutral", { n: me[lang] }) : t("tour_bonus", { n: me[lang], t: TYPE_NAMES[me.type][lang], p: pct })}
            </p>
            {phase === "done" && marks && (
              <>
                <ol className="mt-3 flex flex-wrap gap-x-5 gap-y-1">
                  {order.map((i, k) => (
                    <li key={i} className={`flex items-center gap-2 ${i === 1 ? "font-bold" : ""}`}>
                      <span className="font-pixel flex h-6 w-6 items-center justify-center rounded-full border-2 border-ink text-xs" style={{ background: MEDAL[k] }}>
                        {k + 1}
                      </span>
                      {lane[i][lang]}
                      <span className="text-ink2">{mark(marks[i])}</span>
                    </li>
                  ))}
                </ol>
                <p className="mt-3 font-bold">{t(place === 1 ? "tour_gold" : place === 2 ? "tour_silver" : "tour_bronze")}</p>
              </>
            )}
          </div>
          <p className="mt-3 max-w-xl text-sm text-ink2">{t("tour_note")}</p>
        </div>

        <div className="flex flex-col items-center gap-5" data-reveal>
          <div className="case aspect-square w-[min(88vw,420px)]">
            <div className="screen">
              <Venue id={evId} />
              {/* the friend runs in the middle lane: a lighter lane with a label */}
              <div className="absolute inset-x-0 top-[50%] h-[16%] bg-white/15" />
              <span className="absolute left-[3%] top-[58%] z-[5] -translate-y-1/2 whitespace-nowrap rounded bg-ink px-1.5 text-[10px] font-bold leading-4 text-cream">{t("tour_you")}</span>
              {/* finish line */}
              <div className="absolute top-[34%] h-[48%] w-[10px]" style={{ left: `${FINISH + 5}%`, background: "repeating-conic-gradient(#fff 0 25%, #2a2a36 0 50%) 0 0 / 10px 10px" }} />
              <p className="absolute inset-x-0 top-[9%] text-center">
                {/* the text is written by lineUp and by the race itself */}
                <span ref={clock} className="font-pixel rounded-full border-2 border-ink bg-paper px-3 py-0.5 text-sm" />
              </p>
              {lane.map((m, i) => (
                <div
                  key={i}
                  ref={(el) => {
                    racers.current[i] = el;
                  }}
                  className="absolute -translate-x-1/2"
                  style={{ left: `${START}%`, bottom: `${LANE_BOTTOM[i]}%`, zIndex: i + 1 }}
                >
                  <div style={evId === "swim" ? { clipPath: "inset(0 0 34% 0)", translate: "0 30%" } : undefined}>
                    <Sprite id={m.id} anim={phase === "racing" && !arrived[i] ? "walk_r" : "idle"} scale={2} label={m[lang]} />
                  </div>
                </div>
              ))}
              {phase !== "done" && rivals && (
                <p className="absolute inset-x-0 bottom-[5%] text-center text-xs font-semibold">
                  <span className="rounded-full bg-paper/90 px-2.5 py-0.5">{t("tour_vs", { a: rivals[0][lang], b: rivals[1][lang] })}</span>
                </p>
              )}

              {phase === "done" && marks && (
                <div className="anim-pop absolute inset-0 z-10 flex flex-col items-center justify-end bg-paper pb-[17%]">
                  <p className="font-pixel absolute inset-x-0 top-[15%] text-center text-2xl">{t(place === 1 ? "tour_p1" : place === 2 ? "tour_p2" : "tour_p3")}</p>
                  <div className="flex items-end gap-1.5">
                    {[1, 0, 2].map((k) => {
                      const i = order[k];
                      return (
                        <div key={k} className="flex w-[84px] flex-col items-center">
                          {i === 1 && <span className="mb-0.5 rounded bg-ink px-1.5 text-[10px] font-bold leading-4 text-cream">{t("tour_you")}</span>}
                          <Sprite id={lane[i].id} anim="idle" scale={2} label={lane[i][lang]} />
                          <div className="font-pixel flex w-full items-start justify-center rounded-t-lg border-2 border-ink pt-1 text-lg" style={{ height: [70, 48, 32][k], background: MEDAL[k] }}>
                            {k + 1}
                          </div>
                        </div>
                      );
                    })}
                  </div>
                </div>
              )}
            </div>
          </div>
          <button type="button" className="btn btn-green" disabled={phase === "racing" || !rivals} onClick={phase === "done" ? lineUp : start}>
            {phase === "done" ? t("tour_again") : t("tour_start")}
          </button>
        </div>
      </div>
    </section>
  );
}
