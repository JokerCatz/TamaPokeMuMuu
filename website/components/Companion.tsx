"use client";

// A small copy of the board's care rules (pet.cpp Pet::tick, feed, play, clean):
// one second here is one minute on the board.
import { useEffect, useReducer, useRef, useState } from "react";
import { asset } from "@/lib/asset";
import { STARTERS } from "@/lib/dict";
import { useSite, useT } from "@/lib/state";
import Sprite from "./Sprite";

type Pet = {
  food: number;
  joy: number;
  energy: number;
  hygiene: number;
  poops: number;
  sleeping: boolean;
  minutes: number;
  mistakes: number;
  cooldown: number;
};
type Act = { type: "tick" } | { type: "feed" } | { type: "candy" } | { type: "play" } | { type: "bath" } | { type: "sleep" } | { type: "reset" };

const START: Pet = { food: 70, joy: 70, energy: 80, hygiene: 90, poops: 0, sleeping: false, minutes: 0, mistakes: 0, cooldown: 0 };
const clamp = (v: number) => Math.max(0, Math.min(100, v));
const dropTo = (v: number, d: number, floor: number) => (v > floor ? Math.max(floor, v - d) : v);

function reduce(p: Pet, a: Act): Pet {
  switch (a.type) {
    case "tick": {
      const m = p.minutes + 1;
      let n: Pet;
      if (p.sleeping) {
        n = { ...p, minutes: m, energy: clamp(p.energy + 6) };
        if (m % 2 === 0) {
          n.food = dropTo(n.food, 1, 30);
          n.joy = dropTo(n.joy, 1, 35);
        }
        if (m % 3 === 0) n.hygiene = dropTo(n.hygiene, 1, 45);
        if (n.energy >= 100) n.sleeping = false; // the demo wakes up when rested
      } else {
        const poops = p.food > 40 && p.poops < 3 && Math.random() < 0.15 ? p.poops + 1 : p.poops;
        n = {
          ...p,
          minutes: m,
          poops,
          food: clamp(p.food - 2),
          energy: clamp(p.energy - 1),
          joy: clamp(p.joy - 1),
          hygiene: clamp(p.hygiene - 1 - 4 * poops),
        };
      }
      n.cooldown = Math.max(0, n.cooldown - 1);
      if (Math.min(n.food, n.joy, n.energy, n.hygiene) <= 10 && n.cooldown === 0) {
        n.mistakes++;
        n.cooldown = 60; // at most one an hour
      }
      return n;
    }
    case "feed":
      return p.sleeping ? p : { ...p, food: clamp(p.food + 25) };
    case "candy":
      return p.sleeping ? p : { ...p, food: clamp(p.food + 10), joy: clamp(p.joy + 12) };
    case "play":
      return p.sleeping ? p : { ...p, joy: clamp(p.joy + 25), energy: clamp(p.energy - 10), food: clamp(p.food - 5) };
    case "bath":
      return { ...p, poops: 0, hygiene: 100 };
    case "sleep":
      return { ...p, sleeping: !p.sleeping };
    case "reset":
      return START;
  }
}

const barColor = (v: number) => (v >= 50 ? "var(--color-ok)" : v >= 25 ? "var(--color-warn)" : "var(--color-bad)");

