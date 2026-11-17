"use client";

// A button that plays the clip of the real board where the visitor is, in a dialog,
// instead of sending them down to the Build section. The video is only fetched when
// the dialog opens, so the top of the page stays light.
import { useRef, useState, type ReactNode } from "react";
import { asset } from "@/lib/asset";
import { useT } from "@/lib/state";

export default function WatchClip({ className, children }: { className: string; children: ReactNode }) {
  const t = useT();
  const dialog = useRef<HTMLDialogElement>(null);
  const [open, setOpen] = useState(false);
  return (
    <>
      <button
        type="button"
        className={className}
        onClick={() => {
          setOpen(true);
          dialog.current?.showModal();
        }}
      >
        <svg width="12" height="14" viewBox="0 0 12 14" aria-hidden className="shrink-0">
          <path d="M0 0l12 7-12 7z" fill="currentColor" />
        </svg>
        {children}
      </button>
      <dialog
        ref={dialog}
        onClose={() => setOpen(false)}
        // a click on the dark surround (the dialog itself, not what is in it) closes it
        onClick={(e) => e.target === dialog.current && dialog.current.close()}
        className="m-auto w-[min(92vw,560px)] overflow-visible bg-transparent p-0 backdrop:bg-ink/75"
      >
        {open && (
          <video
            src={asset("/video/board.mp4")}
            poster={asset("/video/board.webp")}
            width={720}
            height={720}
            autoPlay
            muted
            loop
            playsInline
            controls
            aria-label={t("build_clip_alt")}
            className="block-card aspect-square h-auto w-full overflow-hidden bg-ink object-cover"
          />
        )}
        <form method="dialog" className="mt-4 flex justify-center">
          <button className="btn btn-sm">{t("clip_close")}</button>
        </form>
      </dialog>
    </>
  );
}
