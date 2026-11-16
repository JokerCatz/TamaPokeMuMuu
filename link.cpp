#include "link.h"
#include "wifisync.h"
#include "rtcbat.h"
#include "config.h"
#include "friendmsg.h"  // FRIEND_CAPS
#include <WiFi.h>
#include <esp_wifi.h>
#include <esp_now.h>
#include <esp_mac.h>
#include <Preferences.h>

// Ver link.h. Los paquetes llegan en la tarea del Wi-Fi (onRecv): se copian a una cola y se
// procesan en el bucle principal (linkLoop), que es quien toca el estado y la pantalla.

#define LINK_HELLO_MS 300       // cada cuanto se dice "aqui estoy" (+ hasta 100 ms al azar)
#define LINK_RETRY_MS 200       // sin ACK en este tiempo: se repite
#define LINK_TRIES 3            // repeticiones (4 envios en total)
#define LINK_CARD_TRIES 10      // el nombre ya cifrado: el otro puede tardar LINK_SETTLE_MS en cifrar
// la peticion: si el otro aun nos tiene cifrados (perdimos su clave), no la oye hasta que ve que no
// contestamos cifrado (~4,5 s, LINK_KEEP_MS x LINK_CHECKS): se repite mas tiempo que eso
#define LINK_REQ_TRIES 30
#define LINK_CHECK_MS 600       // ping de comprobacion a un amigo
#define LINK_CHECKS 3           // sin respuesta tras estos: no entiende nuestra clave
#define LINK_KEEP_MS 1500       // a un amigo que ya contesto: un ping de vez en cuando (sigue ahi y con la clave)
#define LINK_RXQ 8
#define LINK_OUTS 3
#define LINK_REC_VER 2  // 2: + playDay/photoDay

// misma en todas las placas: ESP-NOW cifra con ella las claves de cada pareja (16 bytes)
static const uint8_t PMK[16] = { 'T', 'a', 'm', 'a', 'P', 'o', 'k', 'e', 'L', 'i', 'n', 'k', 'P', 'M', 'K', '1' };
static const uint8_t BCAST[6] = { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF };

struct RxItem {
  uint8_t mac[6];
  int8_t rssi;
  uint8_t len;
  uint8_t data[LINK_PKT_MAX];
};
struct Out {  // mensaje que espera ACK
  bool on;
  uint8_t mac[6];
  uint64_t to;
  uint16_t seq;
  uint8_t type, tries, maxTries;
  uint32_t next;
  uint8_t len;
  uint8_t buf[LINK_PKT_MAX];
};

static bool active = false;
static QueueHandle_t rxQ = nullptr;
static volatile uint32_t rxDrops = 0;
static uint64_t self = 0;
static uint8_t myMajor = LINK_VER_MAJOR, myMinor = LINK_VER_MINOR;
static uint16_t seqNext = 0;
static uint32_t helloAt = 0, lastTouch = 0, idleMax = LINK_IDLE_MS;
static LinkHello me;
static LinkPeer near[LINK_NEARBY];
static uint8_t nearN = 0;
static LinkFriend fr[LINK_FRIENDS];
static bool frRam[LINK_FRIENDS], frSeen[LINK_FRIENDS];
static uint8_t frN = 0;
static Out outs[LINK_OUTS];
static LinkPair pair;
static uint8_t pairMac[6];
static char pairName[LINK_NAME + 1];
static bool pairRam = false;  // LINK PAIRTEST: el amigo nuevo solo en RAM
static bool sameIdWarned = false;
static LinkGameFn gameFn = nullptr;
// prueba de pings (LINK PING)
static struct {
  uint16_t n = 0, len = 0, sent = 0, got = 0, bad = 0;
  uint16_t seq = 0;
  bool waiting = false;
  uint32_t at = 0, rttMin = 0, rttMax = 0, rttSum = 0;
  int8_t fi = -1;
} pt;

// ---------- utilidades ----------

static void macStr(const uint8_t *m, char *s) {
  snprintf(s, 18, "%02X:%02X:%02X:%02X:%02X:%02X", m[0], m[1], m[2], m[3], m[4], m[5]);
}

static void onRecv(const esp_now_recv_info_t *info, const uint8_t *data, int len) {
  if (!rxQ || len <= 0 || len > LINK_PKT_MAX) return;
  RxItem it;
  memcpy(it.mac, info->src_addr, 6);
  it.rssi = info->rx_ctrl ? (int8_t)info->rx_ctrl->rssi : -127;
  it.len = (uint8_t)len;
  memcpy(it.data, data, len);
  if (xQueueSend(rxQ, &it, 0) != pdTRUE) rxDrops = rxDrops + 1;
}

