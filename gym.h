#pragma once
#include "sport.h"

// Gimnasios (gyms.ino): torneos por equipos, nada de peleas. Cada region tiene 8, en
// orden, y al final su liga (alto mando + campeon). Cada gimnasio es de un tipo y elige
// las 3 pruebas que mejor le van; se juega a 3 contra 3: cada prueba, uno contra uno, y
// gana quien se lleve 2. Los datos (lideres, medallas, equipos) estan en gyms.h.

#define GYMS_PER_REGION 8
#define LEAGUE_SIZE 5  // alto mando (4) + campeon

// las 3 pruebas de un gimnasio: las que mas favorecen a su tipo (y, empatadas, una de
// las de los gimnasios antes que una del torneo, para que no sean siempre las mismas)
static inline void gymEvents(uint8_t type, uint8_t out[3]) {
  uint8_t ord[EV_COUNT];
  for (int i = 0; i < EV_COUNT; i++) ord[i] = (uint8_t)((EV_COUNT - 1 - i + type) % EV_COUNT);
  for (int a = 0; a < EV_COUNT; a++)  // por ventaja del tipo (estable)
    for (int b = a + 1; b < EV_COUNT; b++)
      if (typeBonus(ord[b], type) > typeBonus(ord[a], type)) {
        uint8_t t = ord[a];
        ord[a] = ord[b];
        ord[b] = t;
      }
  for (int i = 0; i < 3; i++) out[i] = ord[i];
}

// dificultad 0..: gimnasio k de la region r = r*8+k; los de la liga, por encima del 8º
static inline uint8_t gymDifficulty(uint8_t region, uint8_t gym) { return region * GYMS_PER_REGION + gym; }
static inline uint8_t leagueDifficulty(uint8_t region, uint8_t member) {
  return region * GYMS_PER_REGION + GYMS_PER_REGION + member;
}

// un pokemon del lider: mejores genes y mas entrenamiento cuanto mas avanzado el gimnasio
static inline Athlete leaderAthlete(int16_t dex, uint8_t difficulty) {
  Athlete a = athleteOf(dex);
  uint8_t g = (uint8_t)(100 + (difficulty / 7 > 10 ? 10 : difficulty / 7));
  a.geneAtk = a.geneDef = a.geneSpe = g;
  uint16_t tr = 8 + difficulty * 13 / 10;
  a.trAtk = a.trDef = a.trSpe = (uint8_t)(tr > 100 ? 100 : tr);
  return a;
}

// quien hace cada prueba: la permutacion (de 6) que mas suma de capacidad da. perm[i] =
// el del equipo que hace la prueba i
static inline void bestAssign(const Athlete team[3], const uint8_t ev[3], uint8_t perm[3]) {
  static const uint8_t P[6][3] = { { 0, 1, 2 }, { 0, 2, 1 }, { 1, 0, 2 }, { 1, 2, 0 }, { 2, 0, 1 }, { 2, 1, 0 } };
  float mejor = -1;
  for (int p = 0; p < 6; p++) {
    float s = 0;
    for (int i = 0; i < 3; i++) s += athleteRating(team[P[p][i]], ev[i]);
    if (s > mejor) {
      mejor = s;
      for (int i = 0; i < 3; i++) perm[i] = P[p][i];
    }
  }
}

// gana la serie quien se lleve al menos 2 de las 3 (empates de marca, para el jugador)
static inline bool wonRound(uint8_t ev, float mine, float theirs) {
  return eventLowerWins(ev) ? mine <= theirs : mine >= theirs;
}
