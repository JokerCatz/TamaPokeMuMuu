"use client";

import { useT } from "@/lib/state";

export default function Footer() {
  const t = useT();
  return (
    <footer className="bg-ink py-12 text-sm text-[#cfd0d8]">
      <div className="mx-auto max-w-6xl space-y-3 px-4">
        <p>{t("foot_fan")}</p>
        <p>{t("foot_credits")}</p>
        <p>
          <a className="text-white underline" href="https://github.com/Mu-Muu/TamaPoke">
            github.com/Mu-Muu/TamaPoke
          </a>
          {" · "}
          <a className="text-white underline" href="https://github.com/socquique/TamaPoke">
            socquique/TamaPoke
          </a>
          {" · "}
          <a className="text-white underline" href="https://github.com/PMDCollab/SpriteCollab">
            PMD SpriteCollab
          </a>
        </p>
      </div>
    </footer>
  );
}
