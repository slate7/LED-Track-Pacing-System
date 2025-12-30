#ifndef WEB_SERVER_H
#define WEB_SERVER_H

#include <WebServer.h>
#include <Preferences.h>
#include "config.h"
#include "pacer.h"
#include "web_page.h"

// Global variables (extern means defined in main .ino file)
extern WebServer server;
extern Preferences preferences;
extern bool systemRunning;
extern int connectedClients;
extern int TOTAL_SEGMENTS;

// Serve the main HTML page
void handleRoot() {
  server.send(200, "text/html", HTML_PAGE);
}

// Handle system status requests (for live updates)
void handleStatus() {
  String json = "{\"running\":";
  json += systemRunning ? "true" : "false";
  json += ",\"clients\":";
  json += connectedClients;
  json += ",\"positions\":[";

  for (int i = 0; i < MAX_PACERS; i++) {
    if (i > 0) json += ",";
    json += "{\"enabled\":";
    json += pacers[i].enabled ? "true" : "false";
    json += ",\"position\":";
    json += String(pacers[i].currentPosition * 5.0 / LOGICAL_UNITS_PER_SEGMENT, 2);
    json += ",\"color\":\"";
    char colorHex[8];
    sprintf(colorHex, "#%02X%02X%02X", pacers[i].color.r, pacers[i].color.g, pacers[i].color.b);
    json += colorHex;
    json += "\"}";
  }

  json += "]}";
  server.send(200, "application/json", json);
}

// Handle save preset request
void handleSavePreset() {
  if (server.hasArg("plain")) {
    String body = server.arg("plain");

    int nameStart = body.indexOf("\"name\":\"") + 8;
    int nameEnd = body.indexOf("\"", nameStart);
    String name = body.substring(nameStart, nameEnd);

    int dataStart = body.indexOf("\"data\":\"") + 8;
    int dataEnd = body.lastIndexOf("\"");
    String data = body.substring(dataStart, dataEnd);

    data.replace("\\\"", "\"");

    String key = "preset_" + name;
    preferences.putString(key.c_str(), data);

    Serial.println("Saved preset: " + name);
    server.send(200, "text/plain", "OK");
  } else {
    server.send(400, "text/plain", "No data");
  }
}

// Handle load preset request
void handleLoadPreset() {
  if (server.hasArg("name")) {
    String name = server.arg("name");
    String key = "preset_" + name;
    String data = preferences.getString(key.c_str(), "");

    if (data.length() > 0) {
      server.send(200, "application/json", data);
    } else {
      server.send(404, "text/plain", "Preset not found");
    }
  } else {
    server.send(400, "text/plain", "No name provided");
  }
}

// Handle list presets request
void handleListPresets() {
  String json = "[";
  bool first = true;

  preferences.begin("trackpacer", true);

  for (int i = 0; i < 20; i++) {
    String key = "preset_" + String(i);
    if (preferences.isKey(key.c_str())) {
      if (!first) json += ",";
      String name = key.substring(7);
      json += "\"" + name + "\"";
      first = false;
    }
  }

  preferences.end();
  preferences.begin("trackpacer", false);

  json += "]";
  server.send(200, "application/json", json);
}

// Handle delete preset request
void handleDeletePreset() {
  if (server.hasArg("plain")) {
    String name = server.arg("plain");
    String key = "preset_" + name;
    preferences.remove(key.c_str());

    Serial.println("Deleted preset: " + name);
    server.send(200, "text/plain", "OK");
  } else {
    server.send(400, "text/plain", "No data");
  }
}

// Handle segment count updates
void handleSegments() {
    if (server.hasArg("plain")) {
        String command = server.arg("plain");

        if (command.startsWith("SET:")) {
            int newSegments = command.substring(4).toInt();

            if (newSegments >= 1 && newSegments <= 80) {
                TOTAL_SEGMENTS = newSegments;
                current_NUM_LEDS = TOTAL_SEGMENTS * LOGICAL_UNITS_PER_SEGMENT;

                systemRunning = false;
                FastLED.clear();
                FastLED.show();

                Serial.print("Segments set to: ");
                Serial.println(TOTAL_SEGMENTS);
                Serial.print("Total Logical LEDs (for pacing): ");
                Serial.println(current_NUM_LEDS);
            }
        }
        server.send(200, "text/plain", "OK");
    } else {
        server.send(400, "text/plain", "No data");
    }
}

// Handle start/stop commands
void handleCommand() {
  if (server.hasArg("plain")) {
    String command = server.arg("plain");

    if (command.startsWith("START:")) {
      parseStartCommand(command);
      systemRunning = true;
    } else if (command == "STOP") {
      systemRunning = false;
      FastLED.clear();
      FastLED.show();
    }

    server.send(200, "text/plain", "OK");
  } else {
    server.send(400, "text/plain", "No data");
  }
}

#endif
