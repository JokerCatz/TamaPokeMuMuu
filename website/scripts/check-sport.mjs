// Prints rating and mark for every Pokémon of lib/sport.ts in every event, for a few
// genes / training / luck values. scripts/check-sport.sh compares it with the firmware.
import { EVENTS, MONS, performance, rating } from "../lib/sport.ts";

const CASES = [
  [100, 100, 100, 0, 0.5],
  [90, 110, 97, 37, 0.0],
  [110, 93, 104, 100, 0.999],
];
for (const m of Object.values(MONS))
  for (const ev of EVENTS)
    for (const [ga, gd, gs, tr, u] of CASES) {
      const r = rating({ mon: m, geneAtk: ga, geneDef: gd, geneSpe: gs, training: tr }, ev);
      console.log(`${m.id} ${ev.id} ${r.toFixed(2)} ${ev.mark(performance(r, u)).toFixed(2)}`);
    }
