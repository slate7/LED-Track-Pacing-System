#ifndef LED_CONTROL_H
#define LED_CONTROL_H

#include <FastLED.h>
#include "config.h"
#include "pacer.h"

// Render LEDs based on current pacer positions
void renderLEDs() {
  FastLED.clear();

  for (int i = 0; i < MAX_PACERS; i++) {
    if (!pacers[i].enabled) continue;

    int startUnit = (int)pacers[i].currentPosition;

    for (int j = 0; j < LEDS_PER_SEGMENT; j++) {
      int unitIndex = (startUnit + j) % current_NUM_LEDS;

      leds[unitIndex] = pacers[i].color;
    }

    for (int k = current_NUM_LEDS; k < MAX_LOGICAL_LEDS; k++) {
        leds[k] = CRGB::Black;
    }
  }
}

#endif
