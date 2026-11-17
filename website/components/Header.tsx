"use client";

import { useEffect, useState } from "react";
import { asset } from "@/lib/asset";
import type { Key } from "@/lib/dict";
import { useSite, useT } from "@/lib/state";

// the sections, in page order
const LINKS: [string, Key][] = [
  ["#meet", "nav_meet"],
  ["#chess", "nav_chess"],
  ["#live", "nav_play"],
  ["#tournament", "nav_sports"],
  ["#regions", "nav_regions"],
  ["#build", "nav_build"],
];

// `page`: the sections live on the home page, so from the guide their links go back there
export default function Header({ page = "home" }: { page?: "home" | "guide" }) {
  const { lang, setLang } = useSite();
  const t = useT();
  // a language that came with ?lang= is not remembered: carry it to the other page
  const q = lang === "zh" ? "?lang=zh" : "";
  const to = (hash: string) => (page === "home" ? hash : asset("/") + q + hash);
  const guide = asset("/guide/") + q;
  const [menu, setMenu] = useState(false); // the sections menu of narrow screens

  useEffect(() => {
    if (!menu) return;
    const onKey = (e: KeyboardEvent) => {
      if (e.key === "Escape") setMenu(false);
    };
    window.addEventListener("keydown", onKey);
    return () => window.removeEventListener("keydown", onKey);
  }, [menu]);

  return (
    <header className="sticky top-0 z-30 border-b-2 border-ink bg-cream/90 backdrop-blur">
      <div className="mx-auto flex max-w-6xl items-center gap-3 px-4 py-3 lg:gap-6">
        <a href={to("#top")} className="flex items-center gap-2 font-pixel text-lg" onClick={() => setMenu(false)}>
          {/* eslint-disable-next-line @next/next/no-img-element */}
          <img src={asset("/img/egg.png")} alt="" width={26} height={26} className="pixelated" />
          TamaPoke
          <span className="hidden text-sm lowercase text-ink2 sm:inline">with Chess</span>
        </a>
        <nav className="ml-auto hidden gap-5 text-sm font-semibold lg:flex">
          {LINKS.map(([href, k]) => (
            <a key={href} href={to(href)} className="hover:underline">
              {t(k)}
            </a>
          ))}
          <a href={guide} aria-current={page === "guide" ? "page" : undefined} className="underline-offset-4 hover:underline aria-[current]:underline">
            {t("nav_guide")}
          </a>
        </nav>
        <a href={asset("/web/")} className="btn btn-green btn-sm ml-auto hidden shrink-0 whitespace-nowrap sm:inline-flex lg:ml-0">
          {t("nav_install")}
        </a>
        <div className="ml-auto flex shrink-0 overflow-hidden whitespace-nowrap rounded-full border-2 border-ink text-xs font-semibold sm:ml-0" role="group" aria-label="Language">
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
        <button
          type="button"
          aria-label={t("nav_menu")}
          aria-expanded={menu}
          aria-controls="sections-menu"
          onClick={() => setMenu((m) => !m)}
          className="flex h-9 w-9 shrink-0 flex-col items-center justify-center gap-[5px] rounded-full border-2 border-ink lg:hidden"
        >
          <span className={`h-[2px] w-4 bg-ink transition-transform ${menu ? "translate-y-[7px] rotate-45" : ""}`} />
          <span className={`h-[2px] w-4 bg-ink ${menu ? "opacity-0" : ""}`} />
          <span className={`h-[2px] w-4 bg-ink transition-transform ${menu ? "-translate-y-[7px] -rotate-45" : ""}`} />
        </button>
      </div>
      {menu && (
        <nav id="sections-menu" className="absolute inset-x-0 top-full grid grid-cols-2 gap-x-4 border-b-2 border-ink bg-cream px-4 py-2 lg:hidden">
          {LINKS.map(([href, k]) => (
            <a key={href} href={to(href)} onClick={() => setMenu(false)} className="border-b border-ink/10 py-3 text-base font-semibold">
              {t(k)}
            </a>
          ))}
          <a href={guide} onClick={() => setMenu(false)} className="col-span-2 border-b border-ink/10 py-3 text-base font-semibold">
            {t("nav_guide_long")}
          </a>
          <a href={asset("/web/")} className="btn btn-green btn-sm col-span-2 my-3 sm:hidden">
            {t("nav_install_long")}
          </a>
        </nav>
      )}
    </header>
  );
}
