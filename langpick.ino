// First-run language picker (Fake Pod fork): English / 繁體中文 / 简体中文, shown before anything else until a language has been stored (i18n.cpp: langSaved()).
// Each label is drawn in its own script, so the font follows gLang row by row (applyLangFont). A tab of TamaPoke.ino: Arduino joins the .ino files.


static const Lang PICK_LANG[3] = { LANG_EN, LANG_ZHT, LANG_ZH };
static const char *const PICK_LABEL[3] = { "English", "繁體中文", "简体中文" };
#define PICK_ROW_Y 150
#define PICK_ROW_H 70
#define PICK_ROW_GAP 12

void renderLangPick() {
  const Lang keep = gLang;
  gfx->fillScreen(RGB565_BLACK);
  fillBackdrop(UI_BG_DAY);
  gLang = LANG_EN; applyLangFont();
  gfx->setTextColor(UI_INK);
  setSize(2);
  setCur(centerX("LANGUAGE", 2), 52);
  printT("LANGUAGE");
  gLang = LANG_ZHT; applyLangFont();
  setSize(2);
  setCur(centerX("語言 / 语言", 2), 88);
  printT("語言 / 语言");
  static const uint16_t ACC[3] = { 0x3C49, 0xEA87, 0x5CB8 };
  for (int i = 0; i < 3; i++) {
    int ry = PICK_ROW_Y + i * (PICK_ROW_H + PICK_ROW_GAP);
    gfx->fillRoundRect(70, ry, 326, PICK_ROW_H, 14, lerp565(ACC[i], UI_WHITE, 6, 8));
    gfx->drawRoundRect(70, ry, 326, PICK_ROW_H, 14, ACC[i]);
    gLang = PICK_LANG[i]; applyLangFont();
    gfx->setTextColor(UI_INK);
    setSize(i == 0 ? 3 : 4);                       // 4 = a 32 px Chinese glyph, about the weight of English at size 3
    setCur(centerX(PICK_LABEL[i], i == 0 ? 3 : 4), ry + 22);
    printT(PICK_LABEL[i]);
  }
  gLang = keep; applyLangFont();
  gfx->flush();
}

bool langPickTap(int16_t x, int16_t y) {
  for (int i = 0; i < 3; i++) {
    int ry = PICK_ROW_Y + i * (PICK_ROW_H + PICK_ROW_GAP);
    if (x >= 70 && x <= 396 && y >= ry && y <= ry + PICK_ROW_H) {
      setLang(PICK_LANG[i]);                       // stores it: the picker never comes back
      applyLangFont();
      gLangPick = false;
      lastRender = 0;
      sfxPlay(SFX_TAP);
      return true;
    }
  }
  return false;
}
