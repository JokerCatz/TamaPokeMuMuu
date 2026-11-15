// Gimnasios (gym.h): pruebas de cada tipo, rivales cada vez mas fuertes y alineacion.
#include "framework.h"
#include "../gym.h"

static bool tiene(const uint8_t ev[3], uint8_t e) { return ev[0] == e || ev[1] == e || ev[2] == e; }

TEST(gym, cada_gimnasio_elige_sus_pruebas) {
  uint8_t ev[3];
  gymEvents(TY_WATER, ev);
  CHECK(tiene(ev, EV_SWIM) && tiene(ev, EV_DIVE));       // agua: nadar y saltar al agua
  gymEvents(TY_ROCK, ev);
  CHECK(tiene(ev, EV_CLIMB) && tiene(ev, EV_LIFT));      // roca: escalada y pesas
  gymEvents(TY_ICE, ev);
  CHECK(tiene(ev, EV_SKATE));
  gymEvents(TY_FIRE, ev);
  CHECK(tiene(ev, EV_TORCH));
  for (int t = 0; t < TY_COUNT; t++) {  // tres distintas y ninguna en contra de su tipo
    gymEvents(t, ev);
    CHECK(ev[0] != ev[1] && ev[1] != ev[2] && ev[0] != ev[2]);
    for (int i = 0; i < 3; i++) CHECK(typeBonus(ev[i], t) >= 1.0f);
  }
}

TEST(gym, los_lideres_son_cada_vez_mas_fuertes) {
  Athlete a = leaderAthlete(95, gymDifficulty(0, 0));  // Onix del primer gimnasio
  Athlete b = leaderAthlete(95, gymDifficulty(0, 7));
  Athlete c = leaderAthlete(95, leagueDifficulty(0, 4));
  CHECK(athleteRating(b, EV_LIFT) > athleteRating(a, EV_LIFT));
  CHECK(athleteRating(c, EV_LIFT) > athleteRating(b, EV_LIFT));
  Athlete z = leaderAthlete(95, 200);
  CHECK(z.trAtk <= 100 && z.geneAtk <= 110);  // con topes
}

TEST(gym, la_alineacion_pone_a_cada_uno_donde_mejor_va) {
  // Blastoise, Machamp y Jolteon contra nadar, pesas y 100 m
  Athlete team[3] = { athleteOf(9), athleteOf(68), athleteOf(135) };
  uint8_t ev[3] = { EV_SWIM, EV_LIFT, EV_SPRINT };
  uint8_t perm[3];
  bestAssign(team, ev, perm);
  CHECK_EQ((int)perm[0], 0);  // Blastoise nada
  CHECK_EQ((int)perm[1], 1);  // Machamp levanta
  CHECK_EQ((int)perm[2], 2);  // Jolteon corre
}

TEST(gym, la_serie_cuenta_bien_cada_prueba) {
  CHECK(wonRound(EV_SPRINT, 11.0f, 12.0f));   // menos tiempo
  CHECK(!wonRound(EV_SPRINT, 13.0f, 12.0f));
  CHECK(wonRound(EV_LIFT, 180, 160));          // mas kilos
  CHECK(wonRound(EV_DIVE, 70, 70));            // empate: para el jugador
}