// el par de ESP-NOW de esa MAC, cifrado con key (nullptr = sin cifrar)
static bool setPeer(const uint8_t mac[6], const uint8_t *key) {
  esp_now_peer_info_t p = {};
  memcpy(p.peer_addr, mac, 6);
  p.channel = 0;  // el canal en el que esta la radio (LINK_CHANNEL)
  p.ifidx = WIFI_IF_STA;
  p.encrypt = key != nullptr;
  if (key) memcpy(p.lmk, key, LINK_KEY);
  esp_err_t e = esp_now_is_peer_exist(mac) ? esp_now_mod_peer(&p) : esp_now_add_peer(&p);
  if (e != ESP_OK) Serial.printf("link: par %s fallo (%d)\n", key ? "cifrado" : "", (int)e);
  return e == ESP_OK;
}

static bool rawSend(const uint8_t mac[6], const uint8_t *buf, size_t n) {
  if (memcmp(mac, BCAST, 6) && !esp_now_is_peer_exist(mac) && !setPeer(mac, nullptr)) return false;
  return esp_now_send(mac, buf, n) == ESP_OK;
}

static bool sendMsg(const uint8_t mac[6], uint8_t type, const uint8_t *body, uint16_t len, uint8_t flags = 0,
                    uint16_t *seqOut = nullptr) {
  LinkHdr h;
  h.major = myMajor;
  h.minor = myMinor;
  h.type = type;
  h.flags = flags;
  h.seq = seqNext++;
  h.len = len;
  h.from = self;
  uint8_t buf[LINK_PKT_MAX];
  size_t n = linkEncode(buf, sizeof(buf), h, body);
  if (!n) return false;
  if (seqOut) *seqOut = h.seq;
  if (flags & LF_ACKREQ) {  // se guarda para repetirlo
    Out *o = nullptr;
    for (auto &x : outs)
      if (!x.on) { o = &x; break; }
    if (!o) {
      Serial.println("link: cola de envio llena");
      return false;
    }
    o->on = true;
    memcpy(o->mac, mac, 6);
    o->to = 0;
    o->seq = h.seq;
    o->type = type;
    o->tries = 0;
    o->maxTries = type == LT_CARD ? LINK_CARD_TRIES : type == LT_PAIR_REQ ? LINK_REQ_TRIES : LINK_TRIES;
    o->next = millis() + LINK_RETRY_MS;
    o->len = (uint8_t)n;
    memcpy(o->buf, buf, n);
  }
  return rawSend(mac, buf, n);
}

static bool sendReliable(const uint8_t mac[6], uint64_t to, uint8_t type, const uint8_t *body, uint16_t len) {
  uint16_t seq;
  if (!sendMsg(mac, type, body, len, LF_ACKREQ, &seq)) return false;
  for (auto &o : outs)
    if (o.on && o.seq == seq) o.to = to;
  return true;
}

static void sendAck(const uint8_t mac[6], uint16_t seq) {
  uint8_t b[2];
  linkPut16(b, seq);
  sendMsg(mac, LT_ACK, b, 2);
}

static void sendNak(const uint8_t mac[6], uint16_t seq, uint8_t why) {
  uint8_t b[3];
  linkPut16(b, seq);
  b[2] = why;
  sendMsg(mac, LT_NAK, b, 3);
}

static void newNonce(uint8_t n[LINK_NONCE]) {  // con la radio encendida esp_random es aleatorio de verdad
  for (int i = 0; i < LINK_NONCE; i += 4) linkPut32(n + i, esp_random());
}

static int nearOf(uint64_t pid) {
  for (int i = 0; i < nearN; i++)
    if (near[i].pid == pid) return i;
  return -1;
}

static int friendOf(uint64_t pid) {
  for (int i = 0; i < frN; i++)
    if (fr[i].pid == pid) return i;
  return -1;
}

static void dropNear(int i) {
  if (memcmp(near[i].mac, BCAST, 6)) esp_now_del_peer(near[i].mac);
  near[i] = near[--nearN];
}

// ---------- amigos en la NVS ----------

static void frKey(uint8_t i, char *k) { snprintf(k, 4, "f%u", i); }

static void loadFriends() {
  Preferences p;
  p.begin("link", true);
  frN = 0;
  for (uint8_t i = 0; i < LINK_FRIENDS; i++) {
    char k[4];
    frKey(i, k);
    LinkFriend f;
    if (p.getBytes(k, &f, sizeof(f)) != sizeof(f) || f.ver != LINK_REC_VER || !f.pid) continue;
    f.name[LINK_NAME] = 0;
    fr[frN] = f;
    frRam[frN] = frSeen[frN] = false;
    frN++;
  }
  p.end();
}

