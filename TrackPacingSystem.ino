#include <WiFi.h>
#include <WebServer.h>
#include <FastLED.h>
#include <math.h>
#include <Preferences.h>

// WiFi Settings
const char* AP_SSID = "TrackPacer";
const char* AP_PASSWORD = "pacer2024";

// LED Configuration 
#define LED_PIN 15

// --- Scalable Configuration ---
#define LOGICAL_UNITS_PER_SEGMENT 50  // 50 chips per 5-meter segment
int TOTAL_SEGMENTS = 1;               // Default: 1 segment (5 meters total), updated by website
#define LEDS_PER_SEGMENT 20            // Length of the pacer in logical units (5 chips long)

// Pacer Configuration
#define MAX_PACERS 3

// The leds array must be sized to the maximum possible segments (e.g., 10 segments * 50 units = 500)
#define MAX_LOGICAL_LEDS 500 
CRGB leds[MAX_LOGICAL_LEDS];
int current_NUM_LEDS = LOGICAL_UNITS_PER_SEGMENT; // Starts at 50

WebServer server(80);
Preferences preferences;

struct Pacer {
  bool enabled;
  float timePerLap;      // seconds to complete 5m lap
  int startPosition;     // meters (0m to 4m)
  CRGB color;
  float currentPosition; // Position in logical units (0 to current_NUM_LEDS - 1)
  unsigned long lastUpdate;
};

Pacer pacers[MAX_PACERS];
bool systemRunning = false;
unsigned long lastStatusUpdate = 0;
int connectedClients = 0;

// Function Prototypes 
void handleRoot();
void handleCommand();
void handleSegments();
void handleStatus();
void handleSavePreset();
void handleLoadPreset();
void handleListPresets();
void handleDeletePreset();
CRGB hexToColor(String hex);
void parseStartCommand(String cmd);
void updatePacers();
void renderLEDs();
String generatePacerPositions();

