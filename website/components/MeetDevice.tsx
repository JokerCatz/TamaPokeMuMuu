"use client";

import dynamic from "next/dynamic";
import { useState } from "react";
import { asset } from "@/lib/asset";
import { DEVICE_SHOTS } from "@/lib/dict";
import { useSite, useT } from "@/lib/state";

const DeviceCanvas = dynamic(() => import("./DeviceCanvas"), {
  ssr: false,
  loading: () => <div className="aspect-square w-full animate-pulse rounded-full bg-paper" />,
});

const CAP: Record<string, { en: string; zh: string }> = {
  care_feed: { en: "Feeding", zh: "喂食" },
  card: { en: "Pet card", zh: "宠物卡片" },
  tournament_race: { en: "Tournament: torch run", zh: "锦标赛：火炬长跑" },
  gyms_detail: { en: "A gym", zh: "一个道馆" },
  fun_orchard: { en: "Fun: orchard", zh: "趣玩：果园" },
  dex_cover: { en: "Pokédex: Kanto", zh: "图鉴：关都" },
  friends_play: { en: "Playing with a friend", zh: "和好友一起玩" },
  scene_christmas: { en: "Christmas", zh: "圣诞节" },
  standby: { en: "Standby clock", zh: "待机时钟" },
};

export default function MeetDevice() {
  const t = useT();
  const { lang } = useSite();
  const [i, setI] = useState(0);
  const name = DEVICE_SHOTS[i];
  const next = () => setI((n) => (n + 1) % DEVICE_SHOTS.length);
  return (
    <section id="meet" className="border-t-2 border-dashed border-ink/20 py-24">
      <div className="mx-auto grid max-w-6xl items-center gap-12 px-4 md:grid-cols-2">
        <div data-reveal>
          <p className="kicker mb-3 text-hint">{t("meet_kicker")}</p>
          <h2 className="text-4xl font-extrabold tracking-tight md:text-5xl">{t("meet_title")}</h2>
          <p className="mt-5 max-w-lg text-lg text-ink2">{t("meet_lead")}</p>
          <p className="mt-4 text-sm text-hint">{t("meet_specs")}</p>
        </div>
        <div className="flex flex-col items-center gap-4" data-reveal>
          <div className="aspect-square w-full max-w-[480px] touch-pan-y">
            <DeviceCanvas url={asset(`/shots/${lang}/${name}.png`)} onTap={next} />
          </div>
          <div className="flex items-center gap-3">
            <button type="button" className="btn btn-sm" onClick={next}>
              {t("meet_tap")} ›
            </button>
            <span className="font-semibold">{CAP[name][lang]}</span>
          </div>
          <p className="max-w-sm text-center text-xs text-hint">{t("meet_model")}</p>
        </div>
      </div>
    </section>
  );
}
