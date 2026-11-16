#include "friendmsg.h"
#include "linkproto.h"  // linkPut16/32, linkGet16/32
#include <string.h>

static void putF(uint8_t *p, float f) {
  uint32_t u;
  memcpy(&u, &f, 4);
  linkPut32(p, u);
}
static float getF(const uint8_t *p) {
  uint32_t u = linkGet32(p);
  float f;
  memcpy(&f, &u, 4);
  return f;
}

size_t fPutAth(uint8_t *b, const FAthlete &a) {
  linkPut16(b, (uint16_t)a.dex);
  b[2] = a.gA; b[3] = a.gD; b[4] = a.gS;
  b[5] = a.tA; b[6] = a.tD; b[7] = a.tS;
  b[8] = a.mega;
  b[9] = a.shiny;
  return F_ATH_LEN;
}

bool fGetAth(const uint8_t *b, size_t n, FAthlete &a) {
  if (n < F_ATH_LEN) return false;
  a.dex = (int16_t)linkGet16(b);
  a.gA = b[2]; a.gD = b[3]; a.gS = b[4];
  a.tA = b[5]; a.tD = b[6]; a.tS = b[7];
  a.mega = b[8];
  a.shiny = b[9];
  return true;
}

size_t fPutInvite(uint8_t *b, const FInvite &v) {
  b[0] = v.kind; b[1] = v.a; b[2] = v.b;
  return F_INVITE_LEN;
}

bool fGetInvite(const uint8_t *b, size_t n, FInvite &v) {
  if (n < F_INVITE_LEN) return false;
  v.kind = b[0]; v.a = b[1]; v.b = b[2];
  return v.kind == FK_RACE || v.kind == FK_GYM;
}

size_t fPutInviteAns(uint8_t *b, const FInviteAns &v) {
  b[0] = v.kind;
  b[1] = v.answer;
  fPutAth(b + 2, v.ath);
  return F_INVITE_ANS_LEN;
}

bool fGetInviteAns(const uint8_t *b, size_t n, FInviteAns &v) {
  if (n < F_INVITE_ANS_LEN) return false;
  v.kind = b[0];
  v.answer = b[1];
  return fGetAth(b + 2, n - 2, v.ath);
}

size_t fPutRace(uint8_t *b, const FRace &r) {
  b[0] = r.ev;
  for (int i = 0; i < 3; i++) {
    linkPut16(b + 1 + 2 * i, (uint16_t)r.dex[i]);
    b[7 + i] = r.shiny[i];
    putF(b + 10 + 4 * i, r.marks[i]);
  }
  return F_RACE_LEN;
}

bool fGetRace(const uint8_t *b, size_t n, FRace &r) {
  if (n < F_RACE_LEN) return false;
  r.ev = b[0];
  for (int i = 0; i < 3; i++) {
    r.dex[i] = (int16_t)linkGet16(b + 1 + 2 * i);
    r.shiny[i] = b[7 + i];
    r.marks[i] = getF(b + 10 + 4 * i);
  }
  return true;
}

size_t fPutRound(uint8_t *b, const FRound &r) {
  b[0] = r.round; b[1] = r.ev; b[2] = r.wins; b[3] = r.lost;
  b[4] = r.league; b[5] = r.member; b[6] = r.region; b[7] = r.gym;
  linkPut16(b + 8, (uint16_t)r.mineDex);
  linkPut16(b + 10, (uint16_t)r.foeDex);
  b[12] = r.mineShiny;
  b[13] = r.mineHelper;
  putF(b + 14, r.marks[0]);
  putF(b + 18, r.marks[1]);
  return F_ROUND_LEN;
}

bool fGetRound(const uint8_t *b, size_t n, FRound &r) {
  if (n < F_ROUND_LEN) return false;
  r.round = b[0]; r.ev = b[1]; r.wins = b[2]; r.lost = b[3];
  r.league = b[4]; r.member = b[5]; r.region = b[6]; r.gym = b[7];
  r.mineDex = (int16_t)linkGet16(b + 8);
  r.foeDex = (int16_t)linkGet16(b + 10);
  r.mineShiny = b[12];
  r.mineHelper = b[13];
  r.marks[0] = getF(b + 14);
  r.marks[1] = getF(b + 18);
  return true;
}

size_t fPutGymEnd(uint8_t *b, const FGymEnd &e) {
  b[0] = e.won; b[1] = e.region; b[2] = e.gym; b[3] = e.member;
  return F_GYM_END_LEN;
}

bool fGetGymEnd(const uint8_t *b, size_t n, FGymEnd &e) {
  if (n < F_GYM_END_LEN) return false;
  e.won = b[0]; e.region = b[1]; e.gym = b[2]; e.member = b[3];
  return true;
}

size_t fPutGift(uint8_t *b, uint32_t tx, uint8_t item) {
  linkPut32(b, tx);
  b[4] = item;
  return 5;
}

bool fGetGift(const uint8_t *b, size_t n, uint32_t &tx, uint8_t &item) {
  if (n < 5) return false;
  tx = linkGet32(b);
  item = b[4];
  return true;
}

bool HiFive::check() {
  if (!haveMine || !haveTheirs) return false;
  int32_t d = (int32_t)(mine - theirs);
  if (d < 0) d = -d;
  if ((uint32_t)d >= FRIEND_TAP_MS) return false;
  reset();  // esta pareja ya choco
  return true;
}

bool HiFive::tapMine(uint32_t now) {
  mine = now;
  haveMine = true;
  return check();
}

bool HiFive::tapTheirs(uint32_t now) {
  theirs = now;
  haveTheirs = true;
  return check();
}

bool GiftSeen::seen(uint32_t t) const {
  for (uint8_t i = 0; i < n; i++)
    if (tx[i] == t) return true;
  return false;
}

void GiftSeen::add(uint32_t t) {
  if (seen(t)) return;
  tx[pos] = t;
  pos = (uint8_t)((pos + 1) % GIFT_SEEN);
  if (n < GIFT_SEEN) n++;
}
