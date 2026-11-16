#pragma once
#include <stdint.h>
#include "linkproto.h"  // LINK_NAME

// Fotos con amigos ("见面"): una por amigo y dia, la primera vez que se ven (friend_play.ino).
// En la SD (/friends.bin, una detras de otra), como el album de recuerdos. Sin SD no hay fotos.
#define PHOTO_VER 1
struct FriendPhoto {
  uint8_t ver, myShiny, frShiny, pad;
  int16_t mySpecies, frSpecies;  // 0 = huevo o ninguno
  uint32_t epoch;                // hora local; 0 = no se sabe
  char name[LINK_NAME + 1];      // el dueño de la otra placa
};

bool photoAdd(const FriendPhoto &p);
int photoCount();
bool photoGet(int i, FriendPhoto &p);  // 0 = la mas reciente