// reescribe todos (son 4 como mucho): asi un hueco borrado no deja nada atras
static void saveFriends() {
  Preferences p;
  p.begin("link", false);
  uint8_t k = 0;
  for (uint8_t i = 0; i < frN; i++) {
    if (frRam[i]) continue;
    char key[4];
    frKey(k++, key);
    p.putBytes(key, &fr[i], sizeof(LinkFriend));
  }
  for (; k < LINK_FRIENDS; k++) {
    char key[4];
    frKey(k, key);
    p.remove(key);
  }
  p.end();
}

static uint64_t loadSelf() {
  Preferences p;
  p.begin("link", false);
  uint64_t id = p.getULong64("pid", 0);
  if (!id) {
    while (!id) id = ((uint64_t)esp_random() << 32) | esp_random();
    p.putULong64("pid", id);
    Serial.println("link: numero de jugador nuevo");
  }
  p.end();
  return id;
}

// ---------- arrancar / parar ----------

bool linkStart(uint32_t idleMs) {
  idleMax = idleMs;
  lastTouch = millis();
  if (active) return true;
  netRadioTake();  // se paran Stack-chan, el servidor del movil y la red de casa
  WiFi.setSleep(false);  // sin ahorro: si no, la radio duerme y se pierden paquetes
  WiFi.mode(WIFI_STA);
  WiFi.disconnect(false, true);  // nada de conectarse a casa mientras tanto
  if (esp_wifi_set_channel(LINK_CHANNEL, WIFI_SECOND_CHAN_NONE) != ESP_OK) Serial.println("link: no se pudo fijar el canal");
  if (esp_now_init() != ESP_OK) {
    Serial.println("link: esp_now_init fallo");
    WiFi.setSleep(true);
    WiFi.mode(WIFI_OFF);
    netRadioGive();
    return false;
  }
  esp_now_set_pmk(PMK);
  rxQ = xQueueCreate(LINK_RXQ, sizeof(RxItem));
  if (!rxQ) {
    Serial.println("link: sin memoria para la cola");
    esp_now_deinit();
    WiFi.setSleep(true);
    WiFi.mode(WIFI_OFF);
    netRadioGive();
    return false;
  }
  esp_now_register_recv_cb(onRecv);
  setPeer(BCAST, nullptr);
  self = loadSelf();
  pair.reset();
  pair.self = self;
  loadFriends();
  nearN = 0;
  memset(outs, 0, sizeof(outs));
  seqNext = (uint16_t)esp_random();
  helloAt = 0;
  pt.n = 0;
  pairRam = false;
  active = true;
  uint8_t mac[6];
  WiFi.macAddress(mac);
  char ms[18];
  macStr(mac, ms);
  Serial.printf("link: activo en el canal %u, %s, version %u.%u, %u amigos\n", LINK_CHANNEL, ms, myMajor, myMinor,
                frN);
  return true;
}

void linkStop() {
  if (!active) return;
  sendMsg(BCAST, LT_BYE, nullptr, 0);
  delay(20);
  // los amigos vistos: su ultima MAC y cuando (una escritura, al salir: la NVS congela ~1 s)
  bool cambio = false;
  uint32_t ahora = rtcEpoch();
  for (uint8_t i = 0; i < frN; i++)
    if (frSeen[i] && !frRam[i]) {
      fr[i].seenAt = ahora;
      cambio = true;
    }
  if (cambio) saveFriends();
  esp_now_unregister_recv_cb();
  esp_now_deinit();
  if (rxQ) {  // ~2 KB de RAM interna: solo mientras esta abierta
    QueueHandle_t q = rxQ;
    rxQ = nullptr;
    vQueueDelete(q);
  }
  active = false;
  nearN = 0;
  WiFi.setSleep(true);  // lo de siempre para la red de casa
  WiFi.mode(WIFI_OFF);
  netRadioGive();
  Serial.printf("link: parado (%lu paquetes perdidos en la cola)\n", (unsigned long)rxDrops);
  rxDrops = 0;
}

bool linkActive() { return active; }
void linkTouch() { lastTouch = millis(); }
uint64_t linkSelfId() { return self; }

void linkSetSelf(int16_t species, bool shiny, uint16_t level) {
  me.species = species;
  me.shiny = shiny;
  me.level = level;
}

// ---------- emparejar ----------

static void pairFailed() {
  int i = nearOf(pair.peer);
  if (i >= 0 && near[i].fr < 0) setPeer(near[i].mac, nullptr);  // vuelve a hablarle sin cifrar
  for (auto &o : outs)
    if (o.on && o.to == pair.peer) o.on = false;
  pairRam = false;
  Serial.println("link: emparejar fallo");
}

