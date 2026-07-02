#include <Arduino.h>

// --- PINOUT (ESP32-S3 Mini) ---
// Channel 0: TRIG=GP1 (Touch1),  ECHO=GP7,  LED=GP10
// Channel 1: TRIG=GP2 (Touch2),  ECHO=GP8,  LED=GP11
// Channel 2: TRIG=GP3 (Touch3),  ECHO=GP9,  LED=GP12
// Switch:    GP13
//
// Sensor jack (TRRS): Tip=+5V, Ring1=TRIG, Ring2=ECHO, Sleeve=GND
// Touch wire connects to Ring2 (ECHO pin) via croco cable

const int SWITCH_PIN = 13;
const int NUM_CHANNELS = 3;

const int TRIG_PINS[NUM_CHANNELS] = {1, 2, 3};
const int ECHO_PINS[NUM_CHANNELS] = {7, 8, 9};
const int LED_PINS[NUM_CHANNELS]  = {10, 11, 12};

const int PWM_FREQ = 25000;
const int PWM_RES = 8;

const float MAX_DIST_CM = 60.0;
const int MIN_OUTPUT = 20;
const float SMOOTH = 0.3;

bool useTouch[NUM_CHANNELS] = {false, false, false};
int touchBaseline[NUM_CHANNELS] = {0, 0, 0};
bool touchCalibrated[NUM_CHANNELS] = {false, false, false};
int calibCount[NUM_CHANNELS] = {0, 0, 0};
float smoothed[NUM_CHANNELS] = {0, 0, 0};
bool active = false;
int loopCount = 0;

// Touch pin = ECHO pin (Ring2 on TRRS jack, where croco cable connects)
int touchPin[NUM_CHANNELS] = {7, 8, 9};

float readDistanceCm(int ch) {
  digitalWrite(TRIG_PINS[ch], LOW);
  delayMicroseconds(2);
  digitalWrite(TRIG_PINS[ch], HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PINS[ch], LOW);
  long duration = pulseIn(ECHO_PINS[ch], HIGH, 30000);
  return duration * 0.0343 / 2.0;
}

bool detectUltrasonic(int ch) {
  pinMode(TRIG_PINS[ch], OUTPUT);
  pinMode(ECHO_PINS[ch], INPUT);
  int hits = 0;
  for (int i = 0; i < 5; i++) {
    float d = readDistanceCm(ch);
    if (d >= 2.0 && d <= 400.0) hits++;  // valid HC-SR04 range only
    delay(50);
  }
  return hits >= 4;  // need 4/5 valid readings
}

void blinkAll(int times) {
  for (int i = 0; i < times; i++) {
    for (int ch = 0; ch < NUM_CHANNELS; ch++) ledcWrite(LED_PINS[ch], 255);
    delay(150);
    for (int ch = 0; ch < NUM_CHANNELS; ch++) ledcWrite(LED_PINS[ch], 0);
    delay(150);
  }
}

void setup() {
  Serial.begin(115200);
  delay(2000);
  pinMode(SWITCH_PIN, INPUT_PULLUP);

  Serial.println("=== MANOU startup ===");

  for (int ch = 0; ch < NUM_CHANNELS; ch++) {
    ledcAttach(LED_PINS[ch], PWM_FREQ, PWM_RES);
  }

  Serial.println("Ready.");
}

void loop() {
  bool switchOn = digitalRead(SWITCH_PIN) == LOW;

  if (switchOn && !active) {
    active = true;
    Serial.println("Switch: ON");
    // Re-detect sensor type and reset calibration (same as boot)
    for (int ch = 0; ch < NUM_CHANNELS; ch++) {
      if (detectUltrasonic(ch)) {
        useTouch[ch] = false;
        Serial.printf("CH%d: Ultrasonic (TRIG=%d, ECHO=%d, LED=%d)\n",
                       ch, TRIG_PINS[ch], ECHO_PINS[ch], LED_PINS[ch]);
      } else {
        useTouch[ch] = true;
        pinMode(touchPin[ch], INPUT);
        Serial.printf("CH%d: Touch (GPIO %d, LED=%d) — calibrates at runtime\n",
                       ch, touchPin[ch], LED_PINS[ch]);
      }
      touchCalibrated[ch] = false;
      calibCount[ch] = 0;
      touchBaseline[ch] = 0;
      smoothed[ch] = 0;
    }
    blinkAll(5);
  } else if (!switchOn && active) {
    active = false;
    Serial.println("Switch: OFF");
    for (int ch = 0; ch < NUM_CHANNELS; ch++) ledcWrite(LED_PINS[ch], 0);
  }

  if (!active) {
    delay(50);
    return;
  }

  for (int ch = 0; ch < NUM_CHANNELS; ch++) {
    int brightness = 0;

    if (useTouch[ch]) {
      int val = touchRead(touchPin[ch]);

      // Runtime calibration: use minimum of first 50 reads as baseline
      if (!touchCalibrated[ch]) {
        calibCount[ch]++;
        if (calibCount[ch] <= 10) {
          // skip first 10 reads (warmup)
        } else if (calibCount[ch] <= 50) {
          if (touchBaseline[ch] == 0 || val < touchBaseline[ch])
            touchBaseline[ch] = val;
        } else {
          touchCalibrated[ch] = true;
          Serial.printf("CH%d: Calibrated baseline=%d (min of 40 reads)\n", ch, touchBaseline[ch]);
        }
        ledcWrite(LED_PINS[ch], 0);
        continue;
      }

      // ESP32-S3: touch values INCREASE when touched
      int rise = val - touchBaseline[ch];
      int deadzone = touchBaseline[ch] / 5;
      // Adaptive baseline: drift toward val when idle (below deadzone)
      if (rise < deadzone) {
        touchBaseline[ch] += (val - touchBaseline[ch]) / 10;
      }
      if (rise > deadzone) {
        brightness = map(rise, deadzone, touchBaseline[ch], 0, 255);
        brightness = constrain(brightness, 0, 255);
      }
      // Asymmetric smoothing: ramp up slowly, ramp down faster
      // Slow rise absorbs motor-stop noise; fast fall feels responsive on release
      float alpha = (brightness > smoothed[ch]) ? 0.15 : 0.4;
      smoothed[ch] += alpha * (brightness - smoothed[ch]);
      brightness = (int)(smoothed[ch] + 0.5);
      if (loopCount % 10 == 0)
        Serial.printf("CH%d TOUCH: val=%d base=%d rise=%d dz=%d bright=%d\n",
                       ch, val, touchBaseline[ch], rise, deadzone, brightness);
    } else {
      float dist = readDistanceCm(ch);
      if (dist > 0 && dist <= MAX_DIST_CM) {
        brightness = 255 - (int)(dist / MAX_DIST_CM * 255);
      }
      // smooth ultrasonic
      smoothed[ch] += SMOOTH * (brightness - smoothed[ch]);
      brightness = (int)(smoothed[ch] + 0.5);
      if (brightness < MIN_OUTPUT) brightness = 0;

      if (loopCount % 10 == 0)
        Serial.printf("CH%d ULTRA: dist=%.1f bright=%d\n", ch, dist, brightness);
    }

    ledcWrite(LED_PINS[ch], brightness);
  }

  loopCount++;
  delay(50);
}
