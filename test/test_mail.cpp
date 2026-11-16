// Correo con amigos lejos (mail.h): mensajes, cifrado y firma, base64, reintentos y repetidos.
#include "framework.h"
#include "../mail.h"
#include "../linkproto.h"
#include <cstring>
#include <string>

static std::string hexs(const uint8_t *d, size_t n) {
  static const char *H = "0123456789abcdef";
  std::string s;
  for (size_t i = 0; i < n; i++) { s += H[d[i] >> 4]; s += H[d[i] & 15]; }
  return s;
}

TEST(mail, el_mensaje_va_y_vuelve) {
  MailMsg m;
  m.type = MT_GIFT; m.id = 0xDEADBEEF; m.from = 0x1122334455667788ull; m.species = 1025; m.shiny = 1;
  m.level = 178; m.phrase = 11; m.sticker = 7; m.item = 9; m.qty = 2; m.ackOf = 0x01020304;
  uint8_t b[MAIL_MSG_LEN];
  CHECK_EQ(mailEncode(b, m), (size_t)MAIL_MSG_LEN);
  MailMsg r;
  CHECK(mailDecode(b, sizeof(b), r));
  CHECK(r.type == MT_GIFT && r.id == 0xDEADBEEF && r.from == 0x1122334455667788ull && r.species == 1025);
  CHECK(r.shiny == 1 && r.level == 178 && r.phrase == 11 && r.sticker == 7 && r.item == 9 && r.qty == 2);
  CHECK(r.ackOf == 0x01020304u);
  b[0] = MAIL_VER + 1;
  CHECK(!mailDecode(b, sizeof(b), r));  // otra version: no
  b[0] = MAIL_VER;
  b[1] = 9;
  CHECK(!mailDecode(b, sizeof(b), r));  // tipo desconocido: no
  CHECK(!mailDecode(b, MAIL_MSG_LEN - 1, r));
}

TEST(mail, hmac_sha256_vector_rfc4231) {
  uint8_t key[20];
  memset(key, 0x0b, sizeof(key));
  uint8_t out[32];
  mailHmac(key, sizeof(key), (const uint8_t *)"Hi There", 8, out);
  CHECK(hexs(out, 32) == "b0344c61d8db38535ca8afceaf0bf12b881dc200c9833da726e9376c2e32cff7");
}

TEST(mail, sellado_se_abre_con_la_misma_clave) {
  uint8_t key[16], nonce[MAIL_NONCE];
  for (int i = 0; i < 16; i++) key[i] = (uint8_t)(i * 7 + 1);
  for (int i = 0; i < MAIL_NONCE; i++) nonce[i] = (uint8_t)(200 - i);
  uint8_t msg[MAIL_MSG_LEN];
  MailMsg m;
  m.type = MT_CARD; m.id = 42; m.phrase = 3;
  mailEncode(msg, m);
  uint8_t sealed[MAIL_SEALED_MAX];
  size_t n = mailSeal(key, nonce, msg, sizeof(msg), sealed);
  CHECK_EQ(n, (size_t)(MAIL_NONCE + MAIL_MSG_LEN + MAIL_TAG));
  CHECK(memcmp(sealed + MAIL_NONCE, msg, sizeof(msg)) != 0);  // no va en claro
  uint8_t back[64];
  size_t bn = 0;
  CHECK(mailOpen(key, sealed, n, back, bn));
  CHECK_EQ(bn, (size_t)MAIL_MSG_LEN);
  CHECK(memcmp(back, msg, bn) == 0);
  // cambiar un byte (del cifrado, del nonce o de la etiqueta): no abre
  for (size_t pos : { (size_t)0, (size_t)MAIL_NONCE + 3, n - 1 }) {
    uint8_t t[MAIL_SEALED_MAX];
    memcpy(t, sealed, n);
    t[pos] ^= 0x10;
    CHECK(!mailOpen(key, t, n, back, bn));
  }
  uint8_t otra[16];
  memcpy(otra, key, 16);
  otra[5] ^= 1;
  CHECK(!mailOpen(otra, sealed, n, back, bn));  // otra pareja: no
  CHECK(!mailOpen(key, sealed, MAIL_NONCE + MAIL_TAG - 1, back, bn));
}

