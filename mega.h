#pragma once
#include <stdint.h>

// Megaevoluciones: solo las que tienen sprites PMD de SpriteCollab con Idle y Walk
// (tracker.json, noviembre de 2026). form = subcarpeta de la especie en SpriteCollab
// (sprite/NNNN/form); tag = lo que se añade al nombre (X / Y, o nada). El sprite va en
// la SD como /mons/mNNN.bin (y msNNN.bin el shiny, si lo hay): tools/pack_pmd.py mega.
// Una piedra megaevolucion lo convierte durante MEGA_SECONDS; luego vuelve a su forma.
struct MegaForm {
  int16_t dex;
  const char *form;
  const char *tag;
};

static const MegaForm MEGA_FORMS[] = {
  { 6, "0001", "X" },   // Charizard (la Y aun no tiene sprites)
  { 65, "0001", "" },   // Alakazam
  { 80, "0002", "" },   // Slowbro
  { 94, "0001", "" },   // Gengar
  { 115, "0001", "" },  // Kangaskhan
  { 142, "0001", "" },  // Aerodactyl
  { 150, "0002", "Y" }, // Mewtwo (la X aun no)
  { 208, "0001", "" },  // Steelix
  { 227, "0001", "" },  // Skarmory
  { 229, "0001", "" },  // Houndoom
  { 248, "0001", "" },  // Tyranitar
  { 254, "0001", "" },  // Sceptile
  { 282, "0001", "" },  // Gardevoir
  { 302, "0001", "" },  // Sableye
  { 303, "0001", "" },  // Mawile
  { 308, "0001", "" },  // Medicham
  { 310, "0001", "" },  // Manectric
  { 319, "0001", "" },  // Sharpedo
  { 323, "0001", "" },  // Camerupt
  { 334, "0001", "" },  // Altaria
  { 354, "0001", "" },  // Banette
  { 359, "0001", "" },  // Absol
  { 362, "0001", "" },  // Glalie
  { 380, "0001", "" },  // Latias
  { 381, "0001", "" },  // Latios
  { 384, "0001", "" },  // Rayquaza
  { 428, "0001", "" },  // Lopunny
  { 448, "0001", "" },  // Lucario
  { 475, "0001", "" },  // Gallade
  { 491, "0002", "" },  // Darkrai
  { 530, "0001", "" },  // Excadrill
  { 560, "0001", "" },  // Scrafty
  { 604, "0001", "" },  // Eelektross
  { 670, "0006", "" },  // Floette
  { 691, "0001", "" },  // Dragalge
  { 701, "0001", "" },  // Hawlucha
  { 718, "0005", "" },  // Zygarde
  { 719, "0001", "" },  // Diancie
  { 780, "0001", "" },  // Drampa
  { 807, "0001", "" },  // Zeraora
  { 978, "0003", "" },  // Tatsugiri
};
static const int MEGA_COUNT = sizeof(MEGA_FORMS) / sizeof(MEGA_FORMS[0]);

// la megaevolucion de una especie, o nullptr si no tiene (o no hay sprites)
static inline const MegaForm *megaFor(int16_t dex) {
  for (int i = 0; i < MEGA_COUNT; i++)
    if (MEGA_FORMS[i].dex == dex) return &MEGA_FORMS[i];
  return nullptr;
}
