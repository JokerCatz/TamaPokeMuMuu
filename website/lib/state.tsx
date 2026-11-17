"use client";

// Site-wide state: the language, the friend hatched at the top of the page and
// the candy bought in the chess section (fed to the friend in the care demo).
import { createContext, useCallback, useContext, useEffect, useMemo, useState, type ReactNode } from "react";
import { dict, type Key, type Lang, type StarterId } from "./dict";

type State = {
  lang: Lang;
  setLang: (l: Lang) => void;
  starter: StarterId;
  hatched: boolean;
  hatch: (s: StarterId) => void;
  candy: number;
  setCandy: (n: number | ((c: number) => number)) => void;
};

const Ctx = createContext<State | null>(null);

// English, unless the address says ?lang=zh or the visitor picked 中文 before
// (the browser's own language is not looked at).
function detectLang(): Lang {
  const q = /[?&]lang=(en|zh)\b/.exec(window.location.search);
  if (q) return q[1] as Lang;
  try {
    const saved = window.localStorage.getItem("tp-lang");
    if (saved === "en" || saved === "zh") return saved;
  } catch {
    /* storage blocked */
  }
  return "en";
}

export function SiteProvider({ children }: { children: ReactNode }) {
  const [lang, setLangState] = useState<Lang>("en");
  const [starter, setStarter] = useState<StarterId>("bulbasaur");
  const [hatched, setHatched] = useState(false);
  const [candy, setCandy] = useState(0);

  useEffect(() => {
    setLangState(detectLang());
  }, []);

  useEffect(() => {
    document.documentElement.lang = lang === "zh" ? "zh-CN" : "en";
  }, [lang]);

  const setLang = useCallback((l: Lang) => {
    setLangState(l);
    try {
      window.localStorage.setItem("tp-lang", l);
    } catch {
      /* ignore */
    }
  }, []);

  const hatch = useCallback((s: StarterId) => {
    setStarter(s);
    setHatched(true);
  }, []);

  const value = useMemo(
    () => ({ lang, setLang, starter, hatched, hatch, candy, setCandy }),
    [lang, setLang, starter, hatched, hatch, candy],
  );
  return <Ctx.Provider value={value}>{children}</Ctx.Provider>;
}

export function useSite() {
  const s = useContext(Ctx);
  if (!s) throw new Error("useSite outside SiteProvider");
  return s;
}

export function useT() {
  const { lang } = useSite();
  return useCallback(
    (k: Key, vars?: Record<string, string | number>) => {
      let s = dict[lang][k] ?? dict.en[k];
      if (vars) for (const [n, v] of Object.entries(vars)) s = s.replace(`{${n}}`, String(v));
      return s;
    },
    [lang],
  );
}