TEST(mail, base64_ida_y_vuelta) {
  const char *v[][2] = { { "", "" }, { "f", "Zg==" }, { "fo", "Zm8=" }, { "foo", "Zm9v" }, { "foobar", "Zm9vYmFy" } };
  for (auto &p : v) {
    char out[32];
    size_t n = b64Encode((const uint8_t *)p[0], strlen(p[0]), out, sizeof(out));
    CHECK(std::string(out, n) == p[1]);
    uint8_t back[32];
    size_t bn = b64Decode(p[1], back, sizeof(back));
    CHECK(std::string((const char *)back, bn) == p[0]);
  }
  uint8_t bin[MAIL_SEALED_MAX];
  for (int i = 0; i < MAIL_SEALED_MAX; i++) bin[i] = (uint8_t)(i * 37);
  char txt[200];
  size_t tn = b64Encode(bin, sizeof(bin), txt, sizeof(txt));
  CHECK(tn > 0);
  uint8_t back[MAIL_SEALED_MAX];
  CHECK_EQ(b64Decode(txt, back, sizeof(back)), sizeof(bin));
  CHECK(memcmp(back, bin, sizeof(bin)) == 0);
  CHECK_EQ(b64Decode("Zm9", back, sizeof(back)), (size_t)0);    // largo malo
  CHECK_EQ(b64Decode("Zm!v", back, sizeof(back)), (size_t)0);   // caracter malo
  char poco[4];
  CHECK_EQ(b64Encode((const uint8_t *)"foo", 3, poco, sizeof(poco)), (size_t)0);  // no cabe
}

TEST(mail, nombres_de_buzon) {
  CHECK(mailBoxValid("tp3f9a01c2d4e5"));
  CHECK(mailBoxValid("ABC_def-123"));
  CHECK(!mailBoxValid("short"));
  CHECK(!mailBoxValid("tiene espacio"));
  CHECK(!mailBoxValid("a/b/../../etc"));
  CHECK(!mailBoxValid(nullptr));
}

TEST(mail, se_repite_hasta_el_acuse_y_luego_se_deja) {
  MailOut o;
  o.on = true;
  o.msg.type = MT_GIFT;
  o.msg.id = 77;
  o.firstAt = o.nextAt = 1000;
  CHECK(mailOutDue(o, 1000));
  mailOutSent(o, 1000);
  CHECK(o.on);
  CHECK(!mailOutDue(o, 1000 + MAIL_RESEND_S - 1));
  CHECK(mailOutDue(o, 1000 + MAIL_RESEND_S));
  CHECK(!mailOutExpired(o, 1000 + MAIL_GIVEUP_S - 1));
  CHECK(mailOutExpired(o, 1000 + MAIL_GIVEUP_S));
  CHECK(!mailOutAcked(o, 78));
  CHECK(mailOutAcked(o, 77));
  CHECK(!o.on);
  CHECK(!mailOutAcked(o, 77));  // un acuse repetido no hace nada
  MailOut v;  // una visita sale una vez y ya
  v.on = true;
  v.msg.type = MT_VISIT;
  mailOutSent(v, 5);
  CHECK(!v.on);
  CHECK(!mailOutExpired(v, 5 + MAIL_GIVEUP_S));
}

TEST(mail, un_repetido_se_reconoce) {
  MailSeen s;
  CHECK(!s.seen(5));
  s.add(5);
  CHECK(s.seen(5));
  for (uint32_t i = 100; i < 100 + MAIL_SEEN; i++) s.add(i);
  CHECK(!s.seen(5));  // los mas viejos salen
  CHECK(s.seen(100 + MAIL_SEEN - 1));
}

TEST(mail, lineas_de_ntfy) {
  uint32_t t = 0;
  char m[64];
  CHECK(mailParseLine("{\"id\":\"abc\",\"time\":1794782000,\"event\":\"message\",\"topic\":\"tpx\",\"message\":\"Zm9v\"}", t, m, sizeof(m)));
  CHECK_EQ(t, (uint32_t)1794782000);
  CHECK(std::string(m) == "Zm9v");
  CHECK(!mailParseLine("{\"id\":\"k\",\"time\":1,\"event\":\"keepalive\",\"topic\":\"tpx\"}", t, m, sizeof(m)));
  CHECK(!mailParseLine("{\"id\":\"o\",\"time\":1,\"event\":\"open\"}", t, m, sizeof(m)));
  CHECK(!mailParseLine("no es json", t, m, sizeof(m)));
}