static void pairDone() {
  int f = friendOf(pair.peer);
  if (f < 0) {
    if (frN >= LINK_FRIENDS) {  // no deberia: se mira antes de empezar
      pair.fail();
      pairFailed();
      return;
    }
    f = frN++;
  }
  LinkFriend &x = fr[f];
  memset(&x, 0, sizeof(x));
  x.ver = LINK_REC_VER;
  x.pid = pair.peer;
  memcpy(x.mac, pairMac, 6);
  memcpy(x.key, pair.key, LINK_KEY);
  strncpy(x.name, pairName, LINK_NAME);
  x.pairedAt = x.seenAt = rtcEpoch();
  frRam[f] = pairRam;
  frSeen[f] = true;
  if (!pairRam) saveFriends();
  int i = nearOf(pair.peer);
  if (i >= 0) {
    near[i].fr = (int8_t)f;
    near[i].fstate = LFS_OK;
    near[i].pings = 0;  // los pings sin respuesta de la clave vieja ya no cuentan
    near[i].pingAt = millis();
  }
  Serial.printf("link: amigos con %s, codigo %04u%s\n", x.name, pair.code(), pairRam ? " (solo RAM)" : "");
  pairRam = false;
}

static uint8_t pairAsk(uint64_t pid) {
  int i = nearOf(pid);
  if (!active || i < 0) return LINK_ASK_GONE;
  if (near[i].major != myMajor) return LINK_ASK_OLD;
  if (friendOf(pid) < 0 && frN >= LINK_FRIENDS) return LINK_ASK_FULL;
  uint8_t n[LINK_NONCE];
  newNonce(n);
  uint8_t r = pair.ask(pid, n, millis());
  if (r == LPA_BUSY) return r;
  memcpy(pairMac, near[i].mac, 6);
  pairName[0] = 0;
  setPeer(pairMac, nullptr);  // si era amigo y se perdio la clave, se empieza sin cifrar
  if (near[i].fr >= 0) near[i].fstate = LFS_LOST;
  sendReliable(pairMac, pid, r == LPA_OK ? LT_PAIR_OK : LT_PAIR_REQ, pair.mine, LINK_NONCE);
  Serial.printf("link: %s\n", r == LPA_OK ? "acepto" : "pido ser amigos");
  return r;
}

static void pairAnswer(bool yes) {
  if (pair.st != LP_PROMPT) return;
  lastTouch = millis();
  if (!yes) {
    pair.decline();
    sendMsg(pairMac, LT_PAIR_NO, nullptr, 0);
    return;
  }
  if (friendOf(pair.peer) < 0 && frN >= LINK_FRIENDS) {
    pair.decline();
    uint8_t b = LNAK_BUSY;
    sendMsg(pairMac, LT_PAIR_NO, &b, 1);
    return;
  }
  uint8_t n[LINK_NONCE];
  newNonce(n);
  if (pair.accept(n, millis())) sendReliable(pairMac, pair.peer, LT_PAIR_OK, pair.mine, LINK_NONCE);
}

// desde la pagina: el amigo se guarda (LINK PAIRTEST usa pairAsk/pairAnswer con pairRam)
uint8_t linkPairAsk(uint64_t pid) {
  pairRam = false;
  return pairAsk(pid);
}

void linkPairAnswer(bool yes) {
  pairRam = false;
  pairAnswer(yes);
}

const LinkPair &linkPairing() { return pair; }
int linkPairNearby() { return pair.st == LP_IDLE ? -1 : nearOf(pair.peer); }
const char *linkPairName() { return pairName; }
void linkPairClear() {
  if (!pair.busy()) pair.reset();
}

// ---------- recibir ----------

static void onAcked(Out &o) {
  o.on = false;
  if (o.to != pair.peer) return;
  if (o.type == LT_PAIR_REQ || o.type == LT_PAIR_OK) pair.ackedMine();
  else if (o.type == LT_CARD && pair.onCardAcked()) pairDone();
}

