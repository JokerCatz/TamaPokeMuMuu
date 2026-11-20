// Tiempo y fase del dia (weather.cpp) con respuestas como las de Open-Meteo.
#include "framework.h"
#include "../weather.h"

TEST(weather, codigos_wmo) {
  CHECK_EQ(wxFromWmo(0), (uint8_t)WX_CLEAR);
  CHECK_EQ(wxFromWmo(2), (uint8_t)WX_CLOUDY);
  CHECK_EQ(wxFromWmo(3), (uint8_t)WX_CLOUDY);
  CHECK_EQ(wxFromWmo(45), (uint8_t)WX_FOG);
  CHECK_EQ(wxFromWmo(53), (uint8_t)WX_RAIN);
  CHECK_EQ(wxFromWmo(66), (uint8_t)WX_RAIN);   // lluvia helada
  CHECK_EQ(wxFromWmo(73), (uint8_t)WX_SNOW);
  CHECK_EQ(wxFromWmo(81), (uint8_t)WX_RAIN);
  CHECK_EQ(wxFromWmo(86), (uint8_t)WX_SNOW);
  CHECK_EQ(wxFromWmo(95), (uint8_t)WX_STORM);
  CHECK_EQ(wxFromWmo(-1), (uint8_t)WX_UNKNOWN);
  CHECK_EQ(wxFromWmo(42), (uint8_t)WX_UNKNOWN);
}

TEST(weather, minuto_del_dia) {
  CHECK_EQ(wxMinuteOf("2026-11-06T07:05"), (int16_t)425);
  CHECK_EQ(wxMinuteOf("2026-11-06T16:48"), (int16_t)1008);
  CHECK_EQ(wxMinuteOf("2026-11-06"), (int16_t)-1);
  CHECK_EQ(wxMinuteOf(nullptr), (int16_t)-1);
}

TEST(weather, respuesta_de_open_meteo) {
  JsonDocument doc;
  deserializeJson(doc, R"({"current":{"time":"2026-11-06T14:00","temperature_2m":-3.6,"weather_code":71},
    "daily":{"time":["2026-11-06"],"sunrise":["2026-11-06T07:05"],"sunset":["2026-11-06T16:48"]}})");
  Weather w;
  CHECK(weatherParse(doc.as<JsonVariantConst>(), w));
  CHECK(w.valid);
  CHECK_EQ(w.kind, (uint8_t)WX_SNOW);
  CHECK_EQ(w.tempC, (int8_t)-4);
  CHECK_EQ(w.tempF, (int16_t)26);  // -3.6 C = 25.5 F
  CHECK_EQ(w.sunriseMin, (int16_t)425);
  CHECK_EQ(w.sunsetMin, (int16_t)1008);
}

// grados F para enseñar: desde los decimales de Open-Meteo, redondeando una sola vez
static int16_t enF(const char *temp) {
  char json[160];
  snprintf(json, sizeof(json), R"({"current":{"temperature_2m":%s,"weather_code":0}})", temp);
  JsonDocument doc;
  deserializeJson(doc, json);
  Weather w;
  weatherParse(doc.as<JsonVariantConst>(), w);
  return w.tempF;
}

TEST(weather, la_temperatura_tambien_en_fahrenheit) {
  CHECK_EQ(enF("0"), (int16_t)32);
  CHECK_EQ(enF("-40"), (int16_t)-40);    // donde las dos escalas coinciden
  CHECK_EQ(enF("100"), (int16_t)212);
  CHECK_EQ(enF("21.4"), (int16_t)71);    // 70.5: desde 21 redondeado saldria 70
  CHECK_EQ(enF("-20"), (int16_t)-4);
  CHECK_EQ(enF("-17.9"), (int16_t)0);    // -0.2 F
  CHECK_EQ(enF("-18.3"), (int16_t)-1);   // -0.9 F: bajo cero redondea hacia abajo
  Weather sin;                           // sin datos (o forzado por consola): 0 C y 32 F
  CHECK_EQ(sin.tempF, (int16_t)32);
}

