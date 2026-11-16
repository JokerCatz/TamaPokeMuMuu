// Juegos con un amigo (friendmsg.h): mensajes, chocar los cinco y regalos.
#include "framework.h"
#include "../friendmsg.h"
#include "../linkproto.h"

TEST(friend, los_mensajes_caben_y_van_y_vuelven) {
  CHECK(F_ATH_LEN <= LINK_BODY_MAX && F_RACE_LEN <= LINK_BODY_MAX && F_ROUND_LEN <= LINK_BODY_MAX);
  uint8_t b[LINK_BODY_MAX];
  FAthlete a;
  a.dex = 1025; a.gA = 91; a.gD = 110; a.gS = 99; a.tA = 7; a.tD = 100; a.tS = 0; a.mega = 1; a.shiny = 1;
  CHECK_EQ(fPutAth(b, a), (size_t)F_ATH_LEN);
  FAthlete c;
  CHECK(fGetAth(b, F_ATH_LEN, c));
  CHECK_EQ(c.dex, (int16_t)1025);
  CHECK(c.gA == 91 && c.gD == 110 && c.gS == 99 && c.tA == 7 && c.tD == 100 && c.tS == 0 && c.mega && c.shiny);
  CHECK(!fGetAth(b, F_ATH_LEN - 1, c));

  FInvite v;
  v.kind = FK_GYM; v.a = 3; v.b = 8;
  CHECK_EQ(fPutInvite(b, v), (size_t)F_INVITE_LEN);
  FInvite w;
  CHECK(fGetInvite(b, F_INVITE_LEN, w));
  CHECK(w.kind == FK_GYM && w.a == 3 && w.b == 8);
  b[0] = 9;  // tipo desconocido
  CHECK(!fGetInvite(b, F_INVITE_LEN, w));

  FInviteAns ans;
  ans.kind = FK_RACE; ans.answer = FW_OK; ans.ath = a;
  CHECK_EQ(fPutInviteAns(b, ans), (size_t)F_INVITE_ANS_LEN);
  FInviteAns ans2;
  CHECK(fGetInviteAns(b, F_INVITE_ANS_LEN, ans2));
  CHECK(ans2.kind == FK_RACE && ans2.answer == FW_OK && ans2.ath.dex == 1025);
}

TEST(friend, la_carrera_llega_igual) {
  uint8_t b[LINK_BODY_MAX];
  FRace r;
  r.ev = 4;
  r.dex[0] = 4; r.dex[1] = 1; r.dex[2] = 999;
  r.shiny[1] = 1;
  r.marks[0] = 1.85f; r.marks[1] = 1.4f; r.marks[2] = 2.05f;
  CHECK_EQ(fPutRace(b, r), (size_t)F_RACE_LEN);
  FRace s;
  CHECK(fGetRace(b, F_RACE_LEN, s));
  CHECK_EQ(s.ev, (uint8_t)4);
  for (int i = 0; i < 3; i++) {
    CHECK_EQ(s.dex[i], r.dex[i]);
    CHECK_EQ(s.shiny[i], r.shiny[i]);
    CHECK(s.marks[i] == r.marks[i]);  // los mismos bits: las dos pantallas ven la misma marca
  }
}

TEST(friend, la_prueba_del_gimnasio_llega_igual) {
  uint8_t b[LINK_BODY_MAX];
  FRound r;
  r.round = 2; r.ev = 9; r.wins = 1; r.lost = 1; r.league = 1; r.member = 4; r.region = 8; r.gym = 8;
  r.mineDex = 25; r.foeDex = 1007; r.mineShiny = 1; r.mineHelper = 1;
  r.marks[0] = 612.5f; r.marks[1] = 600.25f;
  CHECK_EQ(fPutRound(b, r), (size_t)F_ROUND_LEN);
  FRound s;
  CHECK(fGetRound(b, F_ROUND_LEN, s));
  CHECK(s.round == 2 && s.ev == 9 && s.wins == 1 && s.lost == 1 && s.league == 1 && s.member == 4);
  CHECK(s.region == 8 && s.gym == 8 && s.mineDex == 25 && s.foeDex == 1007 && s.mineShiny == 1 && s.mineHelper == 1);
  CHECK(s.marks[0] == 612.5f && s.marks[1] == 600.25f);
  CHECK(!fGetRound(b, F_ROUND_LEN - 1, s));
  FGymEnd e, f;
  e.won = 1; e.region = 2; e.gym = 7; e.member = 0;
  CHECK_EQ(fPutGymEnd(b, e), (size_t)F_GYM_END_LEN);
  CHECK(fGetGymEnd(b, F_GYM_END_LEN, f));
  CHECK(f.won == 1 && f.region == 2 && f.gym == 7);
}

TEST(friend, chocar_los_cinco_a_la_vez) {
  HiFive h;
  CHECK(!h.tapMine(1000));
  CHECK(h.tapTheirs(1600));   // 600 ms despues: choca
  CHECK(!h.tapTheirs(1700));  // esa pareja ya se uso
  CHECK(!h.tapMine(5000));
  CHECK(!h.tapTheirs(6200));  // 1,2 s: demasiado tarde
  CHECK(h.tapMine(6500));     // pero con un toque nuevo mio, si
  HiFive k;
  CHECK(!k.tapTheirs(100));
  CHECK(k.tapMine(1099));     // en el otro orden
  HiFive w;
  CHECK(!w.tapMine(0xFFFFFF00u));
  CHECK(w.tapTheirs(0x00000100u));  // millis() da la vuelta entre los dos
}

TEST(friend, regalos_sin_duplicar_ni_perder) {
  uint8_t b[8];
  uint32_t tx;
  uint8_t it;
  CHECK_EQ(fPutGift(b, 0xCAFE0001u, 6), (size_t)5);
  CHECK(fGetGift(b, 5, tx, it));
  CHECK(tx == 0xCAFE0001u && it == 6);
  GiftOut o;
  o.start(77, 3, 1000);
  CHECK(!o.answer(78));         // otra transaccion: no es la nuestra
  CHECK(!o.expired(1000 + FRIEND_ASK_MS - 1));
  CHECK(o.answer(77));
  CHECK(!o.on);
  CHECK(!o.answer(77));         // una respuesta repetida no cuenta dos veces
  o.start(80, 3, 0);
  CHECK(o.expired(FRIEND_ASK_MS));  // sin respuesta: se devuelve
  GiftSeen s;
  CHECK(!s.seen(5));
  s.add(5);
  CHECK(s.seen(5));
  for (uint32_t t = 100; t < 100 + GIFT_SEEN; t++) s.add(t);
  CHECK(!s.seen(5));  // los mas viejos salen
  CHECK(s.seen(100 + GIFT_SEEN - 1));
}

#include "../pet.h"

TEST(friend, lo_criado_por_un_amigo_es_solo_una_marca) {
  Pet p;
  p.begin();
  uint8_t bits[PET_DEX_BYTES] = {};
  bits[0] = 0x01;                  // #1
  bits[PET_DEX_BYTES - 1] = 0xFF;  // incluye bits mas alla de la 1025: no cuentan
  uint8_t antes = p.unlockedRegions();
  p.addFriendDex(bits, sizeof(bits));
  CHECK(p.isFriendSeen(1));
  CHECK(p.isFriendSeen(1025));
  CHECK(!p.isFriendSeen(2));
  CHECK(!p.isFriendSeen(1026));
  CHECK(!p.isRegistered(1025));            // no es criado
  CHECK_EQ(p.unlockedRegions(), antes);    // ni abre regiones
}
