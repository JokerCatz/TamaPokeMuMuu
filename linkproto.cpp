#include "linkproto.h"
#include <string.h>

void linkPut16(uint8_t *p, uint16_t v) { p[0] = (uint8_t)v; p[1] = (uint8_t)(v >> 8); }
void linkPut32(uint8_t *p, uint32_t v) { for (int i = 0; i < 4; i++) p[i] = (uint8_t)(v >> (8 * i)); }
void linkPut64(uint8_t *p, uint64_t v) { for (int i = 0; i < 8; i++) p[i] = (uint8_t)(v >> (8 * i)); }
uint16_t linkGet16(const uint8_t *p) { return (uint16_t)(p[0] | (p[1] << 8)); }
uint32_t linkGet32(const uint8_t *p) {
  uint32_t v = 0;
  for (int i = 3; i >= 0; i--) v = (v << 8) | p[i];
  return v;
}
uint64_t linkGet64(const uint8_t *p) {
  uint64_t v = 0;
  for (int i = 7; i >= 0; i--) v = (v << 8) | p[i];
  return v;
}

size_t linkEncode(uint8_t *out, size_t cap, const LinkHdr &h, const uint8_t *body) {
  size_t n = LINK_HDR + h.len;
  if (h.len > LINK_BODY_MAX || n > cap) return 0;
  out[0] = 'T';
  out[1] = 'P';
  out[2] = h.major;
  out[3] = h.minor;
  out[4] = h.type;
  out[5] = h.flags;
  linkPut16(out + 6, h.seq);
  linkPut64(out + 8, h.from);
  linkPut16(out + 16, h.len);
  if (h.len) memcpy(out + LINK_HDR, body, h.len);
  return n;
}

LinkErr linkDecode(const uint8_t *in, size_t n, LinkHdr &h, const uint8_t *&body) {
  if (n < LINK_HDR) return LINK_SHORT;
  if (in[0] != 'T' || in[1] != 'P') return LINK_MAGIC;
  h.major = in[2];
  h.minor = in[3];
  h.type = in[4];
  h.flags = in[5];
  h.seq = linkGet16(in + 6);
  h.from = linkGet64(in + 8);
  h.len = linkGet16(in + 16);
  if (h.len > LINK_BODY_MAX || LINK_HDR + (size_t)h.len > n) return LINK_LEN;
  body = in + LINK_HDR;
  return LINK_OK;
}

size_t linkPutHello(uint8_t *b, const LinkHello &h) {
  linkPut32(b, h.caps);
  linkPut16(b + 4, (uint16_t)h.species);
  b[6] = h.shiny;
  linkPut16(b + 7, h.level);
  return LINK_HELLO_LEN;
}

bool linkGetHello(const uint8_t *b, size_t n, LinkHello &h) {
  if (n < LINK_HELLO_LEN) return false;
  h.caps = linkGet32(b);
  h.species = (int16_t)linkGet16(b + 4);
  h.shiny = b[6];
  h.level = linkGet16(b + 7);
  return true;
}

static uint8_t fillByte(uint16_t seq, uint16_t i) { return (uint8_t)(seq * 31u + i * 7u + 1u); }

void linkFillPing(uint8_t *b, uint16_t len, uint16_t seq, uint32_t t0) {
  if (len < LINK_PING_MIN) return;
  linkPut32(b, t0);
  for (uint16_t i = LINK_PING_MIN; i < len; i++) b[i] = fillByte(seq, i);
}

bool linkCheckPing(const uint8_t *b, uint16_t len, uint16_t seq) {
  if (len < LINK_PING_MIN) return false;
  for (uint16_t i = LINK_PING_MIN; i < len; i++)
    if (b[i] != fillByte(seq, i)) return false;
  return true;
}

bool LinkDedupe::dup(uint16_t seq) {
  for (uint8_t i = 0; i < n; i++)
    if (seen[i] == seq) return true;
  seen[pos] = seq;
  pos = (uint8_t)((pos + 1) % LINK_DEDUPE);
  if (n < LINK_DEDUPE) n++;
  return false;
}

// ---------- SHA-256 (FIPS 180-4) ----------

