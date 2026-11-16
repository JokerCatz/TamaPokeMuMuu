// Juegos con un amigo ("见面"): la pagina del amigo (jugar juntos y chocar los cinco, regalos,
// fotos) y las invitaciones a una carrera (sports.ino) o a un gimnasio (gyms.ino). Los mensajes
// estan en friendmsg.h y van por el enlace (link.h), cifrados y solo entre amigos. Quien invita
// decide (sortea y calcula) y el otro enseña lo que le llega. Nada de esto da puntos.
// (pestaña de TamaPoke.ino: Arduino la une al final del principal)

enum : uint8_t { FP_MENU = 0, FP_PLAY, FP_GIFT, FP_PHOTOS };
enum : uint8_t { FPI_NONE = 0, FPI_GIFT, FPI_RACE, FPI_GYM };  // pregunta que llego de la otra placa
enum : uint8_t { FPM_PLAY = 0, FPM_GIFT, FPM_RACE, FPM_GYM, FPM_PHOTO, FPM_ROWS };

static int8_t fpFi = -1;                // el amigo cuya pagina esta abierta (-1 = ninguna)
static uint8_t fpStage = FP_MENU;
static uint32_t fpPlayT0 = 0, fpPlayUntil = 0, fpFiveAt = 0;
static HiFive fpFive;
static GiftOut fpGift;                  // mi regalo, esperando a que lo cojan
static GiftSeen fpGiftSeen;             // regalos ya cogidos (por si repiten: mi respuesta se perdio)
static int8_t fpGiftAsk = -1;           // objeto elegido para regalar, falta confirmar
static uint8_t fpGiftPage = 0, fpPhotoPage = 0;
static uint8_t fpIn = FPI_NONE, fpInFi = 0, fpInItem = 0;
static uint32_t fpInTx = 0, fpInUntil = 0;
static FInvite fpInInv;
static int8_t fpWaitHost = -1;          // dije que si a su carrera o gimnasio: espero lo que mande
static uint32_t fpWaitUntil = 0;
static const char *fpMsg = nullptr;     // aviso en un panel (como acabo el gimnasio al que ayude)
static uint32_t fpMsgUntil = 0;
static bool fpDexSent[LINK_FRIENDS];
static bool fpWasActive = false;
static int fpPhotoN = 0;                // fotos en la SD (se cuentan al abrir la pagina)
// consola FRIENDTEST 1 (solo RAM): los juegos con un amigo no cambian nada en esta placa (sin
// animo ni vinculo, sin mover objetos, sin marcas en la pokedex ni fotos; carreras y gimnasios
// de practica). Para probar con dos placas sin tocar la partida
static bool fpTest = false;
bool friendTestMode() { return fpTest; }
void friendSetTest(bool on) { fpTest = on; }
static char fpToastBuf[64];

#define FP_ROW_X 63
#define FP_ROW_W 340
#define FP_GRID_X (CX - 144)
#define FP_GRID_Y 96
#define FP_CELL 96
#define FP_PH_X 70   // fotos: 4 filas como el album
#define FP_PH_Y 86
#define FP_PH_W 326
#define FP_PH_RH 72

const char *friendName(uint8_t fi) { return fi < linkFriendCount() ? linkFriendAt(fi).name : "?"; }

static const LinkPeer *fpPeer(uint8_t fi) {
  int k = linkFriendNearby(fi);
  return k >= 0 ? &linkNearby(k) : nullptr;
}

static bool fpHas(uint8_t fi, uint32_t cap) {
  const LinkPeer *p = fpPeer(fi);
  return p && (p->caps & cap);
}

// un compañero que llega por el enlace, como atleta (sport.h)
Athlete friendAthlete(const FAthlete &f) {
  Athlete a = athleteOf(f.dex);
  a.geneAtk = f.gA;
  a.geneDef = f.gD;
  a.geneSpe = f.gS;
  a.trAtk = f.tA;
  a.trDef = f.tD;
  a.trSpe = f.tS;
  a.mega = f.mega;
  return a;
}

static FAthlete myFAthlete() {
  Athlete a = petAthlete();
  FAthlete f;
  f.dex = a.dex;
  f.gA = a.geneAtk; f.gD = a.geneDef; f.gS = a.geneSpe;
  f.tA = a.trAtk; f.tD = a.trDef; f.tS = a.trSpe;
  f.mega = a.mega;
  f.shiny = pet.shiny;
  return f;
}

// ---------- mandar (los usan tambien sports.ino y gyms.ino) ----------

bool friendInvite(uint8_t fi, const FInvite &v) {
  uint8_t b[F_INVITE_LEN];
  fPutInvite(b, v);
  return linkSendGame(fi, FM_INVITE, b, sizeof(b), true);
}

void friendSendRace(uint8_t fi, const FRace &r) {
  uint8_t b[F_RACE_LEN];
  fPutRace(b, r);
  linkSendGame(fi, FM_RACE, b, sizeof(b), true);
}

void friendSendRound(uint8_t fi, const FRound &r) {
  uint8_t b[F_ROUND_LEN];
  fPutRound(b, r);
  linkSendGame(fi, FM_ROUND, b, sizeof(b), true);
}