static void handle(const RxItem &it, uint32_t now) {
  LinkHdr h;
  const uint8_t *b;
  if (linkDecode(it.data, it.len, h, b) != LINK_OK) return;  // no es nuestro
  if (h.from == self) {
    if (!sameIdWarned) Serial.println("link: OJO, otra placa con mi numero de jugador (¿partida clonada?)");
    sameIdWarned = true;
    return;
  }
  int i = nearOf(h.from);
  if (i < 0) {
    if (nearN >= LINK_NEARBY) {  // lleno: fuera el que hace mas que no se oye (no el que se empareja)
      int viejo = -1;
      for (int k = 0; k < nearN; k++)
        if (near[k].pid != pair.peer && (viejo < 0 || near[k].heardAt < near[viejo].heardAt)) viejo = k;
      if (viejo < 0) return;
      dropNear(viejo);
    }
    i = nearN++;
    LinkPeer &n = near[i];
    memset(&n, 0, sizeof(n));
    n.pid = h.from;
    n.rssi = it.rssi;
    n.fr = (int8_t)friendOf(h.from);
    n.fstate = LFS_NONE;
  }
  LinkPeer &p = near[i];
  if (memcmp(p.mac, it.mac, 6)) {  // MAC nueva (otra placa con la misma partida, o la primera vez)
    if (p.mac[0] | p.mac[1] | p.mac[2]) esp_now_del_peer(p.mac);
    memcpy(p.mac, it.mac, 6);
    if (p.fstate != LFS_NONE) p.fstate = LFS_NONE;  // se vuelve a comprobar
  }
  p.rssi = (int8_t)((3 * p.rssi + it.rssi) / 4);
  p.heardAt = now;
  p.major = h.major;
  p.minor = h.minor;
  if (h.type == LT_HELLO) {
    LinkHello hl;
    if (linkGetHello(b, h.len, hl)) {
      p.caps = hl.caps;
      p.species = hl.species;
      p.shiny = hl.shiny;
      p.level = hl.level;
    }
    return;
  }
  if (h.type == LT_BYE) {
    if (!(pair.busy() && pair.peer == p.pid)) dropNear(i);
    return;
  }
  if (h.major != myMajor) {  // no nos entendemos: solo se le dice
    if (h.type != LT_ACK && h.type != LT_NAK) sendNak(p.mac, h.seq, LNAK_VERSION);
    return;
  }
  // un amigo que pide emparejar ya no tiene nuestra clave: se le contesta sin cifrar (tambien el ACK)
  if (h.type == LT_PAIR_REQ && p.fr >= 0 && p.fstate != LFS_LOST) {
    p.fstate = LFS_LOST;
    setPeer(p.mac, nullptr);
  }
  // se confirma lo que se va a atender (tambien un repetido: su ACK se pudo perder). El nombre
  // (ya cifrado) solo si se esta emparejando con el o ya es amigo: si no, que lo repita
  bool juego = h.type >= LT_GAME0 && h.type <= LT_GAME1 && p.fr >= 0 && gameFn;  // de un amigo: cifrado
  bool conocido = h.type == LT_PAIR_REQ || h.type == LT_PAIR_OK || h.type == LT_PAIR_NO || juego ||
                  (h.type == LT_CARD && ((pair.peer == p.pid && (pair.st == LP_CARDS || pair.st == LP_DONE)) || p.fr >= 0));
  bool rep = false;
  if ((h.flags & LF_ACKREQ) && conocido) {
    rep = p.dd.dup(h.seq);
    sendAck(p.mac, h.seq);
    if (h.type == LT_PAIR_REQ || h.type == LT_PAIR_OK) pair.ackSent(now);
  }
  switch (h.type) {
    case LT_ACK:
      if (h.len >= 2)
        for (auto &o : outs)
          if (o.on && o.to == p.pid && o.seq == linkGet16(b)) onAcked(o);
      break;
    case LT_NAK:
      if (h.len >= 3) {
        Serial.printf("link: rechazado (%u)\n", b[2]);
        for (auto &o : outs)
          if (o.on && o.to == p.pid && o.seq == linkGet16(b)) {
            o.on = false;
            if (o.to == pair.peer && pair.busy()) {
              pair.fail();
              pairFailed();
            }
          }
      }
      break;
    case LT_PAIR_REQ: {
      if (rep || h.len < LINK_NONCE) break;
      uint8_t r = pair.onReq(p.pid, b, now);
      if (r == LPQ_BUSY) {
        uint8_t why = LNAK_BUSY;
        sendMsg(p.mac, LT_PAIR_NO, &why, 1);
      } else if (r == LPQ_PROMPT) {
        memcpy(pairMac, p.mac, 6);
        pairName[0] = 0;
        Serial.println("link: otra placa quiere ser amiga (pregunta en pantalla)");
      }
      break;
    }
    case LT_PAIR_OK:
      if (!rep && h.len >= LINK_NONCE && pair.onOk(p.pid, b, now)) Serial.println("link: el otro dijo que si");
      break;
    case LT_PAIR_NO:
      if (!rep && pair.onNo(p.pid)) Serial.println("link: el otro dijo que no");
      break;
    case LT_CARD:
      if (rep || pair.peer != p.pid || pair.st != LP_CARDS) break;
      {
        uint16_t n = h.len < LINK_NAME ? h.len : LINK_NAME, k = 0;
        for (uint16_t j = 0; j < n; j++)
          if (b[j] >= 0x20 && b[j] < 0x7F) pairName[k++] = (char)b[j];  // solo ASCII imprimible
        pairName[k] = 0;
      }
      if (pair.onCard()) pairDone();
      break;
    case LT_PING:  // solo a amigos (cifrado: si llega, tenemos la misma clave)
      if (p.fr < 0 || h.len < LINK_PING_MIN) break;
      {
        uint8_t r[7];
        memcpy(r, b, 4);
        linkPut16(r + 4, h.len);
        r[6] = linkCheckPing(b, h.len, h.seq) ? 1 : 0;
        sendMsg(p.mac, LT_PONG, r, 7);
      }
      break;
    case LT_PONG:
      if (p.fr < 0 || h.len < 7) break;
      if (p.fstate != LFS_OK) Serial.printf("link: %s contesta cifrado\n", fr[p.fr].name);
      p.fstate = LFS_OK;
      p.pings = 0;
      frSeen[p.fr] = true;
      if (memcmp(fr[p.fr].mac, p.mac, 6)) memcpy(fr[p.fr].mac, p.mac, 6);  // se guarda al salir
      if (pt.waiting && pt.fi == p.fr && linkGet32(b) == pt.at) {
        uint32_t rtt = millis() - pt.at;
        pt.waiting = false;
        pt.got++;
        if (!b[6] || linkGet16(b + 4) != pt.len) pt.bad++;
        if (!pt.rttMin || rtt < pt.rttMin) pt.rttMin = rtt;
        if (rtt > pt.rttMax) pt.rttMax = rtt;
        pt.rttSum += rtt;
      }
      break;
    default:
      if (juego) {  // a los juegos (friend_play.ino); un repetido ya tiene su ACK
        if (!rep) gameFn((uint8_t)p.fr, h.type, b, h.len);
        break;
      }
      // tipos de una version menor nueva, o un juego de quien no es amigo: no se hacen
      if ((h.flags & LF_ACKREQ) || (h.type >= LT_GAME0 && h.type <= LT_GAME1)) sendNak(p.mac, h.seq, LNAK_TYPE);
      break;
  }
}

