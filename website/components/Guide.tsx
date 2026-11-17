"use client";

// The player's guide: one long page, with a chapter list that follows the scroll
// (a side column on wide screens, a row of chips under the header on narrow ones).
import { useEffect, useRef, useState, type ReactNode } from "react";
import { asset } from "@/lib/asset";
import { GUIDE, GUIDE_QUICK, GUIDE_UI, type Block, type L } from "@/lib/guide";
import { useSite } from "@/lib/state";
import type { Lang } from "@/lib/dict";

const LABEL = {
  tip: { en: "Tip", zh: "小提示" },
  warn: { en: "Careful", zh: "注意" },
};

// **bold** is the only markup inside a text
function rich(s: string): ReactNode[] {
  return s.split("**").map((part, i) => (i % 2 ? <strong key={i} className="font-bold text-ink">{part}</strong> : part));
}

const num = (i: number) => String(i + 1).padStart(2, "0");

// a board screenshot in its red case
function Shot({ name, lang, className }: { name: string; lang: Lang; className: string }) {
  return (
    <div className={`case aspect-square ${className}`}>
      <div className="screen">
        {/* eslint-disable-next-line @next/next/no-img-element */}
        <img src={asset(`/shots/${lang}/${name}.webp`)} alt="" width={466} height={466} loading="lazy" className="h-full w-full" />
      </div>
    </div>
  );
}

// the four swipes of the main screen, each pointing at the page it opens
function Swipes({ lang }: { lang: Lang }) {
  const side = (name: string, label: L, arrow: string) => (
    <figure className="flex flex-col items-center gap-1.5 text-center">
      <span className="font-pixel text-lg leading-none" aria-hidden>
        {arrow}
      </span>
      <Shot name={name} lang={lang} className="w-[84px] sm:w-[120px]" />
      <figcaption className="text-xs font-semibold leading-tight sm:text-sm">{label[lang]}</figcaption>
    </figure>
  );
  return (
    <div className="block-card mt-6 grid grid-cols-3 items-center justify-items-center gap-x-2 gap-y-4 p-4 sm:gap-x-6 sm:p-6">
      <span />
      {side("card", GUIDE_UI.swipeUp, "↑")}
      <span />
      {side("activities", GUIDE_UI.swipeLeft, "←")}
      <figure className="flex flex-col items-center gap-2 text-center">
        <Shot name="care_pat" lang={lang} className="w-[104px] sm:w-[176px]" />
        <figcaption className="kicker text-xs sm:text-sm">{GUIDE_UI.swipeMain[lang]}</figcaption>
      </figure>
      {side("dex_cover", GUIDE_UI.swipeRight, "→")}
      <span />
      {side("settings", GUIDE_UI.swipeDown, "↓")}
      <span />
    </div>
  );
}