const char HTML_PAGE[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>Track LED Pacer</title>
    <style>
        * { box-sizing: border-box; }
        body { 
            font-family: -apple-system, BlinkMacSystemFont, 'Segoe UI', Roboto, Oxygen, Ubuntu, sans-serif;
            background: linear-gradient(135deg, #012169 0%, #003087 100%);
            color: white; 
            padding: 0;
            margin: 0;
            min-height: 100vh;
        }
        .container { 
            max-width: 480px; 
            margin: 0 auto; 
            padding: 20px 16px;
        }
        
        h1 { 
            text-align: center; 
            font-size: 1.75em; 
            margin: 0 0 8px 0;
            color: white;
            font-weight: 700;
            letter-spacing: 0.5px;
        }
        
        .subtitle {
            text-align: center;
            color: rgba(255, 255, 255, 0.8);
            font-size: 0.9em;
            margin: 0 0 24px 0;
            font-weight: 500;
        }
        
        /* Connection Status */
        .connection-status {
            position: fixed;
            top: 12px;
            right: 12px;
            background: rgba(255, 255, 255, 0.95);
            padding: 6px 12px;
            border-radius: 20px;
            display: flex;
            align-items: center;
            gap: 6px;
            font-weight: 600;
            font-size: 0.75em;
            box-shadow: 0 2px 8px rgba(0,0,0,0.15);
            z-index: 1000;
            color: #012169;
        }
        .status-dot {
            width: 8px;
            height: 8px;
            border-radius: 50%;
            background: #10b981;
            animation: pulse 2s ease-in-out infinite;
        }
        .status-dot.disconnected {
            background: #ef4444;
            animation: none;
        }
        @keyframes pulse {
            0%, 100% { opacity: 1; }
            50% { opacity: 0.5; }
        }
        
        .card { 
            background: white;
            color: #012169;
            border-radius: 16px;
            padding: 20px;
            margin-bottom: 16px;
            box-shadow: 0 4px 12px rgba(0,0,0,0.1);
        }
        
        /* Track Visualization */
        .track-viz {
            background: #f8f9fa;
            border-radius: 12px;
            padding: 16px;
            margin-bottom: 20px;
            transition: all 0.3s ease;
            border: 2px solid #e5e7eb;
        }
        .track-viz.collapsed {
            padding: 12px 16px;
        }
        .track-viz.collapsed .track-container {
            display: none;
        }
        .track-viz h3 {
            margin: 0 0 12px 0;
            color: #012169;
            display: flex;
            justify-content: space-between;
            align-items: center;
            cursor: pointer;
            user-select: none;
            font-size: 1em;
            font-weight: 600;
        }
        .track-viz.collapsed h3 {
            margin: 0;
        }
        .toggle-icon {
            font-size: 0.7em;
            transition: transform 0.3s ease;
            color: #6b7280;
        }
        .track-viz.collapsed .toggle-icon {
            transform: rotate(180deg);
        }
        .track-container {
            position: relative;
            width: 100%;
            height: 200px;
            background: #012169;
            border-radius: 100px;
            overflow: hidden;
            box-shadow: inset 0 2px 8px rgba(0,0,0,0.3);
        }
        .track-inner {
            position: absolute;
            top: 18px;
            left: 18px;
            right: 18px;
            bottom: 18px;
            background: linear-gradient(135deg, #f8f9fa 0%, #e9ecef 100%);
            border-radius: 82px;
        }
        .track-markers {
            position: absolute;
            top: 0;
            left: 0;
            right: 0;
            bottom: 0;
            pointer-events: none;
        }
        .distance-marker {
            position: absolute;
            width: 2px;
            height: 12px;
            background: rgba(255, 255, 255, 0.6);
            transform-origin: center;
        }
        .finish-line {
            position: absolute;
            width: 3px;
            height: 16px;
            background: #ef4444;
            box-shadow: 0 0 8px rgba(239, 68, 68, 0.6);
            z-index: 5;
        }
        .track-pacer {
            position: absolute;
            width: 20px;
            height: 20px;
            border-radius: 50%;
            border: 3px solid white;
            box-shadow: 0 3px 10px rgba(0,0,0,0.4);
            transition: all 0.1s linear;
            z-index: 10;
        }
        .track-pacer::before {
            content: '';
            position: absolute;
            width: 100%;
            height: 100%;
            border-radius: 50%;
            background: inherit;
            opacity: 0.3;
            animation: pacer-pulse 1.5s ease-out infinite;
        }
        @keyframes pacer-pulse {
            0% { transform: scale(1); opacity: 0.5; }
            100% { transform: scale(2.5); opacity: 0; }
        }
        .pacer-label {
            position: absolute;
            top: -26px;
            left: 50%;
            transform: translateX(-50%);
            background: rgba(1, 33, 105, 0.9);
            color: white;
            padding: 3px 7px;
            border-radius: 4px;
            font-size: 0.65em;
            font-weight: 700;
            white-space: nowrap;
            box-shadow: 0 2px 4px rgba(0,0,0,0.2);
        }
        
        /* Info Banner */
        .info-banner { 
            background: white;
            color: #012169;
            padding: 12px 16px;
            border-radius: 12px;
            margin-bottom: 16px;
            font-weight: 700;
            text-align: center;
            font-size: 1em;
            box-shadow: 0 2px 8px rgba(0,0,0,0.08);
        }
        
        /* Running Stats Display */
        .running-stats {
            background: white;
            border-radius: 12px;
            padding: 16px;
            margin-bottom: 16px;
            display: none;
            box-shadow: 0 2px 8px rgba(0,0,0,0.08);
        }
        .running-stats.active {
            display: block;
        }
        .stats-grid {
            display: grid;
            grid-template-columns: 1fr 1fr;
            gap: 16px;
        }
        .stat-item {
            text-align: center;
        }
        .stat-label {
            font-size: 0.75em;
            color: #6b7280;
            font-weight: 600;
            text-transform: uppercase;
            letter-spacing: 0.5px;
            margin-bottom: 4px;
        }
        .stat-value {
            font-size: 2em;
            font-weight: 800;
            color: #012169;
            font-family: 'SF Mono', 'Consolas', monospace;
            line-height: 1;
        }
        .stat-value.large {
            font-size: 2.5em;
        }
        
        /* Segment Control */
        .segment-control { 
            display: flex;
            align-items: center;
            justify-content: space-between;
            margin-bottom: 16px;
            padding: 12px 16px;
            background: #f8f9fa;
            border-radius: 12px;
            gap: 12px;
            border: 2px solid #e5e7eb;
        }
        .segment-control label { 
            font-weight: 600; 
            font-size: 0.9em;
            color: #012169;
        }
        .segment-control input { 
            width: 70px;
            padding: 8px;
            border: 2px solid #012169;
            border-radius: 8px;
            font-size: 1.05em;
            text-align: center;
            font-weight: 700;
            color: #012169;
            background: white;
        }
        .segment-control button {
            background: #012169;
            color: white;
            padding: 8px 16px;
            font-weight: 700;
            border-radius: 8px;
            border: none;
            cursor: pointer;
            font-size: 0.85em;
            transition: all 0.2s ease;
        }
        .segment-control button:active {
            transform: scale(0.95);
        }
        
        /* Control Buttons */
        .controls { 
            display: grid;
            grid-template-columns: 1fr 1fr;
            gap: 12px;
            margin-bottom: 16px;
        }
        button { 
            padding: 16px;
            font-size: 1.1em;
            font-weight: 700;
            border: none;
            border-radius: 12px;
            cursor: pointer;
            transition: all 0.2s ease;
            text-transform: uppercase;
            letter-spacing: 0.5px;
        }
        button:active {
            transform: scale(0.95);
        }
        .btn-start { 
            background: #10b981; 
            color: white; 
            position: relative;
            overflow: hidden;
        }
        .btn-start.running {
            background: #059669;
            box-shadow: 0 0 20px rgba(16, 185, 129, 0.5);
            animation: pulse-green 2s ease-in-out infinite;
        }
        .btn-start.running::after {
            content: "RUNNING";
        }
        .btn-start:not(.running)::after {
            content: "START";
        }
        @keyframes pulse-green {
            0%, 100% { box-shadow: 0 0 16px rgba(16, 185, 129, 0.5); }
            50% { box-shadow: 0 0 24px rgba(16, 185, 129, 0.8); }
        }
        .btn-stop { 
            background: #ef4444; 
            color: white; 
            opacity: 0.6;
            transition: all 0.2s ease;
        }
        .btn-stop.active {
            opacity: 1;
        }
        
        /* Preset Section */
        .preset-section {
            background: #f8f9fa;
            border-radius: 12px;
            padding: 16px;
            margin-top: 16px;
            border: 2px solid #e5e7eb;
        }
        .preset-section h3 {
            margin: 0 0 12px 0;
            color: #012169;
            font-size: 1em;
            font-weight: 600;
        }
        .preset-controls {
            display: flex;
            gap: 8px;
            margin-bottom: 12px;
        }
        .preset-controls input {
            flex: 1;
            padding: 10px 12px;
            border: 2px solid #d1d5db;
            border-radius: 8px;
            font-size: 0.95em;
            color: #012169;
        }
        .preset-controls button {
            padding: 10px 16px;
            background: #012169;
            color: white;
            border: none;
            border-radius: 8px;
            cursor: pointer;
            font-weight: 600;
            font-size: 0.9em;
            white-space: nowrap;
        }
        .preset-list {
            display: flex;
            flex-direction: column;
            gap: 8px;
        }
        .preset-item {
            display: flex;
            justify-content: space-between;
            align-items: center;
            padding: 12px;
            background: white;
            border-radius: 8px;
            border: 2px solid #e5e7eb;
        }
        .preset-item span {
            font-weight: 600;
            color: #012169;
            font-size: 0.95em;
        }
        .preset-item button {
            padding: 6px 12px;
            margin-left: 6px;
            border-radius: 6px;
            border: none;
            cursor: pointer;
            font-weight: 600;
            font-size: 0.8em;
        }
        .btn-load {
            background: #10b981;
            color: white;
        }
        .btn-export {
            background: #6366f1;
            color: white;
        }
        .btn-delete {
            background: #ef4444;
            color: white;
        }
        
        /* Pacer Cards */
        .pacer-card {
            background: #f8f9fa;
            padding: 16px;
            border-radius: 12px;
            margin-bottom: 12px;
            border: 2px solid #e5e7eb;
            transition: all 0.3s ease;
        }
        .pacer-card.collapsed .pacer-content {
            display: none;
        }
        .pacer-card.collapsed {
            padding: 12px 16px;
        }
        
        .pacer-header {
            display: flex;
            justify-content: space-between;
            align-items: center;
            margin-bottom: 16px;
            padding-bottom: 12px;
            border-bottom: 2px solid #d1d5db;
            cursor: pointer;
            user-select: none;
        }
        .pacer-card.collapsed .pacer-header {
            margin-bottom: 0;
            padding-bottom: 0;
            border-bottom: none;
        }
        
        .pacer-header-left {
            display: flex;
            align-items: center;
            gap: 12px;
            flex: 1;
        }
        
        .pacer-title {
            font-size: 1.1em;
            font-weight: 700;
            color: #012169;
        }
        
        .pacer-summary {
            font-size: 0.85em;
            color: #6b7280;
            font-weight: 600;
            display: none;
        }
        .pacer-card.collapsed .pacer-summary {
            display: block;
        }
        
        .pacer-expand-icon {
            color: #6b7280;
            font-size: 0.8em;
            transition: transform 0.3s ease;
            margin-left: auto;
        }
        .pacer-card.collapsed .pacer-expand-icon {
            transform: rotate(180deg);
        }
        
        /* Toggle Switch */
        .toggle-switch {
            position: relative;
            width: 56px;
            height: 30px;
        }
        .toggle-switch input { opacity: 0; width: 0; height: 0; }
        .toggle-slider {
            position: absolute;
            cursor: pointer;
            top: 0; left: 0; right: 0; bottom: 0;
            background-color: #d1d5db;
            transition: 0.3s;
            border-radius: 30px;
        }
        .toggle-slider:before {
            position: absolute;
            content: "";
            height: 22px; width: 22px;
            left: 4px; bottom: 4px;
            background-color: white;
            transition: 0.3s;
            border-radius: 50%;
        }
        input:checked + .toggle-slider { background-color: #10b981; }
        input:checked + .toggle-slider:before { transform: translateX(26px); }
        
        /* Time Input Mode Selector */
        .time-mode-selector {
            display: flex;
            gap: 6px;
            margin-bottom: 12px;
            background: white;
            padding: 4px;
            border-radius: 10px;
            border: 2px solid #e5e7eb;
        }
        .time-mode-btn {
            flex: 1;
            padding: 8px 6px;
            background: transparent;
            border: none;
            border-radius: 8px;
            cursor: pointer;
            font-weight: 600;
            color: #6b7280;
            font-size: 0.8em;
            transition: all 0.2s ease;
        }
        .time-mode-btn.active {
            background: #012169;
            color: white;
        }
        
        .time-input-container {
            display: none;
        }
        .time-input-container.active {
            display: block;
        }
        
        .time-inputs {
            display: flex;
            gap: 8px;
            align-items: center;
            justify-content: center;
            margin: 12px 0;
        }
        .time-inputs input {
            width: 65px;
            padding: 10px;
            border: 2px solid #d1d5db;
            border-radius: 8px;
            text-align: center;
            font-size: 1.1em;
            font-weight: 700;
            color: #012169;
            background: white;
        }
        .time-inputs span {
            font-weight: 600;
            color: #012169;
            font-size: 0.95em;
        }
        
        /* Color Palette */
        .color-section {
            margin: 16px 0;
        }
        .color-section label {
            font-weight: 600;
            display: block;
            margin-bottom: 10px;
            color: #012169;
            font-size: 0.9em;
        }
        .color-palette {
            display: grid;
            grid-template-columns: repeat(5, 1fr);
            gap: 10px;
        }
        .color-option {
            height: 50px;
            border-radius: 10px;
            cursor: pointer;
            border: 3px solid transparent;
            transition: all 0.2s ease;
            box-shadow: 0 2px 6px rgba(0,0,0,0.1);
        }
        .color-option.selected { 
            border-color: #012169;
            transform: scale(1.05);
            box-shadow: 0 4px 12px rgba(1,33,105,0.3);
        }
        
        /* Pace Display */
        .pace-value { 
            font-size: 2.5em;
            font-weight: 800;
            text-align: center;
            margin: 16px 0;
            color: #012169;
            font-family: 'SF Mono', 'Consolas', monospace;
        }
        
        /* Range Slider */
        input[type="range"] { 
            width: 100%;
            margin: 12px 0;
            height: 8px;
            border-radius: 4px;
            background: #d1d5db;
            outline: none;
            -webkit-appearance: none;
        }
        input[type="range"]::-webkit-slider-thumb {
            -webkit-appearance: none;
            width: 24px;
            height: 24px;
            border-radius: 50%;
            background: #012169;
            cursor: pointer;
            box-shadow: 0 2px 4px rgba(0,0,0,0.2);
        }
        input[type="range"]::-moz-range-thumb {
            width: 24px;
            height: 24px;
            border-radius: 50%;
            background: #012169;
            cursor: pointer;
            border: none;
            box-shadow: 0 2px 4px rgba(0,0,0,0.2);
        }
        
        /* Start Position Buttons */
        .start-position {
            margin-top: 16px;
        }
        .start-position label { 
            font-weight: 600;
            display: block;
            margin-bottom: 10px;
            color: #012169;
            font-size: 0.9em;
        }
        .position-grid {
            display: grid;
            grid-template-columns: repeat(4, 1fr);
            gap: 8px;
        }
        .position-btn {
            padding: 12px 8px;
            border: 2px solid #d1d5db;
            background: white;
            border-radius: 8px;
            cursor: pointer;
            font-weight: 700;
            color: #012169;
            transition: all 0.2s ease;
            font-size: 0.95em;
        }
        .position-btn.selected { 
            background: #012169;
            color: white;
            border-color: #012169;
        }
        .position-btn:active {
            transform: scale(0.95);
        }
    </style>
</head>
<body>
    <div class="connection-status">
        <div class="status-dot" id="statusDot"></div>
        <span id="statusText">Connected</span>
    </div>

    <div class="container">
        <h1>Track LED Pacer</h1>
        <div class="subtitle">Duke University Track & Field</div>
        
        <div class="info-banner">
            <span id="segmentCount">1</span> Segment (<span id="meterLength">5</span>m)
        </div>

        <div class="running-stats" id="runningStats">
            <div class="stats-grid">
                <div class="stat-item">
                    <div class="stat-label">Elapsed Time</div>
                    <div class="stat-value large" id="elapsedTime">00:00</div>
                </div>
                <div class="stat-item">
                    <div class="stat-label">Fastest Lap</div>
                    <div class="stat-value" id="fastestLap">-</div>
                </div>
            </div>
        </div>

        <div class="card">
            <div class="track-viz" id="trackViz">
                <h3 onclick="toggleTrackViz()">
                    <span>Live Track Visualization</span>
                    <span class="toggle-icon">▼</span>
                </h3>
                <div class="track-container" id="trackContainer">
                    <div class="track-markers" id="trackMarkers"></div>
                    <div class="track-inner"></div>
                </div>
            </div>

            <div class="segment-control">
                <label for="segments">Segments:</label>
                <input type="number" id="segments" min="1" max="80" value="1">
                <button onclick="updateSegments()">APPLY</button>
            </div>

            <div class="controls">
                <button class="btn-start" id="startBtn"></button>
                <button class="btn-stop" id="stopBtn">STOP</button>
            </div>

            <div class="preset-section">
                <h3>Presets</h3>
                <div class="preset-controls">
                    <input type="text" id="presetName" placeholder="Preset name...">
                    <button onclick="savePreset()">Save</button>
                </div>
                <div class="preset-controls" style="margin-top: 10px;">
                    <button onclick="exportCurrentSettings()" style="flex: 1; background: #6366f1;">Export Current</button>
                    <input type="file" id="importFile" accept=".json" style="display:none" onchange="importPresetFile(event)">
                    <button onclick="document.getElementById('importFile').click()" style="flex: 1; background: #8b5cf6;">Import File</button>
                </div>
                <div class="preset-list" id="presetList"></div>
            </div>
        </div>

        <div class="card">
            <div class="pacer-card" id="pacerCard1">
                <div class="pacer-header" onclick="togglePacerCard(1)">
                    <div class="pacer-header-left">
                        <span class="pacer-title">Pacer 1</span>
                        <span class="pacer-summary" id="pacer1Summary">8.0s/lap</span>
                    </div>
                    <label class="toggle-switch" onclick="event.stopPropagation()">
                        <input type="checkbox" id="pace1Enable">
                        <span class="toggle-slider"></span>
                    </label>
                    <span class="pacer-expand-icon">▼</span>
                </div>

                <div class="pacer-content">

                <div class="time-mode-selector">
                    <button class="time-mode-btn active" onclick="setTimeMode(1, 'lap')">Lap Time</button>
                    <button class="time-mode-btn" onclick="setTimeMode(1, 'mile')">Mile Pace</button>
                    <button class="time-mode-btn" onclick="setTimeMode(1, 'total')">Total Time</button>
                </div>

                <div class="time-input-container active" id="lapMode1">
                    <div class="pace-value"><span id="pace1Value">8.0</span> s/lap</div>
                    <input type="range" id="pace1Range" min="2.0" max="30.0" value="8.0" step="0.5">
                </div>

                <div class="time-input-container" id="mileMode1">
                    <div class="time-inputs">
                        <input type="number" id="mile1Min" min="0" max="59" value="6" placeholder="MM">
                        <span>:</span>
                        <input type="number" id="mile1Sec" min="0" max="59" value="0" placeholder="SS">
                        <span>/mile</span>
                    </div>
                    <button onclick="calculateFromMile(1)" style="width: 100%; background: #003087; color: white;">Calculate</button>
                </div>

                <div class="time-input-container" id="totalMode1">
                    <div class="time-inputs">
                        <input type="number" id="total1Min" min="0" max="999" value="10" placeholder="MM">
                        <span>:</span>
                        <input type="number" id="total1Sec" min="0" max="59" value="0" placeholder="SS">
                        <span>total</span>
                    </div>
                    <button onclick="calculateFromTotal(1)" style="width: 100%; background: #003087; color: white;">Calculate</button>
                </div>
                
                <div class="color-section">
                    <label>Color:</label>
                    <div class="color-palette" id="palette1">
                        <div class="color-option selected" style="background: #FF0000;" data-led="#FF0000" onclick="selectColor(1, '#FF0000', this)"></div>
                        <div class="color-option" style="background: #0000FF;" data-led="#0000FF" onclick="selectColor(1, '#0000FF', this)"></div>
                        <div class="color-option" style="background: #00FF00;" data-led="#00FF00" onclick="selectColor(1, '#00FF00', this)"></div>
                        <div class="color-option" style="background: #FFFF00;" data-led="#FFFF00" onclick="selectColor(1, '#FFFF00', this)"></div>
                        <div class="color-option" style="background: #FFFFFF; border: 2px solid #dee2e6;" data-led="#FFFFFF" onclick="selectColor(1, '#FFFFFF', this)"></div>
                    </div>
                </div>
                
                <div class="start-position">
                    <label>Start Position:</label>
                    <div class="position-grid" id="position1">
                        <button class="position-btn selected" onclick="selectPosition(1, 0, this)">0m</button>
                        <button class="position-btn" onclick="selectPosition(1, 0, this)">0m</button>
                        <button class="position-btn" onclick="selectPosition(1, 0, this)">0m</button>
                        <button class="position-btn" onclick="selectPosition(1, 0, this)">0m</button>
                    </div>
                </div>
                </div>
            </div>
        </div>

        <div class="card">
            <div class="pacer-card collapsed" id="pacerCard2">
                <div class="pacer-header" onclick="togglePacerCard(2)">
                    <div class="pacer-header-left">
                        <span class="pacer-title">Pacer 2</span>
                        <span class="pacer-summary" id="pacer2Summary">10.0s/lap</span>
                    </div>
                    <label class="toggle-switch" onclick="event.stopPropagation()">
                        <input type="checkbox" id="pace2Enable">
                        <span class="toggle-slider"></span>
                    </label>
                    <span class="pacer-expand-icon">▼</span>
                </div>

                <div class="pacer-content">

                <div class="time-mode-selector">
                    <button class="time-mode-btn active" onclick="setTimeMode(2, 'lap')">Lap Time</button>
                    <button class="time-mode-btn" onclick="setTimeMode(2, 'mile')">Mile Pace</button>
                    <button class="time-mode-btn" onclick="setTimeMode(2, 'total')">Total Time</button>
                </div>

                <div class="time-input-container active" id="lapMode2">
                    <div class="pace-value"><span id="pace2Value">10.0</span> s/lap</div>
                    <input type="range" id="pace2Range" min="2.0" max="30.0" value="10.0" step="0.5">
                </div>

                <div class="time-input-container" id="mileMode2">
                    <div class="time-inputs">
                        <input type="number" id="mile2Min" min="0" max="59" value="8" placeholder="MM">
                        <span>:</span>
                        <input type="number" id="mile2Sec" min="0" max="59" value="0" placeholder="SS">
                        <span>/mile</span>
                    </div>
                    <button onclick="calculateFromMile(2)" style="width: 100%; background: #003087; color: white;">Calculate</button>
                </div>

                <div class="time-input-container" id="totalMode2">
                    <div class="time-inputs">
                        <input type="number" id="total2Min" min="0" max="999" value="15" placeholder="MM">
                        <span>:</span>
                        <input type="number" id="total2Sec" min="0" max="59" value="0" placeholder="SS">
                        <span>total</span>
                    </div>
                    <button onclick="calculateFromTotal(2)" style="width: 100%; background: #003087; color: white;">Calculate</button>
                </div>
                
                <div class="color-section">
                    <label>Color:</label>
                    <div class="color-palette" id="palette2">
                        <div class="color-option" style="background: #FF0000;" data-led="#FF0000" onclick="selectColor(2, '#FF0000', this)"></div>
                        <div class="color-option selected" style="background: #0000FF;" data-led="#0000FF" onclick="selectColor(2, '#0000FF', this)"></div>
                        <div class="color-option" style="background: #00FF00;" data-led="#00FF00" onclick="selectColor(2, '#00FF00', this)"></div>
                        <div class="color-option" style="background: #FFFF00;" data-led="#FFFF00" onclick="selectColor(2, '#FFFF00', this)"></div>
                        <div class="color-option" style="background: #FFFFFF; border: 2px solid #dee2e6;" data-led="#FFFFFF" onclick="selectColor(2, '#FFFFFF', this)"></div>
                    </div>
                </div>
                
                <div class="start-position">
                    <label>Start Position:</label>
                    <div class="position-grid" id="position2">
                        <button class="position-btn selected" onclick="selectPosition(2, 0, this)">0m</button>
                        <button class="position-btn" onclick="selectPosition(2, 0, this)">0m</button>
                        <button class="position-btn" onclick="selectPosition(2, 0, this)">0m</button>
                        <button class="position-btn" onclick="selectPosition(2, 0, this)">0m</button>
                    </div>
                </div>
                </div>
            </div>
        </div>

        <div class="card">
            <div class="pacer-card collapsed" id="pacerCard3">
                <div class="pacer-header" onclick="togglePacerCard(3)">
                    <div class="pacer-header-left">
                        <span class="pacer-title">Pacer 3</span>
                        <span class="pacer-summary" id="pacer3Summary">15.0s/lap</span>
                    </div>
                    <label class="toggle-switch" onclick="event.stopPropagation()">
                        <input type="checkbox" id="pace3Enable">
                        <span class="toggle-slider"></span>
                    </label>
                    <span class="pacer-expand-icon">▼</span>
                </div>

                <div class="pacer-content">

                <div class="time-mode-selector">
                    <button class="time-mode-btn active" onclick="setTimeMode(3, 'lap')">Lap Time</button>
                    <button class="time-mode-btn" onclick="setTimeMode(3, 'mile')">Mile Pace</button>
                    <button class="time-mode-btn" onclick="setTimeMode(3, 'total')">Total Time</button>
                </div>

                <div class="time-input-container active" id="lapMode3">
                    <div class="pace-value"><span id="pace3Value">15.0</span> s/lap</div>
                    <input type="range" id="pace3Range" min="2.0" max="30.0" value="15.0" step="0.5">
                </div>

                <div class="time-input-container" id="mileMode3">
                    <div class="time-inputs">
                        <input type="number" id="mile3Min" min="0" max="59" value="10" placeholder="MM">
                        <span>:</span>
                        <input type="number" id="mile3Sec" min="0" max="59" value="0" placeholder="SS">
                        <span>/mile</span>
                    </div>
                    <button onclick="calculateFromMile(3)" style="width: 100%; background: #003087; color: white;">Calculate</button>
                </div>

                <div class="time-input-container" id="totalMode3">
                    <div class="time-inputs">
                        <input type="number" id="total3Min" min="0" max="999" value="20" placeholder="MM">
                        <span>:</span>
                        <input type="number" id="total3Sec" min="0" max="59" value="0" placeholder="SS">
                        <span>total</span>
                    </div>
                    <button onclick="calculateFromTotal(3)" style="width: 100%; background: #003087; color: white;">Calculate</button>
                </div>
                
                <div class="color-section">
                    <label>Color:</label>
                    <div class="color-palette" id="palette3">
                        <div class="color-option" style="background: #FF0000;" data-led="#FF0000" onclick="selectColor(3, '#FF0000', this)"></div>
                        <div class="color-option" style="background: #0000FF;" data-led="#0000FF" onclick="selectColor(3, '#0000FF', this)"></div>
                        <div class="color-option" style="background: #00FF00;" data-led="#00FF00" onclick="selectColor(3, '#00FF00', this)"></div>
                        <div class="color-option selected" style="background: #FFFF00;" data-led="#FFFF00" onclick="selectColor(3, '#FFFF00', this)"></div>
                        <div class="color-option" style="background: #FFFFFF; border: 2px solid #dee2e6;" data-led="#FFFFFF" onclick="selectColor(3, '#FFFFFF', this)"></div>
                    </div>
                </div>
                
                <div class="start-position">
                    <label>Start Position:</label>
                    <div class="position-grid" id="position3">
                        <button class="position-btn selected" onclick="selectPosition(3, 0, this)">0m</button>
                        <button class="position-btn" onclick="selectPosition(3, 0, this)">0m</button>
                        <button class="position-btn" onclick="selectPosition(3, 0, this)">0m</button>
                        <button class="position-btn" onclick="selectPosition(3, 0, this)">0m</button>
                    </div>
                </div>
                </div>
            </div>
        </div>
    </div>

    <script>
        let selectedColors = ['#FF0000', '#0000FF', '#FFFF00'];
        let selectedPositions = [0, 0, 0];
        let currentSegments = 1;
        let isRunning = false;
        let statusCheckInterval;
        let pacerPositions = [];
        let startTime = null;
        let elapsedInterval = null;
        let lapCounts = [0, 0, 0];
        let lastPositions = [0, 0, 0];

        function togglePacerCard(cardNum) {
            const card = document.getElementById(`pacerCard${cardNum}`);
            card.classList.toggle('collapsed');
        }

        function updatePacerSummary(pacer) {
            const time = document.getElementById(`pace${pacer}Range`).value;
            document.getElementById(`pacer${pacer}Summary`).textContent = `${time}s/lap`;
        }

        function toggleTrackViz() {
            const viz = document.getElementById('trackViz');
            viz.classList.toggle('collapsed');
        }

        function updateButtonStates(running) {
            const startBtn = document.getElementById('startBtn');
            const stopBtn = document.getElementById('stopBtn');
            const statsDisplay = document.getElementById('runningStats');
            
            if (running) {
                startBtn.classList.add('running');
                stopBtn.classList.add('active');
                statsDisplay.classList.add('active');
                
                if (!startTime) {
                    startTime = Date.now();
                    lapCounts = [0, 0, 0];
                    lastPositions = [0, 0, 0];
                }
                
                if (!elapsedInterval) {
                    elapsedInterval = setInterval(updateElapsedTime, 100);
                }
            } else {
                startBtn.classList.remove('running');
                stopBtn.classList.remove('active');
                statsDisplay.classList.remove('active');
                
                if (elapsedInterval) {
                    clearInterval(elapsedInterval);
                    elapsedInterval = null;
                }
                startTime = null;
            }
        }

        function updateElapsedTime() {
            if (!startTime) return;
            
            const elapsed = Date.now() - startTime;
            const minutes = Math.floor(elapsed / 60000);
            const seconds = Math.floor((elapsed % 60000) / 1000);
            
            document.getElementById('elapsedTime').textContent = 
                `${String(minutes).padStart(2, '0')}:${String(seconds).padStart(2, '0')}`;
        }

        function updateLapCounts() {
            const trackLength = currentSegments * 5;
            let fastestLap = null;
            
            for (let i = 0; i < pacerPositions.length; i++) {
                if (!pacerPositions[i].enabled) continue;
                
                const currentPos = pacerPositions[i].position;
                const lastPos = lastPositions[i];
                
                // Check if pacer crossed the start/finish line (0m)
                if (lastPos > trackLength * 0.8 && currentPos < trackLength * 0.2) {
                    lapCounts[i]++;
                }
                
                lastPositions[i] = currentPos;
                
                // Track fastest lap time
                const lapTime = parseFloat(document.getElementById(`pace${i+1}Range`).value);
                if (lapCounts[i] > 0 && (fastestLap === null || lapTime < fastestLap)) {
                    fastestLap = lapTime;
                }
            }
            
            if (fastestLap !== null) {
                document.getElementById('fastestLap').textContent = fastestLap.toFixed(1) + 's';
            }
        }

        function createTrackMarkers() {
            const container = document.getElementById('trackContainer');
            const markersDiv = document.getElementById('trackMarkers');
            markersDiv.innerHTML = '';
            
            const trackLength = currentSegments * 5;
            const numMarkers = Math.min(trackLength / 5, 20); // One marker every 5m, max 20
            
            // Add finish line at 0 degrees (top)
            const finishLine = document.createElement('div');
            finishLine.className = 'finish-line';
            const flRadius = (Math.min(container.offsetWidth, container.offsetHeight) / 2) - 9;
            finishLine.style.left = (container.offsetWidth / 2 - 1.5) + 'px';
            finishLine.style.top = (container.offsetHeight / 2 - flRadius - 8) + 'px';
            markersDiv.appendChild(finishLine);
            
            // Add distance markers
            for (let i = 1; i < numMarkers; i++) {
                const angle = (i / numMarkers) * 2 * Math.PI - Math.PI / 2;
                const radius = (Math.min(container.offsetWidth, container.offsetHeight) / 2) - 9;
                
                const marker = document.createElement('div');
                marker.className = 'distance-marker';
                
                const x = container.offsetWidth / 2 + radius * Math.cos(angle);
                const y = container.offsetHeight / 2 + radius * Math.sin(angle);
                
                marker.style.left = (x - 1) + 'px';
                marker.style.top = (y - 6) + 'px';
                marker.style.transform = `rotate(${angle + Math.PI / 2}rad)`;
                
                markersDiv.appendChild(marker);
            }
        }

        function setTimeMode(pacer, mode) {
            const modes = ['lap', 'mile', 'total'];
            const parent = document.querySelector(`#pace${pacer}Enable`).closest('.pacer-card');
            
            parent.querySelectorAll('.time-mode-btn').forEach(btn => btn.classList.remove('active'));
            event.target.classList.add('active');
            
            modes.forEach(m => {
                const container = document.getElementById(`${m}Mode${pacer}`);
                if (container) {
                    container.classList.remove('active');
                }
            });
            
            const activeContainer = document.getElementById(`${mode}Mode${pacer}`);
            if (activeContainer) {
                activeContainer.classList.add('active');
            }
        }

        function calculateFromMile(pacer) {
            const minutes = parseInt(document.getElementById(`mile${pacer}Min`).value) || 0;
            const seconds = parseInt(document.getElementById(`mile${pacer}Sec`).value) || 0;
            
            const totalSeconds = minutes * 60 + seconds;
            const metersPerMile = 1609.34;
            const trackLength = currentSegments * 5;
            
            const secondsPerMeter = totalSeconds / metersPerMile;
            const secondsPerLap = secondsPerMeter * trackLength;
            
            document.getElementById(`pace${pacer}Range`).value = secondsPerLap.toFixed(1);
            document.getElementById(`pace${pacer}Value`).textContent = secondsPerLap.toFixed(1);
            
            setTimeMode(pacer, 'lap');
        }

        function calculateFromTotal(pacer) {
            const minutes = parseInt(document.getElementById(`total${pacer}Min`).value) || 0;
            const seconds = parseInt(document.getElementById(`total${pacer}Sec`).value) || 0;
            
            const totalSeconds = minutes * 60 + seconds;
            const trackLength = currentSegments * 5;
            
            const lapsNeeded = prompt(`How many laps for this total time? (Track is ${trackLength}m per lap)`);
            if (!lapsNeeded || lapsNeeded <= 0) return;
            
            const secondsPerLap = totalSeconds / parseFloat(lapsNeeded);
            
            document.getElementById(`pace${pacer}Range`).value = secondsPerLap.toFixed(1);
            document.getElementById(`pace${pacer}Value`).textContent = secondsPerLap.toFixed(1);
            
            setTimeMode(pacer, 'lap');
        }

        function updateConnectionStatus(connected) {
            const dot = document.getElementById('statusDot');
            const text = document.getElementById('statusText');
            
            if (connected) {
                dot.classList.remove('disconnected');
                text.textContent = 'Connected';
            } else {
                dot.classList.add('disconnected');
                text.textContent = 'Disconnected';
            }
        }

        function checkStatus() {
            fetch('/status')
                .then(response => response.json())
                .then(data => {
                    updateConnectionStatus(true);
                    if (data.running !== isRunning) {
                        isRunning = data.running;
                        updateButtonStates(isRunning);
                    }
                    if (data.positions) {
                        pacerPositions = data.positions;
                        updateLapCounts();
                        updateTrackVisualization();
                    }
                })
                .catch(() => updateConnectionStatus(false));
        }

        function updateTrackVisualization() {
            const container = document.getElementById('trackContainer');
            const existingPacers = container.querySelectorAll('.track-pacer');
            existingPacers.forEach(p => p.remove());
            
            if (!pacerPositions || pacerPositions.length === 0) return;
            
            const trackLength = currentSegments * 5;
            
            pacerPositions.forEach((pacer, index) => {
                if (!pacer.enabled) return;
                
                const positionPercent = (pacer.position % trackLength) / trackLength;
                const angle = positionPercent * 2 * Math.PI - Math.PI / 2;
                
                const centerX = container.offsetWidth / 2;
                const centerY = container.offsetHeight / 2;
                const radius = (Math.min(container.offsetWidth, container.offsetHeight) / 2) - 30;
                
                const x = centerX + radius * Math.cos(angle);
                const y = centerY + radius * Math.sin(angle);
                
                const pacerEl = document.createElement('div');
                pacerEl.className = 'track-pacer';
                pacerEl.style.left = (x - 10) + 'px';
                pacerEl.style.top = (y - 10) + 'px';
                pacerEl.style.background = pacer.color;
                
                const label = document.createElement('div');
                label.className = 'pacer-label';
                label.textContent = `P${index + 1}`;
                pacerEl.appendChild(label);
                
                container.appendChild(pacerEl);
            });
        }

        function savePreset() {
            const name = document.getElementById('presetName').value.trim();
            if (!name) {
                alert('Please enter a preset name');
                return;
            }
            
            const preset = {
                segments: currentSegments,
                pacers: []
            };
            
            for (let i = 1; i <= 3; i++) {
                preset.pacers.push({
                    enabled: document.getElementById(`pace${i}Enable`).checked,
                    time: parseFloat(document.getElementById(`pace${i}Range`).value),
                    color: selectedColors[i - 1],
                    position: selectedPositions[i - 1]
                });
            }
            
            fetch('/preset/save', {
                method: 'POST',
                headers: {'Content-Type': 'application/json'},
                body: JSON.stringify({name: name, data: JSON.stringify(preset)})
            })
            .then(() => {
                document.getElementById('presetName').value = '';
                loadPresetList();
            });
        }

        function loadPreset(name) {
            fetch('/preset/load?name=' + encodeURIComponent(name))
                .then(response => response.json())
                .then(preset => {
                    document.getElementById('segments').value = preset.segments;
                    updateSegments();
                    
                    preset.pacers.forEach((p, i) => {
                        const idx = i + 1;
                        document.getElementById(`pace${idx}Enable`).checked = p.enabled;
                        document.getElementById(`pace${idx}Range`).value = p.time;
                        document.getElementById(`pace${idx}Value`).textContent = p.time;
                        selectedColors[i] = p.color;
                        selectedPositions[i] = p.position;
                        
                        const palette = document.getElementById(`palette${idx}`);
                        palette.querySelectorAll('.color-option').forEach(opt => {
                            opt.classList.remove('selected');
                            if (opt.getAttribute('data-led') === p.color) {
                                opt.classList.add('selected');
                            }
                        });
                    });
                    
                    updatePositionButtons();
                    alert('Preset loaded!');
                });
        }

        function deletePreset(name) {
            if (!confirm(`Delete preset "${name}"?`)) return;
            
            fetch('/preset/delete', {
                method: 'POST',
                headers: {'Content-Type': 'text/plain'},
                body: name
            })
            .then(() => loadPresetList());
        }

        function exportCurrentSettings() {
            const preset = {
                name: "exported_" + new Date().toISOString().slice(0,19).replace(/:/g,'-'),
                segments: currentSegments,
                pacers: []
            };
            
            for (let i = 1; i <= 3; i++) {
                preset.pacers.push({
                    enabled: document.getElementById(`pace${i}Enable`).checked,
                    time: parseFloat(document.getElementById(`pace${i}Range`).value),
                    color: selectedColors[i - 1],
                    position: selectedPositions[i - 1]
                });
            }
            
            downloadPresetFile(preset);
        }

        function exportPreset(name) {
            fetch('/preset/load?name=' + encodeURIComponent(name))
                .then(response => response.json())
                .then(preset => {
                    preset.name = name;
                    downloadPresetFile(preset);
                })
                .catch(err => alert('Failed to export preset: ' + err));
        }

        function downloadPresetFile(preset) {
            const dataStr = JSON.stringify(preset, null, 2);
            const dataUri = 'data:application/json;charset=utf-8,'+ encodeURIComponent(dataStr);
            
            const linkElement = document.createElement('a');
            linkElement.setAttribute('href', dataUri);
            linkElement.setAttribute('download', 'track_pacer_' + preset.name + '.json');
            linkElement.click();
            
            // Show feedback
            const originalText = event.target.textContent;
            event.target.textContent = 'Downloaded';
            setTimeout(() => {
                if (event.target) event.target.textContent = originalText;
            }, 2000);
        }

        function importPresetFile(event) {
            const file = event.target.files[0];
            if (!file) return;
            
            const reader = new FileReader();
            reader.onload = function(e) {
                try {
                    const preset = JSON.parse(e.target.result);
                    
                    // Validate preset structure
                    if (!preset.segments || !preset.pacers || preset.pacers.length !== 3) {
                        throw new Error('Invalid preset format');
                    }
                    
                    // Apply the preset
                    document.getElementById('segments').value = preset.segments;
                    currentSegments = preset.segments;
                    updateDisplay();
                    updatePositionButtons();
                    
                    preset.pacers.forEach((p, i) => {
                        const idx = i + 1;
                        document.getElementById(`pace${idx}Enable`).checked = p.enabled;
                        document.getElementById(`pace${idx}Range`).value = p.time;
                        document.getElementById(`pace${idx}Value`).textContent = p.time;
                        selectedColors[i] = p.color;
                        selectedPositions[i] = p.position;
                        
                        // Update color palette selection
                        const palette = document.getElementById(`palette${idx}`);
                        palette.querySelectorAll('.color-option').forEach(opt => {
                            opt.classList.remove('selected');
                            if (opt.getAttribute('data-led') === p.color) {
                                opt.classList.add('selected');
                            }
                        });
                    });
                    
                    updatePositionButtons();
                    
                    // Update segments on ESP32
                    fetch('/segments', {
                        method: 'POST',
                        headers: {'Content-Type': 'text/plain'},
                        body: 'SET:' + preset.segments
                    });
                    
                    alert('Preset imported successfully!\n\nYou can now save it to this device if you want.');
                    
                } catch (error) {
                    alert('Invalid preset file\n\n' + error.message);
                }
            };
            reader.readAsText(file);
            
            // Reset file input so same file can be imported again
            event.target.value = '';
        }

        function loadPresetList() {
            fetch('/preset/list')
                .then(response => response.json())
                .then(presets => {
                    const list = document.getElementById('presetList');
                    list.innerHTML = '';
                    
                    if (presets.length === 0) {
                        list.innerHTML = '<div style="text-align: center; color: #666; padding: 20px;">No saved presets</div>';
                        return;
                    }
                    
                    presets.forEach(name => {
                        const item = document.createElement('div');
                        item.className = 'preset-item';
                        item.innerHTML = `
                            <span style="font-weight: 600;">${name}</span>
                            <div>
                                <button class="btn-load" onclick="loadPreset('${name}')">Load</button>
                                <button class="btn-export" onclick="exportPreset('${name}')" style="background: #6366f1; color: white;">Export</button>
                                <button class="btn-delete" onclick="deletePreset('${name}')">Delete</button>
                            </div>
                        `;
                        list.appendChild(item);
                    });
                });
        }

        function updatePositionButtons() {
            currentSegments = parseInt(document.getElementById('segments').value);
            let totalMeters = currentSegments * 5;
            
            const positions = [];
            for (let i = 0; i < 4; i++) {
                const position = Math.round((i / 4) * totalMeters);
                positions.push(position);
            }
            
            for (let pacer = 1; pacer <= 3; pacer++) {
                let container = document.getElementById('position' + pacer);
                container.innerHTML = '';
                
                positions.forEach((position, index) => {
                    let btn = document.createElement('button');
                    btn.className = 'position-btn' + (position === selectedPositions[pacer - 1] ? ' selected' : '');
                    btn.textContent = position + 'm';
                    btn.onclick = function() { selectPosition(pacer, position, this); };
                    container.appendChild(btn);
                });
                
                if (!positions.includes(selectedPositions[pacer - 1])) {
                    selectedPositions[pacer - 1] = positions[0];
                    setTimeout(() => {
                        const buttons = container.querySelectorAll('.position-btn');
                        buttons.forEach((btn, idx) => {
                            if (positions[idx] === selectedPositions[pacer - 1]) {
                                btn.classList.add('selected');
                            }
                        });
                    }, 0);
                }
            }
        }

        function selectColor(pacer, ledColor, element) {
            const parent = element.parentElement;
            parent.querySelectorAll('.color-option').forEach(opt => opt.classList.remove('selected'));
            element.classList.add('selected');
            selectedColors[pacer - 1] = ledColor;
        }

        function selectPosition(pacer, position, element) {
            const parent = element.parentElement;
            parent.querySelectorAll('.position-btn').forEach(btn => btn.classList.remove('selected'));
            element.classList.add('selected');
            selectedPositions[pacer - 1] = position;
        }

        function updateDisplay() {
            let segments = document.getElementById('segments').value;
            document.getElementById('segmentCount').textContent = segments;
            document.getElementById('meterLength').textContent = segments * 5;
        }

        function updateSegments() {
            let segments = document.getElementById('segments').value;
            updateDisplay();
            updatePositionButtons();
            createTrackMarkers();
            
            fetch('/segments', {
                method: 'POST',
                headers: {'Content-Type': 'text/plain'},
                body: 'SET:' + segments
            });
        }
        
        window.onload = function() {
            updateDisplay();
            updatePositionButtons();
            loadPresetList();
            createTrackMarkers();
            
            // Initialize pacer summaries
            for (let i = 1; i <= 3; i++) {
                updatePacerSummary(i);
            }
            
            statusCheckInterval = setInterval(checkStatus, 500);
            
            document.getElementById('segments').oninput = function() {
                updateDisplay();
                updatePositionButtons();
            };

            document.getElementById('pace1Range').oninput = function() {
                document.getElementById('pace1Value').textContent = this.value;
                updatePacerSummary(1);
            };
            document.getElementById('pace2Range').oninput = function() {
                document.getElementById('pace2Value').textContent = this.value;
                updatePacerSummary(2);
            };
            document.getElementById('pace3Range').oninput = function() {
                document.getElementById('pace3Value').textContent = this.value;
                updatePacerSummary(3);
            };
            
            // Recreate markers on window resize
            window.addEventListener('resize', createTrackMarkers);
        };

        document.getElementById('startBtn').onclick = function() {
            isRunning = true;
            updateButtonStates(true);

            let cmd = 'START:';
            
            if (document.getElementById('pace1Enable').checked) {
                let time = document.getElementById('pace1Range').value;
                cmd += time + ',' + selectedPositions[0] + ',' + selectedColors[0] + '|';
            }
            
            if (document.getElementById('pace2Enable').checked) {
                let time = document.getElementById('pace2Range').value;
                cmd += time + ',' + selectedPositions[1] + ',' + selectedColors[1] + '|';
            }
            
            if (document.getElementById('pace3Enable').checked) {
                let time = document.getElementById('pace3Range').value;
                cmd += time + ',' + selectedPositions[2] + ',' + selectedColors[2] + '|';
            }
            
            fetch('/command', {
                method: 'POST',
                headers: {'Content-Type': 'text/plain'},
                body: cmd
            });
        };

        document.getElementById('stopBtn').onclick = function() {
            isRunning = false;
            updateButtonStates(false);
            pacerPositions = [];
            updateTrackVisualization();
            
            fetch('/command', {
                method: 'POST',
                headers: {'Content-Type': 'text/plain'},
                body: 'STOP'
            });
        };
    </script>
</body>
</html>
)rawliteral";

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

void handleRoot() {
  server.send(200, "text/html", HTML_PAGE);
}

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
