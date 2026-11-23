// Pagina "见面": encontrar otra placa (ESP-NOW, link.cpp) y hacerse amigos. De momento solo la
// conexion: los juegos entre placas vendran despues.
// (pestaña de TamaPoke.ino: Arduino la une al final del principal)

#define LK_ROW_X 63
#define LK_ROW_W 340
#define LK_ROW_H 64
#define LK_ROW_Y0 150
#define LK_ROW_STEP 72
#define LK_ROWS 3
#define LK_RESULT_MS 3500

static uint64_t lkAskPid = 0;   // el niño toco una placa: "要成好友吗？" (0 = no)
static bool lkAskRepair = false;
static uint32_t lkResultUntil = 0;  // como acabo el emparejamiento, en pantalla hasta entonces
static uint8_t lkOrder[LINK_NEARBY], lkN = 0;

void openLink() {
  if (pet.ceremony) return;
  if (!linkStart()) {  // sin radio (no deberia)
    toast(T(S_LINK_FAIL));
    return;
  }
  linkOpen = true;
  lkAskPid = 0;
  lkResultUntil = 0;
}

void closeLink() {
  linkOpen = false;
  lkAskPid = 0;
  friendLinkClosed();
  linkStop();  // la radio vuelve a la red de casa
}

// amigos primero, luego por señal
static void lkSort() {
  lkN = linkNearbyCount();
  for (uint8_t i = 0; i < lkN; i++) lkOrder[i] = i;
  for (uint8_t i = 1; i < lkN; i++)
    for (uint8_t j = i; j > 0; j--) {
      const LinkPeer &a = linkNearby(lkOrder[j - 1]), &b = linkNearby(lkOrder[j]);
      bool antes = (b.fr >= 0) != (a.fr >= 0) ? b.fr >= 0 : b.rssi > a.rssi;
      if (!antes) break;
      uint8_t t = lkOrder[j];
      lkOrder[j] = lkOrder[j - 1];
      lkOrder[j - 1] = t;
    }
}

// el nombre con el que se enseña otra placa: el del dueño si es amigo; si no, su compañero
static const char *lkName(const LinkPeer &p) {
  if (p.fr >= 0) return shownName(linkFriendAt(p.fr).name);
  return p.species > 0 ? dexName(p.species) : "TamaPoke";
}

static const char *lkSub(const LinkPeer &p, uint16_t &fill) {
  if (!linkPeerSameMajor(p)) { fill = UI_TRACK; return T(S_LINK_UPDATE); }
  if (p.fr >= 0) {
    fill = 0xB6F2;
    if (p.fstate == LFS_OK) return T(S_LINK_FRIEND_OK);
    if (p.fstate == LFS_LOST) { fill = C565(0xf8, 0xe0, 0xa8); return T(S_LINK_REPAIR); }
    return T(S_LINK_FRIEND);
  }
  if (!linkPeerNear(p)) { fill = UI_TRACK; return T(S_LINK_FAR); }
  fill = UI_WHITE;
  return T(S_LINK_TAP);
}

static void lkRow(int row, const LinkPeer &p) {
  int y = LK_ROW_Y0 + row * LK_ROW_STEP;
  uint16_t fill;
  const char *sub = lkSub(p, fill);
  gfx->fillRoundRect(LK_ROW_X, y, LK_ROW_W, LK_ROW_H, 18, fill);
  gfx->drawRoundRect(LK_ROW_X, y, LK_ROW_W, LK_ROW_H, 18, UI_INK);
  const uint8_t *t = p.species > 0 ? thumbs.get(p.species) : nullptr;
  if (t) drawThumbIn(t, LK_ROW_X + 8, y + 4, 1, false, 56);
  else drawMap(SPR_EGG, 32, LK_ROW_X + 20, y + 16, 1, false);
  gfx->setTextColor(UI_INK);
  setSize(2);
  setCur(LK_ROW_X + 72, y + 10);
  printT(lkName(p));
  setCur(LK_ROW_X + 72, y + 36);
  printT(sub);
  // señal: 1-3 rayas
  int bars = p.rssi >= -55 ? 3 : p.rssi >= LINK_NEAR_RSSI ? 2 : 1;
  for (int k = 0; k < 3; k++) {
    int h = 8 + k * 7, x = LK_ROW_X + LK_ROW_W - 48 + k * 11;
    gfx->fillRect(x, y + LK_ROW_H / 2 + 14 - h, 7, h, k < bars ? UI_INK : UI_TRACK);
  }
}