// ---------- el bucle ----------

static void sendPing(LinkPeer &p, uint16_t len) {
  uint8_t b[LINK_BODY_MAX];
  if (len < LINK_PING_MIN) len = LINK_PING_MIN;
  if (len > LINK_BODY_MAX) len = LINK_BODY_MAX;
  uint16_t seq = seqNext;  // el que va a llevar (el relleno depende de el)
  uint32_t t0 = millis();
  linkFillPing(b, len, seq, t0);
  sendMsg(p.mac, LT_PING, b, len);
  p.pingAt = t0;
  pt.at = t0;
}

static void pingTestTick(uint32_t now) {
  if (!pt.n) return;
  if (pt.waiting && (int32_t)(now - pt.at) < 300) return;
  pt.waiting = false;
  int i = -1;
  for (int k = 0; k < nearN; k++)
    if (near[k].fr == pt.fi) i = k;
  if (pt.sent >= pt.n || i < 0) {
    uint16_t lost = pt.sent - pt.got;
    Serial.printf("link: ping %u enviados, %u vuelven, %u perdidos (%u%%), %u mal; ida y vuelta %lu-%lu ms (media %lu); senal %d dBm\n",
                  pt.sent, pt.got, lost, pt.sent ? (unsigned)(lost * 100 / pt.sent) : 0, pt.bad,
                  (unsigned long)pt.rttMin, (unsigned long)pt.rttMax, (unsigned long)(pt.got ? pt.rttSum / pt.got : 0),
                  i >= 0 ? near[i].rssi : 0);
    Serial.println("DONE");
    pt.n = 0;
    return;
  }
  sendPing(near[i], pt.len);
  pt.sent++;
  pt.waiting = true;
}

