#pragma once
// GENERADO por tools/gen_gyms.py desde tools/gyms_data.json - no editar.
// Gimnasios de las 9 regiones (8 cada una) y sus ligas (alto mando + campeon), con
// los nombres oficiales en ingles y chino simplificado, el chino tradicional (convertido con OpenCC s2twp) y equipos del anime.
#include <stdint.h>
#include "sport.h"

struct GymPerson {
  const char *en, *zh, *zht;
  uint8_t type;      // TY_* (0xFF: de varios tipos)
  int16_t team[3];   // el mas fuerte, el ultimo
};
struct GymInfo {
  GymPerson leader;
  const char *cityEn, *cityZh, *cityZht, *badgeEn, *badgeZh, *badgeZht;
};

static const GymInfo GYMS[9][8] = {
  {  // kanto
    { { "Brock", "小刚", "小剛", TY_ROCK, { 74, 185, 95 } }, "Pewter City", "深灰市", "深灰市", "Boulder Badge", "灰色徽章", "灰色徽章" },
    { { "Misty", "小霞", "小霞", TY_WATER, { 54, 130, 121 } }, "Cerulean City", "华蓝市", "華藍市", "Cascade Badge", "蓝色徽章", "藍色徽章" },
    { { "Lt. Surge", "马志士", "馬志士", TY_ELECTRIC, { 100, 25, 26 } }, "Vermilion City", "枯叶市", "枯葉市", "Thunder Badge", "橙色徽章", "橙色徽章" },
    { { "Erika", "莉佳", "莉佳", TY_GRASS, { 114, 70, 44 } }, "Celadon City", "玉虹市", "玉虹市", "Rainbow Badge", "彩虹徽章", "彩虹徽章" },
    { { "Koga", "阿桔", "阿桔", TY_POISON, { 42, 49, 110 } }, "Fuchsia City", "浅红市", "淺紅市", "Soul Badge", "粉红徽章", "粉紅徽章" },
    { { "Sabrina", "娜姿", "娜姿", TY_PSYCHIC, { 93, 122, 64 } }, "Saffron City", "金黄市", "金黃市", "Marsh Badge", "金色徽章", "金色徽章" },
    { { "Blaine", "夏伯", "夏伯", TY_FIRE, { 38, 112, 126 } }, "Cinnabar Island", "红莲镇", "紅蓮鎮", "Volcano Badge", "深红徽章", "深紅徽章" },
    { { "Giovanni", "坂木", "坂木", TY_GROUND, { 53, 76, 112 } }, "Viridian City", "常青市", "常青市", "Earth Badge", "绿色徽章", "綠色徽章" },
  },
  {  // johto
    { { "Falkner", "阿速", "阿速", TY_FLYING, { 163, 85, 18 } }, "Violet City", "桔梗市", "桔梗市", "Zephyr Badge", "飞翼徽章", "飛翼徽章" },
    { { "Bugsy", "阿笔", "阿筆", TY_BUG, { 167, 11, 123 } }, "Azalea Town", "桧皮镇", "檜皮鎮", "Hive Badge", "昆虫徽章", "昆蟲徽章" },
    { { "Whitney", "小茜", "小茜", TY_NORMAL, { 35, 30, 241 } }, "Goldenrod City", "满金市", "滿金市", "Plain Badge", "常规徽章", "常規徽章" },
    { { "Morty", "松叶", "松葉", TY_GHOST, { 92, 93, 94 } }, "Ecruteak City", "缘朱市", "緣朱市", "Fog Badge", "幻影徽章", "幻影徽章" },
    { { "Chuck", "阿四", "阿四", TY_FIGHTING, { 57, 67, 62 } }, "Cianwood City", "湛蓝市", "湛藍市", "Storm Badge", "打击徽章", "打擊徽章" },
    { { "Jasmine", "阿蜜", "阿蜜", TY_STEEL, { 181, 81, 208 } }, "Olivine City", "浅葱市", "淺蔥市", "Mineral Badge", "钢铁徽章", "鋼鐵徽章" },
    { { "Pryce", "柳伯", "柳伯", TY_ICE, { 86, 87, 221 } }, "Mahogany Town", "卡吉镇", "卡吉鎮", "Glacier Badge", "冰雪徽章", "冰雪徽章" },
    { { "Clair", "小椿", "小椿", TY_DRAGON, { 130, 148, 230 } }, "Blackthorn City", "烟墨市", "煙墨市", "Rising Badge", "升龙徽章", "升龍徽章" },
  },
  {  // hoenn
    { { "Roxanne", "杜娟", "杜娟", TY_ROCK, { 74, 141, 299 } }, "Rustboro City", "卡那兹市", "卡那茲市", "Stone Badge", "岩石徽章", "岩石徽章" },
    { { "Brawly", "藤树", "藤樹", TY_FIGHTING, { 307, 67, 297 } }, "Dewford Town", "武斗镇", "武鬥鎮", "Knuckle Badge", "拳击徽章", "拳擊徽章" },
    { { "Wattson", "铁旋", "鐵旋", TY_ELECTRIC, { 100, 82, 310 } }, "Mauville City", "紫堇市", "紫堇市", "Dynamo Badge", "电力徽章", "電力徽章" },
    { { "Flannery", "亚莎", "亞莎", TY_FIRE, { 218, 219, 324 } }, "Lavaridge Town", "釜炎镇", "釜炎鎮", "Heat Badge", "烈焰徽章", "烈焰徽章" },
    { { "Norman", "千里", "千里", TY_NORMAL, { 287, 288, 289 } }, "Petalburg City", "橙华市", "橙華市", "Balance Badge", "天秤徽章", "天秤徽章" },
    { { "Winona", "娜琪", "娜琪", TY_FLYING, { 277, 227, 334 } }, "Fortree City", "茵郁市", "茵鬱市", "Feather Badge", "白羽徽章", "白羽徽章" },
    { { "Tate & Liza", "小枫与小南", "小楓與小南", TY_PSYCHIC, { 178, 337, 338 } }, "Mossdeep City", "绿岭市", "綠嶺市", "Mind Badge", "心灵徽章", "心靈徽章" },
    { { "Wallace", "米可利", "米可利", TY_WATER, { 340, 364, 350 } }, "Sootopolis City", "琉璃市", "琉璃市", "Rain Badge", "雨滴徽章", "雨滴徽章" },
  },
  {  // sinnoh
    { { "Roark", "瓢太", "瓢太", TY_ROCK, { 74, 95, 409 } }, "Oreburgh City", "黑金市", "黑金市", "Coal Badge", "煤炭徽章", "煤炭徽章" },
    { { "Gardenia", "菜种", "菜種", TY_GRASS, { 420, 387, 407 } }, "Eterna City", "百代市", "百代市", "Forest Badge", "森林徽章", "森林徽章" },
    { { "Maylene", "阿李", "阿李", TY_FIGHTING, { 307, 67, 448 } }, "Veilstone City", "帷幕市", "帷幕市", "Cobble Badge", "铺石徽章", "鋪石徽章" },
    { { "Crasher Wake", "吉宪", "吉憲", TY_WATER, { 130, 195, 419 } }, "Pastoria City", "野原市", "野原市", "Fen Badge", "湿地徽章", "溼地徽章" },
    { { "Fantina", "梅丽莎", "梅麗莎", TY_GHOST, { 94, 426, 429 } }, "Hearthome City", "家缘市", "家緣市", "Relic Badge", "遗迹徽章", "遺蹟徽章" },
    { { "Byron", "东瓜", "東瓜", TY_STEEL, { 436, 208, 411 } }, "Canalave City", "水脉市", "水脈市", "Mine Badge", "矿山徽章", "礦山徽章" },
    { { "Candice", "小菘", "小菘", TY_ICE, { 215, 459, 460 } }, "Snowpoint City", "雪峰市", "雪峰市", "Icicle Badge", "冰河徽章", "冰河徽章" },
    { { "Volkner", "电次", "電次", TY_ELECTRIC, { 135, 466, 405 } }, "Sunyshore City", "滨海市", "濱海市", "Beacon Badge", "灯塔徽章", "燈塔徽章" },
  },
  {  // unova
    { { "Cilan", "天桐", "天桐", TY_GRASS, { 618, 558, 511 } }, "Striaton City", "三曜市", "三曜市", "Trio Badge", "三元徽章", "三元徽章" },
    { { "Lenora", "芦荟", "蘆薈", TY_NORMAL, { 507, 115, 505 } }, "Nacrene City", "七宝市", "七寶市", "Basic Badge", "基础徽章", "基礎徽章" },
    { { "Burgh", "亚堤", "亞堤", TY_BUG, { 557, 544, 542 } }, "Castelia City", "飞云市", "飛雲市", "Insect Badge", "甲虫徽章", "甲蟲徽章" },
    { { "Elesa", "小菊儿", "小菊兒", TY_ELECTRIC, { 602, 587, 523 } }, "Nimbasa City", "雷文市", "雷文市", "Bolt Badge", "伏特徽章", "伏特徽章" },
    { { "Clay", "菊老大", "菊老大", TY_GROUND, { 552, 536, 530 } }, "Driftveil City", "帆巴市", "帆巴市", "Quake Badge", "震动徽章", "震動徽章" },
    { { "Skyla", "风露", "風露", TY_FLYING, { 528, 521, 581 } }, "Mistralton City", "吹寄市", "吹寄市", "Jet Badge", "喷射徽章", "噴射徽章" },
    { { "Brycen", "哈奇库", "哈奇庫", TY_ICE, { 583, 615, 614 } }, "Icirrus City", "雪花市", "雪花市", "Freeze Badge", "冰柱徽章", "冰柱徽章" },
    { { "Iris", "艾莉丝", "艾莉絲", TY_DRAGON, { 443, 149, 612 } }, "Opelucid City", "双龙市", "雙龍市", "Legend Badge", "传说徽章", "傳說徽章" },
  },
  {  // kalos
    { { "Viola", "紫罗兰", "紫羅蘭", TY_BUG, { 283, 284, 666 } }, "Santalune City", "白檀市", "白檀市", "Bug Badge", "虫虫徽章", "蟲蟲徽章" },
    { { "Grant", "查克洛", "查克洛", TY_ROCK, { 95, 698, 696 } }, "Cyllage City", "遥香市", "遙香市", "Cliff Badge", "岩壁徽章", "巖壁徽章" },
    { { "Korrina", "可尔妮", "可爾妮", TY_FIGHTING, { 67, 620, 448 } }, "Shalour City", "娑罗市", "娑羅市", "Rumble Badge", "战斗徽章", "戰鬥徽章" },
    { { "Ramos", "福爷", "福爺", TY_GRASS, { 189, 70, 673 } }, "Coumarine City", "比翼市", "比翼市", "Plant Badge", "植物徽章", "植物徽章" },
    { { "Clemont", "希特隆", "希特隆", TY_ELECTRIC, { 82, 695, 405 } }, "Lumiose City", "密阿雷市", "密阿雷市", "Voltage Badge", "电压徽章", "電壓徽章" },
    { { "Valerie", "玛绣", "瑪繡", TY_FAIRY, { 303, 122, 700 } }, "Laverre City", "香薰市", "香薰市", "Fairy Badge", "妖精徽章", "妖精徽章" },
    { { "Olympia", "葛吉花", "葛吉花", TY_PSYCHIC, { 561, 199, 678 } }, "Anistar City", "百刻市", "百刻市", "Psychic Badge", "超能徽章", "超能徽章" },
    { { "Wulfric", "得抚", "得撫", TY_ICE, { 872, 713, 460 } }, "Snowbelle City", "映雪市", "映雪市", "Iceberg Badge", "冰山徽章", "冰山徽章" },
  },
  {  // alola
    { { "Ilima", "伊利马", "伊利馬", TY_NORMAL, { 235, 115, 133 } }, "Verdant Cavern", "葱郁洞窟", "蔥鬱洞窟", "Normalium Z", "一般Z", "一般Z" },
    { { "Hala", "哈拉", "哈拉", TY_FIGHTING, { 57, 740, 297 } }, "Iki Town", "利利小镇", "利利小鎮", "Fightinium Z", "格斗Z", "格鬥Z" },
    { { "Lana", "水莲", "水蓮", TY_WATER, { 746, 752, 730 } }, "Brooklet Hill", "潺潺之丘", "潺潺之丘", "Waterium Z", "水Z", "水Z" },
    { { "Kiawe", "卡奇", "卡奇", TY_FIRE, { 105, 6, 776 } }, "Wela Volcano Park", "维拉火山公园", "維拉火山公園", "Firium Z", "火Z", "火Z" },
    { { "Mallow", "玛奥", "瑪奧", TY_GRASS, { 765, 754, 763 } }, "Lush Jungle", "树荫丛林", "樹蔭叢林", "Grassium Z", "草Z", "草Z" },
    { { "Sophocles", "马玛内", "馬瑪內", TY_ELECTRIC, { 737, 738, 777 } }, "Hokulani Observatory", "辉克拉尼天文台", "輝克拉尼天文臺", "Electrium Z", "电Z", "電Z" },
    { { "Nanu", "默丹", "默丹", TY_DARK, { 302, 553, 53 } }, "Malie City", "马利埃静市", "馬利埃靜市", "Darkinium Z", "恶Z", "惡Z" },
    { { "Hapu", "哈普乌", "哈普烏", TY_GROUND, { 330, 623, 750 } }, "Vast Poni Canyon", "波尼大峡谷", "波尼大峽谷", "Groundium Z", "地面Z", "地面Z" },
  },
  {  // galar
    { { "Milo", "亚洛", "亞洛", TY_GRASS, { 831, 829, 830 } }, "Turffield", "草路镇", "草路鎮", "Grass Badge", "草之徽章", "草之徽章" },
    { { "Nessa", "露璃娜", "露璃娜", TY_WATER, { 118, 846, 834 } }, "Hulbury", "水舟镇", "水舟鎮", "Water Badge", "水之徽章", "水之徽章" },
    { { "Kabu", "卡芜", "卡蕪", TY_FIRE, { 38, 59, 851 } }, "Motostoke", "机擎市", "機擎市", "Fire Badge", "火之徽章", "火之徽章" },
    { { "Bea", "彩豆", "彩豆", TY_FIGHTING, { 67, 865, 68 } }, "Stow-on-Side", "溯传镇", "溯傳鎮", "Fighting Badge", "格斗徽章", "格鬥徽章" },
    { { "Opal", "波普菈", "波普菈", TY_FAIRY, { 303, 468, 869 } }, "Ballonlea", "舞姿镇", "舞姿鎮", "Fairy Badge", "妖精徽章", "妖精徽章" },
    { { "Gordie", "玛瓜", "瑪瓜", TY_ROCK, { 213, 838, 839 } }, "Circhester", "战竞镇", "戰競鎮", "Rock Badge", "岩石徽章", "岩石徽章" },
    { { "Piers", "聂梓", "聶梓", TY_DARK, { 812, 849, 862 } }, "Spikemuth", "尖钉镇", "尖釘鎮", "Dark Badge", "恶之徽章", "惡之徽章" },
    { { "Raihan", "奇巴纳", "奇巴納", TY_DRAGON, { 330, 706, 884 } }, "Hammerlocke", "拳关市", "拳關市", "Dragon Badge", "龙之徽章", "龍之徽章" },
  },
  {  // paldea
    { { "Katy", "阿枫", "阿楓", TY_BUG, { 917, 920, 217 } }, "Cortondo", "圆模镇", "圓模鎮", "Gym Badge (Bug)", "道馆徽章（虫）", "道館徽章（蟲）" },
    { { "Brassius", "寇沙", "寇沙", TY_GRASS, { 548, 928, 185 } }, "Artazon", "深钵镇", "深缽鎮", "Gym Badge (Grass)", "道馆徽章（草）", "道館徽章（草）" },
    { { "Iono", "奇树", "奇樹", TY_ELECTRIC, { 940, 404, 939 } }, "Levincia", "酿光市", "釀光市", "Gym Badge (Electric)", "道馆徽章（电）", "道館徽章（電）" },
    { { "Kofu", "海岱", "海岱", TY_WATER, { 961, 976, 740 } }, "Cascarrafa", "玻瓶市", "玻瓶市", "Gym Badge (Water)", "道馆徽章（水）", "道館徽章（水）" },
    { { "Larry", "青木", "青木", TY_NORMAL, { 820, 982, 398 } }, "Medali", "锦汇镇", "錦匯鎮", "Gym Badge (Normal)", "道馆徽章（一般）", "道館徽章（一般）" },
    { { "Ryme", "莱姆", "萊姆", TY_GHOST, { 778, 972, 849 } }, "Montenevera", "冰柜镇", "冰櫃鎮", "Gym Badge (Ghost)", "道馆徽章（幽灵）", "道館徽章（幽靈）" },
    { { "Tulip", "莉普", "莉普", TY_PSYCHIC, { 282, 956, 671 } }, "Alfornada", "焙固镇", "焙固鎮", "Gym Badge (Psychic)", "道馆徽章（超能力）", "道館徽章（超能力）" },
    { { "Grusha", "古鲁夏", "古魯夏", TY_ICE, { 974, 975, 334 } }, "Glaseado Mountain", "霜抹山", "霜抹山", "Gym Badge (Ice)", "道馆徽章（冰）", "道館徽章（冰）" },
  },
};

