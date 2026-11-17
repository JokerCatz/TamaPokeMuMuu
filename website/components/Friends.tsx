"use client";

import { asset } from "@/lib/asset";
import { useSite, useT } from "@/lib/state";

export default function Friends() {
  const t = useT();
  const { lang } = useSite();
  const shots: [string, string][] = [
    ["friends_play", t("friends_play")],
    ["friends_card", t("friends_card")],
  ];
  return (
    <section
      className="py-24 text-nightink"
      style={{
        background:
          "radial-gradient(2px 2px at 12% 18%, #fff 50%, transparent 51%), radial-gradient(2px 2px at 78% 12%, #fff 50%, transparent 51%), radial-gradient(1.5px 1.5px at 42% 8%, #fff 50%, transparent 51%), radial-gradient(1.5px 1.5px at 90% 40%, #fff 50%, transparent 51%), radial-gradient(2px 2px at 6% 70%, #fff 50%, transparent 51%), linear-gradient(var(--color-night), var(--color-night2))",
      }}
    >
      <div className="mx-auto max-w-6xl px-4">
        <div data-reveal>
          <p className="kicker mb-3 text-[#9aa3c7]">{t("friends_kicker")}</p>
          <h2 className="text-4xl font-extrabold tracking-tight text-white md:text-5xl">{t("friends_title")}</h2>
          <p className="mt-5 max-w-2xl text-lg text-[#b8bfdc]">{t("friends_lead")}</p>
        </div>
        <div className="mt-12 flex flex-col items-center justify-center gap-6 md:flex-row" data-reveal>
          {shots.map(([name, cap], i) => (
            <div key={name} className="contents">
              {i === 1 && (
                <div className="relative h-16 w-16 shrink-0 rotate-90 md:rotate-0" aria-hidden>
                  {[0, 0.8, 1.6].map((d) => (
                    <span key={d} className="absolute inset-0 rounded-full border-[3px] border-[#ff8fa3] opacity-0" style={{ animation: `wave 2.4s ${d}s infinite` }} />
                  ))}
                </div>
              )}
              <figure className="text-center">
                <div className="case aspect-square w-[min(76vw,300px)]">
                  <div className="screen">
                    {/* eslint-disable-next-line @next/next/no-img-element */}
                    <img src={asset(`/shots/${lang}/${name}.webp`)} alt={cap} className="h-full w-full" loading="lazy" />
                  </div>
                </div>
                <figcaption className="mt-4 font-bold">{cap}</figcaption>
              </figure>
            </div>
          ))}
        </div>
      </div>
    </section>
  );
}