void linkLoop(uint32_t now) {
  if (!active) return;
  RxItem it;
  for (int k = 0; k < LINK_RXQ && xQueueReceive(rxQ, &it, 0) == pdTRUE; k++) handle(it, now);
  // aqui estoy
  if ((int32_t)(now - helloAt) >= 0) {
    uint8_t b[LINK_HELLO_LEN];
    me.caps = FRIEND_CAPS;
    linkPutHello(b, me);
    sendMsg(BCAST, LT_HELLO, b, LINK_HELLO_LEN);
    helloAt = now + LINK_HELLO_MS + random(100);
  }
  // repetir lo que no tiene ACK
  for (auto &o : outs) {
    if (!o.on || (int32_t)(now - o.next) < 0) continue;
    if (o.tries >= o.maxTries) {
      o.on = false;
      Serial.printf("link: sin respuesta (tipo %02X)\n", o.type);
      if (o.to == pair.peer && pair.busy()) {
        pair.fail();
        pairFailed();
      }
      continue;
    }
    o.tries++;
    o.buf[5] |= LF_RESEND;
    rawSend(o.mac, o.buf, o.len);
    o.next = now + LINK_RETRY_MS;
  }
  // emparejar: plazos y paso a cifrado
  uint8_t was = pair.tick(now);
  if (was == LP_PROMPT) sendMsg(pairMac, LT_PAIR_NO, nullptr, 0);
  else if (was != LP_IDLE) pairFailed();
  if (pair.readyToEncrypt(now)) {
    setPeer(pairMac, pair.key);
    const uint8_t n = sizeof(OWNER_NAME) - 1;  // config.h: 16 como mucho (LINK_NAME)
    sendReliable(pairMac, pair.peer, LT_CARD, (const uint8_t *)OWNER_NAME, n);
    Serial.printf("link: clave puesta, codigo %04u\n", pair.code());
  }
  // los que se fueron; los amigos que llegan se comprueban (¿tienen aun nuestra clave?)
  for (int k = nearN - 1; k >= 0; k--) {
    LinkPeer &p = near[k];
    if ((int32_t)(now - p.heardAt) > (int32_t)LINK_GONE_MS && !(pair.busy() && pair.peer == p.pid)) {
      dropNear(k);
      continue;
    }
    if (p.fr < 0 || p.major != myMajor || p.fstate == LFS_LOST) continue;
    if (pair.busy() && pair.peer == p.pid) continue;
    if (p.fstate == LFS_NONE) {
      setPeer(p.mac, fr[p.fr].key);
      p.fstate = LFS_CHECKING;
      p.pings = 0;
      p.pingAt = now - LINK_CHECK_MS;
    }
    // comprobando: cada LINK_CHECK_MS; ya OK: cada LINK_KEEP_MS (el PONG tambien cuenta como "sigue
    // aqui", por si los HELLO de un par cifrado no llegaran)
    if ((int32_t)(now - p.pingAt) >= (p.fstate == LFS_OK ? LINK_KEEP_MS : LINK_CHECK_MS)) {
      if (p.pings >= LINK_CHECKS) {  // no entiende nuestra clave: hay que emparejar otra vez
        p.fstate = LFS_LOST;
        setPeer(p.mac, nullptr);
        Serial.printf("link: %s no contesta cifrado (emparejar otra vez)\n", fr[p.fr].name);
        continue;
      }
      if (pt.n) {
        p.pingAt = now;  // la prueba de pings ya pregunta
      } else {
        sendPing(p, LINK_PING_MIN);
        p.pings++;
      }
    }
  }
  pingTestTick(now);
  // sin tocar la pagina un rato (y sin nada a medias): se cierra sola. Con signo: `now` se leyo al
  // empezar la vuelta y linkStart/linkTouch pueden haber puesto lastTouch un poco despues
  if ((int32_t)(now - lastTouch) > (int32_t)idleMax && !pair.busy() && !pt.n) {
    Serial.println("link: se cierra (sin usar)");
    linkStop();
  }
}

// ---------- para la pagina ----------

uint8_t linkNearbyCount() { return nearN; }
const LinkPeer &linkNearby(uint8_t i) { return near[i]; }
bool linkPeerSameMajor(const LinkPeer &p) { return p.major == myMajor; }
uint8_t linkFriendCount() { return frN; }
const LinkFriend &linkFriendAt(uint8_t i) { return fr[i]; }
bool linkFriendRam(uint8_t i) { return i < frN && frRam[i]; }

bool linkForget(uint8_t i) {
  if (i >= frN) return false;
  bool ram = frRam[i];
  for (int k = 0; k < nearN; k++) {
    if (near[k].fr == i) {
      near[k].fr = -1;
      near[k].fstate = LFS_NONE;
      setPeer(near[k].mac, nullptr);
    } else if (near[k].fr > i) {
      near[k].fr--;
    }
  }
  for (uint8_t k = i; k + 1 < frN; k++) {
    fr[k] = fr[k + 1];
    frRam[k] = frRam[k + 1];
    frSeen[k] = frSeen[k + 1];
  }
  frN--;
  if (!ram) saveFriends();
  return true;
}

// ---------- juegos con un amigo ----------

