// Pruebas del sports day (sport.h): las juegan solas segun lo que es cada pokemon.
#include "framework.h"
#include "../sport.h"

// atleta de una especie con genes normales y sin entrenar
static Athlete normal(int16_t dex) {
  Athlete a = athleteOf(dex);
  return a;
}

TEST(sport, etapa_de_evolucion) {
  CHECK_EQ((int)evoStage(4), 1);    // Charmander
  CHECK_EQ((int)evoStage(5), 2);    // Charmeleon
  CHECK_EQ((int)evoStage(6), 3);    // Charizard
  CHECK_EQ((int)evoStage(150), 3);  // Mewtwo: legendario, como forma final
  CHECK_EQ((int)evoStage(129), 1);  // Magikarp
  CHECK_EQ((int)evoStage(130), 2);  // Gyarados
}

TEST(sport, evolucionado_y_rapido_corre_mas) {
  for (int ev = 0; ev < EV_COUNT; ev++) {
    CHECK(athleteRating(normal(6), ev) > athleteRating(normal(5), ev));  // Charizard > Charmeleon
    CHECK(athleteRating(normal(5), ev) > athleteRating(normal(4), ev));  // Charmeleon > Charmander
  }
  // cada prueba mira lo suyo: Snorlax (lento y fuerte) salta mejor que corre, comparado con Jolteon
  CHECK(athleteRating(normal(135), EV_SPRINT) > athleteRating(normal(143), EV_SPRINT));
  CHECK(athleteRating(normal(143), EV_SWIM) > athleteRating(normal(143), EV_SPRINT));
}

TEST(sport, el_tipo_de_cada_especie) {
  CHECK_EQ((int)typeOf(7), (int)TY_WATER);      // Squirtle
  CHECK_EQ((int)typeOf(4), (int)TY_FIRE);       // Charmander
  CHECK_EQ((int)typeOf(25), (int)TY_ELECTRIC);  // Pikachu
  CHECK_EQ((int)typeOf(66), (int)TY_FIGHTING);  // Machop
  CHECK_EQ((int)typeOf(74), (int)TY_ROCK);      // Geodude
}

TEST(sport, cada_prueba_favorece_a_unos_tipos) {
  CHECK(typeBonus(EV_SWIM, TY_WATER) > 1.2f);       // nadar: agua
  CHECK(typeBonus(EV_SWIM, TY_FIRE) < 1.0f);        // al fuego no le va el agua
  CHECK(typeBonus(EV_SPRINT, TY_ELECTRIC) > 1.0f);
  CHECK(typeBonus(EV_HIGHJUMP, TY_FLYING) > 1.2f);
  CHECK(typeBonus(EV_LONGJUMP, TY_FIGHTING) > 1.0f);
  CHECK_EQ(typeBonus(EV_SWIM, TY_PSYCHIC), 1.0f);   // a los demas, ni bien ni mal
  // Blastoise (agua) nada mejor que Charizard (fuego), aunque Charizard corre mas
  CHECK(athleteRating(normal(9), EV_SWIM) > athleteRating(normal(6), EV_SWIM));
  CHECK(athleteRating(normal(6), EV_SPRINT) > athleteRating(normal(9), EV_SPRINT));
}

TEST(sport, genes_entrenamiento_y_mega_suman) {
  Athlete a = normal(6), b = normal(6);
  b.geneSpe = 110;
  CHECK(athleteRating(b, EV_SPRINT) > athleteRating(a, EV_SPRINT));
  b = a;
  b.trAtk = b.trSpe = b.trDef = 100;
  CHECK(athleteRating(b, EV_SPRINT) > athleteRating(a, EV_SPRINT));
  b = a;
  b.mega = true;
  CHECK(athleteRating(b, EV_SPRINT) > athleteRating(a, EV_SPRINT) * 1.1f);
}

TEST(sport, marcas_razonables_y_mejores_cuanto_mas_fuerte) {  // en las 11 pruebas
  for (int ev = 0; ev < EV_COUNT; ev++) {
    float flojo = eventMark(ev, 50), medio = eventMark(ev, 100), fuerte = eventMark(ev, 150);
    if (eventLowerWins(ev)) CHECK(flojo > medio && medio > fuerte);
    else CHECK(flojo < medio && medio < fuerte);
  }
  float t = eventMark(EV_SPRINT, 100);
  CHECK(t > 10.5f && t < 13.5f);  // 100 m de un buen corredor: ~12 s
  float m = eventMark(EV_LONGJUMP, 100);
  CHECK(m > 6.0f && m < 8.0f);
  float h = eventMark(EV_HIGHJUMP, 100);
  CHECK(h > 1.5f && h < 2.0f);
  CHECK(fmodf(eventMark(EV_HIGHJUMP, 123) * 100 + 0.01f, 5.0f) < 0.1f);  // el liston va de 5 en 5 cm
}