function BlockView({ b, lang }: { b: Block; lang: Lang }) {
  switch (b.k) {
    case "p":
      return <p className="mt-4 leading-relaxed text-ink2">{rich(b.t[lang])}</p>;
    case "h":
      return <h3 className="mt-9 text-xl font-extrabold tracking-tight">{b.t[lang]}</h3>;
    case "list":
      return (
        <ul className="mt-4 space-y-2.5">
          {b.items.map((it, i) => (
            <li key={i} className="flex gap-3 leading-relaxed text-ink2">
              <span className="mt-[0.6em] h-2 w-2 shrink-0 bg-ink" aria-hidden />
              <span>{rich(it[lang])}</span>
            </li>
          ))}
        </ul>
      );
    case "steps":
      return (
        <ol className="mt-5 space-y-3">
          {b.items.map((it, i) => (
            <li key={i} className="block-card flex gap-4 p-4">
              <span className="font-pixel flex h-8 w-8 shrink-0 items-center justify-center rounded-full bg-ink text-sm text-cream">{i + 1}</span>
              <span className="min-w-0 break-words pt-0.5 leading-relaxed text-ink2">{rich(it[lang])}</span>
            </li>
          ))}
        </ol>
      );
    case "table":
      return (
        <div className="block-card mt-5 overflow-x-auto">
          <table className={`w-full border-collapse text-left text-[0.95rem] ${b.tight ? "" : b.head.length > 3 ? "min-w-[640px]" : b.head.length > 2 ? "min-w-[480px]" : ""}`}>
            <thead>
              <tr className="bg-ink text-cream">
                {b.head.map((h, i) => (
                  <th key={i} className="px-4 py-2.5 font-bold">
                    {h[lang]}
                  </th>
                ))}
              </tr>
            </thead>
            <tbody>
              {b.rows.map((r, i) => (
                <tr key={i} className="border-t border-ink/15 align-top">
                  {r.map((c, j) => (
                    <td key={j} className={`px-4 py-2.5 leading-snug ${j ? "text-ink2" : "font-semibold"} ${!j && r.length === 2 ? "w-[42%]" : ""}`}>
                      {rich(c[lang])}
                    </td>
                  ))}
                </tr>
              ))}
            </tbody>
          </table>
        </div>
      );
    case "tip":
    case "warn":
      return (
        <p className={`mt-5 rounded-2xl border-2 border-ink px-4 py-3 leading-relaxed ${b.k === "tip" ? "bg-cgreen" : "bg-cyellow"}`}>
          <span className="kicker mr-2">{LABEL[b.k][lang]}</span>
          {rich(b.t[lang])}
        </p>
      );
    case "shots":
      return (
        <div className="mt-7 flex flex-wrap gap-x-5 gap-y-6 sm:gap-x-7">
          {b.items.map((s) => (
            <figure key={s.name} className="w-[132px] sm:w-[150px]">
              <Shot name={s.name} lang={lang} className="w-full" />
              <figcaption className="mt-2.5 text-center text-sm text-ink2">{s.cap[lang]}</figcaption>
            </figure>
          ))}
        </div>
      );
    case "swipes":
      return <Swipes lang={lang} />;
    case "links":
      return (
        <div className="mt-5 flex flex-wrap gap-3">
          {b.items.map((l, i) => (
            <a key={l.href} href={l.local ? asset(l.href) : l.href} className={`btn btn-sm ${i ? "" : "btn-green"}`}>
              {l.t[lang]}
            </a>
          ))}
        </div>
      );
    case "qa":
      return (
        <div className="mt-5 space-y-3">
          {b.items.map((x, i) => (
            <details key={i} className="block-card group px-4 py-3">
              <summary className="flex cursor-pointer list-none items-start justify-between gap-3 font-bold [&::-webkit-details-marker]:hidden">
                {x.q[lang]}
                <span className="font-pixel shrink-0 transition-transform group-open:rotate-45" aria-hidden>
                  +
                </span>
              </summary>
              <p className="mt-2 leading-relaxed text-ink2">{rich(x.a[lang])}</p>
            </details>
          ))}
        </div>
      );
  }
}

