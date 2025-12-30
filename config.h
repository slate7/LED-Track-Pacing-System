#ifndef CONFIG_H
#define CONFIG_H

// WiFi Settings
const char* AP_SSID = "TrackPacer";
const char* AP_PASSWORD = "pacer2024";

// LED Configuration
// Hardware: 24V WS2811 LED strips with 5V data input
// Power: 24V AC adapter with step-down converter to 5V for ESP32
#define LED_PIN 12

// --- Scalable Configuration ---
#define LOGICAL_UNITS_PER_SEGMENT 50  // 50 chips per 5-meter segment
#define LEDS_PER_SEGMENT 20            // Length of the pacer in logical units (5 chips long)

// Pacer Configuration
#define MAX_PACERS 3

// Maximum possible segments (e.g., 80 segments * 50 units = 4000, but we cap at 500 for memory)
#define MAX_LOGICAL_LEDS 500

#endif