// alto mando (4) y campeon de cada region
static const GymPerson LEAGUE[9][5] = {
  {  // kanto
    { "Lorelei", "科拿", "科拿", TY_ICE, { 87, 91, 124 } },
    { "Bruno", "希巴", "希巴", TY_FIGHTING, { 95, 107, 68 } },
    { "Agatha", "菊子", "菊子", TY_GHOST, { 42, 93, 94 } },
    { "Lance", "阿渡", "阿渡", TY_DRAGON, { 130, 148, 149 } },
    { "Blue", "青绿", "青綠", 0xFF, { 197, 59, 9 } },
  },
  {  // johto
    { "Will", "一树", "一樹", TY_PSYCHIC, { 124, 80, 178 } },
    { "Koga", "阿桔", "阿桔", TY_POISON, { 49, 89, 169 } },
    { "Bruno", "希巴", "希巴", TY_FIGHTING, { 237, 107, 68 } },
    { "Karen", "梨花", "梨花", TY_DARK, { 197, 198, 229 } },
    { "Lance", "阿渡", "阿渡", TY_DRAGON, { 621, 130, 149 } },
  },
  {  // hoenn
    { "Sidney", "花月", "花月", TY_DARK, { 262, 275, 359 } },
    { "Phoebe", "芙蓉", "芙蓉", TY_GHOST, { 302, 354, 356 } },
    { "Glacia", "波妮", "波妮", TY_ICE, { 362, 364, 365 } },
    { "Drake", "源治", "源治", TY_DRAGON, { 372, 334, 373 } },
    { "Steven", "大吾", "大吾", TY_STEEL, { 346, 306, 376 } },
  },
  {  // sinnoh
    { "Aaron", "阿柳", "阿柳", TY_BUG, { 267, 416, 452 } },
    { "Bertha", "菊野", "菊野", TY_GROUND, { 472, 76, 450 } },
    { "Flint", "大叶", "大葉", TY_FIRE, { 136, 467, 392 } },
    { "Lucian", "悟松", "悟松", TY_PSYCHIC, { 65, 203, 437 } },
    { "Cynthia", "竹兰", "竹蘭", 0xFF, { 442, 350, 445 } },
  },
  {  // unova
    { "Shauntal", "婉龙", "婉龍", TY_GHOST, { 563, 593, 609 } },
    { "Grimsley", "越橘", "越橘", TY_DARK, { 510, 553, 625 } },
    { "Caitlin", "嘉德丽雅", "嘉德麗雅", TY_PSYCHIC, { 518, 579, 576 } },
    { "Marshal", "连武", "連武", TY_FIGHTING, { 538, 539, 534 } },
    { "Alder", "阿戴克", "阿戴克", 0xFF, { 626, 589, 637 } },
  },
  {  // kalos
    { "Malva", "帕琦拉", "帕琦拉", TY_FIRE, { 668, 663, 229 } },
    { "Siebold", "志米", "志米", TY_WATER, { 693, 689, 9 } },
    { "Wikstrom", "雁铠", "雁鎧", TY_STEEL, { 212, 589, 681 } },
    { "Drasna", "朵拉塞娜", "朵拉塞娜", TY_DRAGON, { 691, 715, 334 } },
    { "Diantha", "卡露妮", "卡露妮", 0xFF, { 699, 706, 282 } },
  },
  {  // alola
    { "Molayne", "马睿因", "馬睿因", TY_STEEL, { 707, 462, 51 } },
    { "Olivia", "丽姿", "麗姿", TY_ROCK, { 76, 476, 745 } },
    { "Acerola", "阿塞萝拉", "阿塞蘿拉", TY_GHOST, { 778, 781, 770 } },
    { "Kahili", "卡希丽", "卡希麗", TY_FLYING, { 227, 741, 733 } },
    { "Professor Kukui", "库库伊博士", "庫庫伊博士", 0xFF, { 628, 395, 727 } },
  },
  {  // galar
    { "Marnie", "玛俐", "瑪俐", TY_DARK, { 510, 877, 861 } },
    { "Hop", "赫普", "赫普", TY_NORMAL, { 845, 143, 832 } },
    { "Bede", "彼特", "彼特", TY_FAIRY, { 859, 78, 282 } },
    { "Allister", "欧尼奥", "歐尼奧", TY_GHOST, { 778, 864, 94 } },
    { "Leon", "丹帝", "丹帝", 0xFF, { 887, 815, 6 } },
  },
  {  // paldea
    { "Rika", "辛俐", "辛俐", TY_GROUND, { 340, 51, 980 } },
    { "Poppy", "波琵", "波琵", TY_STEEL, { 462, 879, 959 } },
    { "Larry", "青木", "青木", TY_FLYING, { 741, 398, 973 } },
    { "Hassel", "八朔", "八朔", TY_DRAGON, { 443, 840, 998 } },
    { "Geeta", "也慈", "也慈", 0xFF, { 969, 983, 970 } },
  },
};