TEST(sport, la_suerte_decide_entre_parecidos_y_no_entre_muy_distintos) {
  // con la misma fuerza gana cada uno mas o menos la mitad; con mucha diferencia, casi siempre el fuerte
  int parejos = 0, distintos = 0;
  for (int i = 0; i < 400; i++) {
    float u1 = (i * 37 % 400) / 400.0f, u2 = (i * 113 % 400) / 400.0f;
    if (performance(100, u1) > performance(100, u2)) parejos++;
    if (performance(100, u1) > performance(160, u2)) distintos++;
  }
  CHECK(parejos > 120 && parejos < 280);
  CHECK(distintos < 10);
}

TEST(sport, rivales_al_azar_no_siempre_flojos_ni_siempre_fuertes) {
  // contra rivales sorteados, un Charizard normal gana unas veces y pierde otras
  float yo = athleteRating(normal(6), EV_SPRINT);
  int gana = 0, pierde = 0;
  uint32_t s = 12345;
  auto u = [&s]() { s = s * 1103515245u + 12345u; return ((s >> 8) & 0xffff) / 65536.0f; };
  for (int i = 0; i < 300; i++) {
    Athlete r = rivalAthlete(1 + (int)(u() * DEX_COUNT), u(), u(), u(), u());
    float me = eventMark(EV_SPRINT, performance(yo, u())), el = eventMark(EV_SPRINT, performance(athleteRating(r, EV_SPRINT), u()));
    if (me <= el) gana++; else pierde++;
  }
  CHECK(gana > 30);
  CHECK(pierde > 30);
}

TEST(sport, cada_tipo_tiene_al_menos_una_prueba_a_su_favor) {
  for (int t = 0; t < TY_COUNT; t++) {
    bool alguna = false;
    for (int ev = 0; ev < EV_COUNT; ev++)
      if (typeBonus(ev, t) > 1.0f) alguna = true;
    CHECK_MSG(alguna, "un tipo sin prueba a favor");
  }
  CHECK(typeBonus(EV_LIFT, TY_FIGHTING) > 1.1f);
  CHECK(typeBonus(EV_CLIMB, TY_ROCK) > 1.1f);
  CHECK(typeBonus(EV_SKATE, TY_ICE) > 1.2f);
  CHECK(typeBonus(EV_TORCH, TY_FIRE) > 1.2f);
}

TEST(sport, las_nuevas_pruebas_tienen_marcas_razonables) {
  CHECK_EQ((int)eventUnit(EV_LIFT), (int)UNIT_KG);
  CHECK_EQ((int)eventUnit(EV_DIVE), (int)UNIT_POINTS);
  CHECK_EQ((int)eventUnit(EV_TORCH), (int)UNIT_MINSEC);
  CHECK(!eventLowerWins(EV_LIFT) && !eventLowerWins(EV_DIVE));
  CHECK(eventLowerWins(EV_CLIMB) && eventLowerWins(EV_SKATE) && eventLowerWins(EV_TORCH) && eventLowerWins(EV_TRAIL));
  CHECK(fmodf(eventMark(EV_LIFT, 117), 5.0f) == 0.0f);   // pesas de 5 en 5 kg
  float kg = eventMark(EV_LIFT, 100);
  CHECK(kg >= 150 && kg <= 170);
  float min = eventMark(EV_TORCH, 100) / 60;
  CHECK(min > 11 && min < 15);                            // 3000 m en unos 13 minutos
}

TEST(sport, puesto_con_empates_a_favor_del_jugador) {
  CHECK_EQ((int)playerPlace(10, 11, 12, true), 1);   // carrera: el menor tiempo
  CHECK_EQ((int)playerPlace(12, 11, 13, true), 2);
  CHECK_EQ((int)playerPlace(14, 11, 13, true), 3);
  CHECK_EQ((int)playerPlace(11, 11, 13, true), 1);   // empate: gana el jugador
  CHECK_EQ((int)playerPlace(6.0f, 5.0f, 7.0f, false), 2);  // salto: el mayor
  CHECK_EQ((int)playerPlace(7.0f, 7.0f, 5.0f, false), 1);
}