static const uint32_t K256[64] = {
  0x428a2f98, 0x71374491, 0xb5c0fbcf, 0xe9b5dba5, 0x3956c25b, 0x59f111f1, 0x923f82a4, 0xab1c5ed5,
  0xd807aa98, 0x12835b01, 0x243185be, 0x550c7dc3, 0x72be5d74, 0x80deb1fe, 0x9bdc06a7, 0xc19bf174,
  0xe49b69c1, 0xefbe4786, 0x0fc19dc6, 0x240ca1cc, 0x2de92c6f, 0x4a7484aa, 0x5cb0a9dc, 0x76f988da,
  0x983e5152, 0xa831c66d, 0xb00327c8, 0xbf597fc7, 0xc6e00bf3, 0xd5a79147, 0x06ca6351, 0x14292967,
  0x27b70a85, 0x2e1b2138, 0x4d2c6dfc, 0x53380d13, 0x650a7354, 0x766a0abb, 0x81c2c92e, 0x92722c85,
  0xa2bfe8a1, 0xa81a664b, 0xc24b8b70, 0xc76c51a3, 0xd192e819, 0xd6990624, 0xf40e3585, 0x106aa070,
  0x19a4c116, 0x1e376c08, 0x2748774c, 0x34b0bcb5, 0x391c0cb3, 0x4ed8aa4a, 0x5b9cca4f, 0x682e6ff3,
  0x748f82ee, 0x78a5636f, 0x84c87814, 0x8cc70208, 0x90befffa, 0xa4506ceb, 0xbef9a3f7, 0xc67178f2,
};

static inline uint32_t ror(uint32_t x, int n) { return (x >> n) | (x << (32 - n)); }

static void shaBlock(uint32_t st[8], const uint8_t *p) {
  uint32_t w[64];
  for (int i = 0; i < 16; i++)
    w[i] = ((uint32_t)p[4 * i] << 24) | ((uint32_t)p[4 * i + 1] << 16) | ((uint32_t)p[4 * i + 2] << 8) | p[4 * i + 3];
  for (int i = 16; i < 64; i++) {
    uint32_t s0 = ror(w[i - 15], 7) ^ ror(w[i - 15], 18) ^ (w[i - 15] >> 3);
    uint32_t s1 = ror(w[i - 2], 17) ^ ror(w[i - 2], 19) ^ (w[i - 2] >> 10);
    w[i] = w[i - 16] + s0 + w[i - 7] + s1;
  }
  uint32_t a = st[0], b = st[1], c = st[2], d = st[3], e = st[4], f = st[5], g = st[6], h = st[7];
  for (int i = 0; i < 64; i++) {
    uint32_t t1 = h + (ror(e, 6) ^ ror(e, 11) ^ ror(e, 25)) + ((e & f) ^ (~e & g)) + K256[i] + w[i];
    uint32_t t2 = (ror(a, 2) ^ ror(a, 13) ^ ror(a, 22)) + ((a & b) ^ (a & c) ^ (b & c));
    h = g; g = f; f = e; e = d + t1; d = c; c = b; b = a; a = t1 + t2;
  }
  st[0] += a; st[1] += b; st[2] += c; st[3] += d; st[4] += e; st[5] += f; st[6] += g; st[7] += h;
}

void linkSha256(const uint8_t *d, size_t n, uint8_t out[32]) {
  uint32_t st[8] = { 0x6a09e667, 0xbb67ae85, 0x3c6ef372, 0xa54ff53a, 0x510e527f, 0x9b05688c, 0x1f83d9ab, 0x5be0cd19 };
  size_t i = 0;
  for (; i + 64 <= n; i += 64) shaBlock(st, d + i);
  uint8_t last[128] = {};
  size_t r = n - i;
  memcpy(last, d + i, r);
  last[r] = 0x80;
  size_t tot = r + 9 <= 64 ? 64 : 128;
  uint64_t bits = (uint64_t)n * 8;
  for (int k = 0; k < 8; k++) last[tot - 1 - k] = (uint8_t)(bits >> (8 * k));
  shaBlock(st, last);
  if (tot == 128) shaBlock(st, last + 64);
  for (int k = 0; k < 8; k++)
    for (int j = 0; j < 4; j++) out[4 * k + j] = (uint8_t)(st[k] >> (24 - 8 * j));
}

void linkDeriveKey(uint64_t a, const uint8_t na[LINK_NONCE], uint64_t b, const uint8_t nb[LINK_NONCE],
                   uint8_t key[LINK_KEY]) {
  if (b < a) {  // primero el numero de jugador mas bajo: los dos lo ordenan igual
    uint64_t t = a; a = b; b = t;
    const uint8_t *tn = na; na = nb; nb = tn;
  }
  static const char TAG[] = "TamaPoke link v1";
  uint8_t in[16 + 2 * (8 + LINK_NONCE)];
  memcpy(in, TAG, 16);
  linkPut64(in + 16, a);
  memcpy(in + 24, na, LINK_NONCE);
  linkPut64(in + 40, b);
  memcpy(in + 48, nb, LINK_NONCE);
  uint8_t h[32];
  linkSha256(in, sizeof(in), h);
  memcpy(key, h, LINK_KEY);
}