void friendSendGymEnd(uint8_t fi, const FGymEnd &e) {
  uint8_t b[F_GYM_END_LEN];
  fPutGymEnd(b, e);
  linkSendGame(fi, FM_GYM_END, b, sizeof(b), true);
}

void friendCancel(uint8_t fi) { linkSendGame(fi, FM_CANCEL, nullptr, 0, true); }

static void fpAnswerGift(uint8_t fi, uint32_t tx, bool ok) {
  uint8_t b[5];
  fPutGift(b, tx, ok ? 1 : 0);
  linkSendGame(fi, FM_GIFT_ANS, b, sizeof(b), true);
}

static void fpAnswerInvite(uint8_t fi, uint8_t kind, uint8_t answer) {
  FInviteAns a;
  a.kind = kind;
  a.answer = answer;
  if (answer == FW_OK) a.ath = myFAthlete();
  uint8_t b[F_INVITE_ANS_LEN];
  fPutInviteAns(b, a);
  linkSendGame(fi, FM_INVITE_ANS, b, sizeof(b), true);
}

// ---------- la pagina del amigo ----------

bool friendScreenOpen() { return fpFi >= 0; }

void openFriendMenu(uint8_t fi) {
  fpFi = fi;
  fpStage = FP_MENU;
  fpGiftAsk = -1;
  fpPhotoN = photoCount();
}

static void closeFriendMenu() {
  fpFi = -1;
  fpGiftAsk = -1;
  visitorPmd.unload();
}

// se cerro "见面" (o se paro solo): nada a medias
void friendLinkClosed() {
  closeFriendMenu();
  fpIn = FPI_NONE;
  fpWaitHost = -1;
}

// libre para lo que propone el amigo fi: en "见面" sin nada a medias. Si aqui se esta en algo con ese
// mismo amigo que se puede dejar (su menu de carreras o de gimnasios, el podio, jugar, elegir un
// regalo), se deja para contestarle: antes salia "ocupado" en cuanto los dos niños abrian a la vez
// "一起比赛" y uno elegia la prueba
static bool fpFree(uint8_t fi) {
  if (!linkOpen || fpIn != FPI_NONE || fpWaitHost >= 0 || fpGift.on || fpGiftAsk >= 0 || linkPairing().busy())
    return false;
  if (chOpen && !sportsLeaveForFriend(fi)) return false;
  if (gymsOpen && !gymsLeaveForFriend(fi)) return false;
  if (fpFi >= 0 && fpStage != FP_MENU && fpStage != FP_PHOTOS) {
    if (fpFi != (int8_t)fi) return false;  // jugando o eligiendo regalo con otro amigo
    fpStage = FP_MENU;                      // con el mismo: se deja
    visitorPmd.unload();
  }
  return true;
}

static const char *fpWhyNot() {  // por que el compañero no puede jugar ahora (nullptr = si puede)
  if (pet.vacant || pet.isEgg()) return T(S_NO_PARTNER);
  if (pet.sleeping && !fpTest) return T(S_ASLEEP);  // FRIENDTEST: se prueba aunque duerma
  return nullptr;
}

// carrera o gimnasio: lo de siempre (dormido, enfermo, cansado...); con FRIENDTEST solo hace falta
// un compañero (no cambia nada y asi se prueba de noche)
static const char *fpSportsBlocked() {
  if (fpTest) return pet.vacant || pet.isEgg() ? T(S_NO_PARTNER) : nullptr;
  return sportsBlocked();
}

static void fpHighFive(uint32_t now) {
  fpFiveAt = now;
  sfxPlay(SFX_MEDAL);
  wakeScreen(now);
}

static void fpStartPlay(uint8_t fi, bool mine) {
  const LinkPeer *p = fpPeer(fi);
  uint32_t now = millis();
  fpFi = fi;
  fpStage = FP_PLAY;
  fpPlayT0 = now;
  fpPlayUntil = now + FRIEND_PLAY_MS;
  fpFiveAt = 0;
  fpFive.reset();
  visitorPmd.unload();
  if (p && p->species > 0) visitorPmd.load(p->species, p->shiny);
  if (mine) linkSendGame(fi, FM_PLAY, nullptr, 0, true);
  if (!fpTest && linkFriendTakeDay(fi, LDAY_PLAY)) pet.friendPlay();  // la primera vez del dia con este amigo
  wakeScreen(now);
}

// ---------- lo que llega ----------

