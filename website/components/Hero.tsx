"use client";

import { useRef, useState } from "react";
import { useGSAP } from "@gsap/react";
import gsap from "gsap";
import { asset } from "@/lib/asset";
import { STARTERS, type StarterId } from "@/lib/dict";
import { useSite, useT } from "@/lib/state";
import Sprite from "./Sprite";

gsap.registerPlugin(useGSAP);

function HatchEgg() {
  const t = useT();
  const { lang, hatch } = useSite();
  const [taps, setTaps] = useState(0);
  const [shake, setShake] = useState(0);
  const [baby, setBaby] = useState<StarterId | null>(null);
  const [cracking, setCracking] = useState(false);

  const tap = () => {
    if (baby || cracking) return;
    const n = taps + 1;
    setTaps(n);
    if (n < 3) {
      setShake((s) => s + 1);
      return;
    }
    setCracking(true);
    window.setTimeout(() => {
      const pick = STARTERS[Math.floor(Math.random() * STARTERS.length)];
      setBaby(pick.id);
      setCracking(false);
      hatch(pick.id);
    }, 480);
  };

  const again = () => {
    setTaps(0);
    setBaby(null);
  };

  const s = baby ? STARTERS.find((x) => x.id === baby)! : null;
  const hint = taps === 0 ? t("hatch_tap") : taps === 1 ? t("hatch_moved") : t("hatch_almost");

  return (
    <div className="flex w-full flex-col items-center gap-5">
      <div className="case aspect-square w-full max-w-[400px]">
        <div className={`screen ${cracking ? "anim-flash" : ""}`} style={{ background: "linear-gradient(#a8d8ff 0 58%, #8fd07a 58% 63%, #6fb85c 63% 100%)" }}>
          {/* clouds */}
          <div className="pointer-events-none absolute left-0 top-[14%] h-6 w-20 rounded-full bg-white/80" style={{ animation: "drift 26s linear infinite" }} />
          <div className="pointer-events-none absolute left-0 top-[26%] h-4 w-14 rounded-full bg-white/70" style={{ animation: "drift 34s linear infinite", animationDelay: "-12s" }} />
          {!baby && (
            <button
              type="button"
              onClick={tap}
              aria-label={t("hatch_tap")}
              className="absolute left-1/2 top-[30%] w-[34%] -translate-x-1/2 cursor-pointer focus-visible:outline-dashed focus-visible:outline-4 focus-visible:outline-white"
            >
              {/* eslint-disable-next-line @next/next/no-img-element */}
              <img
                key={shake}
                src={asset("/img/egg.png")}
                alt=""
                className={`pixelated w-full ${cracking ? "anim-crack" : shake ? "anim-shake" : "anim-float"}`}
              />
            </button>
          )}
          {s && (
            <>
              <p className="absolute left-1/2 top-[13%] -translate-x-1/2 whitespace-nowrap rounded-xl border-2 border-ink bg-white px-3 py-1 text-sm font-bold anim-pop">
                {t("hatch_hi", { n: s[lang] })}
              </p>
              <div className="absolute inset-x-0 top-[30%] flex h-[36%] items-end justify-center anim-pop">
                <Sprite id={s.id} anim="idle" scale={5} label={s[lang]} />
              </div>
            </>
          )}
          {!baby && (
            <p className="absolute inset-x-0 bottom-[13%] text-center text-sm font-bold text-white drop-shadow-[0_2px_0_rgba(0,0,0,.35)]">{hint}</p>
          )}
        </div>
      </div>
      {baby ? (
        <div className="flex flex-col items-center gap-2">
          <button type="button" className="btn btn-sm" onClick={again}>
            {t("hatch_again")}
          </button>
          <p className="text-sm text-hint">{t("hatch_note")}</p>
        </div>
      ) : (
        <div className="h-[62px]" />
      )}
    </div>
  );
}

export default function Hero() {
  const t = useT();
  const root = useRef<HTMLElement>(null);
  useGSAP(
    () => {
      if (window.matchMedia("(prefers-reduced-motion: reduce)").matches) return;
      gsap.from(".hero-line", { y: 40, opacity: 0, duration: 0.8, stagger: 0.12, ease: "power3.out" });
    },
    { scope: root },
  );
  return (
    <section
      id="top"
      ref={root}
      className="relative overflow-hidden"
      style={{ background: "radial-gradient(1100px 420px at 85% -10%, #cfe6ff 0, transparent 70%), var(--color-cream)" }}
    >
      <div className="mx-auto grid max-w-6xl items-center gap-12 px-4 pb-20 pt-14 md:grid-cols-[1.1fr_.9fr]">
        <div>
          <p className="kicker hero-line mb-4 text-hint">{t("hero_kicker")}</p>
          <h1 className="text-[clamp(2.6rem,6.2vw,5rem)] font-extrabold leading-[1.05] tracking-tight [word-break:keep-all]">
            <span className="hero-line block">{t("hero_title_1")}</span>
            <span className="hero-line block text-case">{t("hero_title_2")}</span>
          </h1>
          <p className="hero-line mt-6 max-w-xl text-lg text-ink2 md:text-xl">{t("hero_lead")}</p>
          <div className="hero-line mt-8 flex flex-wrap gap-3">
            <a href="#meet" className="btn btn-green">
              {t("hero_meet")}
            </a>
            <a href="#chess" className="btn">
              {t("hero_how")}
            </a>
          </div>
          <p className="hero-line mt-5 text-sm text-hint">{t("hero_facts")}</p>
        </div>
        <HatchEgg />
      </div>
    </section>
  );
}
