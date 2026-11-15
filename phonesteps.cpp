#include "phonesteps.h"
#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <ESPmDNS.h>
#include <Preferences.h>

static WebServer *srv = nullptr;
static portMUX_TYPE mux = portMUX_INITIALIZER_UNLOCKED;
static uint32_t pendSteps = 0;
static bool pendNew = false;
static uint16_t pendHockey = 0;  // minutos de hockey que aun no ha recogido el bucle principal
static uint16_t code = 0;

uint16_t phoneStepsCode() {
  if (code) return code;
  Preferences p;
  p.begin("phone", false);
  code = p.getUShort("code", 0);
  if (code < 1000 || code > 9999) {
    code = 1000 + esp_random() % 9000;
    p.putUShort("code", code);
  }
  p.end();
  return code;
}

// GET /steps?n=8123&k=1234 (n puede venir con decimales: la suma de Salud)
static void pasos() {
  if (!srv->hasArg("k") || srv->arg("k").toInt() != phoneStepsCode()) {
    srv->send(403, "application/json", "{\"ok\":false,\"error\":\"code\"}");
    return;
  }
  float n = srv->hasArg("n") ? srv->arg("n").toFloat() : -1;
  if (n < 0 || n > 200000) {
    srv->send(400, "application/json", "{\"ok\":false,\"error\":\"n\"}");
    return;
  }
  portENTER_CRITICAL(&mux);
  pendSteps = (uint32_t)(n + 0.5f);
  pendNew = true;
  portEXIT_CRITICAL(&mux);
  char r[48];
  snprintf(r, sizeof(r), "{\"ok\":true,\"steps\":%lu}", (unsigned long)(uint32_t)(n + 0.5f));
  srv->send(200, "application/json", r);
}

// GET /hockey?min=15&k=1234: minutos de practica de hockey (SuperDeker)
static void hockey() {
  if (!srv->hasArg("k") || srv->arg("k").toInt() != phoneStepsCode()) {
    srv->send(403, "application/json", "{\"ok\":false,\"error\":\"code\"}");
    return;
  }
  long m = srv->hasArg("min") ? srv->arg("min").toInt() : 0;
  if (m < 1 || m > 180) {
    srv->send(400, "application/json", "{\"ok\":false,\"error\":\"min\"}");
    return;
  }
  portENTER_CRITICAL(&mux);
  pendHockey = pendHockey + m > 600 ? 600 : pendHockey + m;
  portEXIT_CRITICAL(&mux);
  char r[40];
  snprintf(r, sizeof(r), "{\"ok\":true,\"min\":%ld}", m);
  srv->send(200, "application/json", r);
}

void phoneStepsLoop() {
  if (WiFi.status() != WL_CONNECTED) return;
  if (!srv) {
    phoneStepsCode();
    srv = new WebServer(80);
    srv->on("/steps", HTTP_GET, pasos);
    srv->on("/hockey", HTTP_GET, hockey);
    srv->onNotFound([] { srv->send(200, "text/plain", "TamaPoke"); });
    srv->begin();
    if (MDNS.begin("tamapoke")) MDNS.addService("http", "tcp", 80);
    Serial.printf("pasos del movil: http://tamapoke.local/steps (%s)\n", WiFi.localIP().toString().c_str());
  }
  srv->handleClient();
}

void phoneStepsStop() {
  if (!srv) return;
  srv->stop();
  delete srv;
  srv = nullptr;
  MDNS.end();
}

bool phoneStepsOn() { return srv != nullptr; }

bool phoneHockeyTake(uint16_t &min) {
  portENTER_CRITICAL(&mux);
  min = pendHockey;
  pendHockey = 0;
  portEXIT_CRITICAL(&mux);
  return min > 0;
}

bool phoneStepsTake(uint32_t &n) {
  bool hay;
  portENTER_CRITICAL(&mux);
  hay = pendNew;
  n = pendSteps;
  pendNew = false;
  portEXIT_CRITICAL(&mux);
  return hay;
}
