#include <Arduino.h>

// Pinout (ESP32-S3 Mini)
// Sensor jack (TRRS): Tip=+5V, Ring1=TRIG, Ring2=ECHO, Sleeve=GND
// Actor jack (TRRS):  Tip=+5V, Sleeve=OUT-, Ring1/Ring2=unused
// Touch connects to ECHO pin (Ring2) via crocodile cable

const int SWITCH_PIN = 13;
const int NUM_CH = 3;
const int TRIG[] = {1, 2, 3};
const int ECHO[] = {7, 8, 9};
const int ACT[]  = {10, 11, 12};

const float MAX_DIST = 60.0;
const int CALIB_SKIP = 10;
const int CALIB_SAMPLES = 40;

bool isTouch[NUM_CH];
int baseline[NUM_CH];
int calibCount[NUM_CH];
float smoothed[NUM_CH];
bool active = false;

float readDist(int ch) {
  digitalWrite(TRIG[ch], LOW);
  delayMicroseconds(2);
  digitalWrite(TRIG[ch], HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG[ch], LOW);
  return pulseIn(ECHO[ch], HIGH, 30000) * 0.0343 / 2.0;
}

bool detectUltrasonic(int ch) {
  pinMode(TRIG[ch], OUTPUT);
  pinMode(ECHO[ch], INPUT);
  delay(100);
  int hits = 0;
  for (int i = 0; i < 5; i++) {
    float d = readDist(ch);
    Serial.printf("CH%d probe %d: %.1f cm\n", ch, i, d);
    if (d >= 2.0 && d <= 400.0) hits++;
    delay(50);
  }
  return hits >= 3;
}

void blinkAll(int times) {
  for (int i = 0; i < times; i++) {
    for (int ch = 0; ch < NUM_CH; ch++) ledcWrite(ACT[ch], 255);
    delay(150);
    for (int ch = 0; ch < NUM_CH; ch++) ledcWrite(ACT[ch], 0);
    delay(150);
  }
}

void detectAndCalibrate() {
  for (int ch = 0; ch < NUM_CH; ch++) {
    isTouch[ch] = !detectUltrasonic(ch);
    if (isTouch[ch]) pinMode(ECHO[ch], INPUT);
    baseline[ch] = 0;
    calibCount[ch] = 0;
    smoothed[ch] = 0;
    Serial.printf("CH%d: %s\n", ch, isTouch[ch] ? "Touch" : "Ultrasonic");
  }
}

void setup() {
  Serial.begin(115200);
  delay(2000);
  pinMode(SWITCH_PIN, INPUT_PULLUP);
  for (int ch = 0; ch < NUM_CH; ch++)
    ledcAttach(ACT[ch], 25000, 8);
  Serial.println("MANOU ready");
}

void loop() {
  bool switchOn = digitalRead(SWITCH_PIN) == LOW;

  if (switchOn && !active) {
    active = true;
    Serial.println("Switch: ON");
    detectAndCalibrate();
    blinkAll(5);
  } else if (!switchOn && active) {
    active = false;
    Serial.println("Switch: OFF");
    for (int ch = 0; ch < NUM_CH; ch++) ledcWrite(ACT[ch], 0);
  }

  if (!active) { delay(50); return; }

  for (int ch = 0; ch < NUM_CH; ch++) {
    int bright = 0;

    if (isTouch[ch]) {
      int val = touchRead(ECHO[ch]);

      // Calibration: skip warmup, then take min of samples
      if (calibCount[ch] < CALIB_SKIP + CALIB_SAMPLES) {
        calibCount[ch]++;
        if (calibCount[ch] > CALIB_SKIP && (baseline[ch] == 0 || val < baseline[ch]))
          baseline[ch] = val;
        if (calibCount[ch] == CALIB_SKIP + CALIB_SAMPLES)
          Serial.printf("CH%d: baseline=%d\n", ch, baseline[ch]);
        ledcWrite(ACT[ch], 0);
        continue;
      }

      int rise = val - baseline[ch];
      int deadzone = baseline[ch] / 5;

      // Drift baseline when idle
      if (rise < deadzone)
        baseline[ch] += (val - baseline[ch]) / 10;

      if (rise > deadzone)
        bright = constrain(map(rise, deadzone, baseline[ch], 0, 255), 0, 255);

      // Asymmetric smoothing: fast rise, faster fall
      float alpha = (bright > smoothed[ch]) ? 0.5 : 0.7;
      smoothed[ch] += alpha * (bright - smoothed[ch]);
      bright = (int)(smoothed[ch] + 0.5);
    } else {
      float dist = readDist(ch);
      if (dist > 0 && dist <= MAX_DIST)
        bright = 255 - (int)(dist / MAX_DIST * 255);

      smoothed[ch] += 0.6 * (bright - smoothed[ch]);
      bright = (int)(smoothed[ch] + 0.5);
      if (bright < 20) bright = 0;
    }

    ledcWrite(ACT[ch], bright);
  }

  delay(20);
}
