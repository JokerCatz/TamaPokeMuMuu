// Enlace entre placas (linkproto.h): formato, versiones, duplicados, clave y emparejamiento.
#include "framework.h"
#include "../linkproto.h"
#include <cstring>

static std::string hex(const uint8_t *d, size_t n) {
  static const char *H = "0123456789abcdef";
  std::string s;
  for (size_t i = 0; i < n; i++) { s += H[d[i] >> 4]; s += H[d[i] & 15]; }
  return s;
}

TEST(link, cabecera_ida_y_vuelta) {
  LinkHdr h;
  h.type = LT_PAIR_REQ;
  h.flags = LF_ACKREQ;
  h.seq = 0xBEEF;
  h.from = 0x0123456789ABCDEFull;
  uint8_t body[LINK_NONCE];
  for (int i = 0; i < LINK_NONCE; i++) body[i] = (uint8_t)i;
  h.len = sizeof(body);
  uint8_t buf[LINK_PKT_MAX];
  size_t n = linkEncode(buf, sizeof(buf), h, body);
  CHECK_EQ(n, (size_t)(LINK_HDR + LINK_NONCE));
  CHECK(buf[0] == 'T' && buf[1] == 'P');
  CHECK_EQ(buf[6], (uint8_t)0xEF);  // little-endian
  LinkHdr g;
  const uint8_t *b = nullptr;
  CHECK_EQ((int)linkDecode(buf, n, g, b), (int)LINK_OK);
  CHECK_EQ(g.major, (uint8_t)LINK_VER_MAJOR);
  CHECK_EQ(g.minor, (uint8_t)LINK_VER_MINOR);
  CHECK_EQ(g.type, (uint8_t)LT_PAIR_REQ);
  CHECK_EQ(g.flags, (uint8_t)LF_ACKREQ);
  CHECK_EQ(g.seq, (uint16_t)0xBEEF);
  CHECK(g.from == 0x0123456789ABCDEFull);
  CHECK_EQ(g.len, (uint16_t)LINK_NONCE);
  CHECK(memcmp(b, body, LINK_NONCE) == 0);
}

TEST(link, cabe_en_un_paquete_v1) {
  uint8_t body[LINK_BODY_MAX + 1] = {};
  uint8_t buf[LINK_PKT_MAX + 8];
  LinkHdr h;
  h.len = LINK_BODY_MAX;
  CHECK_EQ(linkEncode(buf, sizeof(buf), h, body), (size_t)LINK_PKT_MAX);
  CHECK_EQ(LINK_PKT_MAX, 250);  // ESP_NOW_MAX_DATA_LEN (v1)
  h.len = LINK_BODY_MAX + 1;
  CHECK_EQ(linkEncode(buf, sizeof(buf), h, body), (size_t)0);  // demasiado largo
  h.len = 10;
  CHECK_EQ(linkEncode(buf, LINK_HDR + 9, h, body), (size_t)0);  // no cabe en el sitio dado
}

TEST(link, se_rechaza_lo_que_no_es_nuestro) {
  uint8_t buf[LINK_PKT_MAX];
  LinkHdr h, g;
  const uint8_t *b;
  h.len = 4;
  uint8_t body[4] = { 1, 2, 3, 4 };
  size_t n = linkEncode(buf, sizeof(buf), h, body);
  CHECK_EQ((int)linkDecode(buf, LINK_HDR - 1, g, b), (int)LINK_SHORT);
  CHECK_EQ((int)linkDecode(buf, n - 1, g, b), (int)LINK_LEN);  // cortado
  buf[16] = 0xFF;  // largo imposible
  CHECK_EQ((int)linkDecode(buf, n, g, b), (int)LINK_LEN);
  buf[16] = 4;
  buf[0] = 'X';
  CHECK_EQ((int)linkDecode(buf, n, g, b), (int)LINK_MAGIC);
  buf[0] = 'T';
  uint8_t largo[LINK_PKT_MAX];
  memcpy(largo, buf, n);
  CHECK_EQ((int)linkDecode(largo, n + 3, g, b), (int)LINK_OK);  // bytes de mas: se ignoran
}