void friendRx(uint8_t fi, uint8_t type, const uint8_t *b, uint16_t n) {
  uint32_t now = millis();
  switch (type) {
    case FM_DEX:
      if (!fpTest) pet.addFriendDex(b, n);
      Serial.printf("amigo: pokedex de %s (%u bytes)%s\n", friendName(fi), n, fpTest ? " (prueba: no se apunta)" : "");
      break;
    case FM_PLAY:  // si esta libre y su compañero puede, se juega; si no, el otro juega solo
      if (!fpWhyNot() && fpFree(fi)) fpStartPlay(fi, false);
      break;
    case FM_TAP:
      if (fpFi == fi && fpStage == FP_PLAY && fpFive.tapTheirs(now)) fpHighFive(now);
      break;
    case FM_GIFT: {
      uint32_t tx;
      uint8_t it;
      if (!fGetGift(b, n, tx, it) || it >= ITEM_COUNT) break;
      if (fpGiftSeen.seen(tx)) { fpAnswerGift(fi, tx, true); break; }  // ya lo cogi: se perdio mi si
      if (fpIn == FPI_GIFT && fpInTx == tx) break;                      // repetido mientras se pregunta
      if (!fpFree(fi)) { fpAnswerGift(fi, tx, false); break; }
      fpIn = FPI_GIFT;
      fpInFi = fi;
      fpInTx = tx;
      fpInItem = it;
      fpInUntil = now + FRIEND_ASK_MS;
      wakeScreen(now);
      break;
    }
    case FM_GIFT_ANS: {
      uint32_t tx;
      uint8_t ok;
      if (!fGetGift(b, n, tx, ok) || !fpGift.answer(tx)) break;
      if (ok) {
        toast(T(S_FR_GIFT_SENT));
        sfxPlay(SFX_MEDAL);
      } else {
        if (!fpTest) pet.give(fpGift.item, 1);  // vuelve a la mochila
        toast(T(S_FR_GIFT_BACK));
      }
      Serial.printf("amigo: regalo %s\n", ok ? "cogido" : "devuelto");
      break;
    }
    case FM_INVITE: {
      FInvite v;
      if (!fGetInvite(b, n, v)) break;
      if (!fpFree(fi)) { fpAnswerInvite(fi, v.kind, FW_BUSY); break; }
      if (fpSportsBlocked()) { fpAnswerInvite(fi, v.kind, FW_CANT); break; }  // dormido, cansado...
      fpIn = v.kind == FK_RACE ? FPI_RACE : FPI_GYM;
      fpInFi = fi;
      fpInInv = v;
      fpInUntil = now + FRIEND_ASK_MS;
      wakeScreen(now);
      break;
    }
    case FM_INVITE_ANS: {
      FInviteAns a;
      if (!fGetInviteAns(b, n, a)) break;
      if (a.kind == FK_RACE) sportsFriendAnswer(fi, a);
      else gymsFriendAnswer(fi, a);
      break;
    }
    case FM_RACE: {
      FRace r;
      if (fpWaitHost != fi || !fGetRace(b, n, r)) break;
      fpWaitHost = -1;
      sportsJoinRace(fi, r);
      break;
    }
    case FM_ROUND: {
      FRound r;
      if (fpWaitHost != fi || !fGetRound(b, n, r)) break;
      fpWaitUntil = now + 180000;  // la siguiente puede tardar (el otro mira el resultado, o el orden)
      gymsHelperRound(r);
      break;
    }
    case FM_GYM_END: {
      FGymEnd e;
      if (fpWaitHost != fi || !fGetGymEnd(b, n, e)) break;
      fpWaitHost = -1;
      gymsHelperEnd();
      if (!fpTest) pet.sportsDone(e.won);  // su compañero tambien compitio: cansa un poco y alegra
      Serial.printf("amigo: gimnasio %s\n", e.won ? "ganado" : "perdido");
      fpMsg = T(e.won ? S_FR_WON : S_FR_LOST);
      fpMsgUntil = now + 4000;
      sfxPlay(e.won ? SFX_MEDAL : SFX_TAP);
      break;
    }
    case FM_CANCEL:  // el otro lo dejo
      if (fpWaitHost == fi) {
        fpWaitHost = -1;
        gymsHelperEnd();
        toast(T(S_FR_GONE));
      }
      if ((fpIn == FPI_RACE || fpIn == FPI_GYM) && fpInFi == fi) fpIn = FPI_NONE;
      sportsFriendGone(fi);
      gymsFriendGone(fi);
      break;
  }
}

// ---------- cada vuelta del bucle ----------