TEST(weather, respuesta_rota_no_vale) {
  JsonDocument doc;
  deserializeJson(doc, R"({"error":true,"reason":"bad"})");
  Weather w;
  CHECK(!weatherParse(doc.as<JsonVariantConst>(), w));
  CHECK(!w.valid);
}

TEST(weather, fases_del_dia_con_el_sol_de_hoy) {
  int rise = 425, set = 1008;  // 7:05 y 16:48
  CHECK_EQ(dayPhase(6 * 60, rise, set), (uint8_t)PH_NIGHT);
  CHECK_EQ(dayPhase(rise - 10, rise, set), (uint8_t)PH_TWILIGHT);  // clarea
  CHECK_EQ(dayPhase(rise + 30, rise, set), (uint8_t)PH_TWILIGHT);
  CHECK_EQ(dayPhase(12 * 60, rise, set), (uint8_t)PH_DAY);
  CHECK_EQ(dayPhase(set - 20, rise, set), (uint8_t)PH_TWILIGHT);  // atardece
  CHECK_EQ(dayPhase(set + 20, rise, set), (uint8_t)PH_TWILIGHT);
  CHECK_EQ(dayPhase(set + 40, rise, set), (uint8_t)PH_NIGHT);
  CHECK_EQ(dayPhase(23 * 60, rise, set), (uint8_t)PH_NIGHT);
}

TEST(weather, sin_datos_del_sol_usa_6_30_y_19_30) {
  CHECK_EQ(dayPhase(5 * 60, -1, -1), (uint8_t)PH_NIGHT);
  CHECK_EQ(dayPhase(6 * 60 + 30, -1, -1), (uint8_t)PH_TWILIGHT);
  CHECK_EQ(dayPhase(13 * 60, -1, -1), (uint8_t)PH_DAY);
  CHECK_EQ(dayPhase(21 * 60, -1, -1), (uint8_t)PH_NIGHT);
}

TEST(weather, el_desfase_del_lugar_viene_con_el_tiempo) {
  JsonDocument doc;
  deserializeJson(doc, R"({"utc_offset_seconds":39600,"timezone":"Australia/Sydney",
    "current":{"temperature_2m":21.0,"weather_code":1}})");
  Weather w;
  CHECK(weatherParse(doc.as<JsonVariantConst>(), w));
  CHECK_EQ(w.utcOffset, (int32_t)39600);
  deserializeJson(doc, R"({"current":{"temperature_2m":21.0,"weather_code":1}})");  // sin el campo
  Weather v;
  CHECK(weatherParse(doc.as<JsonVariantConst>(), v));
  CHECK_EQ(v.utcOffset, (int32_t)INT32_MIN);
}

// geocoding-api.open-meteo.com/v1/search (recortada: trae tambien elevation, postcodes...)
TEST(weather, ciudad_del_geocodificador) {
  JsonDocument doc;
  deserializeJson(doc, R"({"results":[{"id":4250542,"name":"Springfield","latitude":39.80172,
    "longitude":-89.64371,"country_code":"US","timezone":"America/Chicago","country":"United States",
    "admin1":"Illinois"}],"generationtime_ms":0.5})");
  Place p;
  CHECK(placeParse(doc.as<JsonVariantConst>(), p));
  CHECK_EQ(std::string(p.name), std::string("Springfield, Illinois, United States"));
  CHECK_EQ(std::string(p.lat), std::string("39.80"));
  CHECK_EQ(std::string(p.lon), std::string("-89.64"));
  CHECK_EQ(std::string(p.tz), std::string("America/Chicago"));
}

