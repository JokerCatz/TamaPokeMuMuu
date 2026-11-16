// The tournament rules of the board (sport.h), for the four races of the mini-game:
// what a Pokémon is worth in an event comes from its species' stats, its genes and
// training, its evolution stage and its type, plus ±15% of luck on the day.
// scripts/check-sport.sh compares the numbers with the firmware's own functions.

export type Ty = "normal" | "fire" | "water" | "grass" | "rock" | "bug";
export type Mon = { id: string; en: string; zh: string; type: Ty; hp: number; atk: number; def: number; spe: number };

// base stats and first type as in dex.h. All are first forms (stage factor 0.9).
export const MONS: Record<string, Mon> = {
  bulbasaur: { id: "bulbasaur", en: "Bulbasaur", zh: "妙蛙种子", type: "grass", hp: 45, atk: 49, def: 49, spe: 45 },
  charmander: { id: "charmander", en: "Charmander", zh: "小火龙", type: "fire", hp: 39, atk: 52, def: 43, spe: 65 },
  squirtle: { id: "squirtle", en: "Squirtle", zh: "杰尼龟", type: "water", hp: 44, atk: 48, def: 65, spe: 43 },
  eevee: { id: "eevee", en: "Eevee", zh: "伊布", type: "normal", hp: 55, atk: 55, def: 50, spe: 55 },
  psyduck: { id: "psyduck", en: "Psyduck", zh: "可达鸭", type: "water", hp: 50, atk: 52, def: 48, spe: 55 },
  oddish: { id: "oddish", en: "Oddish", zh: "走路草", type: "grass", hp: 45, atk: 50, def: 55, spe: 30 },
  geodude: { id: "geodude", en: "Geodude", zh: "小拳石", type: "rock", hp: 40, atk: 80, def: 100, spe: 20 },
  vulpix: { id: "vulpix", en: "Vulpix", zh: "六尾", type: "fire", hp: 38, atk: 41, def: 40, spe: 65 },
  caterpie: { id: "caterpie", en: "Caterpie", zh: "绿毛虫", type: "bug", hp: 45, atk: 30, def: 35, spe: 45 },
  pidgey: { id: "pidgey", en: "Pidgey", zh: "波波", type: "normal", hp: 40, atk: 45, def: 40, spe: 56 },
  rattata: { id: "rattata", en: "Rattata", zh: "小拉达", type: "normal", hp: 30, atk: 56, def: 35, spe: 72 },
};
// on the board any two of the 1,025 species can turn up; here, a few first forms
export const RIVALS = ["eevee", "psyduck", "oddish", "geodude", "vulpix", "caterpie", "pidgey", "rattata"] as const;

export const TYPE_NAMES: Record<Ty, { en: string; zh: string }> = {
  normal: { en: "Normal", zh: "一般" },
  fire: { en: "Fire", zh: "火" },
  water: { en: "Water", zh: "水" },
  grass: { en: "Grass", zh: "草" },
  rock: { en: "Rock", zh: "岩石" },
  bug: { en: "Bug", zh: "虫" },
};

export type EventId = "sprint" | "swim" | "torch" | "trail";
export type SportEvent = {
  id: EventId;
  en: string;
  zh: string;
  hint: { en: string; zh: string }; // what the event asks for, in a few words
  w: [number, number, number, number]; // weight of HP, attack, defence and speed (EV_W)
  mods: Partial<Record<Ty, number>>; // TYPE_MODS, for the types in MONS
  mark: (perf: number) => number; // seconds (eventMark)
  minsec: boolean; // shown as m:ss, not as seconds with a decimal
};
export const EVENTS: SportEvent[] = [
  { id: "sprint", en: "100m dash", zh: "100米短跑", hint: { en: "Mostly speed", zh: "主要看速度" }, w: [0, 0.25, 0, 0.75], mods: { normal: 1.05, rock: 0.9 }, mark: (p) => Math.min(25, 7 + 500 / p), minsec: false },
  { id: "swim", en: "50m swim", zh: "50米游泳", hint: { en: "Speed and stamina", zh: "速度和耐力" }, w: [0.5, 0, 0, 0.5], mods: { water: 1.25, fire: 0.85, rock: 0.85 }, mark: (p) => Math.min(45, 13 + 700 / p), minsec: false },
  { id: "torch", en: "Torch run", zh: "火炬长跑", hint: { en: "Stamina", zh: "比耐力" }, w: [0.5, 0, 0.1, 0.4], mods: { fire: 1.25, normal: 1.05, water: 0.9 }, mark: (p) => Math.min(1800, 480 + 30000 / p), minsec: true },
  { id: "trail", en: "Trail run", zh: "越野障碍", hint: { en: "A bit of everything", zh: "样样都要一点" }, w: [0.2, 0.3, 0.2, 0.3], mods: { grass: 1.1, bug: 1.05, rock: 0.9 }, mark: (p) => Math.min(900, 120 + 9000 / p), minsec: true },
];

export type Athlete = { mon: Mon; geneAtk: number; geneDef: number; geneSpe: number; training: number };

// the visitor's friend: normal genes, not trained yet
export const fresh = (mon: Mon): Athlete => ({ mon, geneAtk: 100, geneDef: 100, geneSpe: 100, training: 0 });

// a rival: random genes (90..110%) and training (0..100), u* in [0,1) (rivalAthlete)
export const rival = (mon: Mon, uAtk: number, uDef: number, uSpe: number, uTr: number): Athlete => ({
  mon,
  geneAtk: Math.floor(90 + uAtk * 21),
  geneDef: Math.floor(90 + uDef * 21),
  geneSpe: Math.floor(90 + uSpe * 21),
  training: Math.floor(uTr * 101),
});

export const typeBonus = (ev: SportEvent, type: Ty) => ev.mods[type] ?? 1;

// what the athlete is worth in the event (athleteRating; a good one is around 100)
export function rating(a: Athlete, ev: SportEvent) {
  const m = a.mon;
  const base = ev.w[0] * m.hp + (ev.w[1] * m.atk * a.geneAtk) / 100 + (ev.w[2] * m.def * a.geneDef) / 100 + (ev.w[3] * m.spe * a.geneSpe) / 100;
  return base * 0.9 * (1 + (0.12 * a.training) / 100) * typeBonus(ev, m.type);
}

// the day's performance: the rating with ±15% of luck (u in [0,1))
export const performance = (r: number, u: number) => Math.max(20, r * (0.85 + 0.3 * u));

// the board prints seconds cut to one decimal, or minutes and seconds
export function formatMark(ev: SportEvent, v: number) {
  if (ev.minsec) return `${Math.floor(v / 60)}:${String(Math.floor(v) % 60).padStart(2, "0")}`;
  return `${Math.floor(v)}.${Math.floor(v * 10) % 10}`;
}

// place (1..3) of the first of three marks: less time is better, ties go to the visitor (playerPlace)
export const placeOf = (me: number, a: number, b: number) => 1 + (a < me ? 1 : 0) + (b < me ? 1 : 0);
