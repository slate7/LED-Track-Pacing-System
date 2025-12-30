#ifndef PACER_H
#define PACER_H

#include <FastLED.h>
#include "config.h"

// Pacer Structure
struct Pacer {
  bool enabled;
  float timePerLap;      // seconds to complete 5m lap
  int startPosition;     // meters (0m to 4m)
  CRGB color;
  float currentPosition; // Position in logical units (0 to current_NUM_LEDS - 1)
  unsigned long lastUpdate;
};

// Global Variables (extern means defined elsewhere, in main .ino)
extern Pacer pacers[MAX_PACERS];
extern CRGB leds[MAX_LOGICAL_LEDS];
extern int current_NUM_LEDS;
extern int TOTAL_SEGMENTS;

// Function to convert hex string to CRGB color
CRGB hexToColor(String hex) {
  if (hex.startsWith("#")) {
    hex = hex.substring(1);
  }

  long number = strtol(hex.c_str(), NULL, 16);
  int r = (number >> 16) & 0xFF;
  int g = (number >> 8) & 0xFF;
  int b = number & 0xFF;

  return CRGB(r, g, b);
}

// Parse START command and configure pacers
void parseStartCommand(String cmd) {
  cmd = cmd.substring(6);

  int pacerIndex = 0;
  int lastPos = 0;

  for (int i = 0; i < MAX_PACERS; i++) {
    pacers[i].enabled = false;
  }

  while (cmd.indexOf('|', lastPos) != -1 && pacerIndex < MAX_PACERS) {
    int pipePos = cmd.indexOf('|', lastPos);
    String pacerData = cmd.substring(lastPos, pipePos);

    int comma1 = pacerData.indexOf(',');
    int comma2 = pacerData.indexOf(',', comma1 + 1);

    if (comma1 != -1 && comma2 != -1) {
      float timePerLap = pacerData.substring(0, comma1).toFloat();
      int startMeters = pacerData.substring(comma1 + 1, comma2).toInt();
      String colorHex = pacerData.substring(comma2 + 1);

      pacers[pacerIndex].enabled = true;
      pacers[pacerIndex].timePerLap = timePerLap;
      pacers[pacerIndex].startPosition = startMeters;

      float unitsPerMeter = (float)LOGICAL_UNITS_PER_SEGMENT / 5.0;
      pacers[pacerIndex].currentPosition = startMeters * unitsPerMeter;
      pacers[pacerIndex].color = hexToColor(colorHex);
      pacers[pacerIndex].lastUpdate = millis();

      pacerIndex++;
    }

    lastPos = pipePos + 1;
  }
}

// Update pacer positions based on elapsed time
void updatePacers() {
  unsigned long now = millis();

  for (int i = 0; i < MAX_PACERS; i++) {
    if (!pacers[i].enabled) continue;

    if (pacers[i].timePerLap <= 0) {
        pacers[i].timePerLap = 1.0;
    }

    unsigned long elapsed = now - pacers[i].lastUpdate;
    pacers[i].lastUpdate = now;

    float unitsPerSecond = (float)current_NUM_LEDS / pacers[i].timePerLap;
    float unitsMoved = unitsPerSecond * (elapsed / 1000.0);

    pacers[i].currentPosition += unitsMoved;

    pacers[i].currentPosition = fmod(pacers[i].currentPosition, (float)current_NUM_LEDS);
  }
}

#endif