TEST(link, una_version_mayor_distinta_se_ve) {
  uint8_t buf[LINK_PKT_MAX];
  LinkHdr h, g;
  const uint8_t *b;
  h.major = LINK_VER_MAJOR + 1;
  h.minor = 7;
  size_t n = linkEncode(buf, sizeof(buf), h, nullptr);
  CHECK_EQ((int)linkDecode(buf, n, g, b), (int)LINK_OK);  // se lee igual: la placa decide
  CHECK_EQ(g.major, (uint8_t)(LINK_VER_MAJOR + 1));
  CHECK_EQ(g.minor, (uint8_t)7);
}

TEST(link, hello_y_uno_mas_largo_de_una_version_nueva) {
  LinkHello h;
  h.caps = 0x80000001u;
  h.species = 1025;
  h.shiny = 1;
  h.level = 999;
  uint8_t b[LINK_HELLO_LEN + 5] = {};
  CHECK_EQ(linkPutHello(b, h), (size_t)LINK_HELLO_LEN);
  LinkHello g;
  CHECK(linkGetHello(b, LINK_HELLO_LEN + 5, g));  // campos nuevos al final: se ignoran
  CHECK_EQ(g.caps, 0x80000001u);
  CHECK_EQ(g.species, (int16_t)1025);
  CHECK_EQ(g.shiny, (uint8_t)1);
  CHECK_EQ(g.level, (uint16_t)999);
  CHECK(!linkGetHello(b, LINK_HELLO_LEN - 1, g));
}

TEST(link, el_ping_detecta_bytes_cambiados) {
  uint8_t b[LINK_BODY_MAX];
  linkFillPing(b, sizeof(b), 77, 123456);
  CHECK_EQ(linkGet32(b), (uint32_t)123456);
  CHECK(linkCheckPing(b, sizeof(b), 77));
  CHECK(!linkCheckPing(b, sizeof(b), 78));  // otro numero: otro relleno
  b[100] ^= 1;
  CHECK(!linkCheckPing(b, sizeof(b), 77));
  CHECK(!linkCheckPing(b, 3, 77));
}

TEST(link, un_reenvio_se_reconoce) {
  LinkDedupe d;
  CHECK(!d.dup(5));
  CHECK(d.dup(5));
  for (uint16_t s = 100; s < 100 + LINK_DEDUPE - 1; s++) CHECK(!d.dup(s));
  CHECK(d.dup(5));    // aun en la ventana
  CHECK(!d.dup(200)); // la llena: el 5 sale
  CHECK(!d.dup(5));
  d.reset();
  CHECK(!d.dup(100));
}

TEST(link, sha256_vectores_conocidos) {
  uint8_t h[32];
  linkSha256((const uint8_t *)"abc", 3, h);
  CHECK(hex(h, 32) == "ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad");
  linkSha256((const uint8_t *)"", 0, h);
  CHECK(hex(h, 32) == "e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855");
  const char *m = "abcdbcdecdefdefgefghfghighijhijkijkljklmklmnlmnomnopnopq";  // 56 bytes: dos bloques
  linkSha256((const uint8_t *)m, strlen(m), h);
  CHECK(hex(h, 32) == "248d6a61d20638b8e5c026930c3e6039a33ce45964ff2167f6ecedd419db06c1");
  uint8_t mil[1000];
  memset(mil, 'a', sizeof(mil));
  linkSha256(mil, sizeof(mil), h);
  CHECK(hex(h, 32) == "41edece42d63e8d9bf515a9ba6932e1c20cbc9f5a5d134645adb5db1b9737ea3");
}

static void nonce(uint8_t *n, uint8_t v) { for (int i = 0; i < LINK_NONCE; i++) n[i] = (uint8_t)(v + i); }

