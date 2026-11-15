// Ocio (fun.h): que hace cada tipo, el huerto, lo que se saca y las frutas.
#include "framework.h"
#include "../fun.h"

TEST(fun, cada_tipo_tiene_su_ocio) {
  CHECK_EQ((int)funOf(TY_GRASS), (int)FUN_GARDEN);
  CHECK_EQ((int)funOf(TY_BUG), (int)FUN_GARDEN);
  CHECK_EQ((int)funOf(TY_GROUND), (int)FUN_GARDEN);
  CHECK_EQ((int)funOf(TY_WATER), (int)FUN_FISH);
  CHECK_EQ((int)funOf(TY_ICE), (int)FUN_FISH);
  CHECK_EQ((int)funOf(TY_FIRE), (int)FUN_MINE);  // Charizard: a la mina
  CHECK_EQ((int)funOf(TY_ROCK), (int)FUN_MINE);
  CHECK_EQ((int)funOf(TY_GHOST), (int)FUN_QUEST);
  int n[FUN_COUNT] = {};
  for (int t = 0; t < TY_COUNT; t++) n[funOf(t)]++;
  for (int f = 0; f < FUN_COUNT; f++) CHECK(n[f] >= 3);  // todos tienen varios tipos
}

TEST(fun, el_huerto_madura_antes_si_se_riega) {
  uint32_t t0 = 1000000;
  CHECK_EQ((int)plotState(0, false, t0), (int)PLOT_EMPTY);
  CHECK_EQ((int)plotState(t0, true, t0 + 11 * 3600), (int)PLOT_GROWING);
  CHECK_EQ((int)plotState(t0, true, t0 + 12 * 3600), (int)PLOT_RIPE);
  CHECK_EQ((int)plotState(t0, false, t0 + 12 * 3600), (int)PLOT_GROWING);  // sin regar: 24 h
  CHECK_EQ((int)plotState(t0, false, t0 + 24 * 3600), (int)PLOT_RIPE);
  CHECK_EQ(plotLeft(t0, true, t0 + 3600), 11UL * 3600);
  CHECK_EQ(plotLeft(t0, true, t0 + 13 * 3600), 0UL);
}

TEST(fun, el_ajedrez_da_suerte) {
  float u[5] = { 0.9f, 0.5f, 0.5f, 0.5f, 0.5f };
  CHECK_EQ(gardenLoot(u, LUCK_NONE).total(), 2);
  CHECK_EQ(gardenLoot(u, LUCK_PLAYED).total(), 3);  // se jugo: una mas
  float v[5] = { 0.1f, 0.07f, 0.5f, 0.5f, 0.5f };
  CHECK_EQ(gardenLoot(v, LUCK_NONE).total(), 3);
  FunLoot sin = gardenLoot(v, LUCK_NONE), con = gardenLoot(v, LUCK_WON);
  bool oroSin = false, oroCon = false;  // 0.07: dorada solo si se gano (10% en vez de 5%)
  for (int i = 0; i < sin.n; i++) oroSin |= sin.item[i] == ITEM_FRUIT_GOLD;
  for (int i = 0; i < con.n; i++) oroCon |= con.item[i] == ITEM_FRUIT_GOLD;
  CHECK(!oroSin && oroCon);
  // pesca: el huevo solo bien recogido, y mas facil si se gano
  CHECK_EQ((int)fishLoot(true, 0.06f, LUCK_WON).item[0], (int)ITEM_EGG_WATER);
  CHECK(fishLoot(true, 0.06f, LUCK_NONE).item[0] != ITEM_EGG_WATER);
  CHECK(fishLoot(false, 0.0f, LUCK_WON).item[0] != ITEM_EGG_WATER);
  CHECK_EQ((int)mineLoot(0.2f, LUCK_WON).item[0], (int)ITEM_EGG_FOSSIL);
  CHECK(mineLoot(0.2f, LUCK_NONE).item[0] != ITEM_EGG_FOSSIL);
}

TEST(fun, siempre_se_saca_algo_valido) {
  for (int k = 0; k < 200; k++) {
    float u[5];
    for (int i = 0; i < 5; i++) u[i] = (k * 37 + i * 11) % 100 / 100.0f;
    for (uint8_t luck = LUCK_NONE; luck <= LUCK_WON; luck++) {
      FunLoot ls[4];
      ls[0] = gardenLoot(u, luck);
      ls[1] = fishLoot(k % 2, u[0], luck);
      ls[2] = mineLoot(u[1], luck);
      uint8_t acc = (k % 3) ? 2 : 0xFF;
      ls[3] = questLoot(u[2], u[3], luck, acc);
      for (int f = 0; f < 4; f++) {
        CHECK(ls[f].total() >= 1 || (f == 3 && acc != 0xFF));
        for (int i = 0; i < ls[f].n; i++) CHECK(ls[f].item[i] < ITEM_COUNT && ls[f].qty[i] >= 1);
      }
      for (int i = 0; i < ls[0].n; i++) CHECK(isFruit(ls[0].item[i]));
    }
  }
}

TEST(fun, frutas_suben_su_capacidad) {
  uint8_t s = 10, r = 20, v = 99;
  CHECK(fruitRaise(ITEM_FRUIT_STA, s, r, v));
  CHECK_EQ((int)s, 13);
  CHECK(fruitRaise(ITEM_FRUIT_VIT, s, r, v));
  CHECK_EQ((int)v, 100);  // no pasa de 100
  CHECK(!fruitRaise(ITEM_FRUIT_VIT, s, r, v));
  CHECK(fruitRaise(ITEM_FRUIT_GOLD, s, r, v));  // la dorada, las tres
  CHECK_EQ((int)s, 16);
  CHECK_EQ((int)r, 23);
  CHECK(!fruitRaise(ITEM_FISH, s, r, v));
}