// caja en medio (como la de preguntar), con 1-3 lineas (tambien la usa friend_play.ino)
void lkPanel(const char *l1, const char *l2, const char *big) {
  gfx->fillRoundRect(64, 160, 338, 160, 16, UI_WHITE);
  gfx->drawRoundRect(64, 160, 338, 160, 16, UI_INK);
  gfx->setTextColor(UI_INK);
  setSize(2);
  if (l1) { setCur(centerX(l1, 2), 182); printT(l1); }
  if (big) {
    setSize(4);
    setCur(centerX(big, 4), 214);
    printT(big);
    setSize(2);
  }
  if (l2) { setCur(centerX(l2, 2), big ? 276 : 222); printT(l2); }
}

// ondas que salen del centro (buscando)
static void lkRadar(int cx, int cy, int rMax, uint32_t now) {
  for (int k = 0; k < 3; k++) {
    int r = (int)((now / 12 + k * (rMax / 3)) % rMax);
    uint16_t c = r < rMax / 2 ? C565(0x8c, 0x9c, 0xe8) : C565(0xc0, 0xc8, 0xf0);
    gfx->drawCircle(cx, cy, r, c);
    gfx->drawCircle(cx, cy, r + 1, c);
  }
  drawMap(SPR_ICON_LINK, 16, cx - 24, cy - 24, 3, false);
}

void renderLink() {
  if (friendScreenOpen()) {  // la pagina de un amigo (friend_play.ino)
    renderFriend();
    return;
  }
  uint32_t now = millis();
  gfx->fillScreen(RGB565_BLACK);
  fillBackdrop(UI_BG_DAY);
  gfx->setTextColor(UI_INK);
  setSize(3);
  setCur(centerX(T(S_LINK_TITLE), 3), 50);
  printT(T(S_LINK_TITLE));
  lkSort();
  if (!lkN) {
    lkRadar(CX, 214, 96, now);
    gfx->setTextColor(UI_INK);
    setSize(2);
    setCur(centerX(T(S_LINK_WAITING), 2), 322);
    printT(T(S_LINK_WAITING));
    gfx->setTextColor(UI_HINT);
    setCur(centerX(T(S_HUB_LINK_HINT), 2), 352);
    printT(T(S_HUB_LINK_HINT));
  } else {
    lkRadar(CX, 112, 30, now);
    for (int r = 0; r < LK_ROWS && r < lkN; r++) lkRow(r, linkNearby(lkOrder[r]));
  }
  gfx->setTextColor(UI_HINT);
  setSize(2);
  setCur(centerX(T(S_SWIPE_R_BACK), 2), 392);
  printT(T(S_SWIPE_R_BACK));

  // encima: preguntas y como va el emparejamiento
  const LinkPair &pr = linkPairing();
  int pi = linkPairNearby();
  char l1[48];
  if (pr.st == LP_PROMPT) {  // otra placa quiere ser amiga
    snprintf(l1, sizeof(l1), T(S_LINK_ASK_FMT), pi >= 0 ? lkName(linkNearby(pi)) : "TamaPoke");
    drawAskBox(l1, T(S_LINK_ASKED2));
  } else if (lkAskPid) {
    int i = -1;
    for (uint8_t k = 0; k < linkNearbyCount(); k++)
      if (linkNearby(k).pid == lkAskPid) i = k;
    if (i < 0) {
      lkAskPid = 0;  // se fue
    } else if (lkAskRepair) {
      drawAskBox(lkName(linkNearby(i)), T(S_LINK_REPAIR_Q));
    } else {
      snprintf(l1, sizeof(l1), T(S_LINK_ASK_FMT), lkName(linkNearby(i)));
      drawAskBox(l1, T(S_LINK_ASK2));
    }
  } else if (pr.st == LP_ASKING) {
    lkPanel(nullptr, T(S_LINK_WAIT_YES), nullptr);
  } else if (pr.st == LP_KEYING || pr.st == LP_CARDS) {
    char code[8];  // el mismo en las dos pantallas: asi se ve que se emparejaron entre ellas
    snprintf(code, sizeof(code), "%04u", pr.code());
    lkPanel(nullptr, T(S_LINK_CODE_HINT), code);
  } else if (pr.st == LP_DONE || pr.st == LP_FAILED || pr.st == LP_DECLINED) {
    if (!lkResultUntil) {
      lkResultUntil = now + LK_RESULT_MS;
      if (pr.st == LP_DONE) sfxPlay(SFX_MEDAL);
    }
    if (pr.st == LP_DONE) {
      snprintf(l1, sizeof(l1), T(S_LINK_DONE_FMT), shownName(linkPairName()));
      char code[8];
      snprintf(code, sizeof(code), "%04u", pr.code());
      lkPanel(l1, T(S_LINK_CODE_HINT), code);
    } else {
      lkPanel(nullptr, T(pr.st == LP_DECLINED ? S_LINK_SAID_NO : S_LINK_FAIL), nullptr);
    }
    if (!timeLeft(lkResultUntil)) {
      lkResultUntil = 0;
      linkPairClear();
    }
  }
  friendDrawOverlay();  // lo de los juegos (un regalo, una invitacion...) va encima de todo
  drawToast();
  gfx->flush();
}