void friendLoop(uint32_t now) {
  bool act = linkActive();
  if (act != fpWasActive) {  // "见面" se abrio o se cerro
    fpWasActive = act;
    for (auto &d : fpDexSent) d = false;
    if (!act) friendLinkClosed();
  }
  if (fpGift.on && (!act || fpGift.expired(now))) {  // sin respuesta: el regalo vuelve
    fpGift.on = false;
    if (!fpTest) pet.give(fpGift.item, 1);
    toast(T(S_FR_GIFT_BACK));
  }
  if (!act) return;
  if (fpIn != FPI_NONE && (int32_t)(now - fpInUntil) >= 0) {  // nadie contesto: es un no
    if (fpIn == FPI_GIFT) fpAnswerGift(fpInFi, fpInTx, false);
    else fpAnswerInvite(fpInFi, fpInInv.kind, FW_NO);
    fpIn = FPI_NONE;
  }
  if (fpWaitHost >= 0 && (int32_t)(now - fpWaitUntil) >= 0) {
    fpWaitHost = -1;
    gymsHelperEnd();
    toast(T(S_FR_GONE));
  }
  if (fpFi >= 0 && fpStage == FP_PLAY && (int32_t)(now - fpPlayUntil) >= 0) {
    fpStage = FP_MENU;
    visitorPmd.unload();
  }
  if (fpFi >= 0 && fpFi >= (int8_t)linkFriendCount()) closeFriendMenu();  // lo olvidaron (consola)
  if (fpMsg && timeLeft(fpMsgUntil) && chOpen) fpMsgUntil = now + 4000;  // el aviso, cuando se cierre la prueba
  // "见面" sigue abierta mientras se toca cualquier cosa o se hace algo juntos
  if (idleSince(now, lastUserAt) < 1000 || fpStage == FP_PLAY || fpWaitHost >= 0 || fpIn != FPI_NONE ||
      fpGift.on || sportsWithFriendNow() || gymsWithFriendNow())
    linkTouch();
  // un amigo que contesta: lo que he criado (para su pokedex) y la foto del dia
  for (uint8_t fi = 0; fi < linkFriendCount(); fi++) {
    int k = linkFriendNearby(fi);
    if (k < 0) continue;
    const LinkPeer &p = linkNearby(k);
    if (!fpDexSent[fi] && (p.caps & CAP_PLAY)) fpDexSent[fi] = linkSendGame(fi, FM_DEX, pet.dexReg, PET_DEX_BYTES, true);
    if (!fpTest && (p.caps & CAP_PLAY) && linkFriendTakeDay(fi, LDAY_PHOTO)) {
      FriendPhoto ph = {};
      ph.ver = PHOTO_VER;
      ph.mySpecies = pet.isEgg() || pet.vacant ? 0 : pet.speciesId;
      ph.myShiny = pet.shiny;
      ph.frSpecies = p.species;
      ph.frShiny = p.shiny;
      ph.epoch = rtcEpoch();
      strncpy(ph.name, linkFriendAt(fi).name, LINK_NAME);
      Serial.printf("amigo: foto con %s %s\n", ph.name, photoAdd(ph) ? "guardada" : "sin SD");
      fpPhotoN = photoCount();
    }
  }
}

// ---------- encima de "见面" y de la pagina del amigo: preguntas y esperas ----------

static const char *fpLeaderName(uint8_t r, uint8_t g) {
  if (r >= REGION_COUNT) return "?";
  const GymPerson &p = g >= 8 ? LEAGUE[r][0] : GYMS[r][g < 8 ? g : 0].leader;
  return gLang == LANG_ZH ? p.zh : p.en;
}

bool friendOverlay() { return fpIn != FPI_NONE || fpWaitHost >= 0 || fpGift.on || fpGiftAsk >= 0 || timeLeft(fpMsgUntil); }

void friendDrawOverlay() {
  char l1[64], l2[64];
  if (fpIn == FPI_GIFT) {
    snprintf(l1, sizeof(l1), T(S_FR_GIFT_IN_FMT), friendName(fpInFi));
    snprintf(l2, sizeof(l2), T(S_FR_GIFT_IN2_FMT), T(ITEM_NAME[fpInItem]));
    drawAskBox(l1, l2);
  } else if (fpIn == FPI_RACE || fpIn == FPI_GYM) {
    snprintf(l1, sizeof(l1), T(fpIn == FPI_RACE ? S_FR_RACE_IN_FMT : S_FR_GYM_IN_FMT), friendName(fpInFi));
    snprintf(l2, sizeof(l2), T(S_FR_GO_Q_FMT),
             fpIn == FPI_RACE ? T(EV_NAME[fpInInv.a < EV_COUNT ? fpInInv.a : 0]) : fpLeaderName(fpInInv.a, fpInInv.b));
    drawAskBox(l1, l2);
  } else if (fpGiftAsk >= 0) {
    drawAskBox(T(ITEM_NAME[fpGiftAsk]), T(S_FR_GIFT_Q));
  } else if (fpGift.on) {
    lkPanel(nullptr, T(S_FR_GIFT_WAIT), nullptr);
  } else if (fpWaitHost >= 0) {
    lkPanel(nullptr, T(S_FR_WAIT_FRIEND), nullptr);
  } else if (timeLeft(fpMsgUntil) && fpMsg) {
    lkPanel(nullptr, fpMsg, nullptr);
  }
}

