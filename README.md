# LED Track Pacing System

An ESP32-based LED pacing system for track and field training at Duke University. This system creates moving LED patterns on a track to help runners maintain consistent pacing.

## Features

- **Up to 3 simultaneous pacers** with independent speeds and colors
- **Scalable track length** - supports 1-80 segments (5-400 meters)
- **Multiple time input modes:**
  - Direct lap time (seconds per lap)
  - Mile pace conversion (MM:SS/mile)
  - Total time calculation (divide total time by number of laps)
- **Live track visualization** in web interface
- **Preset system** - save, load, export, and import configurations
- **WiFi control** - configure via mobile device or laptop
- **Real-time statistics** - elapsed time and fastest lap tracking

## Hardware Requirements

- ESP32 development board
- WS2811 LED strip (RGB color order: RBG)
- 5V power supply (appropriate for LED strip length)
- Data line connected to GPIO pin 15

## Software Requirements

- Arduino IDE or PlatformIO
- Required libraries:
  - `WiFi.h` (ESP32 built-in)
  - `WebServer.h` (ESP32 built-in)
  - `FastLED` ([install from Library Manager](https://github.com/FastLED/FastLED))
  - `Preferences.h` (ESP32 built-in)

## Installation

1. Clone this repository:
   ```bash
   git clone https://github.com/slate7/LED-Track-Pacing-System.git
   cd LED-Track-Pacing-System
   ```

2. Open `TrackPacingSystem.ino` in Arduino IDE

3. Install FastLED library:
   - Arduino IDE: Sketch → Include Library → Manage Libraries → Search "FastLED" → Install

4. Select your ESP32 board:
   - Tools → Board → ESP32 Arduino → (your ESP32 board)

5. Connect your ESP32 via USB and click Upload

## Usage

### First Time Setup

1. Power on the ESP32
2. Connect to WiFi network: **TrackPacer** (password: `pacer2024`)
3. Open browser and navigate to: `http://192.168.4.1`

### Configuring Pacers

1. **Set track length:**
   - Enter number of segments (1 segment = 5 meters)
   - Click APPLY

2. **Configure each pacer:**
   - Toggle pacer ON/OFF with switch
   - Choose time input mode (Lap Time / Mile Pace / Total Time)
   - Set pace using slider or calculator
   - Select color from palette
   - Choose starting position

3. **Start the system:**
   - Click START button
   - LEDs will begin moving around the track
   - Track visualization shows real-time positions

### Saving Presets

1. Configure your pacers as desired
2. Enter a preset name
3. Click "Save"
4. Load saved presets from the preset list

### Exporting/Importing Configurations

- **Export Current**: Downloads current configuration as JSON file
- **Export Preset**: Downloads a saved preset as JSON file
- **Import File**: Upload a previously exported JSON configuration

## Project Structure

```
TrackPacingSystem/
├── TrackPacingSystem.ino    # Main Arduino sketch
├── config.h                  # Configuration constants
├── pacer.h                   # Pacer logic and functions
├── led_control.h             # LED rendering functions
├── web_server.h              # HTTP request handlers
├── web_page.h                # Embedded HTML/CSS/JavaScript interface
├── .gitignore               # Git ignore file
└── README.md                 # This file
```

## Configuration

### WiFi Settings
Edit in `config.h`:
```cpp
const char* AP_SSID = "TrackPacer";
const char* AP_PASSWORD = "pacer2024";
```

### LED Configuration
Edit in `config.h`:
```cpp
#define LED_PIN 15                    // Data pin
#define LOGICAL_UNITS_PER_SEGMENT 50  // LEDs per 5-meter segment
#define LEDS_PER_SEGMENT 20            // Length of pacer in LEDs
#define MAX_PACERS 3                  // Number of simultaneous pacers
```

### LED Strip Type
Edit in `TrackPacingSystem.ino`:
```cpp
FastLED.addLeds<WS2811, LED_PIN, RBG>(leds, MAX_LOGICAL_LEDS);
```

Change `WS2811` to your LED type (`WS2812B`, `APA102`, etc.) and adjust color order (`RGB`, `GRB`, `RBG`) as needed.

## Troubleshooting

### Can't connect to WiFi
- Verify ESP32 is powered on
- Check WiFi network name in your device's WiFi settings
- Try forgetting the network and reconnecting

### LEDs not lighting up
- Check power supply voltage and amperage
- Verify data pin connection (GPIO 15)
- Confirm LED strip type matches code configuration
- Test with a simple FastLED example sketch first

### Web interface not loading
- Confirm you're connected to "TrackPacer" WiFi
- Try `http://192.168.4.1` instead of domain name
- Clear browser cache
- Try a different browser

### Pacers moving incorrectly
- Verify segment count matches your physical track
- Check `LOGICAL_UNITS_PER_SEGMENT` matches your LED density
- Ensure power supply can handle the full LED strip

## License

Open source

## Credits

Developed for Duke University Track & Field

## Contact

For questions or support, please open an issue on GitHub.