TEST(link, las_dos_placas_sacan_la_misma_clave) {
  uint8_t na[LINK_NONCE], nb[LINK_NONCE], k1[LINK_KEY], k2[LINK_KEY], k3[LINK_KEY];
  nonce(na, 10);
  nonce(nb, 90);
  linkDeriveKey(111, na, 222, nb, k1);
  linkDeriveKey(222, nb, 111, na, k2);
  CHECK(memcmp(k1, k2, LINK_KEY) == 0);
  CHECK_EQ(linkPairCode(k1), linkPairCode(k2));
  CHECK(linkPairCode(k1) < 10000);
  nb[0] ^= 1;  // otra mitad: otra clave
  linkDeriveKey(111, na, 222, nb, k3);
  CHECK(memcmp(k1, k3, LINK_KEY) != 0);
}

// dos maquinas de emparejamiento que se hablan a mano (lo que haria la radio)
struct Two {
  LinkPair a, b;
  uint8_t na[LINK_NONCE], nb[LINK_NONCE];
  Two() {
    a.self = 0xA;
    b.self = 0xB;
    nonce(na, 1);
    nonce(nb, 50);
  }
};

TEST(link, emparejar_normal) {
  Two t;
  CHECK_EQ(t.a.ask(0xB, t.na, 0), (uint8_t)LPA_REQ);       // A: si a B
  CHECK_EQ(t.b.onReq(0xA, t.na, 10), (uint8_t)LPQ_PROMPT); // B: le pregunta a su niño
  t.b.ackSent(10);                                         // B confirma el REQ
  t.a.ackedMine();
  CHECK(t.b.accept(t.nb, 3000));                           // el niño de B: si
  CHECK_EQ(t.b.st, (uint8_t)LP_KEYING);
  CHECK(!t.b.readyToEncrypt(3001));                        // su OK aun sin ACK
  CHECK(t.a.onOk(0xB, t.nb, 3010));
  t.a.ackSent(3010);
  t.b.ackedMine();
  CHECK_EQ(t.a.code(), t.b.code());                        // mismo codigo en las dos pantallas
  CHECK(memcmp(t.a.key, t.b.key, LINK_KEY) == 0);
  CHECK(t.b.readyToEncrypt(3012));                         // B: ya
  CHECK(!t.a.readyToEncrypt(3100));                        // A espera por si B repite el OK
  CHECK(t.a.readyToEncrypt(3010 + LINK_SETTLE_MS));
  CHECK(!t.a.onCard());                                    // llega el de B; el suyo sin ACK
  CHECK(t.a.onCardAcked());
  CHECK_EQ(t.a.st, (uint8_t)LP_DONE);
  CHECK(!t.b.onCardAcked());
  CHECK(t.b.onCard());
  CHECK_EQ(t.b.st, (uint8_t)LP_DONE);
}

TEST(link, emparejar_a_la_vez) {
  Two t;
  CHECK_EQ(t.a.ask(0xB, t.na, 0), (uint8_t)LPA_REQ);
  CHECK_EQ(t.b.ask(0xA, t.nb, 5), (uint8_t)LPA_REQ);
  CHECK_EQ(t.a.onReq(0xB, t.nb, 20), (uint8_t)LPQ_BOTH);  // sin preguntar otra vez
  CHECK_EQ(t.b.onReq(0xA, t.na, 22), (uint8_t)LPQ_BOTH);
  t.a.ackSent(20);
  t.b.ackSent(22);
  t.a.ackedMine();
  t.b.ackedMine();
  CHECK(memcmp(t.a.key, t.b.key, LINK_KEY) == 0);
  CHECK(t.a.readyToEncrypt(20 + LINK_SETTLE_MS));
  CHECK(t.b.readyToEncrypt(22 + LINK_SETTLE_MS));
}

TEST(link, decir_que_si_a_quien_ya_lo_pedia_es_aceptar) {
  Two t;
  t.a.ask(0xB, t.na, 0);
  CHECK_EQ(t.b.onReq(0xA, t.na, 10), (uint8_t)LPQ_PROMPT);
  CHECK_EQ(t.b.ask(0xA, t.nb, 500), (uint8_t)LPA_OK);  // B toca a A en la lista: es un si
  CHECK_EQ(t.b.st, (uint8_t)LP_KEYING);
}

