# ISL Smart Glove

An ESP32-powered smart glove that recognizes three simple hand gestures and communicates the corresponding phrases through an OLED display and DFPlayer Mini audio module.

## Demo gestures

| Gesture | Spoken phrase | OLED label |
| --- | --- | --- |
| Open hand + shake | “Hi everyone!” | Wave detected |
| Open hand + hand on chest | “Are you all doing well?” | Hand on chest |
| Thumbs up | “I am Consty” | Thumbs up |

The project is designed as an assistive human-computer interface and a starting point for gesture-based communication experiments. Thresholds are calibrated for the glove used in the original build and will normally need adjustment for a different glove, sensor placement, or user.

## Hardware

- ESP32 development board
- Five flex sensors connected to ADC pins 32, 33, 34, 35, and 36
- MPU6050 accelerometer/gyroscope at I2C address `0x68`
- 128x64 SSD1306 I2C OLED
- DFPlayer Mini with a microSD card and speaker
- Glove, voltage dividers, wiring, and a suitable power source

### Pin map

| Component | ESP32 connection |
| --- | --- |
| Thumb flex sensor | GPIO 32 |
| Index flex sensor | GPIO 33 |
| Middle flex sensor | GPIO 34 |
| Ring flex sensor | GPIO 35 |
| Pinky flex sensor | GPIO 36 |
| I2C SDA | GPIO 21 |
| I2C SCL | GPIO 22 |
| DFPlayer RX/TX | GPIO 16 / GPIO 17 |

Use the correct voltage-divider arrangement for each flex sensor. Do not connect a sensor output above the ESP32 ADC voltage limit.

## Audio files

Copy these files to the root of the DFPlayer microSD card:

```text
0001.mp3  Hi everyone
0002.mp3  Are you all doing well?
0003.mp3  I am Consty
```

## Arduino IDE setup

1. Install ESP32 board support in Arduino IDE.
2. Install `U8g2` and `DFRobotDFPlayerMini` from the Library Manager.
3. Open `src/ISL_Smart_Glove.ino`.
4. Select the matching ESP32 board and upload.
5. Open Serial Monitor at `115200` baud to inspect sensor values and gesture detection.

## Calibration

The constants near the top of the sketch contain the original calibration values. Watch the serial output while the glove is relaxed, open, bent, and held in each target pose. Adjust the `*_STR`, `*_BNT`, orientation, and motion thresholds as needed.

The sketch applies a moving filter to the flex readings, requires a hold for the static gestures, and applies a cooldown after triggering a phrase. Wave detection is triggered by a short high-motion window.

## Project structure

```text
.
├── README.md
└── src/
    └── ISL_Smart_Glove.ino
```

## Notes

- This repository contains a reconstructed sketch extracted from the supplied PDF. The PDF text included formatting artifacts, so the code was normalized into a compilable Arduino source file.
- Always test the glove at low volume and confirm the power and wiring before wearing it.
- The project is not a complete sign-language translation system; it currently recognizes three configured gestures.

## License

Choose a license before publishing this project publicly. MIT is a simple default for hardware/software projects if you want others to reuse and modify the work.