export default function Guide() {
  const { lang } = useSite();
  const [active, setActive] = useState(0);
  const chips = useRef<HTMLElement>(null);

  // the chapter being read: the last one whose title has passed the top of the window
  useEffect(() => {
    const find = () => {
      let cur = 0;
      GUIDE.forEach((c, i) => {
        const el = document.getElementById(c.id);
        if (el && el.getBoundingClientRect().top <= 150) cur = i;
      });
      setActive(cur);
    };
    find();
    window.addEventListener("scroll", find, { passive: true });
    return () => window.removeEventListener("scroll", find);
  }, []);

  // narrow screens: keep the current chip in sight
  useEffect(() => {
    const bar = chips.current;
    const chip = bar?.children[active] as HTMLElement | undefined;
    if (bar && chip) bar.scrollTo({ left: chip.offsetLeft - (bar.clientWidth - chip.clientWidth) / 2 });
  }, [active]);

  const q = lang === "zh" ? "?lang=zh" : "";

  return (
    <div id="top">
      <section className="mx-auto max-w-6xl px-4 pb-10 pt-12 md:pt-16">
        <p className="kicker mb-3 text-ink2">{GUIDE_UI.kicker[lang]}</p>
        <h1 className="text-balance text-4xl font-extrabold tracking-tight md:text-6xl">{GUIDE_UI.title[lang]}</h1>
        <p className="mt-5 max-w-2xl text-lg text-ink2">{GUIDE_UI.lead[lang]}</p>

        <h2 className="kicker mb-3 mt-10 text-ink2">{GUIDE_UI.quick[lang]}</h2>
        <ol className="grid gap-4 sm:grid-cols-2 lg:grid-cols-4">
          {GUIDE_QUICK.map((s, i) => (
            <li key={s.to}>
              <a href={`#${s.to}`} className="block-card block h-full p-4 transition-transform hover:-translate-y-0.5">
                <span className="font-pixel text-sm text-hint">{num(i)}</span>
                <span className="mt-1 block text-lg font-extrabold">{s.t[lang]}</span>
                <span className="mt-1 block text-sm leading-snug text-ink2">{s.d[lang]}</span>
              </a>
            </li>
          ))}
        </ol>
      </section>

      <nav ref={chips} aria-label={GUIDE_UI.toc[lang]} className="sticky top-[60px] z-20 flex gap-2 overflow-x-auto border-y-2 border-ink bg-cream/95 px-4 py-2 backdrop-blur [scrollbar-width:none] lg:hidden [&::-webkit-scrollbar]:hidden">
        {GUIDE.map((c, i) => (
          <a
            key={c.id}
            href={`#${c.id}`}
            aria-current={i === active}
            className={`shrink-0 whitespace-nowrap rounded-full border-2 border-ink px-3 py-1 text-sm font-semibold ${i === active ? "bg-ink text-cream" : "bg-paper"}`}
          >
            <span className="font-pixel mr-1.5 text-xs opacity-70">{num(i)}</span>
            {c.title[lang]}
          </a>
        ))}
      </nav>

      <div className="mx-auto max-w-6xl px-4 pb-24 lg:grid lg:grid-cols-[230px_minmax(0,1fr)] lg:gap-12">
        <nav aria-label={GUIDE_UI.toc[lang]} className="sticky top-[84px] hidden max-h-[calc(100vh-108px)] self-start overflow-y-auto pb-4 lg:block">
          <p className="kicker mb-3 text-ink2">{GUIDE_UI.toc[lang]}</p>
          <ol className="space-y-0.5">
            {GUIDE.map((c, i) => (
              <li key={c.id}>
                <a
                  href={`#${c.id}`}
                  aria-current={i === active}
                  className={`flex gap-2.5 rounded-xl px-3 py-1.5 text-sm font-semibold leading-snug ${i === active ? "bg-ink text-cream" : "hover:bg-ink/10"}`}
                >
                  <span className="font-pixel text-xs opacity-70">{num(i)}</span>
                  {c.title[lang]}
                </a>
              </li>
            ))}
          </ol>
          <a href={asset("/") + q} className="mt-5 block px-3 text-sm font-semibold underline">
            {GUIDE_UI.home[lang]}
          </a>
        </nav>

        <div className="min-w-0 max-w-3xl">
          {GUIDE.map((c, i) => (
            <section key={c.id} id={c.id} className="scroll-mt-12 border-t-2 border-ink/15 pt-10 first:border-t-0 lg:scroll-mt-2 lg:first:pt-2 [&:not(:first-child)]:mt-14">
              <p className="font-pixel text-sm text-hint">{num(i)}</p>
              <h2 className="mt-1 text-balance text-3xl font-extrabold tracking-tight md:text-4xl">{c.title[lang]}</h2>
              <p className="mt-3 text-lg text-ink2">{c.lead[lang]}</p>
              {c.blocks.map((b, j) => (
                <BlockView key={j} b={b} lang={lang} />
              ))}
            </section>
          ))}
          <p className="mt-16 flex flex-wrap gap-3">
            <a href="#top" className="btn btn-sm">
              {GUIDE_UI.top[lang]}
            </a>
            <a href={asset("/") + q} className="btn btn-sm">
              {GUIDE_UI.home[lang]}
            </a>
          </p>
        </div>
      </div>
    </div>
  );
}
