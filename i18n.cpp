#include "i18n.h"
#include "pet.h"        // MED_COUNT
#include <Preferences.h>

Lang gLang = LANG_DEFAULT;

// Tabla de cadenas [idioma][id]. Sin acentos ni enes: la fuente bitmap del
// firmware no los tiene (por eso el espanol ya iba "Esta", "bano", etc.).
static const char *const STRINGS[LANG_COUNT][STR_COUNT] = {
  // ---------------- EN ----------------
  {
    "Evolving!", "Yum yum!", "It likes it!", "It's hungry!", "Needs a bath!",
    "Worn out...", "Feeling sad...", "A bit chubby...", "It's SHINY!!", "It's happy",
    "THANKS! Farewell", "It ran away...", "Bye! Waving goodbye...",
    "EGG", "Legendary egg!?", "Rare egg!", "Tap the egg...", "It moves!", "Almost there!",
    "POKEDEX %u/1025",
    "%s%s Lv.%u",
    "Release %s?", "YES", "NO",
    "SETTINGS", "swipe up: back", "Lang",
    "MEDAL!", "AWESOME!", "%u DAY STREAK!",
    "STREAK %u  best %u", "BOND",
    "AGE %lu days", "tap name: rename",
    "ABILITY", "STA", "RES", "VIT", "WGT",
    "MEDALS %d/%d", "tap: back",
    "NAME:", "tap: back",
    "FOOD", "JOY", "ENE", "HYG",
    "PROGRESS", "Lv.%u", "%u min to Lv.%u", "EVOLUTION", "Final form",
    "Ready to evolve!", "All needs >=40 to evolve",
    "Evolves in %ud %uh", "Slip-ups: %u",
    "SND ON", "SND OFF",
    "EVOLVE!", "%s wants to tell you...", "%s feels abandoned...",
    "Evolve?", "Keep form", "Say goodbye?", "Goodbye", "Stay together",
    "Choose your starter",
    "No sprites", "Load them onto the SD",

    "Feeling sick...",

    "Collect", "Release", "FARM %u", "Farm is empty", "Swap in", "Hatch the egg first",
    "Off to the farm!",

    "Needs an SD card",

    "KANTO", "JOHTO", "HOENN", "SINNOH", "UNOVA", "KALOS", "ALOLA", "GALAR", "PALDEA",

    "SHOP  %ld", "BAG",
    "CANDY", "MEDICINE", "STA POTION", "VIT POTION", "CHARM",
    "EGG", "RARE EGG", "LEGEND EGG",
    "Bought!", "Not enough points", "None left - shop", "Done!", "Already at max", "Not sick",
    "Used on next egg",
    "Only with no partner", "No partner", "Pick an egg", "SHOP",

    "%u games %u wins", "+%ld points", "New record!",

    "WI-FI", "Phone: join TamaPoke", "open 192.168.4.1", "Wi-Fi saved", "Syncing chess...",
    "Sync failed",
    "Happy New Year!", "Happy Lunar New Year!", "Lantern Festival!", "Happy Valentine's!",
    "Happy Easter!", "Happy Mother's Day!", "Dragon Boat Festival!", "Happy Father's Day!",
    "Happy Canada Day!", "Mid-Autumn Festival!", "Happy Thanksgiving!", "Happy Halloween!",
    "Merry Christmas!", "Happy Birthday %s!",
    "%s DEX DONE!", "+%d pts  Shiny egg!", "SHINY LEGEND",
    "Loves the rain!", "Hates the rain...", "Snow! Yay!", "Brr... so cold", "Lovely sunshine!",
    "STEPS TODAY %lu",
    "ALBUM", "ALBUM %d", "No memories yet",
    "Farewell", "Ran away", "Released", "To the farm",
    "Easygoing", "Glutton", "Playful", "Tidy", "Lazy",
    "Good morning!", "Getting sleepy...", "I want a berry...", "Bath time?", "Play with me!",
    "So tired...", "You won %u games!", "Good games yesterday!", "I love you, %s!", "Hi!",
    "My tummy rumbles", "Let's play!", "I like being clean", "Five more minutes", "What a nice day",
    "Bow", "Flower", "Cap", "Witch hat", "Top hat", "Crown",
    "ACCESSORIES", "OWNED", "WEARING", "Put on!", "Taken off",
    "%s came to visit!",
    "No backup yet", "Restore this backup?", "Restoring...",
    "So comfy~", "FARM HARVEST!", "+%u pts",
    "Hehe~", "Giggle giggle!", "Haha, that tickles!", "Hey, stop it!", "Wheee!",
    "Yummy!", "So fun!", "*big stretch*", "Took my medicine!",
    "EVO POTION", "Not needed now",
    "LOCKED", "Register every egg Pokemon of", "NEW REGION!",
    "energy drains %u%% slower", "joy drains %u%% slower", "gets sick %u%% less",
    "%u/%u", "SUN", "MON", "TUE", "WED", "THU", "FRI", "SAT",
    "STATS", "Raised", "Pokedex", "Shiny", "Regions open", "Regions done", "Best streak", "Medals",
    "Steps", "Chess points", "Farm",
    "tap: open",
    "SYNC NOW", "No new games",
    "also your chess.com name", "Set chess.com name in WI-FI",
    "10,000 steps! +10 pts",
    "ACTIVITIES", "TOURNAMENT", "Win a Mega Stone", "Asleep...", "swipe right: back",
    "MEGA STONE", "MEGA %s%s", "No Mega form", "Already Mega", "MEGA EVOLVED!", "Back to normal",
    "100m DASH", "HURDLES", "50m SWIM", "LONG JUMP", "HIGH JUMP",
    "Speed counts", "Speed and spring", "Speed and stamina", "Power and speed", "Power counts",
    "tap: start", "GO!", "GOLD!", "SILVER", "BRONZE",
    "Got a Mega Stone!", "Today's stone is won", "%u.%u s", "%u.%02u m", "vs %s & %s", "Too tired", "YOU",
    "Bonus: Electric, Fighting", "Bonus: Fighting, Bug", "Bonus: Water, Ice", "Bonus: Fighting, Flying",
    "Bonus: Flying, Psychic", "You +%u%%", "You -%u%%",
    "In the grass: ",
    "Phone steps code",
    "WEIGHTS", "CLIMBING", "DIVING", "SKATING",
    "TORCH RUN", "TRAIL RUN", "Power counts", "Power and agility",
    "Technique counts", "Speed counts", "Stamina counts", "A bit of everything",
    "Bonus: Fighting, Steel, Rock", "Bonus: Rock, Ground, Bug", "Bonus: Water, Psychic, Flying", "Bonus: Ice, Fairy",
    "Bonus: Fire, Dragon", "Bonus: Dark, Ghost, Poison", "%u kg", "%u.%u pts",
    "%u:%02u", "GYMS", "%s GYMS", "Badges %u/8",
    "LEAGUE", "CHAMPION", "swipe: regions", "Win all 8 badges first",
    "Already tried today", "Win the badge before", "Need 2 in the farm", "Elite Four",
    "Champion", "CHALLENGE", "REMATCH", "PICK 2",
    "NEXT", "LINEUP", "Tap yours to swap", "START",
    "%s  Round %u/3", "You win!", "You lose", "tap: continue",
    "Got the %s!", "Also: %s", "Lost. Try again tomorrow", "CHAMPION!",
    "League over. Try tomorrow", "Normal", "Fire", "Water",
    "Grass", "Electric", "Ice", "Fighting",
    "Poison", "Ground", "Flying", "Psychic",
    "Bug", "Rock", "Ghost", "Dragon",
    "Dark", "Steel", "Fairy",
    "%u badges",
    "FUN", "Fish, grow, dig...", "FUN  %ld", "ORCHARD",
    "FISHING", "MINING", "TREASURE", "Grass Bug Ground Poison Fairy",
    "Needs Water Ice Dragon", "Needs Rock Steel Fighting Fire", "Normal Fly Elec Psy Ghost Dark", "%u can go",
    "%u pts", "Won at chess: double luck!", "Played chess: +1 loot", "Play chess for good luck",
    "WHO GOES?", "Out treasure hunting", "%u min left", "%u h left",
    "%u ready", "Back! Collect", "PLANT", "WATER",
    "HARVEST", "Watered", "Ripe!", "Clock not set yet",
    "CAST", "Wait for a bite...", "Now! Tap!", "Great catch!",
    "Just a small one", "DIG", "Tap the rock!", "SET OFF",
    "Back in 1 hour", "COLLECT", "Someone is already out", "You got",
    "STA FRUIT", "RES FRUIT", "VIT FRUIT", "GOLD FRUIT",
    "FISH", "PEARL", "MEGA SHARD", "WATER EGG",
    "FOSSIL EGG", "Full today: just a snack", "10 shards = 1 Mega Stone", "STA +3",
    "RES +3", "VIT +3", "All +3, so happy!", "So happy!",
    "Yum, so full!",
    "FARM", "%u Pokemon", "RESTORE", "x%u",
    "Good at: %s, %s", "HP %u  ATK %u  DEF %u  SPD %u", "STA %u  RES %u  VIT %u", "Try other teammates,",
    "or train with fruits", "Badges", "League titles", "Fun finds",
    "%u h", "%u min",
    "Buy for %u pts?", "Spend %u pts?",
  },
  // ---------------- ZH ----------------
  // Chino simplificado. Los especificadores van en el orden de la fila
  // inglesa: T(id) se rellena con varargs en un orden fijo en cada llamada. Todo debe estar en GB2312, que es lo que
  // trae la fuente (lo comprueba test/test_tools.py).
  {
    "正在进化！", "好吃！", "它很喜欢！", "肚子饿了！", "该洗澡了！",
    "累坏了...", "好难过...", "有点胖了...", "是异色！！", "心情很好",
    "谢谢！再见", "它离家出走了...", "拜拜！挥手告别...",

    "蛋", "传说之蛋！？", "稀有的蛋！", "摸摸蛋...", "动了！", "快孵出来了！",

    "图鉴 %u/1025",

    "%s%s Lv.%u",

    "放生%s？", "是", "否",

    "设置", "上滑：返回", "语言",

    "奖章！", "太棒了！", "连续 %u 天！",

    "连续 %u  最高 %u", "羁绊",

    "年龄 %lu 天", "点名字：改名",

    "能力", "体力", "抵抗力", "活力", "体重",

    "奖章 %d/%d", "点一下：返回",

    "名字：", "点一下：返回",

    "饱食", "心情", "精力", "清洁",

    "成长", "Lv.%u", "%u分钟后 Lv.%u", "进化", "最终形态",

    "可以进化了！", "全部40以上才能进化",

    "%u天%u小时后可进化", "照顾失误：%u",

    "声音开", "声音关",

    "进化！", "%s有话想说...", "%s觉得被冷落了...",

    "要进化吗？", "保持原样", "要告别吗？", "再见", "继续在一起",

    "选择你的初始宝可梦",

    "没有图片", "请放进SD卡",

    "生病了...",

    "收藏", "放生", "农场 %u", "农场还是空的", "换出来", "先把蛋孵出来",
    "去农场啦！",

    "需要SD卡",

    "关都", "城都", "丰缘", "神奥", "合众", "卡洛斯", "阿罗拉", "伽勒尔", "帕底亚",

    "商店  %ld分", "背包",
    "糖果", "药", "体力药剂", "活力药剂", "异色符",
    "普通蛋", "稀有蛋", "传说蛋",
    "买好了！", "积分不够", "没有了，去商店买", "用好了！", "已经满了", "没有生病",
    "下一颗蛋自动生效",
    "没有伙伴时才能孵", "没有伙伴", "选一颗蛋来孵", "去商店",

    "%u盘 赢%u盘", "+%ld分", "创纪录了！",

    "Wi-Fi", "手机连 TamaPoke", "打开 192.168.4.1", "Wi-Fi 已保存", "正在同步棋局...",
    "同步失败",

    "新年快乐！", "春节快乐！", "元宵节快乐！", "情人节快乐！",
    "复活节快乐！", "母亲节快乐！", "端午安康！", "父亲节快乐！",
    "加拿大国庆日快乐！", "中秋节快乐！", "感恩节快乐！", "万圣节快乐！",
    "圣诞快乐！", "%s 生日快乐！",

    "%s图鉴集齐！", "+%d分 送异色传说蛋", "异色传说蛋",

    "下雨真开心！", "讨厌下雨…", "下雪啦，好开心！", "好冷…", "晒太阳真舒服！",

    "今日步数 %lu",

    "回忆录", "回忆录 %d", "还没有回忆",

    "告别", "离家出走", "放生", "进了农场",

    "性格：乖巧", "性格：贪吃", "性格：爱玩", "性格：爱干净", "性格：懒洋洋",

    "早上好！", "有点困了…", "想吃树果…", "想洗澡…", "陪我玩嘛！",
    "好累呀…", "昨天赢了%u盘，好厉害！", "昨天下棋辛苦啦！", "最喜欢 %s 了！", "嗨！",
    "肚子咕咕叫…", "我们来玩吧！", "我爱干干净净的", "再躺一会儿…", "今天也很开心",

    "蝴蝶结", "小花", "棒球帽", "巫师帽", "礼帽", "皇冠",
    "饰品", "已拥有", "戴着", "戴上了", "摘下了",

    "%s来串门啦！",

    "还没有备份", "用这份备份覆盖现在的进度？", "正在恢复…",

    "好舒服～", "农场收获！", "+%u分",

    "嘿嘿～", "咯咯咯！", "哈哈哈，好痒！", "别闹啦！", "转圈圈～",

    "好吃！", "好开心！", "伸个懒腰～", "自己吃了药！",

    "进化药水", "现在用不上",

    "未解锁", "集齐上一地区蛋里能孵的宝可梦", "新地区解锁！",

    "精力消耗慢 %u%%", "心情消耗慢 %u%%", "生病几率低 %u%%",

    "%u月%u日", "周日", "周一", "周二", "周三", "周四", "周五", "周六",

    "统计", "养过的宝可梦", "图鉴登记", "异色", "已解锁地区", "集齐地区", "最长连续照顾", "获得奖章",
    "总步数", "下棋赚的积分", "农场",
    "点一下：查看",
    "立即同步", "没有新对局",
    "还能设置 chess.com 名字", "先在 Wi-Fi 里设置 chess.com 名字",
    "走满一万步！+10 分",
    "活动", "锦标赛", "赢了得Mega石", "在睡觉...", "右滑：返回",
    "Mega石", "超级%s%s", "它没有超级进化", "已经是超级形态", "超级进化！", "变回来了",
    "100米短跑", "跨栏", "50米游泳", "跳远", "跳高",
    "比速度", "比速度和弹跳", "比速度和耐力", "比力量和速度", "比力量",
    "点一下：开始", "开始！", "金牌！", "银牌", "铜牌",
    "获得Mega石！", "今天的Mega石已拿过", "%u.%u 秒", "%u.%02u 米", "对手：%s、%s", "太累了", "你",
    "电、格斗属性有加成", "格斗、虫属性有加成", "水、冰属性有加成", "格斗、飞行属性有加成",
    "飞行、超能力属性有加成", "你 +%u%%", "你 -%u%%",
    "草丛里有：",
    "手机步数配对码",
    "举重", "攀岩", "跳水", "速度滑冰",
    "火炬长跑", "越野障碍", "比力量", "比力量和敏捷",
    "比技巧", "比速度", "比耐力", "什么都比一点",
    "格斗、钢、岩石属性有加成", "岩石、地面、虫属性有加成", "水、超能力、飞行属性有加成", "冰、妖精属性有加成",
    "火、龙属性有加成", "恶、幽灵、毒属性有加成", "%u 公斤", "%u.%u 分",
    "%u:%02u", "道馆", "%s道馆", "徽章 %u/8",
    "联盟大赛", "冠军", "左右滑：换地区", "先集齐 8 枚徽章",
    "今天已经挑战过了", "先拿到上一枚徽章", "农场里要有 2 只宝可梦", "四天王",
    "冠军", "挑战", "再挑战", "选两只队友",
    "下一步", "排阵容", "点自己的宝可梦换人", "开始",
    "%s  第 %u/3 局", "赢了！", "输了", "点一下：继续",
    "获得%s！", "还得到：%s", "这次输了，明天再来", "你是冠军！",
    "联盟挑战结束，明天再来", "一般属性", "火属性", "水属性",
    "草属性", "电属性", "冰属性", "格斗属性",
    "毒属性", "地面属性", "飞行属性", "超能力属性",
    "虫属性", "岩石属性", "幽灵属性", "龙属性",
    "恶属性", "钢属性", "妖精属性",
    "已有 %u 枚徽章",
    "趣玩", "钓鱼、种果子、挖宝……", "趣玩  %ld分", "果园",
    "钓鱼", "挖矿", "寻宝", "要草、虫、地面、毒、妖精属性",
    "要水、冰、龙属性", "要岩石、钢、格斗、火属性", "要一般、飞行、电、超能力、幽灵、恶属性", "%u 只能去",
    "%u 分", "下棋赢了：好运翻倍！", "下了棋：收获多 1 个", "下盘棋会带来好运",
    "派谁去？", "出门寻宝去了", "还要 %u 分钟", "还要 %u 小时",
    "%u 块熟了", "回来了，快领取", "种下", "浇水",
    "收获", "浇过水了", "熟了！", "时间还没同步",
    "抛竿", "等鱼上钩……", "快点屏幕收竿！", "钓得好！",
    "只钓到一条小鱼", "开挖", "点石头把它敲开！", "出发",
    "1 小时后回来", "领取", "已经有宝可梦出门了", "收获",
    "体力果", "抵抗果", "活力果", "金果",
    "鱼", "珍珠", "Mega石碎片", "水属性蛋",
    "化石蛋", "今天吃够了，只填饱肚子", "集齐 10 片自动变成 Mega石", "体力 +3",
    "抵抗力 +3", "活力 +3", "三项都 +3，好开心！", "好开心！",
    "好吃，吃得好饱！",
    "农场", "%u 只宝可梦", "恢复备份", "有 %u",
    "擅长：%s、%s", "HP %u  攻 %u  防 %u  速 %u", "体力 %u  抵抗力 %u  活力 %u", "试试换别的队友，",
    "或者吃果子练训练值", "徽章总数", "联盟冠军", "趣玩收获",
    "%u 小时", "%u 分钟",
    "花 %u 分购买？", "花 %u 分？",
  },
};

