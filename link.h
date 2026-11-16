#pragma once
#include <Arduino.h>
#include "linkproto.h"

// Enlace entre dos placas por ESP-NOW (el formato y el emparejamiento estan en linkproto.h).
// Solo mientras esta abierta la pagina "见面" (o la consola LINK ON): coge la radio
// (netRadioTake: se paran el Wi-Fi de casa, Stack-chan y el servidor del movil), canal fijo
// LINK_CHANNEL, y al cerrar la devuelve. Sin USB tampoco hay light sleep mientras tanto.
//
// Quien es quien: un numero de jugador aleatorio (8 bytes) en la NVS "link", que va en la
// copia diaria. Es de la partida, no de la placa: si la partida pasa a otra placa (como el
// 15-11-2026), los amigos la siguen reconociendo. La MAC solo es "donde esta ahora".
// Los amigos (como mucho LINK_FRIENDS) guardan su numero, su ultima MAC, el nombre del dueño y
// la clave de la pareja; con ella ESP-NOW cifra todo lo que va de uno a otro (los HELLO, a
// todos, van sin cifrar y sin nombre).

#define LINK_CHANNEL 1
#define LINK_FRIENDS 4
#define LINK_NEARBY 6
#define LINK_NEAR_RSSI (-70)  // mas debil: "离得远" (no se empareja). Ajustar con LINK PING en las placas
#define LINK_GONE_MS 3000UL   // sin oir nada de una placa: se fue
#define LINK_IDLE_MS (3UL * 60 * 1000)  // la pagina se cierra sola sin tocarla

// estado de un amigo que esta cerca: ¿entiende aun nuestra clave?
enum : uint8_t { LFS_NONE = 0, LFS_CHECKING, LFS_OK, LFS_LOST };
// linkPairAsk: ademas de LPA_* (linkproto.h)
enum : uint8_t { LINK_ASK_FULL = 10, LINK_ASK_GONE, LINK_ASK_OLD };

struct LinkPeer {  // una placa que se oye ahora
  uint64_t pid;
  uint8_t mac[6];
  int8_t rssi;  // media de los ultimos
  uint8_t major, minor;
  uint32_t caps;
  int16_t species;
  uint8_t shiny;
  uint16_t level;
  uint32_t heardAt;  // millis
  int8_t fr;         // indice en los amigos, -1 = no es amigo
  uint8_t fstate;    // LFS_*
  uint8_t pings;     // pings de comprobacion sin respuesta
  uint32_t pingAt;
  LinkDedupe dd;
};

struct LinkFriend {  // NVS "link"/"f0".."f3"
  uint8_t ver;
  uint8_t mac[6];
  uint8_t key[LINK_KEY];
  char name[LINK_NAME + 1];
  uint64_t pid;
  uint32_t pairedAt, seenAt;  // hora local (epoch de pared); 0 = no se sabe
  uint16_t playDay, photoDay; // v2: ultimo dia (epoch/86400) en que jugaron juntos / se hicieron la foto
};
enum : uint8_t { LDAY_PLAY = 0, LDAY_PHOTO };

bool linkStart(uint32_t idleMs = LINK_IDLE_MS);  // false: no se pudo (sin radio)
void linkStop();
bool linkActive();
void linkLoop(uint32_t now);  // cada vuelta del bucle principal
void linkSetSelf(int16_t species, bool shiny, uint16_t level);  // lo que dice el HELLO
void linkTouch();             // el niño toco la pagina: aleja el cierre automatico
uint64_t linkSelfId();

uint8_t linkNearbyCount();
const LinkPeer &linkNearby(uint8_t i);
inline bool linkPeerNear(const LinkPeer &p) { return p.rssi >= LINK_NEAR_RSSI; }
bool linkPeerSameMajor(const LinkPeer &p);

uint8_t linkFriendCount();
const LinkFriend &linkFriendAt(uint8_t i);
bool linkFriendRam(uint8_t i);  // de prueba (LINK PAIRTEST): no se guarda
bool linkForget(uint8_t i);

// emparejar
uint8_t linkPairAsk(uint64_t pid);  // el niño dijo que si: LPA_* o LINK_ASK_*
void linkPairAnswer(bool yes);      // a la pregunta de otra placa
const LinkPair &linkPairing();
int linkPairNearby();               // indice del otro en linkNearby, -1
const char *linkPairName();         // el nombre del otro (cuando acaba bien)
void linkPairClear();               // ya se enseño como acabo

// juegos con un amigo (friend_play.ino, friendmsg.h): los mensajes 0x20..0x7E de un amigo
// cerca llegan a esta funcion (en el bucle principal); se le manda con linkSendGame
typedef void (*LinkGameFn)(uint8_t fi, uint8_t type, const uint8_t *body, uint16_t len);
void linkSetGameHandler(LinkGameFn fn);
bool linkSendGame(uint8_t fi, uint8_t type, const uint8_t *body, uint16_t len, bool reliable);
int linkFriendNearby(uint8_t fi);  // indice en linkNearby si esta cerca y contesta cifrado, -1 si no
bool linkFriendTakeDay(uint8_t fi, uint8_t what);  // LDAY_*: true la primera vez hoy (y lo apunta)

// pruebas (consola LINK ...)
void linkInfo();
bool linkPairTest();                         // con la placa mas cercana, solo en RAM
bool linkPingTest(uint16_t n, uint16_t len); // pings cifrados al primer amigo cerca
void linkSetVersion(uint8_t major, uint8_t minor);  // fingir otra version (solo RAM)