TEST(weather, ciudad_sin_region_repetida_ni_resultados) {
  JsonDocument doc;
  deserializeJson(doc, R"({"results":[{"name":"Singapore","latitude":1.28967,"longitude":103.85,
    "timezone":"Asia/Singapore","country":"Singapore","admin1":"Singapore"}]})");
  Place p;
  CHECK(placeParse(doc.as<JsonVariantConst>(), p));
  CHECK_EQ(std::string(p.name), std::string("Singapore, Singapore"));  // la region no se repite
  deserializeJson(doc, R"({"generationtime_ms":0.07})");  // no encontrada: sin "results"
  Place q;
  CHECK(!placeParse(doc.as<JsonVariantConst>(), q));
  CHECK_EQ(q.name[0], '\0');
  deserializeJson(doc, R"({"results":[{"name":"X","latitude":1,"longitude":2}]})");  // sin zona
  CHECK(!placeParse(doc.as<JsonVariantConst>(), q));
}

TEST(weather, url_encode_de_la_ciudad) {
  char out[64];
  urlEncode("Lyon, France", out, sizeof(out));
  CHECK_EQ(std::string(out), std::string("Lyon%2C%20France"));
  urlEncode("北京", out, sizeof(out));
  CHECK_EQ(std::string(out), std::string("%E5%8C%97%E4%BA%AC"));
  urlEncode("a~b-c_d.e", out, sizeof(out));
  CHECK_EQ(std::string(out), std::string("a~b-c_d.e"));
  urlEncode("ab,c", out, 5);  // "ab" + "%2C" no cabe en 5: se corta antes del %
  CHECK_EQ(std::string(out), std::string("ab"));
}

// ipapi.co/json/ (recortada; la de una IP publica de ejemplo)
TEST(weather, lugar_por_la_conexion) {
  JsonDocument doc;
  deserializeJson(doc, R"({"ip":"8.8.8.8","city":"Mountain View","region":"California","country":"US",
    "country_name":"United States","latitude":37.42301,"longitude":-122.083352,
    "timezone":"America/Los_Angeles","utc_offset":"-0800"})");
  Place p;
  CHECK(placeFromIp(doc.as<JsonVariantConst>(), p));
  CHECK_EQ(std::string(p.name), std::string("Mountain View, California, United States"));
  CHECK_EQ(std::string(p.lat), std::string("37.42"));
  CHECK_EQ(std::string(p.lon), std::string("-122.08"));
  CHECK_EQ(std::string(p.tz), std::string("America/Los_Angeles"));
  deserializeJson(doc, R"({"ip":"127.0.0.1","error":true,"reason":"Reserved IP Address"})");
  Place q;
  CHECK(!placeFromIp(doc.as<JsonVariantConst>(), q));
  deserializeJson(doc, R"({"error":true,"reason":"RateLimited","city":"X","latitude":1,"longitude":2,"timezone":"UTC"})");
  CHECK(!placeFromIp(doc.as<JsonVariantConst>(), q));
}

// la zona del movil (portal): cual vale y que ciudad la nombra
TEST(weather, zona_del_movil) {
  CHECK(zoneOk("America/Chicago"));
  CHECK(zoneOk("America/Argentina/Buenos_Aires"));
  CHECK(zoneOk("America/Port-au-Prince"));
  CHECK(!zoneOk("UTC"));
  CHECK(!zoneOk("Etc/GMT+4"));
  CHECK(!zoneOk(""));
  CHECK(!zoneOk(nullptr));
  CHECK(!zoneOk("America/<script>"));
  CHECK(!zoneOk("America/Aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"));  // no cabe en Place::tz
  char c[48];
  zoneCity("America/Argentina/Buenos_Aires", c, sizeof(c));
  CHECK_EQ(std::string(c), std::string("Buenos Aires"));
  zoneCity("Asia/Shanghai", c, sizeof(c));
  CHECK_EQ(std::string(c), std::string("Shanghai"));
  zoneCity("America/New_York", c, 5);
  CHECK_EQ(std::string(c), std::string("New "));
}