// un toque con algo encima: true si era para eso
bool friendOverlayTap(int16_t x, int16_t y) {
  uint32_t now = millis();
  if (fpIn == FPI_GIFT) {
    bool si = askTap(x, y) == 1;
    if (si) {
      if (!fpTest) pet.give(fpInItem, 1);
      fpGiftSeen.add(fpInTx);
      snprintf(fpToastBuf, sizeof(fpToastBuf), T(S_FR_GIFT_GOT_FMT), T(ITEM_NAME[fpInItem]));
      toast(fpToastBuf);
      sfxPlay(SFX_MEDAL);
    }
    fpAnswerGift(fpInFi, fpInTx, si);
    Serial.printf("amigo: regalo de %s %s%s\n", friendName(fpInFi), si ? "cogido" : "rechazado", fpTest ? " (prueba)" : "");
    fpIn = FPI_NONE;
    return true;
  }
  if (fpIn == FPI_RACE || fpIn == FPI_GYM) {
    uint8_t kind = fpInInv.kind;
    fpIn = FPI_NONE;
    if (askTap(x, y) != 1) {
      fpAnswerInvite(fpInFi, kind, FW_NO);
      return true;
    }
    const char *why = fpSportsBlocked();
    if (why) {
      toast(why);
      fpAnswerInvite(fpInFi, kind, FW_CANT);
      return true;
    }
    sfxPlay(SFX_TAP);
    fpAnswerInvite(fpInFi, kind, FW_OK);
    fpWaitHost = fpInFi;
    fpWaitUntil = now + (kind == FK_RACE ? 15000 : 180000);  // el gimnasio: el otro elige equipo y orden
    return true;
  }
  if (fpGiftAsk >= 0) {
    uint8_t it = (uint8_t)fpGiftAsk;
    fpGiftAsk = -1;
    if (askTap(x, y) != 1 || fpFi < 0 || !pet.bag[it]) return true;
    uint32_t tx = esp_random();
    uint8_t b[5];
    fPutGift(b, tx, it);
    if (!linkSendGame(fpFi, FM_GIFT, b, sizeof(b), true)) {
      toast(T(S_FR_GONE));
      return true;
    }
    if (!fpTest) {
      pet.bag[it]--;  // fuera de la mochila mientras el otro decide (si no lo coge, vuelve)
      pet.markDirty();
    }
    fpGift.start(tx, it, now);
    sfxPlay(SFX_TAP);
    return true;
  }
  if (fpGift.on || fpWaitHost >= 0) return true;  // esperando: el toque no hace nada
  if (timeLeft(fpMsgUntil)) {
    fpMsgUntil = 0;
    return true;
  }
  return false;
}

// deslizar con algo encima: cancela la pregunta (como un no)
bool friendOverlaySwipe() {
  if (fpIn == FPI_GIFT) {
    fpAnswerGift(fpInFi, fpInTx, false);
    fpIn = FPI_NONE;
    return true;
  }
  if (fpIn == FPI_RACE || fpIn == FPI_GYM) {
    fpAnswerInvite(fpInFi, fpInInv.kind, FW_NO);
    fpIn = FPI_NONE;
    return true;
  }
  if (fpGiftAsk >= 0) {
    fpGiftAsk = -1;
    return true;
  }
  if (fpWaitHost >= 0) {  // ya no quiero esperar
    friendCancel(fpWaitHost);
    fpWaitHost = -1;
    gymsHelperEnd();
    return true;
  }
  return fpGift.on;  // el regalo espera su respuesta (o el plazo)
}

// ---------- dibujar ----------

static void fpMenuRow(int row, uint16_t fill, const char *const *icon, bool on, StrId title, const char *sub) {
  hubRow(row, fill, icon, on, T(title), sub);
}

static void renderFriendMenu() {
  gfx->fillScreen(RGB565_BLACK);
  gfx->fillCircle(CX, CY, 231, UI_BG_DAY);
  gfx->setTextColor(UI_INK);
  setSize(3);
  const char *n = friendName(fpFi);
  setCur(centerX(n, 3), 50);
  printT(n);
  bool cerca = fpPeer(fpFi) != nullptr;
  char ph[24];
  snprintf(ph, sizeof(ph), T(S_FR_PHOTO_SUB_FMT), (unsigned)fpPhotoN);
  const bool on[FPM_ROWS] = { cerca && fpHas(fpFi, CAP_PLAY), cerca && fpHas(fpFi, CAP_GIFT), cerca && fpHas(fpFi, CAP_RACE),
                              cerca && fpHas(fpFi, CAP_GYM), true };
  const char *old = cerca ? T(S_FR_OLD) : T(S_FR_GONE);
  fpMenuRow(FPM_PLAY, C565(0xf8, 0xd0, 0xc8), SPR_ICON_PLAY, on[0], S_FR_PLAY, on[0] ? T(S_FR_PLAY_SUB) : old);
  fpMenuRow(FPM_GIFT, 0xFF2E, SPR_ICON_CANDY, on[1], S_FR_GIFT, on[1] ? T(S_FR_GIFT_SUB) : old);
  fpMenuRow(FPM_RACE, 0xB6F2, SPR_ICON_TROPHY, on[2], S_FR_RACE, on[2] ? T(S_FR_RACE_SUB) : old);
  fpMenuRow(FPM_GYM, 0xAEFF, SPR_ICON_GYM, on[3], S_FR_GYM, on[3] ? T(S_FR_GYM_SUB) : old);
  fpMenuRow(FPM_PHOTO, C565(0xd8, 0xd8, 0xf8), SPR_ICON_STAR, on[4], S_FR_PHOTO, ph);
  gfx->setTextColor(UI_HINT);
  setSize(2);
  setCur(centerX(T(S_SWIPE_R_BACK), 2), 392);
  printT(T(S_SWIPE_R_BACK));
}

// fuegos artificiales: anillos de puntos que se abren
static void fpFireworks(int cx, int cy, uint32_t t) {
  static const uint16_t COL[4] = { C565(0xf8, 0x60, 0x60), C565(0xf8, 0xd0, 0x40), C565(0x60, 0xc0, 0xf8),
                                   C565(0x90, 0xe0, 0x70) };
  for (int ring = 0; ring < 3; ring++) {
    int r = (int)(t / 6) - ring * 30;
    if (r <= 0 || r > 150) continue;
    for (int k = 0; k < 12; k++) {
      float a = k * 0.5236f + ring * 0.26f;
      gfx->fillCircle(cx + (int)(cosf(a) * r), cy + (int)(sinf(a) * r * 0.8f), 4, COL[(k + ring) & 3]);
    }
  }
}

