#pragma once
#include <stdint.h>
#include <stddef.h>

// Enlace entre dos placas (la pagina "见面"): formato de los mensajes, reglas de version,
// duplicados, clave del emparejamiento y su maquina de estados. Sin hardware, para
// probarlo en el PC; la radio (ESP-NOW) esta en link.cpp.
//
// Un mensaje = cabecera de LINK_HDR bytes + cuerpo, todo en little-endian, como mucho
// LINK_PKT_MAX bytes (lo que cabe en un paquete de ESP-NOW v1, que tambien lee una v2):
//   0  "TP"           2  version mayor   3  version menor   4  tipo   5  flags
//   6  numero (seq)   8  jugador que lo manda (8 bytes)    16  largo del cuerpo   18  cuerpo
// Version mayor distinta: no se entienden (la pagina dice "要更新"). Menor distinta: si; lo
// que uno no conozca lo rechaza con LT_NAK, y el HELLO dice que sabe hacer cada uno (caps).

#define LINK_VER_MAJOR 1
#define LINK_VER_MINOR 1  // 1: juegos con amigos (friendmsg.h)
#define LINK_PKT_MAX 250
#define LINK_HDR 18
#define LINK_BODY_MAX (LINK_PKT_MAX - LINK_HDR)
#define LINK_NONCE 16
#define LINK_KEY 16
#define LINK_NAME 16  // el nombre del dueño (OWNER_NAME: 16 bytes como mucho)
#define LINK_CAPS 0x00000001u  // bit 0: el enlace (emparejar, ping). Los juegos añadiran bits

enum : uint8_t {
  LT_HELLO = 0x01,     // difusion: aqui estoy (sin nombre: solo el numero de jugador y el compañero)
  LT_PAIR_REQ = 0x02,  // mi niño dijo que si: mi mitad del secreto
  LT_PAIR_OK = 0x03,   // el otro tambien: su mitad
  LT_PAIR_NO = 0x04,   // no (o ahora no puedo)
  LT_CARD = 0x05,      // ya cifrado: el nombre del dueño (prueba que los dos tienen la misma clave)
  LT_PING = 0x06,      // cifrado: ¿sigues teniendo nuestra clave? (y pruebas de alcance)
  LT_PONG = 0x07,
  LT_ACK = 0x10,       // recibido (para los que llevan LF_ACKREQ)
  LT_NAK = 0x11,       // no lo hago (LNAK_*)
  LT_GAME0 = 0x20,     // 0x20..0x7E: para los juegos (aun no hay)
  LT_GAME1 = 0x7E,
  LT_BYE = 0x7F,       // difusion: me voy (cierro la pagina)
};
enum : uint8_t { LF_ACKREQ = 1, LF_RESEND = 2 };
enum : uint8_t { LNAK_TYPE = 1, LNAK_BUSY = 2, LNAK_VERSION = 3 };

struct LinkHdr {
  uint8_t major = LINK_VER_MAJOR, minor = LINK_VER_MINOR, type = 0, flags = 0;
  uint16_t seq = 0, len = 0;
  uint64_t from = 0;  // numero de jugador (aleatorio, va con la partida: ver link.cpp)
};
enum LinkErr : uint8_t { LINK_OK = 0, LINK_SHORT, LINK_MAGIC, LINK_LEN };

// mensaje entero en out (h.len = largo de body). 0 si no cabe o el cuerpo es demasiado largo
size_t linkEncode(uint8_t *out, size_t cap, const LinkHdr &h, const uint8_t *body);
// body apunta dentro de in (h.len bytes). Bytes de mas al final se ignoran
LinkErr linkDecode(const uint8_t *in, size_t n, LinkHdr &h, const uint8_t *&body);

// cuerpos
struct LinkHello {
  uint32_t caps = LINK_CAPS;
  int16_t species = 0;  // el compañero (0 = huevo o ninguno)
  uint8_t shiny = 0;
  uint16_t level = 0;
};
#define LINK_HELLO_LEN 9
size_t linkPutHello(uint8_t *b, const LinkHello &h);
bool linkGetHello(const uint8_t *b, size_t n, LinkHello &h);  // uno mas largo (version menor nueva) vale

// ping: hora de salida y relleno comprobable (para ver que llegan enteros); el pong devuelve
// la hora, cuanto llego y si el relleno estaba bien
#define LINK_PING_MIN 4
void linkFillPing(uint8_t *b, uint16_t len, uint16_t seq, uint32_t t0);
bool linkCheckPing(const uint8_t *b, uint16_t len, uint16_t seq);

void linkPut16(uint8_t *p, uint16_t v);
void linkPut32(uint8_t *p, uint32_t v);
void linkPut64(uint8_t *p, uint64_t v);
uint16_t linkGet16(const uint8_t *p);
uint32_t linkGet32(const uint8_t *p);
uint64_t linkGet64(const uint8_t *p);

