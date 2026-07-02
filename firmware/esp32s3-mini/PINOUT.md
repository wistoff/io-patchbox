# ESP32-S3 Mini Pinout

```
                        ┌───────────┐
                        │  USB-C    │
                        ├───────────┤
                  5V  ──┤           ├──  TX
                 GND  ──┤           ├──  RX
            3V3(OUT)  ──┤           ├──  GP13 ← Switch
Sensor 1 TRIG/Touch → GP1  ──┤           ├──  GP12 ← Actor 3
Sensor 2 TRIG/Touch → GP2  ──┤           ├──  GP11 ← Actor 2
Sensor 3 TRIG/Touch → GP3  ──┤           ├──  GP10 ← Actor 1
                 GP4  ──┤           ├──  GP9  ← Sensor 3 ECHO
                 GP5  ──┤           ├──  GP8  ← Sensor 2 ECHO
                 GP6  ──┤           ├──  GP7  ← Sensor 1 ECHO
                        │           ├──  GP16
                        │           ├──  GP15
                        │           ├──  GP14
                        └───────────┘
```

## Channel Assignment

|         | Sensor       | Sensor | Actor  |
|---------|--------------|--------|--------|
|         | TRIG / Touch | ECHO   | LED    |
| **1**   | GP1          | GP7    | GP10   |
| **2**   | GP2          | GP8    | GP11   |
| **3**   | GP3          | GP9    | GP12   |
| **Switch** | -         | -      | GP13   |

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

### Switch
```
Toggle switch between GP13 and GND
```

Each channel auto-detects the sensor type at boot.
LED anode → LED pin (with resistor), cathode → GND.