// Nombres de medalla en sus tres longitudes [idioma][medalla].
static const char *const MED_NAME[LANG_COUNT][MED_COUNT] = {
  { "Lv.10", "Lv.25", "Lv.50", "7 STREAK", "BOND", "TOP FORM", "IN SHAPE",
    "CURED", "SHINY", "LEGEND", "TIRELESS", "LIVELY", "30 STREAK", "CHESS WIN", "CHESS RECORD" },
  { "Lv.10", "Lv.25", "Lv.50", "连续7天", "羁绊", "最终形态", "精神饱满",
    "康复", "异色", "传说", "精力充沛", "活力满满", "连续30天", "棋局胜利", "棋艺新高" },
};
static const char *const MED_LBL[LANG_COUNT][MED_COUNT] = {
  { "Lv10", "Lv25", "Lv50", "7DAYS", "BOND", "TOP", "FIT",
    "CURED", "SHINY", "LEGEND", "STA", "VIT", "30DAYS", "WIN", "RECORD" },
  { "Lv10", "Lv25", "Lv50", "7天", "羁绊", "最终", "健康",
    "康复", "异色", "传说", "体力", "活力", "30天", "胜利", "新高" },
};
static const char *const MED_DSC[LANG_COUNT][MED_COUNT] = {
  { "LEVEL 10", "LEVEL 25", "LEVEL 50", "7 DAY STREAK", "MAX BOND", "FINAL FORM", "IN SHAPE",
    "GOT CURED", "SHINY FORM", "LEGENDARY", "STAMINA 50", "VITALITY 50",
    "30DAY STREAK", "WON CHESS", "CHESS RECORD" },
  { "等级 10", "等级 25", "等级 50", "连续 7 天", "羁绊 MAX", "最终进化", "精神饱满",
    "吃药治好病", "异色伙伴", "传说宝可梦", "体力练到 50", "活力练到 50",
    "连续 30 天", "陪伴时下棋赢了", "陪伴时棋艺新高" },
};

const char *T(StrId id) { return STRINGS[gLang][id]; }
const char *medalName(int i)  { return MED_NAME[gLang][i]; }
const char *medalLabel(int i) { return MED_LBL[gLang][i]; }
const char *medalDesc(int i)  { return MED_DSC[gLang][i]; }

// La clave es "lang2" y no "lang": con 9 idiomas el ingles era el 1 y el chino
// el 8. Releer esos numeros con la tabla nueva cambiaria el idioma de golpe.
void loadLang() {
  Preferences p;
  p.begin("tamapoke", true);  // solo lectura
  uint8_t v = p.getUChar("lang2", LANG_DEFAULT);
  p.end();
  gLang = (v < LANG_COUNT) ? (Lang)v : LANG_DEFAULT;
}

void setLang(Lang l) {
  if (l >= LANG_COUNT) return;
  gLang = l;
  Preferences p;
  p.begin("tamapoke", false);
  p.putUChar("lang2", (uint8_t)l);
  p.end();
}
