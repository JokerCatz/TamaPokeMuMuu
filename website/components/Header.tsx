"use client";

import { asset } from "@/lib/asset";
import { useSite, useT } from "@/lib/state";

export default function Header() {
  const { lang, setLang } = useSite();
  const t = useT();
  const links: [string, Parameters<typeof t>[0]][] = [
    ["#meet", "nav_meet"],
    ["#chess", "nav_chess"],
    ["#regions", "nav_regions"],
    ["#live", "nav_play"],
    ["#build", "nav_build"],
  ];
  return (
    <header className="sticky top-0 z-30 border-b-2 border-ink bg-cream/90 backdrop-blur">
      <div className="mx-auto flex max-w-6xl items-center gap-3 px-4 py-3 md:gap-6">
        <a href="#top" className="flex items-center gap-2 font-pixel text-lg">
          {/* eslint-disable-next-line @next/next/no-img-element */}
          <img src={asset("/img/egg.png")} alt="" width={26} height={26} className="pixelated" />
          TamaPoke
          <span className="hidden text-sm lowercase text-ink2 sm:inline">with Chess</span>
        </a>
        <nav className="ml-auto hidden gap-5 text-sm font-semibold md:flex">
          {links.map(([href, k]) => (
            <a key={href} href={href} className="hover:underline">
              {t(k)}
            </a>
          ))}
        </nav>
        <a href={asset("/web/")} className="btn btn-green btn-sm ml-auto shrink-0 whitespace-nowrap md:ml-0">
          {t("nav_install")}
        </a>
        <div className="flex shrink-0 overflow-hidden whitespace-nowrap rounded-full border-2 border-ink text-xs font-semibold" role="group" aria-label="Language">
          {(["en", "zh"] as const).map((l) => (
            <button
              key={l}
              type="button"
              aria-pressed={lang === l}
              onClick={() => setLang(l)}
              className={`px-3 py-1.5 ${lang === l ? "bg-ink text-cream" : ""}`}
            >
              {l === "en" ? "EN" : "中文"}
            </button>
          ))}
        </div>
      </div>
    </header>
  );
}
