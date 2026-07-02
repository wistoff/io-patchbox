# ESP32 WROOM DevKit Pinout

```
                          ┌───────────┐
                          │  USB/UART │
                          ├───────────┤
                 3.3V  ──┤           ├──  VIN
                  GND  ──┤           ├──  GND
Sensor 1 TRIG / Touch →  D15  ──┤           ├──  D13
                          D2   ──┤           ├──  D12
Sensor 2 TRIG / Touch →  D4   ──┤           ├──  D14
    Sensor 1 ECHO  →  D16  ──┤           ├──  D27  ← Sensor 3 TRIG / Touch
    Sensor 2 ECHO  →  D17  ──┤           ├──  D26
    Sensor 3 ECHO  →  D5   ──┤           ├──  D25
        Actor 1  →  D18  ──┤           ├──  D33
        Actor 2  →  D19  ──┤           ├──  D32
        Actor 3  →  D21  ──┤           ├──  D35
              D3 (RX)  ──┤           ├──  D34
              D1 (TX)  ──┤           ├──  D39
      Switch →  D22  ──┤           ├──  D36
                  D23  ──┤           ├──  EN
                          └───────────┘
```

## Channel Assignment

|         | Sensor       | Sensor | Actor  |
|---------|--------------|--------|--------|
|         | TRIG / Touch | ECHO   | LED    |
| **1**   | D15 (GPIO 15)| D16    | D18    |
| **2**   | D4  (GPIO 4) | D17    | D19    |
| **3**   | D27 (GPIO 27)| D5     | D21    |

## Wiring per Channel

### Ultrasonic (HC-SR04)
```
HC-SR04 VCC  → 5V
HC-SR04 GND  → GND
HC-SR04 TRIG → TRIG pin
HC-SR04 ECHO → ECHO pin
```

### Touch (wire/cable)
```
Wire → TRIG pin (no other connections needed)
```

Each channel auto-detects the sensor type at boot.
LED anode → LED pin (with resistor), cathode → GND.