// ---------- jugar juntos ----------
// Tres juegos seguidos de FRIEND_PLAY_SEG ms con el mismo guion en las dos placas (empiezan a la
// vez): correr (uno persigue al otro hasta alcanzarlo), pasarse la pelota y bailar. Empieza (persigue
// y saca) el de numero de jugador mas bajo, asi las dos pantallas cuentan lo mismo. En cada placa el
// compañero propio sale a la izquierda. Chocar los cinco se puede en cualquier momento.
#define FRIEND_PLAY_SEG (FRIEND_PLAY_MS / 3)
#define FP_PASS_MS 1500   // la pelota, de uno a otro
static bool fpFirst() { return fpFi >= 0 && fpFi < (int8_t)linkFriendCount() && linkSelfId() < linkFriendAt(fpFi).pid; }

static float fpTri(float x) {  // 0 -> 1 -> 0 en cada unidad
  x -= floorf(x);
  return x < 0.5f ? x * 2 : 2 - x * 2;
}

static uint8_t fpCan(PmdMon &m, uint8_t a) { return m.has(a) ? a : (uint8_t)PMD_IDLE; }  // si no tiene esa accion, quieto

// correr: el que va delante recorre la pradera de un lado a otro y el otro le sigue cada vez mas cerca
static float fpRunX(int32_t ms) { return 100 + 266 * fpTri(ms / 2600.0f); }

// pelota (un icono de pokeball): de x0 a x1 en un arco
static void fpBall(int x0, int x1, float f) {
  int x = x0 + (int)((x1 - x0) * f);
  int y = PET_GROUND - 80 - (int)(120 * sinf(f * 3.14159f));
  drawMap(SPR_ICON_PLAY, 16, x - 16, y - 16, 2, false);
}

// nota musical flotando
static void fpNote(int x, int y, uint16_t c) {
  gfx->fillCircle(x, y, 5, c);
  gfx->fillRect(x + 3, y - 18, 3, 18, c);
  gfx->fillRect(x + 3, y - 18, 10, 3, c);
}

static void fpLabel(const char *big, const char *small) {
  gfx->fillRoundRect(CX - 165, 322, 330, 68, 14, UI_WHITE);
  gfx->drawRoundRect(CX - 165, 322, 330, 68, 14, UI_INK);
  gfx->setTextColor(UI_INK);
  setSize(3);
  setCur(CX - textW(big, 3) / 2, 330);
  printT(big);
  setSize(2);
  gfx->setTextColor(UI_HINT);
  setCur(CX - textW(small, 2) / 2, 364);
  printT(small);
}

static void renderFriendPlay(uint32_t now) {
  drawBackground();
  uint32_t t = now - fpPlayT0;
  bool five = fpFiveAt && now - fpFiveAt < 1800;
  int seg = (int)(t / FRIEND_PLAY_SEG);
  int32_t ts = (int32_t)(t % FRIEND_PLAY_SEG);
  bool first = fpFirst();
  PmdMon &me = pmd, &fr = visitorPmd;
  int xm = 140, xf = 326;
  uint8_t am = PMD_IDLE, af = PMD_IDLE;
  StrId titulo = S_FR_RUN;
  if (five) {  // chocan: los dos al centro
    xm = 190;
    xf = 276;
  } else if (seg == 0) {  // correr: persigue el primero; al final le alcanza
    titulo = S_FR_RUN;
    int32_t lag = 60 + 700 * (FRIEND_PLAY_SEG - ts) / FRIEND_PLAY_SEG;
    int32_t tt = ts < (int32_t)FRIEND_PLAY_SEG - 900 ? ts : (int32_t)FRIEND_PLAY_SEG - 900;  // al final, quietos
    float lead = fpRunX(tt), chase = fpRunX(tt - lag);
    bool derecha = fpRunX(tt + 40) > lead;
    bool parados = ts >= (int32_t)FRIEND_PLAY_SEG - 900;
    uint8_t walk = derecha ? PMD_WALKR : PMD_WALKL;
    if (first) {  // yo persigo
      xm = (int)chase; xf = (int)lead;
    } else {
      xm = (int)lead; xf = (int)chase;
    }
    am = parados ? fpCan(me, PMD_HOP) : fpCan(me, walk);
    af = parados ? fpCan(fr, PMD_HOP) : fpCan(fr, walk);
    if (parados) {  // alcanzado: "!" y un corazon
      int xl = first ? xf : xm;
      gfx->fillRoundRect(xl - 14, PET_GROUND - 190, 28, 34, 8, UI_WHITE);
      gfx->setTextColor(UI_BAR_BAD);
      setSize(3);
      setCur(xl - textW("!", 3) / 2, PET_GROUND - 184);
      printT("!");
      drawMap(SPR_HEART, 32, (xm + xf) / 2 - 16, PET_GROUND - 170, 1, false);
    }
  } else if (seg == 1) {  // pelota: saca el primero (a la izquierda en su placa, a la derecha en la otra)
    titulo = S_FR_CATCH;
    int32_t k = ts / FP_PASS_MS;
    float f = (ts % FP_PASS_MS) / (float)FP_PASS_MS;
    bool lanzaPrimero = (k % 2) == 0;
    bool lanzoYo = lanzaPrimero == first;
    int x0 = lanzoYo ? xm : xf, x1 = lanzoYo ? xf : xm;
    if (ts < (int32_t)FRIEND_PLAY_SEG - 600) {
      fpBall(x0, x1, f);
      uint8_t lanza = f < 0.25f ? PMD_ATTACK : PMD_IDLE, coge = f > 0.7f ? PMD_HOP : PMD_IDLE;
      am = lanzoYo ? fpCan(me, lanza) : fpCan(me, coge);
      af = lanzoYo ? fpCan(fr, coge) : fpCan(fr, lanza);
    }
  } else {  // bailar: saltan a la vez, pose, vuelta y otra vez a saltar
    titulo = S_FR_DANCE;
    xm = 175;
    xf = 291;
    uint8_t a = PMD_HOP;
    if (ts >= 2000 && ts < 3000) a = PMD_POSE;
    else if (ts >= 3000 && ts < 4500) a = ((ts / 180) & 1) ? PMD_WALKL : PMD_WALKR;  // vuelta
    am = fpCan(me, a);
    af = fpCan(fr, a == PMD_WALKL ? PMD_WALKR : a == PMD_WALKR ? PMD_WALKL : a);  // la vuelta, en espejo
    static const uint16_t NC[3] = { C565(0xf8, 0x60, 0x90), C565(0x60, 0x90, 0xf8), C565(0xf0, 0xb0, 0x30) };
    for (int k = 0; k < 3; k++) {
      int y = 250 - (int)((ts / 15 + k * 55) % 160);
      fpNote(CX - 70 + k * 70 + (int)(sinf(ts / 250.0f + k) * 10), y, NC[k]);
    }
  }
  if (me.loaded) drawPmdActM(me, am, xm, PET_GROUND, t, true, false, 3);
  if (fr.loaded) drawPmdActM(fr, af, xf, PET_GROUND, t, true, false, 3);
  if (five) {
    fpFireworks(CX, 170, now - fpFiveAt);
    fpLabel(T(S_FR_HIFIVE), T(S_FR_HIFIVE_HINT));
  } else {
    fpLabel(T(titulo), T(S_FR_HIFIVE_HINT));
  }
}

