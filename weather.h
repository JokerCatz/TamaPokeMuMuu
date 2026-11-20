#pragma once
#include <stdint.h>
#include <ArduinoJson.h>

// Tiempo real del lugar (Open-Meteo, sin clave) para el fondo: lluvia,
// nieve, niebla, tormenta, nubes; y la salida/puesta del sol para el dia y la
// noche. Aqui solo lo que no depende del hardware (se prueba en el PC); la
// descarga esta en wifisync.cpp.

#define WX_EVERY_MS (60UL * 60 * 1000)      // se actualiza cada hora (con USB)
#define WX_EVERY_BATT_MS (3UL * 60 * 60 * 1000)  // con bateria, cada 3 h (la radio es lo que mas gasta)
#define WX_BATT_QUIET_UNTIL 6                // con bateria, de 0 a 6 h no se conecta
#define WX_STALE_MS (4UL * 60 * 60 * 1000)  // sin datos de hace 4 h: cielo normal (con bateria se pide cada 3)

enum : uint8_t { WX_UNKNOWN = 0, WX_CLEAR, WX_CLOUDY, WX_FOG, WX_RAIN, WX_SNOW, WX_STORM };

struct Weather {
  bool valid = false;
  uint8_t kind = WX_UNKNOWN;
  int16_t sunriseMin = -1, sunsetMin = -1;  // minuto del dia local, -1 = no se sabe
  int8_t tempC = 0;
  int16_t tempF = 32;  // la misma en Fahrenheit, solo para enseñarla (la logica usa tempC)
  int32_t utcOffset = INT32_MIN;  // desfase de ahora en el lugar (segundos, con el verano); MIN = no vino
};

// codigo WMO de Open-Meteo -> WX_*
uint8_t wxFromWmo(int code);
// "2026-11-06T07:05" -> 425 (minuto del dia); -1 si no se entiende
int16_t wxMinuteOf(const char *iso);
// respuesta de /v1/forecast?current=weather_code,temperature_2m&daily=sunrise,sunset
bool weatherParse(JsonVariantConst doc, Weather &w);

// el lugar: el de config.h (WX_LAT, WX_LON, WX_TZ) o la ciudad del portal Wi-Fi, que se
// busca con el geocodificador de Open-Meteo (entiende "Springfield, IL" o "Lyon, France")
struct Place {
  char name[96] = "";       // "Springfield, Illinois, United States"
  char lat[12] = "", lon[12] = "";  // con 2 decimales (~1 km), como van en la URL
  char tz[40] = "";         // zona IANA, "America/Chicago"
};
// respuesta de geocoding-api.open-meteo.com/v1/search: el primer resultado; false si no hay
bool placeParse(JsonVariantConst doc, Place &p);
// respuesta de ipapi.co/json/ (donde esta la conexion a internet); false si dice error
bool placeFromIp(JsonVariantConst doc, Place &p);
// la zona IANA que manda el movil desde el portal: "America/Chicago" si, "UTC" o "Etc/GMT+4" no
bool zoneOk(const char *iana);
// la ciudad que da nombre a la zona: "America/Argentina/Buenos_Aires" -> "Buenos Aires"
void zoneCity(const char *iana, char *out, size_t cap);
// para la URL: lo que no sea letra, cifra o -_.~ va como %XX (UTF-8 incluido)
void urlEncode(const char *s, char *out, size_t cap);

// fase del dia por la hora local: dia, amanecer/atardecer (cerca de la salida o
// la puesta del sol) o noche. Sin datos del sol: sale a las 6:30 y se pone a las 19:30.
enum : uint8_t { PH_DAY = 0, PH_TWILIGHT, PH_NIGHT };
#define TWILIGHT_BEFORE 30  // minutos de "luz de amanecer/atardecer" antes...
#define TWILIGHT_AFTER 40   // ...y despues de la salida (al reves en la puesta)
uint8_t dayPhase(int minuteOfDay, int sunriseMin, int sunsetMin);
