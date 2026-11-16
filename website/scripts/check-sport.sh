#!/bin/bash
# Checks that the mini-game's rules (lib/sport.ts) give the same numbers as the
# firmware's sport.h: compiles a small program against the real header and compares.
#   bash website/scripts/check-sport.sh
set -e
cd "$(dirname "$0")/.."
TMP=$(mktemp -d)
cat > "$TMP/check.cpp" <<'CPP'
#include <stdio.h>
#include "sport.h"
int main() {
  struct { const char *id; int dex; } M[] = { {"bulbasaur",1}, {"charmander",4}, {"squirtle",7}, {"eevee",133}, {"psyduck",54},
    {"oddish",43}, {"geodude",74}, {"vulpix",37}, {"caterpie",10}, {"pidgey",16}, {"rattata",19} };
  struct { const char *id; uint8_t ev; } E[] = { {"sprint",EV_SPRINT}, {"swim",EV_SWIM}, {"torch",EV_TORCH}, {"trail",EV_TRAIL} };
  float C[3][5] = { {100,100,100,0,0.5f}, {90,110,97,37,0.0f}, {110,93,104,100,0.999f} };
  for (auto &m : M) for (auto &e : E) for (auto &c : C) {
    Athlete a = athleteOf(m.dex);
    a.geneAtk = c[0]; a.geneDef = c[1]; a.geneSpe = c[2];
    a.trAtk = a.trDef = a.trSpe = c[3];
    float r = athleteRating(a, e.ev);
    printf("%s %s %.2f %.2f\n", m.id, e.id, r, eventMark(e.ev, performance(r, c[4])));
  }
}
CPP
# dex.h pulls in Arduino.h: the host tests' shim stands in for it
c++ -std=c++17 -I .. -I ../test/shim -o "$TMP/check" "$TMP/check.cpp" ../test/shim/shim.cpp
"$TMP/check" > "$TMP/firmware.txt"
node --experimental-strip-types --no-warnings scripts/check-sport.mjs > "$TMP/web.txt"
# the firmware works in 32-bit floats, so the last digit may differ: 0.02 of tolerance
paste -d' ' "$TMP/firmware.txt" "$TMP/web.txt" | awk '
  function abs(x) { return x < 0 ? -x : x }
  $1 != $5 || $2 != $6 || abs($3 - $7) > 0.02 || abs($4 - $8) > 0.02 { print "DIFFERENT: " $0; bad++ }
  END { if (bad) exit 1; print "OK: " NR " values match sport.h" }'