static int fpGiftItems(uint8_t out[ITEM_COUNT]) {
  int n = 0;
  for (int i = 0; i < ITEM_COUNT; i++)
    if (pet.bag[i]) out[n++] = (uint8_t)i;
  return n;
}

static void renderFriendGift() {
  gfx->fillScreen(RGB565_BLACK);
  gfx->fillCircle(CX, CY, 231, UI_BG_DAY);
  gfx->setTextColor(UI_INK);
  setSize(2);
  setCur(centerX(T(S_FR_GIFT_PICK), 2), 56);
  printT(T(S_FR_GIFT_PICK));
  uint8_t its[ITEM_COUNT];
  int n = fpGiftItems(its);
  if (!n) {
    gfx->setTextColor(UI_HINT);
    setCur(centerX(T(S_FR_GIFT_EMPTY), 2), CY - 8);
    printT(T(S_FR_GIFT_EMPTY));
  }
  if (fpGiftPage * 9 >= n) fpGiftPage = 0;
  for (int k = 0; k < 9 && fpGiftPage * 9 + k < n; k++) {
    uint8_t it = its[fpGiftPage * 9 + k];
    int x = FP_GRID_X + (k % 3) * FP_CELL, y = FP_GRID_Y + (k / 3) * FP_CELL;
    gfx->fillRoundRect(x + 4, y + 4, FP_CELL - 8, FP_CELL - 8, 12, UI_WHITE);
    gfx->drawRoundRect(x + 4, y + 4, FP_CELL - 8, FP_CELL - 8, 12, UI_TRACK);
    drawItemIcon(it, x + 24, y + 10);
    char c[8];
    snprintf(c, sizeof(c), "x%u", pet.bag[it]);
    gfx->setTextColor(UI_INK);
    setSize(2);
    setCur(x + FP_CELL / 2 - textW(c, 2) / 2, y + 64);
    printT(c);
  }
  gfx->setTextColor(UI_HINT);
  setSize(2);
  setCur(centerX(T(S_SWIPE_R_BACK), 2), 396);
  printT(T(S_SWIPE_R_BACK));
}

