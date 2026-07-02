# CLAUDE.md

## Project overview

manou is a finished ESP32-S3 Mini project: 3-channel sensor-to-actor controller with a custom single-sided carrier PCB. The firmware auto-detects ultrasonic vs. touch sensors per channel and drives outputs via PWM.

## Repository layout

- `firmware/esp32s3-mini/` — current PlatformIO project (ESP32-S3 Mini target)
- `firmware/esp32-wroom-dev-board/` — earlier prototype, kept for reference
- `hardware/` — KiCad 10 PCB project (schematic, layout, gerbers, libraries)

## Firmware

- Framework: Arduino on ESP-IDF via PlatformIO
- Board: `lolin_s3_mini` (ESP32-S3, USB CDC)
- Build: `cd firmware/esp32s3-mini && pio run`
- Upload: `pio run -t upload`
- Monitor: `pio device monitor` (115200 baud)

## Hardware / PCB

- KiCad 10 project in `hardware/`
- Single-sided, designed for home etching (toner transfer on F.Cu)
- PJ-320A library (TRRS jacks) is a local copy in `hardware/libs/PJ-320A/`
- The `sym-lib-table` currently has a stale absolute path — update in KiCad's library manager if reopening the project

## Key conventions

- Pin assignments are documented in each firmware target's `PINOUT.md`
- Actor load adaptation (LED resistor values, motor resistors) lives in the device cable, not on the PCB — the board is a generic 5V PWM low-side driver
- Touch sensors use the ECHO pin (Ring2 on the TRRS jack) via crocodile cable
- The switch on GP13 uses internal pull-up; pulling to GND activates
