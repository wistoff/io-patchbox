#include <Arduino.h>

// --- PINOUT ---
// Channel 0: TRIG=D15 (Touch3), ECHO=D16, LED=D18
// Channel 1: TRIG=D4  (Touch0), ECHO=D17, LED=D19
// Channel 2: TRIG=D27 (Touch7), ECHO=D5,  LED=D21

const int SWITCH_PIN = 22;
const int NUM_CHANNELS = 3;

const int TRIG_PINS[NUM_CHANNELS] = {15, 4, 27};  // Touch3, Touch0, Touch7 (D27 on VIN side)
const int ECHO_PINS[NUM_CHANNELS] = {16, 17, 5};
const int LED_PINS[NUM_CHANNELS]  = {18, 19, 21};

const int PWM_FREQ = 5000;
const int PWM_RES = 8;

const float MAX_DIST_CM = 60.0;

bool useTouch[NUM_CHANNELS] = {false, false, false};
int touchBaseline[NUM_CHANNELS] = {0, 0, 0};
bool active = false;

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
  for (int i = 0; i < 5; i++) {
    if (readDistanceCm(ch) > 0) return true;
    delay(50);
  }
  return false;
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
  pinMode(SWITCH_PIN, INPUT_PULLUP);

  for (int ch = 0; ch < NUM_CHANNELS; ch++) {
    ledcAttach(LED_PINS[ch], PWM_FREQ, PWM_RES);

    if (detectUltrasonic(ch)) {
      useTouch[ch] = false;
      Serial.printf("CH%d: Ultrasonic (TRIG=%d, ECHO=%d, LED=%d)\n",
                     ch, TRIG_PINS[ch], ECHO_PINS[ch], LED_PINS[ch]);
    } else {
      useTouch[ch] = true;
      pinMode(TRIG_PINS[ch], INPUT);
      int sum = 0;
      for (int i = 0; i < 10; i++) {
        sum += touchRead(TRIG_PINS[ch]);
        delay(20);
      }
      touchBaseline[ch] = sum / 10;
      Serial.printf("CH%d: Touch (GPIO %d, baseline=%d, LED=%d)\n",
                     ch, TRIG_PINS[ch], touchBaseline[ch], LED_PINS[ch]);
    }
  }
}

void loop() {
  bool switchOn = digitalRead(SWITCH_PIN) == LOW;

  if (switchOn && !active) {
    active = true;
    Serial.println("Switch: ON");
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
      int val = touchRead(TRIG_PINS[ch]);
      int drop = touchBaseline[ch] - val;
      if (drop > 0) {
        brightness = map(drop, 0, touchBaseline[ch] / 2, 0, 255);
        brightness = constrain(brightness, 0, 255);
      }
      Serial.printf("CH%d TOUCH: pin=%d val=%d base=%d drop=%d bright=%d\n",
                     ch, TRIG_PINS[ch], val, touchBaseline[ch], drop, brightness);
    } else {
      float dist = readDistanceCm(ch);
      if (dist > 0 && dist <= MAX_DIST_CM) {
        brightness = 255 - (int)(dist / MAX_DIST_CM * 255);
      }
      Serial.printf("CH%d ULTRA: pin=%d dist=%.1f bright=%d\n",
                     ch, TRIG_PINS[ch], dist, brightness);
    }

    ledcWrite(LED_PINS[ch], brightness);
  }

  delay(100);
}
