#include <WiFi.h>
#include <WebServer.h>
#include <FastLED.h>
#include <math.h>
#include <Preferences.h>

// Include our modular headers
#include "config.h"
#include "pacer.h"
#include "led_control.h"
#include "web_page.h"
#include "web_server.h"

// Global Variables
WebServer server(80);
Preferences preferences;

Pacer pacers[MAX_PACERS];
CRGB leds[MAX_LOGICAL_LEDS];
int current_NUM_LEDS = LOGICAL_UNITS_PER_SEGMENT; // Starts at 50
int TOTAL_SEGMENTS = 1; // Default: 1 segment (5 meters total)

bool systemRunning = false;
unsigned long lastStatusUpdate = 0;
int connectedClients = 0;

void setup() {
  Serial.begin(115200);

  preferences.begin("trackpacer", false);

  FastLED.addLeds<WS2811, LED_PIN, RBG>(leds, MAX_LOGICAL_LEDS);
  FastLED.setBrightness(255);
  FastLED.clear();
  FastLED.show();

  for (int i = 0; i < MAX_PACERS; i++) {
    pacers[i].enabled = false;
    pacers[i].currentPosition = 0;
    pacers[i].lastUpdate = 0;
  }

  WiFi.softAP(AP_SSID, AP_PASSWORD);
  IPAddress IP = WiFi.softAPIP();

  Serial.print("Connect to: ");
  Serial.println(AP_SSID);
  Serial.print("Go to: http://");
  Serial.println(IP);

  server.on("/", HTTP_GET, handleRoot);
  server.on("/command", HTTP_POST, handleCommand);
  server.on("/segments", HTTP_POST, handleSegments);
  server.on("/status", HTTP_GET, handleStatus);
  server.on("/preset/save", HTTP_POST, handleSavePreset);
  server.on("/preset/load", HTTP_GET, handleLoadPreset);
  server.on("/preset/list", HTTP_GET, handleListPresets);
  server.on("/preset/delete", HTTP_POST, handleDeletePreset);
  server.begin();
}

void loop() {
  server.handleClient();
  connectedClients = WiFi.softAPgetStationNum();

  if (systemRunning) {
    updatePacers();
    renderLEDs();
    FastLED.show();
  }
}
