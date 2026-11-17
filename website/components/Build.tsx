"use client";

import { useEffect, useRef } from "react";
import { asset } from "@/lib/asset";
import { useT } from "@/lib/state";

const MAKERWORLD = "https://makerworld.com/es/models/2937822-tamapoke-a-pokemon-pokeball-tamagotchi";
const WAVESHARE = "https://www.waveshare.com/wiki/ESP32-S3-Touch-AMOLED-1.75";
const SD_ZIP = "https://github.com/Mu-Muu/TamaPoke/releases/latest/download/tamapoke-sd.zip";
const GITHUB = "https://github.com/Mu-Muu/TamaPoke";

// The clip of the real board. Nothing of it is fetched until it comes on screen
// (the poster aside); it plays only while visible.
function Clip({ label }: { label: string }) {
  const ref = useRef<HTMLVideoElement>(null);
  useEffect(() => {
    const v = ref.current;
    if (!v || window.matchMedia("(prefers-reduced-motion: reduce)").matches) return;
    const io = new IntersectionObserver(
      ([e]) => {
        if (e.isIntersecting) v.play().catch(() => {});
        else v.pause();
      },
      { threshold: 0.4 },
    );
    io.observe(v);
    return () => io.disconnect();
  }, []);
  return (
    <video
      ref={ref}
      src={asset("/video/board.mp4")}
      poster={asset("/video/board.webp")}
      width={720}
      height={720}
      muted
      loop
      playsInline
      controls
      preload="none"
      aria-label={label}
      className="block-card aspect-square h-auto w-full overflow-hidden bg-ink object-cover"
    />
  );
}

export default function Build() {
  const t = useT();
  const steps: [string, string, string?, string?, boolean?][] = [
    [t("build_s1"), t("build_s1_txt"), t("build_s1_btn"), asset("/web/"), true],
    [t("build_s2"), t("build_s2_txt"), t("build_s2_btn"), SD_ZIP],
    [t("build_s3"), t("build_s3_txt")],
  ];
  return (
    <section id="build" className="py-24">
      <div className="mx-auto max-w-6xl px-4">
        <div data-reveal>
          <p className="kicker mb-3 text-hint">{t("build_kicker")}</p>
          <h2 className="text-4xl font-extrabold tracking-tight md:text-5xl">{t("build_title")}</h2>
        </div>
        <figure className="mt-10 grid items-center gap-6 md:grid-cols-[minmax(0,400px)_1fr] md:gap-10" data-reveal>
          <Clip label={t("build_clip_alt")} />
          <figcaption className="max-w-xl text-lg text-ink2">{t("build_clip")}</figcaption>
        </figure>
        <div className="mt-10 grid items-start gap-6 md:grid-cols-2">
          <div className="block-card p-6" data-reveal>
            <h3 className="text-xl font-bold">{t("build_parts")}</h3>
            <ul className="mt-4 list-disc space-y-3 pl-5 text-ink2">
              <li>
                <a href={WAVESHARE} className="font-semibold text-ink underline">
                  {t("build_board")}
                </a>
              </li>
              <li>{t("build_sd")}</li>
              <li>{t("build_battery")}</li>
              <li>
                <a href={MAKERWORLD} className="font-semibold text-ink underline">
                  {t("build_case")}
                </a>
              </li>
            </ul>
            <a href={GITHUB} className="btn btn-sm mt-6">
              {t("build_github")}
            </a>
          </div>
          <ol className="grid gap-4" data-reveal>
            {steps.map(([title, txt, btn, href, primary], i) => (
              <li key={title} className="block-card relative p-6 pl-20">
                <span className="font-pixel absolute left-6 top-6 flex h-9 w-9 items-center justify-center rounded-full bg-ink text-cream">{i + 1}</span>
                <h3 className="text-lg font-bold">{title}</h3>
                <p className="mt-2 text-ink2">{txt}</p>
                {btn && href && (
                  <a href={href} className={`btn btn-sm mt-4 ${primary ? "btn-green" : ""}`}>
                    {btn}
                  </a>
                )}
              </li>
            ))}
          </ol>
        </div>
      </div>
    </section>
  );
}
