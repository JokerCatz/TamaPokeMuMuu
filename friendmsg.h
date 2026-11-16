#pragma once
#include <stdint.h>
#include <stddef.h>

// Juegos con un amigo (friend_play.ino) por el enlace de "见面" (link.h): los mensajes y las
// reglas que no necesitan la placa, para probarlas en el PC. Van cifrados y solo entre amigos
// que se oyen; tipos 0x20.. de linkproto.h. Nada de esto da puntos.
//
// Quien empieza algo (una carrera, un gimnasio) es el que manda: sortea y calcula, y el otro
// solo enseña lo que le llega. Asi las dos pantallas ven lo mismo.

// lo que sabe hacer cada placa (HELLO.caps): asi una puede ir por delante de la otra
enum : uint32_t {
  CAP_LINK = 1u << 0, CAP_PLAY = 1u << 1, CAP_GIFT = 1u << 2, CAP_RACE = 1u << 3, CAP_GYM = 1u << 4,
};
#define FRIEND_CAPS (CAP_LINK | CAP_PLAY | CAP_GIFT | CAP_RACE | CAP_GYM)

enum : uint8_t {
  FM_DEX = 0x20,         // lo que he criado (la pokedex en bits): para su "visto por un amigo"
  FM_PLAY = 0x21,        // vamos a jugar juntos (los dos a la pantalla de jugar)
  FM_TAP = 0x22,         // toque a mi compañero mientras se juega: dos a la vez = chocar los cinco
  FM_GIFT = 0x23,        // te regalo esto (txid, objeto)
  FM_GIFT_ANS = 0x24,    // lo cojo / no
  FM_INVITE = 0x25,      // ¿vienes? (carrera o gimnasio)
  FM_INVITE_ANS = 0x26,  // si (con mi compañero como atleta) / no
  FM_RACE = 0x27,        // la carrera ya decidida
  FM_ROUND = 0x28,       // una prueba del gimnasio ya decidida
  FM_GYM_END = 0x29,     // como acabo el gimnasio
  FM_CANCEL = 0x2A,      // lo dejo (el que invito se fue)
};
enum : uint8_t { FK_RACE = 1, FK_GYM = 2 };
enum : uint8_t { FW_OK = 0, FW_NO, FW_BUSY, FW_CANT };  // respuesta a una invitacion

#define FRIEND_TAP_MS 1000UL     // chocar los cinco: los dos toques con menos de esto
#define FRIEND_ASK_MS 30000UL    // lo que se espera a que el otro conteste (regalo, invitacion)
#define FRIEND_PLAY_MS 18000UL   // lo que dura jugar juntos (tres juegos de 6 s: correr, pelota, bailar)

// un compañero como atleta (sport.h Athlete) y como se ve
struct FAthlete {
  int16_t dex = 0;
  uint8_t gA = 100, gD = 100, gS = 100, tA = 0, tD = 0, tS = 0;
  uint8_t mega = 0, shiny = 0;
};
#define F_ATH_LEN 10

struct FInvite {
  uint8_t kind = 0;  // FK_*
  uint8_t a = 0;     // carrera: la prueba; gimnasio: la region
  uint8_t b = 0;     // gimnasio: 0..7 (8 = la liga)
};
#define F_INVITE_LEN 3

struct FInviteAns {
  uint8_t kind = 0, answer = FW_NO;
  FAthlete ath;  // con FW_OK
};
#define F_INVITE_ANS_LEN (2 + F_ATH_LEN)

struct FRace {  // en el orden del que invita: 0 el, 1 el invitado, 2 el rival
  uint8_t ev = 0;
  int16_t dex[3] = { 0, 0, 0 };
  uint8_t shiny[3] = { 0, 0, 0 };
  float marks[3] = { 0, 0, 0 };
};
#define F_RACE_LEN 22

struct FRound {  // una prueba del gimnasio: el equipo (los dos amigos) contra el lider
  uint8_t round = 0, ev = 0, wins = 0, lost = 0;  // marcador antes de esta prueba
  uint8_t league = 0, member = 0;                 // liga: 0..4 a quien se reta; gimnasio: 0
  uint8_t region = 0, gym = 0;                    // gym 8 = la liga
  int16_t mineDex = 0, foeDex = 0;
  uint8_t mineShiny = 0, mineHelper = 0;          // mineHelper: lo hace el compañero del invitado
  float marks[2] = { 0, 0 };                      // 0 el equipo, 1 el lider
};
#define F_ROUND_LEN 22

struct FGymEnd {
  uint8_t won = 0, region = 0, gym = 0, member = 0;
};
#define F_GYM_END_LEN 4

size_t fPutAth(uint8_t *b, const FAthlete &a);
bool fGetAth(const uint8_t *b, size_t n, FAthlete &a);
size_t fPutInvite(uint8_t *b, const FInvite &v);
bool fGetInvite(const uint8_t *b, size_t n, FInvite &v);
size_t fPutInviteAns(uint8_t *b, const FInviteAns &v);
bool fGetInviteAns(const uint8_t *b, size_t n, FInviteAns &v);
size_t fPutRace(uint8_t *b, const FRace &r);
bool fGetRace(const uint8_t *b, size_t n, FRace &r);
size_t fPutRound(uint8_t *b, const FRound &r);
bool fGetRound(const uint8_t *b, size_t n, FRound &r);
size_t fPutGymEnd(uint8_t *b, const FGymEnd &e);
bool fGetGymEnd(const uint8_t *b, size_t n, FGymEnd &e);
size_t fPutGift(uint8_t *b, uint32_t tx, uint8_t item);  // 5 bytes (y la respuesta: tx, si/no)
bool fGetGift(const uint8_t *b, size_t n, uint32_t &tx, uint8_t &item);

// chocar los cinco: cada placa apunta su toque y el del otro (al llegar); si estan a menos de
// FRIEND_TAP_MS, choca (una vez por pareja de toques)
struct HiFive {
  uint32_t mine = 0, theirs = 0;
  bool haveMine = false, haveTheirs = false;
  bool tapMine(uint32_t now);    // true: acaba de chocar
  bool tapTheirs(uint32_t now);
  void reset() { haveMine = haveTheirs = false; }
 private:
  bool check();
};

// regalos: el que regala lo saca de la mochila al ofrecerlo (asi no se usa mientras) y lo
// recupera si le dicen que no o no contestan; el que recibe apunta los txid ya cogidos para
// no cogerlo dos veces si la respuesta se perdio y el otro repite
struct GiftOut {
  bool on = false;
  uint32_t tx = 0, until = 0;
  uint8_t item = 0;
  void start(uint32_t t, uint8_t it, uint32_t now) { on = true; tx = t; item = it; until = now + FRIEND_ASK_MS; }
  // llega la respuesta: true si era la nuestra (si dice que no, quien llama devuelve item)
  bool answer(uint32_t t) { if (!on || t != tx) return false; on = false; return true; }
  bool expired(uint32_t now) const { return on && (int32_t)(now - until) >= 0; }
};
#define GIFT_SEEN 8
struct GiftSeen {
  uint32_t tx[GIFT_SEEN] = {};
  uint8_t n = 0, pos = 0;
  bool seen(uint32_t t) const;
  void add(uint32_t t);
};