static void renderFriendPhotos() {
  gfx->fillScreen(RGB565_BLACK);
  gfx->fillCircle(CX, CY, 231, UI_BG_DAY);
  gfx->setTextColor(UI_INK);
  setSize(3);
  setCur(centerX(T(S_FR_PHOTO), 3), 44);
  printT(T(S_FR_PHOTO));
  if (!fpPhotoN) {
    gfx->setTextColor(UI_HINT);
    setSize(2);
    setCur(centerX(T(S_FR_NO_PHOTOS), 2), CY - 8);
    printT(T(S_FR_NO_PHOTOS));
  }
  for (int k = 0; k < 4; k++) {
    FriendPhoto p;
    if (!photoGet(fpPhotoPage * 4 + k, p)) break;
    int y = FP_PH_Y + k * FP_PH_RH;
    gfx->fillRoundRect(FP_PH_X, y, FP_PH_W, FP_PH_RH - 6, 10, UI_WHITE);
    const uint8_t *a = p.mySpecies > 0 ? thumbs.get(p.mySpecies) : nullptr;
    const uint8_t *b = p.frSpecies > 0 ? thumbs.get(p.frSpecies) : nullptr;
    if (a) drawThumbIn(a, FP_PH_X + 4, y + 3, 1, false, 60);
    if (b) drawThumbIn(b, FP_PH_X + 58, y + 3, 1, false, 60);
    char d[24];
    albumDate(p.epoch, d, sizeof(d));
    gfx->setTextColor(UI_INK);
    setSize(2);
    setCur(FP_PH_X + 126, y + 10);
    printT(p.name);
    gfx->setTextColor(UI_HINT);
    setCur(FP_PH_X + 126, y + 36);
    printT(d);
  }
  gfx->setTextColor(UI_HINT);
  setSize(2);
  setCur(centerX(T(S_SWIPE_R_BACK), 2), 396);
  printT(T(S_SWIPE_R_BACK));
}

void renderFriend() {
  uint32_t now = millis();
  switch (fpStage) {
    case FP_PLAY: renderFriendPlay(now); wakeScreen(now); break;  // mirando no se atenua
    case FP_GIFT: renderFriendGift(); break;
    case FP_PHOTOS: renderFriendPhotos(); break;
    default: renderFriendMenu();
  }
  friendDrawOverlay();
  drawToast();
  gfx->flush();
}

// ---------- tocar ----------

void friendTap(int16_t x, int16_t y) {
  uint32_t now = millis();
  if (fpStage == FP_PLAY) {  // tocar a mi compañero (la mitad izquierda): si el otro tambien, chocan
    if (x < CX && y > 110 && y < PET_GROUND + 20) {
      linkSendGame(fpFi, FM_TAP, nullptr, 0, false);
      if (fpFive.tapMine(now)) fpHighFive(now);
      else sfxPlay(SFX_TAP);
    }
    return;
  }
  if (fpStage == FP_GIFT) {
    if (x < FP_GRID_X || y < FP_GRID_Y) return;
    int c = (x - FP_GRID_X) / FP_CELL, r = (y - FP_GRID_Y) / FP_CELL;
    if (c > 2 || r > 2) return;
    uint8_t its[ITEM_COUNT];
    int n = fpGiftItems(its), k = fpGiftPage * 9 + r * 3 + c;
    if (k >= n) return;
    sfxPlay(SFX_TAP);
    fpGiftAsk = its[k];
    return;
  }
  if (fpStage == FP_PHOTOS) return;
  // menu: las mismas filas que actividades
  if (x < FP_ROW_X || x >= FP_ROW_X + FP_ROW_W || y < HUB_ROW_Y0) return;
  int row = (y - HUB_ROW_Y0) / HUB_ROW_STEP;
  if (row >= FPM_ROWS || y - HUB_ROW_Y0 - row * HUB_ROW_STEP > HUB_ROW_H) return;
  if (row == FPM_PHOTO) {
    sfxPlay(SFX_TAP);
    fpPhotoPage = 0;
    fpStage = FP_PHOTOS;
    return;
  }
  static const uint32_t NEED[4] = { CAP_PLAY, CAP_GIFT, CAP_RACE, CAP_GYM };
  if (!fpPeer(fpFi)) { toast(T(S_FR_GONE)); return; }
  if (!fpHas(fpFi, NEED[row])) { toast(T(S_FR_OLD)); return; }
  if (row == FPM_PLAY) {
    const char *why = fpWhyNot();
    if (why) { toast(why); return; }
    fpStartPlay(fpFi, true);
  } else if (row == FPM_GIFT) {
    sfxPlay(SFX_TAP);
    fpGiftPage = 0;
    fpStage = FP_GIFT;
  } else {
    const char *why = fpSportsBlocked();
    if (why) { toast(why); return; }
    sfxPlay(SFX_TAP);
    if (row == FPM_RACE) sportsWithFriend(fpFi);
    else gymsWithFriend(fpFi);
  }
}

void friendSwipe(int dir) {
  if (fpStage == FP_GIFT && dir < 0) {  // izquierda: mas objetos
    uint8_t its[ITEM_COUNT];
    int n = fpGiftItems(its);
    fpGiftPage = (uint8_t)((fpGiftPage + 1) * 9 < n ? fpGiftPage + 1 : 0);
    return;
  }
  if (fpStage == FP_PHOTOS) {
    if (dir < 0) {
      if ((fpPhotoPage + 1) * 4 < fpPhotoN) fpPhotoPage++;
    } else if (fpPhotoPage > 0) {
      fpPhotoPage--;
    } else {
      fpStage = FP_MENU;
    }
    return;
  }
  if (dir <= 0) return;
  if (fpStage != FP_MENU) {  // a la derecha: al menu del amigo
    fpStage = FP_MENU;
    visitorPmd.unload();
    return;
  }
  closeFriendMenu();  // y del menu, a "见面"
}