uint16_t linkPairCode(const uint8_t key[LINK_KEY]) {
  uint8_t in[4 + LINK_KEY] = { 'c', 'o', 'd', 'e' };
  memcpy(in + 4, key, LINK_KEY);
  uint8_t h[32];
  linkSha256(in, sizeof(in), h);
  return (uint16_t)(linkGet32(h) % 10000);
}

// ---------- emparejamiento ----------

void LinkPair::reset() {
  uint64_t me = self;
  *this = LinkPair();
  self = me;
}

void LinkPair::makeKey() { linkDeriveKey(self, mine, peer, theirs, key); }

uint8_t LinkPair::ask(uint64_t p, const uint8_t nonce[LINK_NONCE], uint32_t now) {
  if (st == LP_PROMPT && peer == p) return accept(nonce, now) ? LPA_OK : LPA_BUSY;  // ya me lo pedia: es un si
  if (busy()) return LPA_BUSY;
  reset();
  st = LP_ASKING;
  peer = p;
  memcpy(mine, nonce, LINK_NONCE);
  until = now + LINK_ASK_MS;
  return LPA_REQ;
}

uint8_t LinkPair::onReq(uint64_t p, const uint8_t nonce[LINK_NONCE], uint32_t now) {
  if (busy() && peer != p) return LPQ_BUSY;
  if (st == LP_ASKING) {  // los dos dijeron que si a la vez: su REQ es su OK
    memcpy(theirs, nonce, LINK_NONCE);
    haveTheirs = true;
    makeKey();
    st = LP_KEYING;
    until = now + LINK_CARDS_MS;
    return LPQ_BOTH;
  }
  if (busy()) return LPQ_AGAIN;  // repetido (ya lo tengo)
  reset();
  st = LP_PROMPT;
  peer = p;
  memcpy(theirs, nonce, LINK_NONCE);
  haveTheirs = true;
  until = now + LINK_ASK_MS;
  return LPQ_PROMPT;
}

bool LinkPair::accept(const uint8_t nonce[LINK_NONCE], uint32_t now) {
  if (st != LP_PROMPT) return false;
  memcpy(mine, nonce, LINK_NONCE);
  makeKey();
  st = LP_KEYING;
  myAcked = false;  // hasta que el OK tenga ACK
  until = now + LINK_CARDS_MS;
  return true;
}

bool LinkPair::decline() {
  if (st != LP_PROMPT) return false;
  st = LP_IDLE;
  return true;
}

bool LinkPair::onOk(uint64_t p, const uint8_t nonce[LINK_NONCE], uint32_t now) {
  if (st != LP_ASKING || peer != p) return false;
  memcpy(theirs, nonce, LINK_NONCE);
  haveTheirs = true;
  makeKey();
  st = LP_KEYING;
  until = now + LINK_CARDS_MS;
  return true;
}

bool LinkPair::onNo(uint64_t p) {
  if ((st != LP_ASKING && st != LP_PROMPT) || peer != p) return false;
  st = LP_DECLINED;
  return true;
}

bool LinkPair::readyToEncrypt(uint32_t now) {
  if (st != LP_KEYING || !haveTheirs || !myAcked) return false;
  if (answered && (int32_t)(now - lastAckAt) < (int32_t)LINK_SETTLE_MS) return false;
  st = LP_CARDS;
  return true;
}

bool LinkPair::doneCheck() {
  if (st != LP_CARDS || !cardIn || !cardAcked) return false;
  st = LP_DONE;
  return true;
}

bool LinkPair::onCard() {
  if (st != LP_CARDS) return false;
  cardIn = true;
  return doneCheck();
}

bool LinkPair::onCardAcked() {
  if (st != LP_CARDS) return false;
  cardAcked = true;
  return doneCheck();
}

uint8_t LinkPair::tick(uint32_t now) {
  if (!busy() || (int32_t)(now - until) < 0) return LP_IDLE;
  uint8_t was = st;
  st = was == LP_PROMPT ? LP_IDLE : LP_FAILED;
  return was;
}