void linkSetGameHandler(LinkGameFn fn) { gameFn = fn; }

int linkFriendNearby(uint8_t fi) {
  for (int k = 0; k < nearN; k++)
    if (near[k].fr == (int8_t)fi && near[k].fstate == LFS_OK && near[k].major == myMajor) return k;
  return -1;
}

bool linkSendGame(uint8_t fi, uint8_t type, const uint8_t *body, uint16_t len, bool reliable) {
  int k = active ? linkFriendNearby(fi) : -1;
  if (k < 0 || type < LT_GAME0 || type > LT_GAME1) return false;
  return reliable ? sendReliable(near[k].mac, near[k].pid, type, body, len) : sendMsg(near[k].mac, type, body, len);
}

bool linkFriendTakeDay(uint8_t fi, uint8_t what) {
  if (fi >= frN) return false;
  uint32_t e = rtcEpoch();
  uint16_t hoy = (uint16_t)(e / 86400);
  if (!e) return false;  // sin hora no se sabe que dia es
  uint16_t &d = what == LDAY_PLAY ? fr[fi].playDay : fr[fi].photoDay;
  if (d == hoy) return false;
  d = hoy;
  frSeen[fi] = true;  // se guarda al salir (con su ultima vez)
  return true;
}

// ---------- pruebas ----------

void linkInfo() {
  uint8_t mac[6];
  esp_read_mac(mac, ESP_MAC_WIFI_STA);  // la de fabrica (WiFi.macAddress no vale con la radio apagada)
  char ms[18];
  macStr(mac, ms);
  Serial.printf("link: %s, version %u.%u, canal %u, yo %08lX%08lX (%s), cola perdida %lu\n",
                active ? "activo" : "parado", myMajor, myMinor, LINK_CHANNEL, (unsigned long)(self >> 32),
                (unsigned long)self, ms, (unsigned long)rxDrops);
  for (int k = 0; k < nearN; k++) {
    const LinkPeer &p = near[k];
    static const char *const FS[] = { "-", "comprobando", "OK", "sin clave" };
    macStr(p.mac, ms);
    Serial.printf("  cerca %08lX %s v%u.%u %d dBm%s hace %lu ms #%d%s %s%s\n", (unsigned long)(p.pid >> 32), ms,
                  p.major, p.minor, p.rssi, linkPeerNear(p) ? "" : " (lejos)", (unsigned long)(millis() - p.heardAt),
                  p.species, p.shiny ? "*" : "", p.fr >= 0 ? "amigo " : "", p.fr >= 0 ? FS[p.fstate] : "");
  }
  for (int k = 0; k < frN; k++) {
    macStr(fr[k].mac, ms);
    Serial.printf("  amigo %d: %s %08lX %s%s\n", k, fr[k].name, (unsigned long)(fr[k].pid >> 32), ms,
                  frRam[k] ? " (solo RAM)" : "");
  }
  if (pair.st != LP_IDLE) {
    static const char *const ST[] = { "-", "pidiendo", "pregunta", "clave", "nombres", "hecho", "fallo", "dijo que no" };
    Serial.printf("  emparejar: %s con %08lX, codigo %04u\n", ST[pair.st], (unsigned long)(pair.peer >> 32),
                  pair.st >= LP_KEYING ? pair.code() : 0);
  }
}

bool linkPairTest() {
  if (!active) return false;
  if (pair.st == LP_PROMPT) {  // el otro ya lo pidio: se acepta
    pairRam = true;
    pairAnswer(true);
    return true;
  }
  int best = -1;
  for (int k = 0; k < nearN; k++)
    if (near[k].fr < 0 && near[k].major == myMajor && (best < 0 || near[k].rssi > near[best].rssi)) best = k;
  if (best < 0) return false;
  pairRam = true;
  uint8_t r = pairAsk(near[best].pid);
  return r == LPA_REQ || r == LPA_OK;
}

bool linkPingTest(uint16_t n, uint16_t len) {
  if (!active) return false;
  for (int k = 0; k < nearN; k++)
    if (near[k].fr >= 0 && near[k].fstate == LFS_OK) {
      pt = {};
      pt.n = n;
      pt.len = len < LINK_PING_MIN ? LINK_PING_MIN : len > LINK_BODY_MAX ? LINK_BODY_MAX : len;
      pt.fi = near[k].fr;
      lastTouch = millis();
      return true;
    }
  return false;
}

void linkSetVersion(uint8_t major, uint8_t minor) {
  myMajor = major;
  myMinor = minor;
  for (int k = 0; k < nearN; k++) near[k].fstate = LFS_NONE;  // se vuelve a comprobar con la version nueva
}