// los ultimos LINK_DEDUPE numeros vistos de una placa: un reenvio (se perdio el ACK) se
// vuelve a confirmar pero no se procesa otra vez
#define LINK_DEDUPE 16
struct LinkDedupe {
  uint16_t seen[LINK_DEDUPE];
  uint8_t n = 0, pos = 0;
  bool dup(uint16_t seq);  // true si ya estaba; si no, lo apunta
  void reset() { n = pos = 0; }
};

// SHA-256 (para la clave; pequeño y sin librerias, igual en la placa y en el PC)
void linkSha256(const uint8_t *d, size_t n, uint8_t out[32]);
// clave de la pareja: la misma en las dos placas (ordena por numero de jugador)
void linkDeriveKey(uint64_t a, const uint8_t na[LINK_NONCE], uint64_t b, const uint8_t nb[LINK_NONCE],
                   uint8_t key[LINK_KEY]);
uint16_t linkPairCode(const uint8_t key[LINK_KEY]);  // 0..9999: se enseña en las dos pantallas

// ---------- emparejamiento ----------
// A dice que si a B: REQ (mitad de A). B pregunta a su niño; si: OK (mitad de B). Los dos
// tienen ya la clave (y el codigo en pantalla). Si los dos dijeron que si a la vez, los dos
// REQ valen como OK. Cada uno pasa a cifrar cuando su ultimo mensaje sin cifrar tiene ACK y
// lleva LINK_SETTLE_MS sin tener que contestar a uno del otro (si se perdio un ACK, el otro
// repite sin cifrar y aun se le entiende). Ya cifrados, cada uno manda su nombre (LT_CARD):
// listo cuando llega el del otro y el suyo tiene ACK. Asi se sabe que la clave es la misma.
enum : uint8_t { LP_IDLE = 0, LP_ASKING, LP_PROMPT, LP_KEYING, LP_CARDS, LP_DONE, LP_FAILED, LP_DECLINED };
enum : uint8_t { LPQ_PROMPT = 0, LPQ_BOTH, LPQ_AGAIN, LPQ_BUSY };  // que hacer con un REQ
enum : uint8_t { LPA_REQ = 0, LPA_OK, LPA_BUSY };                  // que mandar tras decir que si
#define LINK_ASK_MS 30000UL    // lo que se espera a que el otro niño conteste
#define LINK_SETTLE_MS 400UL
#define LINK_CARDS_MS 6000UL   // de tener la clave a acabar

struct LinkPair {
  uint8_t st = LP_IDLE;
  uint64_t self = 0, peer = 0;
  uint8_t mine[LINK_NONCE] = {}, theirs[LINK_NONCE] = {}, key[LINK_KEY] = {};
  bool haveTheirs = false, myAcked = false, cardIn = false, cardAcked = false, answered = false;
  uint32_t until = 0, lastAckAt = 0;

  bool busy() const { return st == LP_ASKING || st == LP_PROMPT || st == LP_KEYING || st == LP_CARDS; }
  void reset();
  // mi niño dijo que si a p: LPA_REQ (mandar REQ con mine), LPA_OK (ya me lo habia pedido el:
  // mandar OK con mine) o LPA_BUSY (ocupado con otro)
  uint8_t ask(uint64_t p, const uint8_t nonce[LINK_NONCE], uint32_t now);
  uint8_t onReq(uint64_t p, const uint8_t nonce[LINK_NONCE], uint32_t now);  // LPQ_*
  bool accept(const uint8_t nonce[LINK_NONCE], uint32_t now);                // a la pregunta: mandar OK
  bool decline();                                                            // mandar NO
  bool onOk(uint64_t p, const uint8_t nonce[LINK_NONCE], uint32_t now);      // false: repetido o no venia a cuento
  bool onNo(uint64_t p);
  void ackedMine() { if (st == LP_ASKING || st == LP_KEYING) myAcked = true; }
  void ackSent(uint32_t now) { lastAckAt = now; answered = true; }  // conteste a uno suyo sin cifrar
  bool readyToEncrypt(uint32_t now);  // KEYING -> CARDS: poner la clave y mandar CARD
  bool onCard();                      // true si acaba de quedar hecho
  bool onCardAcked();
  // caduco algo: el estado que caduco (LP_PROMPT: mandar NO; los demas acaban en LP_FAILED), o LP_IDLE
  uint8_t tick(uint32_t now);
  void fail() { if (busy()) st = LP_FAILED; }
  uint16_t code() const { return linkPairCode(key); }

 private:
  void makeKey();
  bool doneCheck();
};