TEST(link, si_se_pierde_el_ack_del_ok_aun_se_entienden) {
  Two t;
  t.a.ask(0xB, t.na, 0);
  t.b.onReq(0xA, t.na, 10);
  t.b.ackSent(10);
  t.a.ackedMine();
  t.b.accept(t.nb, 2000);
  t.a.onOk(0xB, t.nb, 2005);
  t.a.ackSent(2005);                      // este ACK se pierde
  CHECK(!t.b.readyToEncrypt(2200));       // B no cifra
  CHECK(!t.a.onOk(0xB, t.nb, 2205));      // B repite el OK: no es nuevo...
  t.a.ackSent(2205);                      // ...pero se vuelve a confirmar, aun sin cifrar
  CHECK(!t.a.readyToEncrypt(2205 + LINK_SETTLE_MS - 1));
  t.b.ackedMine();
  CHECK(t.b.readyToEncrypt(2210));
  CHECK(t.a.readyToEncrypt(2205 + LINK_SETTLE_MS));
}

TEST(link, un_no_y_ocupado) {
  Two t;
  t.a.ask(0xB, t.na, 0);
  t.b.onReq(0xA, t.na, 10);
  CHECK(t.b.decline());
  CHECK_EQ(t.b.st, (uint8_t)LP_IDLE);
  CHECK(t.a.onNo(0xB));
  CHECK_EQ(t.a.st, (uint8_t)LP_DECLINED);
  CHECK(!t.a.onNo(0xC));                  // de otro: no cuenta
  LinkPair c;
  c.self = 0xC;
  t.b.onReq(0xA, t.na, 20);               // B: pregunta de A pendiente
  CHECK_EQ(t.b.onReq(0xC, t.na, 30), (uint8_t)LPQ_BUSY);  // C a la vez: ocupado
  CHECK_EQ(t.b.onReq(0xA, t.na, 40), (uint8_t)LPQ_AGAIN); // A repite
  CHECK_EQ(t.b.ask(0xC, t.nb, 50), (uint8_t)LPA_BUSY);
}

TEST(link, se_acaba_el_tiempo) {
  Two t;
  t.a.ask(0xB, t.na, 1000);
  CHECK_EQ(t.a.tick(1000 + LINK_ASK_MS - 1), (uint8_t)LP_IDLE);
  CHECK_EQ(t.a.tick(1000 + LINK_ASK_MS), (uint8_t)LP_ASKING);
  CHECK_EQ(t.a.st, (uint8_t)LP_FAILED);
  t.b.onReq(0xA, t.na, 0);
  CHECK_EQ(t.b.tick(LINK_ASK_MS), (uint8_t)LP_PROMPT);  // nadie contesto: se manda un NO
  CHECK_EQ(t.b.st, (uint8_t)LP_IDLE);
  // con la clave, pero sin llegar a cifrar (el ACK no llega nunca)
  Two u;
  u.a.ask(0xB, u.na, 0);
  u.b.onReq(0xA, u.na, 0);
  u.b.accept(u.nb, 100);
  CHECK_EQ(u.b.tick(100 + LINK_CARDS_MS), (uint8_t)LP_KEYING);
  CHECK_EQ(u.b.st, (uint8_t)LP_FAILED);
}

TEST(link, el_reloj_que_da_la_vuelta_no_rompe_los_plazos) {
  Two t;
  uint32_t casi = 0xFFFFFFFFu - 1000;
  t.a.ask(0xB, t.na, casi);
  CHECK_EQ(t.a.tick(casi + 2000), (uint8_t)LP_IDLE);  // millis() dio la vuelta: aun no
  CHECK_EQ(t.a.tick(casi + LINK_ASK_MS), (uint8_t)LP_ASKING);
}
