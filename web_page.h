#ifndef WEB_PAGE_H
#define WEB_PAGE_H

// Embedded HTML/CSS/JavaScript for the web interface
// This file contains the complete user interface served to connected clients

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

    <script src="web/script.js"></script>
</body>
</html>
)rawliteral";

#endif
