"use client";

import { useEffect, useRef, useState } from "react";
import { asset } from "@/lib/asset";
import { REGIONS } from "@/lib/dict";
import { useSite, useT } from "@/lib/state";

export default function Regions() {
  const t = useT();
  const { lang } = useSite();
  const [open, setOpen] = useState<number | null>(null);
  const dlg = useRef<HTMLDialogElement>(null);

  useEffect(() => {
    const d = dlg.current;
    if (!d) return;
    if (open !== null && !d.open) d.showModal();
    if (open === null && d.open) d.close();
  }, [open]);

  const r = open !== null ? REGIONS[open] : null;

  return (
    <section id="regions" className="py-24" style={{ background: "var(--color-cblue)" }}>
      <div className="mx-auto max-w-6xl px-4">
        <div data-reveal>
          <p className="kicker mb-3 text-ink2">{t("regions_kicker")}</p>
          <h2 className="text-4xl font-extrabold tracking-tight md:text-5xl">{t("regions_title")}</h2>
          <p className="mt-5 max-w-2xl text-lg text-ink2">{t("regions_lead")}</p>
        </div>
        <ol className="mt-10 grid grid-cols-1 gap-4 sm:grid-cols-2 lg:grid-cols-3" data-reveal>
          {REGIONS.map((g, i) => (
            <li key={g.id}>
              <button
                type="button"
                onClick={() => setOpen(i)}
                className="block-card group block w-full overflow-hidden text-left transition-transform hover:-translate-y-1"
              >
                {/* eslint-disable-next-line @next/next/no-img-element */}
                <img src={asset(`/regions/${g.id}.jpg`)} alt="" className="aspect-[495/159] w-full object-cover" loading="lazy" />
                <div className="flex items-baseline justify-between gap-3 px-4 py-3">
                  <span className="text-lg font-bold">
                    <span className="font-pixel mr-2 text-sm text-hint">{String(i + 1).padStart(2, "0")}</span>
                    {g[lang]}
                  </span>
                  <span className="text-sm text-ink2">{t("regions_species", { n: g.b - g.a + 1 })}</span>
                </div>
              </button>
            </li>
          ))}
        </ol>
      </div>

      <dialog
        ref={dlg}
        onClose={() => setOpen(null)}
        onClick={(e) => e.target === dlg.current && setOpen(null)}
        className="m-auto w-[min(92vw,640px)] rounded-3xl border-2 border-ink bg-paper p-0 shadow-[0_6px_0_var(--color-ink)] backdrop:bg-ink/50"
      >
        {r && open !== null && (
          <div>
            {/* eslint-disable-next-line @next/next/no-img-element */}
            <img src={asset(`/regions/${r.id}.jpg`)} alt="" className="aspect-[495/159] w-full object-cover" />
            <div className="p-6">
              <h3 className="text-3xl font-extrabold">{r[lang]}</h3>
              <ul className="mt-4 space-y-2 text-ink2">
                <li>
                  {t("regions_range", { a: r.a, b: r.b })} · {t("regions_species", { n: r.b - r.a + 1 })}
                </li>
                <li>{t("regions_gyms")}</li>
                <li>{open === 0 ? t("regions_first") : t("regions_unlock", { r: REGIONS[open - 1][lang] })}</li>
              </ul>
              <button type="button" className="btn btn-sm mt-6" onClick={() => setOpen(null)}>
                {t("regions_close")}
              </button>
            </div>
          </div>
        )}
      </dialog>
    </section>
  );
}
