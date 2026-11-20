#include "weather.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

uint8_t wxFromWmo(int c) {
  if (c < 0) return WX_UNKNOWN;
  if (c <= 1) return WX_CLEAR;                       // despejado / casi
  if (c <= 3) return WX_CLOUDY;                      // nubes, cubierto
  if (c == 45 || c == 48) return WX_FOG;
  if (c >= 51 && c <= 67) return WX_RAIN;            // llovizna, lluvia, helada
  if (c >= 71 && c <= 77) return WX_SNOW;
  if (c >= 80 && c <= 82) return WX_RAIN;            // chubascos
  if (c == 85 || c == 86) return WX_SNOW;            // chubascos de nieve
  if (c >= 95 && c <= 99) return WX_STORM;
  return WX_UNKNOWN;
}

int16_t wxMinuteOf(const char *iso) {
  if (!iso) return -1;
  const char *t = strchr(iso, 'T');
  if (!t || strlen(t) < 6 || t[3] != ':') return -1;
  int h = atoi(t + 1), m = atoi(t + 4);
  if (h < 0 || h > 23 || m < 0 || m > 59) return -1;
  return (int16_t)(h * 60 + m);
}

bool weatherParse(JsonVariantConst doc, Weather &w) {
  JsonVariantConst cur = doc["current"];
  if (cur.isNull() || cur["weather_code"].isNull()) return false;
  w.kind = wxFromWmo(cur["weather_code"] | -1);
  float t = cur["temperature_2m"] | 0.0f;
  w.tempC = (int8_t)(t < 0 ? t - 0.5f : t + 0.5f);
  float f = t * 1.8f + 32.0f;  // desde los decimales: redondear dos veces podia fallar un grado
  w.tempF = (int16_t)(f < 0 ? f - 0.5f : f + 0.5f);
  w.sunriseMin = wxMinuteOf(doc["daily"]["sunrise"][0] | (const char *)nullptr);
  w.sunsetMin = wxMinuteOf(doc["daily"]["sunset"][0] | (const char *)nullptr);
  JsonVariantConst off = doc["utc_offset_seconds"];
  w.utcOffset = off.is<int32_t>() ? off.as<int32_t>() : INT32_MIN;
  w.valid = true;
  return true;
}

uint8_t dayPhase(int m, int rise, int set) {
  if (rise < 0 || set < 0 || set <= rise) {
    rise = 6 * 60 + 30;
    set = 19 * 60 + 30;
  }
  if (m < rise - TWILIGHT_BEFORE || m >= set + TWILIGHT_BEFORE) return PH_NIGHT;
  if (m < rise + TWILIGHT_AFTER || m >= set - TWILIGHT_AFTER) return PH_TWILIGHT;
  return PH_DAY;
}

// "nombre, region, pais" (sin repetir la region si se llama igual que la ciudad)
static bool placeFrom(const char *nombre, const char *region, const char *pais, JsonVariantConst lat,
                      JsonVariantConst lon, const char *tz, Place &p) {
  if (!nombre || !*nombre || !tz || !lat.is<float>() || !lon.is<float>()) return false;
  if (strlen(tz) >= sizeof(p.tz)) return false;
  Place out;
  size_t n = (size_t)snprintf(out.name, sizeof(out.name), "%s", nombre);
  if (region && *region && strcmp(region, nombre) && n < sizeof(out.name))
    n += (size_t)snprintf(out.name + n, sizeof(out.name) - n, ", %s", region);
  if (pais && *pais && n < sizeof(out.name)) snprintf(out.name + n, sizeof(out.name) - n, ", %s", pais);
  snprintf(out.lat, sizeof(out.lat), "%.2f", lat.as<float>());
  snprintf(out.lon, sizeof(out.lon), "%.2f", lon.as<float>());
  snprintf(out.tz, sizeof(out.tz), "%s", tz);
  p = out;
  return true;
}

bool placeParse(JsonVariantConst doc, Place &p) {
  JsonVariantConst r = doc["results"][0];
  return placeFrom(r["name"] | (const char *)nullptr, r["admin1"] | "", r["country"] | "", r["latitude"],
                   r["longitude"], r["timezone"] | (const char *)nullptr, p);
}

bool placeFromIp(JsonVariantConst doc, Place &p) {
  if (doc["error"] | false) return false;  // {"error": true, "reason": "RateLimited"...}
  return placeFrom(doc["city"] | (const char *)nullptr, doc["region"] | "", doc["country_name"] | "",
                   doc["latitude"], doc["longitude"], doc["timezone"] | (const char *)nullptr, p);
}

bool zoneOk(const char *z) {
  if (!z) return false;
  size_t n = strlen(z);
  if (n < 3 || n >= sizeof(Place().tz) || !strchr(z, '/') || !strncmp(z, "Etc/", 4)) return false;
  for (; *z; z++)
    if (!((*z >= 'A' && *z <= 'Z') || (*z >= 'a' && *z <= 'z') || (*z >= '0' && *z <= '9') || strchr("_+-/", *z)))
      return false;
  return true;
}

void zoneCity(const char *z, char *out, size_t cap) {
  if (!cap) return;
  const char *ultima = z ? strrchr(z, '/') : nullptr;
  snprintf(out, cap, "%s", ultima ? ultima + 1 : (z ? z : ""));
  for (char *c = out; *c; c++)
    if (*c == '_') *c = ' ';
}

void urlEncode(const char *s, char *out, size_t cap) {
  static const char HEX_[] = "0123456789ABCDEF";
  size_t n = 0;
  for (; s && *s && n + 1 < cap; s++) {
    unsigned char c = (unsigned char)*s;
    if ((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') || strchr("-_.~", c)) {
      out[n++] = (char)c;
    } else {
      if (n + 4 > cap) break;  // no se corta un %XX a medias
      out[n++] = '%';
      out[n++] = HEX_[c >> 4];
      out[n++] = HEX_[c & 15];
    }
  }
  if (cap) out[n] = 0;
}