export default function Companion() {
  const t = useT();
  const { lang, starter, candy, setCandy } = useSite();
  const [pet, dispatch] = useReducer(reduce, START);
  const [anim, setAnim] = useState<string | null>(null);
  const [visible, setVisible] = useState(false);
  const root = useRef<HTMLElement>(null);
  const animTimer = useRef(0);

  // runs only while the section is on screen
  useEffect(() => {
    const el = root.current;
    if (!el) return;
    const io = new IntersectionObserver(([e]) => setVisible(e.isIntersecting), { threshold: 0.25 });
    io.observe(el);
    return () => io.disconnect();
  }, []);
  useEffect(() => {
    if (!visible) return;
    const id = window.setInterval(() => dispatch({ type: "tick" }), 1000);
    return () => window.clearInterval(id);
  }, [visible]);

  const show = (a: string, ms: number) => {
    setAnim(a);
    window.clearTimeout(animTimer.current);
    animTimer.current = window.setTimeout(() => setAnim(null), ms);
  };
  const act = (type: Act["type"]) => {
    if (pet.sleeping && type !== "sleep" && type !== "bath") return;
    if (type === "candy") {
      if (candy <= 0) return;
      setCandy((c) => c - 1);
    }
    dispatch({ type } as Act);
    if (type === "feed" || type === "candy") show("eat", 1800);
    if (type === "play") show("hop", 1600);
    if (type === "bath") show("nod", 1400);
  };

  const s = STARTERS.find((x) => x.id === starter)!;
  const level = 1 + Math.floor(pet.minutes / 60);
  const status = pet.sleeping
    ? t("live_sleeping")
    : pet.food < 25
      ? t("live_hungry")
      : pet.hygiene < 25
        ? t("live_dirty")
        : pet.energy < 25
          ? t("live_tired")
          : pet.joy < 25
            ? t("live_bored")
            : t("live_happy");
  const spriteAnim = pet.sleeping ? "sleep" : anim ?? "idle";
  const bars: [string, number][] = [
    [t("live_food"), pet.food],
    [t("live_joy"), pet.joy],
    [t("live_energy"), pet.energy],
    [t("live_hygiene"), pet.hygiene],
  ];
  const buttons: [Act["type"], string, string][] = [
    ["feed", "/img/icon_food.png", t("live_feed")],
    ["play", "/img/icon_play.png", t("live_play")],
    ["sleep", "/img/icon_light.png", pet.sleeping ? t("live_wake") : t("live_sleep")],
    ["bath", "/img/icon_clean.png", t("live_bath")],
  ];

  return (
    <section id="live" ref={root} className="py-24" style={{ background: "var(--color-cyellow)" }}>
      <div className="mx-auto grid max-w-6xl items-center gap-12 px-4 md:grid-cols-[1fr_auto]">
        <div data-reveal>
          <p className="kicker mb-3 text-ink2">{t("live_kicker")}</p>
          <h2 className="text-4xl font-extrabold tracking-tight md:text-5xl">{t("live_title")}</h2>
          <p className="mt-5 max-w-xl text-lg text-ink2">{t("live_lead")}</p>
          <div className="mt-8 flex flex-wrap items-center gap-4">
            <button type="button" className="btn" disabled={candy <= 0 || pet.sleeping} onClick={() => act("candy")}>
              {/* eslint-disable-next-line @next/next/no-img-element */}
              <img src={asset("/img/icon_candy.png")} alt="" width={28} height={28} className="pixelated" />
              {t("live_candy")} × {candy}
            </button>
            <span className="block-card px-4 py-2 font-semibold">
              {t("live_mistakes")}: <span className="font-pixel">{pet.mistakes}</span>
            </span>
          </div>
          <p className="mt-6 max-w-xl text-sm text-ink2">{t("live_note")}</p>
        </div>

        <div className="case aspect-square w-[min(88vw,420px)]" data-reveal>
          <div className="screen bg-cream">
            {/* scene */}
            <div className="absolute inset-x-0 top-0 h-[67%] overflow-hidden">
              {/* eslint-disable-next-line @next/next/no-img-element */}
              <img src={asset("/regions/kanto.jpg")} alt="" className="h-full w-full object-cover" />
              <div className="absolute inset-0 transition-colors duration-700" style={{ background: pet.sleeping ? "rgba(14,18,40,.62)" : "transparent" }} />
              <div className="absolute inset-x-0 top-[9%] text-center">
                <p className="font-pixel text-[clamp(14px,4.4vw,20px)] text-white" style={{ textShadow: "0 0 3px #2a2a36, 0 2px 0 #2a2a36" }}>
                  {s[lang].toUpperCase()} {t("live_level")}
                  {level}
                </p>
                <p className="mt-1 text-sm font-bold text-white" style={{ textShadow: "0 0 3px #2a2a36, 0 1px 0 #2a2a36" }}>
                  {status}
                </p>
              </div>
              <div className="absolute inset-x-0 bottom-[6%] flex items-end justify-center">
                <Sprite id={starter} anim={spriteAnim} scale={4} label={s[lang]} />
              </div>
              <div className="absolute bottom-[7%] right-[16%] flex gap-1">
                {Array.from({ length: pet.poops }, (_, i) => (
                  // eslint-disable-next-line @next/next/no-img-element
                  <img key={i} src={asset("/img/poop.png")} alt="" width={36} height={36} className="pixelated" />
                ))}
              </div>
            </div>
            {/* panel */}
            <div className="absolute inset-x-0 bottom-0 h-[33%] px-[16%] pt-[3%]">
              <div className="grid grid-cols-2 gap-x-4 gap-y-1">
                {bars.map(([label, v]) => (
                  <div key={label} className="flex items-center gap-2 text-[11px] font-semibold">
                    <span className="w-9 shrink-0">{label}</span>
                    <span className="h-2 flex-1 overflow-hidden rounded-full bg-[#d8d2bd]">
                      <span className="block h-full rounded-full transition-all duration-500" style={{ width: `${v}%`, background: barColor(v) }} />
                    </span>
                  </div>
                ))}
              </div>
              <div className="mt-2 flex justify-center gap-2">
                {buttons.map(([type, icon, label]) => (
                  <button
                    key={type}
                    type="button"
                    onClick={() => act(type)}
                    aria-label={label}
                    title={label}
                    disabled={pet.sleeping && type !== "sleep" && type !== "bath"}
                    className="flex h-11 w-11 items-center justify-center rounded-xl border-2 border-ink bg-paper shadow-[0_3px_0_var(--color-ink)] active:translate-y-0.5 disabled:opacity-40"
                  >
                    {/* eslint-disable-next-line @next/next/no-img-element */}
                    <img src={asset(icon)} alt="" width={28} height={28} className="pixelated" />
                  </button>
                ))}
              </div>
            </div>
          </div>
        </div>
      </div>
    </section>
  );
}
