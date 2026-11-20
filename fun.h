#pragma once
#include "pet.h"
#include "sport.h"

// Ocio (fun.ino): cosas que hacer segun el tipo, que cuestan puntos (los del ajedrez) y no
// dan puntos: dan frutas, comida, fragmentos de piedra mega, huevos... Cuatro, y cada tipo
// tiene la suya:
//   huerto (planta, Bicho, Tierra, Veneno, Hada): 3 parcelas, en tiempo real; regada madura
//          en 12 h y sin regar en 24; da 2-3 frutas
//   pesca (Agua, Hielo, Dragon): se recoge el sedal cuando pican; bien a tiempo, mejor pesca
//   mina (Roca, Acero, Lucha, Fuego): se rompe la piedra a toques; fragmentos o un fosil
//   expedicion (Normal, Volador, Electrico, Psiquico, Fantasma, Siniestro): sale una hora
// El ajedrez del dia da suerte: si se jugo, una cosa mas; si se gano, lo raro el doble.

enum : uint8_t { FUN_GARDEN = 0, FUN_FISH, FUN_MINE, FUN_QUEST, FUN_COUNT };
static const uint8_t FUN_COST[FUN_COUNT] = { 8, 5, 10, 8 };
#define GARDEN_WET_SECS (12UL * 3600)
#define GARDEN_DRY_SECS (24UL * 3600)
#define QUEST_SECS 3600UL

static inline uint8_t funOf(uint8_t type) {
  switch (type) {
    case TY_GRASS: case TY_BUG: case TY_GROUND: case TY_POISON: case TY_FAIRY: return FUN_GARDEN;
    case TY_WATER: case TY_ICE: case TY_DRAGON: return FUN_FISH;
    case TY_ROCK: case TY_STEEL: case TY_FIGHTING: case TY_FIRE: return FUN_MINE;
    default: return FUN_QUEST;  // Normal, Volador, Electrico, Psiquico, Fantasma, Siniestro
  }
}

// ---------- huerto ----------
enum : uint8_t { PLOT_EMPTY = 0, PLOT_GROWING, PLOT_RIPE };
static inline uint32_t plotGrowSecs(bool wet) { return wet ? GARDEN_WET_SECS : GARDEN_DRY_SECS; }
static inline uint8_t plotState(uint32_t plantedAt, bool wet, uint32_t now, uint32_t div = 1) {
  if (!plantedAt) return PLOT_EMPTY;
  return now >= plantedAt + plotGrowSecs(wet) / div ? PLOT_RIPE : PLOT_GROWING;
}
// segundos que le faltan (0 = madura)
static inline uint32_t plotLeft(uint32_t plantedAt, bool wet, uint32_t now, uint32_t div = 1) {
  uint32_t end = plantedAt + plotGrowSecs(wet) / div;
  return now >= end ? 0 : end - now;
}

// ---------- lo que se saca ----------
struct FunLoot {
  uint8_t n = 0;
  uint8_t item[6];
  uint8_t qty[6];
  void add(uint8_t it, uint8_t q) {
    for (int i = 0; i < n; i++)
      if (item[i] == it) { qty[i] += q; return; }
    if (n < 6) { item[n] = it; qty[n] = q; n++; }
  }
  int total() const { int s = 0; for (int i = 0; i < n; i++) s += qty[i]; return s; }
};

// u[]: numeros al azar en [0, 1). Frutas: 2 o 3 (+1 si hoy se jugo al ajedrez); cada una de
// una capacidad, o dorada (5%, 10% si se gano)
static inline FunLoot gardenLoot(const float u[5], uint8_t luck) {
  FunLoot l;
  int n = 2 + (u[0] < 0.5f ? 1 : 0) + (luck >= LUCK_PLAYED ? 1 : 0);
  float oro = luck == LUCK_WON ? 0.10f : 0.05f;
  for (int i = 0; i < n; i++) {
    float v = u[1 + i];
    if (v < oro) l.add(ITEM_FRUIT_GOLD, 1);
    else l.add(ITEM_FRUIT_STA + (int)((v - oro) / (1 - oro) * 3) % 3, 1);
  }
  return l;
}

// pesca: bien = recogido mientras picaba; si no, solo un pez pequeño
static inline FunLoot fishLoot(bool good, float u, uint8_t luck) {
  FunLoot l;
  float huevo = good ? (luck == LUCK_WON ? 0.08f : 0.04f) : 0;
  float perla = good ? 0.25f : 0.08f;
  if (u < huevo) l.add(ITEM_EGG_WATER, 1);
  else if (u < huevo + perla) l.add(ITEM_PEARL, 1);
  else l.add(ITEM_FISH, good ? 2 : 1);
  if (luck >= LUCK_PLAYED) l.add(ITEM_FISH, 1);
  return l;
}

// mina: fragmentos (1-2 o 3) o un fosil (12%, 24% si se gano)
static inline FunLoot mineLoot(float u, uint8_t luck) {
  FunLoot l;
  float fosil = luck == LUCK_WON ? 0.24f : 0.12f;
  if (u < fosil) l.add(ITEM_EGG_FOSSIL, 1);
  else if (u < fosil + 0.18f) l.add(ITEM_MEGA_SHARD, 3);
  else l.add(ITEM_MEGA_SHARD, u < 0.6f ? 2 : 1);
  if (luck >= LUCK_PLAYED) l.add(ITEM_MEGA_SHARD, 1);
  return l;
}

// expedicion: chuches, medicina, una pocion, un complemento que falte (acc = cual, 0xFF si
// los tiene todos: entonces chuches) o un amuleto shiny (5%, 10% si se gano)
static inline FunLoot questLoot(float u, float u2, uint8_t luck, uint8_t &acc) {
  FunLoot l;
  float amuleto = luck == LUCK_WON ? 0.10f : 0.05f;
  uint8_t quiere = acc;
  acc = 0xFF;
  if (u < amuleto) l.add(ITEM_SHINY_CHARM, 1);
  else if (u < amuleto + 0.15f && quiere != 0xFF) acc = quiere;
  else if (u < amuleto + 0.35f)  // una de las tres pociones (la de resistencia solo sale asi y en la hierba)
    l.add(u2 < 1.0f / 3 ? ITEM_POTION_STR : u2 < 2.0f / 3 ? ITEM_POTION_SPD : ITEM_POTION_END, 1);
  else if (u < amuleto + 0.55f) l.add(ITEM_MEDICINE, 1);
  else l.add(ITEM_CANDY, 2);
  if (luck >= LUCK_PLAYED) l.add(ITEM_CANDY, 1);
  return l;
}

// una fruta en las capacidades de un pokemon (el compañero o uno de la granja)
static inline bool fruitRaise(uint8_t item, uint8_t &sta, uint8_t &res, uint8_t &vit) {
  uint8_t *v[3] = { &sta, &res, &vit };
  bool subio = false;
  for (int i = 0; i < 3; i++) {
    if (item != ITEM_FRUIT_GOLD && item != ITEM_FRUIT_STA + i) continue;
    if (*v[i] >= 100) continue;
    *v[i] = *v[i] + FRUIT_GAIN > 100 ? 100 : *v[i] + FRUIT_GAIN;
    subio = true;
  }
  return subio;
}
static inline bool isFruit(uint8_t item) { return item >= ITEM_FRUIT_STA && item <= ITEM_FRUIT_GOLD; }