void linkTap(int16_t x, int16_t y) {
  linkTouch();
  if (friendOverlayTap(x, y)) return;
  if (friendScreenOpen()) {
    friendTap(x, y);
    return;
  }
  const LinkPair &pr = linkPairing();
  if (pr.st == LP_PROMPT) {
    bool si = askTap(x, y) == 1;
    linkPairAnswer(si);
    if (si) sfxPlay(SFX_TAP);
    return;
  }
  if (lkAskPid) {
    uint64_t pid = lkAskPid;
    lkAskPid = 0;
    if (askTap(x, y) != 1) return;
    sfxPlay(SFX_TAP);
    uint8_t r = linkPairAsk(pid);
    if (r == LINK_ASK_FULL) toast(T(S_LINK_FULL));
    else if (r == LINK_ASK_OLD) toast(T(S_LINK_UPDATE));
    else if (r != LPA_REQ && r != LPA_OK) toast(T(S_LINK_FAIL));
    return;
  }
  if (pr.busy()) return;  // a medias: se espera
  if (pr.st != LP_IDLE) {  // tocar quita el resultado
    lkResultUntil = 0;
    linkPairClear();
    return;
  }
  if (x < LK_ROW_X || x >= LK_ROW_X + LK_ROW_W || y < LK_ROW_Y0) return;
  int row = (y - LK_ROW_Y0) / LK_ROW_STEP;
  if (row >= LK_ROWS || row >= lkN || y - LK_ROW_Y0 - row * LK_ROW_STEP > LK_ROW_H) return;
  const LinkPeer &p = linkNearby(lkOrder[row]);
  if (!linkPeerSameMajor(p)) { toast(T(S_LINK_UPDATE)); return; }
  if (p.fr >= 0) {
    if (p.fstate == LFS_LOST) {  // ya no tiene nuestra clave (o la perdimos): emparejar otra vez
      lkAskPid = p.pid;
      lkAskRepair = true;
    } else if (p.fstate == LFS_OK) {  // a su pagina: jugar, regalos, carrera, gimnasio, fotos
      sfxPlay(SFX_TAP);
      openFriendMenu((uint8_t)p.fr);
    }
    return;
  }
  if (!linkPeerNear(p)) { toast(T(S_LINK_FAR)); return; }
  sfxPlay(SFX_TAP);
  lkAskPid = p.pid;
  lkAskRepair = false;
}

void linkSwipe(int dir) {
  linkTouch();
  if (friendOverlaySwipe()) return;
  if (friendScreenOpen()) {
    friendSwipe(dir);
    return;
  }
  if (lkAskPid) { lkAskPid = 0; return; }  // deslizar cancela la pregunta
  if (linkPairing().st == LP_PROMPT) { linkPairAnswer(false); return; }
  if (dir > 0) closeLink();  // a la derecha: vuelta a actividades
}
