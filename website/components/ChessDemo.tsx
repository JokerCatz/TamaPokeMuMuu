"use client";

// A one-move chess puzzle: checkmate earns the same 10 points a won game gives
// on the board, and the points buy a candy for the friend in the care demo.
import { useMemo, useRef, useState } from "react";
import { Chess, type Square } from "chess.js";
import gsap from "gsap";
import { asset } from "@/lib/asset";
import { useSite, useT } from "@/lib/state";

const PUZZLES = [
  "6k1/5ppp/8/8/8/8/5PPP/3R2K1 w - - 0 1", // back-rank mate
  "r1bqkb1r/pppp1ppp/2n2n2/4p2Q/2B1P3/8/PPPP1PPP/RNB1K1NR w KQkq - 4 4", // Qxf7#
  "k7/8/1K6/8/8/8/8/7Q w - - 0 1", // Qh8#
];

const GLYPH: Record<string, string> = { k: "♚", q: "♛", r: "♜", b: "♝", n: "♞", p: "♟" };
const FILES = "abcdefgh";

export default function ChessDemo() {
  const t = useT();
  const { setCandy } = useSite();
  const [pi, setPi] = useState(0);
  const [fen, setFen] = useState(PUZZLES[0]);
  const [sel, setSel] = useState<Square | null>(null);
  const [msg, setMsg] = useState<"" | "wrong" | "win" | "bought">("");
  const [points, setPoints] = useState(0);
  const coin = useRef<HTMLDivElement>(null);

  const game = useMemo(() => new Chess(fen), [fen]);
  const board = game.board();
  const targets = useMemo(
    () => (sel ? new Set(game.moves({ square: sel, verbose: true }).map((m) => m.to)) : new Set<string>()),
    [game, sel],
  );

  const click = (sq: Square) => {
    if (msg === "win" || msg === "bought") return;
    const piece = game.get(sq);
    if (sel && targets.has(sq)) {
      const g = new Chess(fen);
      g.move({ from: sel, to: sq, promotion: "q" });
      setSel(null);
      setFen(g.fen());
      if (g.isCheckmate()) {
        setMsg("win");
        setPoints((p) => p + 10);
        if (coin.current) gsap.fromTo(coin.current, { scale: 0.6, rotate: -20 }, { scale: 1, rotate: 0, duration: 0.6, ease: "back.out(3)" });
      } else {
        setMsg("wrong");
        window.setTimeout(() => {
          setFen(PUZZLES[pi]);
          setMsg("");
        }, 900);
      }
      return;
    }
    setSel(piece && piece.color === "w" ? sq : null);
  };

  const nextPuzzle = () => {
    const n = (pi + 1) % PUZZLES.length;
    setPi(n);
    setFen(PUZZLES[n]);
    setSel(null);
    setMsg("");
  };

  const buy = () => {
    if (points < 10) return;
    setPoints((p) => p - 10);
    setCandy((c) => c + 1);
    setMsg("bought");
  };

  return (
    <section id="chess" className="py-24" style={{ background: "var(--color-cgreen)" }}>
      <div className="mx-auto grid max-w-6xl items-center gap-12 px-4 md:grid-cols-[1fr_auto]">
        <div data-reveal>
          <p className="kicker mb-3 text-ink2">{t("chess_kicker")}</p>
          <h2 className="text-4xl font-extrabold tracking-tight md:text-5xl">{t("chess_title")}</h2>
          <p className="mt-5 max-w-xl text-lg text-ink2">{t("chess_lead")}</p>
          <div className="mt-8 flex flex-wrap items-center gap-5">
            <div
              ref={coin}
              className="flex h-28 w-28 flex-col items-center justify-center rounded-full border-[3px] border-ink shadow-[0_5px_0_var(--color-ink)]"
              style={{ background: "radial-gradient(circle at 35% 30%, #ffd77a, #e8a23c 70%)" }}
            >
              <span className="font-pixel text-3xl leading-none">{points}</span>
              <span className="kicker mt-1">{t("chess_points")}</span>
            </div>
            <div className="flex flex-col gap-3">
              <button type="button" className="btn" disabled={points < 10} onClick={buy}>
                {/* eslint-disable-next-line @next/next/no-img-element */}
                <img src={asset("/img/icon_candy.png")} alt="" width={28} height={28} className="pixelated" />
                {t("chess_buy")}
              </button>
              <button type="button" className="btn btn-sm self-start" onClick={nextPuzzle}>
                {t("chess_reset")}
              </button>
            </div>
          </div>
          <p className="mt-6 max-w-xl text-sm text-ink2">{t("chess_more")}</p>
        </div>

        <div className="flex flex-col items-center gap-4" data-reveal>
          <p className="font-bold">{t("chess_puzzle")}</p>
          <div className="grid grid-cols-8 overflow-hidden rounded-xl border-[3px] border-ink shadow-[0_6px_0_var(--color-ink)]" style={{ width: "min(88vw, 400px)" }}>
            {board.map((row, r) =>
              row.map((cell, f) => {
                const sq = `${FILES[f]}${8 - r}` as Square;
                const dark = (r + f) % 2 === 1;
                const isSel = sel === sq;
                const isTarget = targets.has(sq);
                return (
                  <button
                    key={sq}
                    type="button"
                    onClick={() => click(sq)}
                    aria-label={sq + (cell ? ` ${cell.color === "w" ? "white" : "black"} ${cell.type}` : "")}
                    className="relative flex aspect-square items-center justify-center text-[min(8vw,38px)] leading-none"
                    style={{ background: isSel ? "#f6d365" : dark ? "#7fa650" : "#eeeed2" }}
                  >
                    {cell && (
                      <span
                        style={{
                          color: cell.color === "w" ? "#fff" : "#2a2a36",
                          textShadow: cell.color === "w" ? "0 0 2px #2a2a36, 0 0 1px #2a2a36, 0 2px 0 #2a2a36" : "none",
                        }}
                      >
                        {GLYPH[cell.type]}
                      </span>
                    )}
                    {isTarget && <span className="absolute h-1/4 w-1/4 rounded-full bg-ink/35" />}
                  </button>
                );
              }),
            )}
          </div>
          <p className="h-6 font-bold" aria-live="polite">
            {msg === "wrong" ? t("chess_wrong") : msg === "win" ? t("chess_win") : msg === "bought" ? t("chess_bought") : ""}
          </p>
        </div>
      </div>
    </section>
  );
}
