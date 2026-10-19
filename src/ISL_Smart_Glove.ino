#include <Wire.h>
#include <U8g2lib.h>
#include <DFRobotDFPlayerMini.h>
#include <math.h>

U8G2_SSD1306_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0, U8X8_PIN_NONE, 22, 21);
HardwareSerial dfSerial(2);
DFRobotDFPlayerMini df;

const uint8_t flexPins[] = {32, 33, 34, 35, 36};
const int flexStraight[] = {1950, 1960, 1700, 1700, 2465};
const int flexBent[] = {2200, 2160, 2100, 2100, 2900};

const int thumbUpThumbBent = 2350;
const float shakeThreshold = 8000.0f;
const float chestPitchMin = 85.0f;
const float chestPitchMax = 135.0f;
const float chestMotionMax = 3000.0f;
const float thumbUpPitchMin = 40.0f;
const float thumbUpPitchMax = 100.0f;
const float thumbUpRollMin = -90.0f;
const float thumbUpRollMax = -40.0f;

const uint32_t holdMs = 800;
const uint32_t cooldownMs = 4000;
const uint32_t shakeWindowMs = 600;

const char *phraseLine1[] = {"", "Hi everyone!", "Are you all well?", "I am Consty"};
const char *phraseLine2[] = {"", "", "Hand on chest", "Mechatronics Dept"};
const char *phraseLine3[] = {"", "Wave detected", "", "Thumbs up"};

float filteredFlex[] = {1700, 1625, 1500, 1500, 2230};
int candidateGesture = 0;
int confirmedGesture = 0;
uint32_t gestureStart = 0;
uint32_t lastTrigger = 0;
uint32_t shakeStart = 0;
bool shakeActive = false;

int averageAdc(uint8_t pin) {
  long total = 0;
  for (int sample = 0; sample < 32; ++sample) {
    total += analogRead(pin);
    delayMicroseconds(200);
  }
  return static_cast<int>(total / 32);
}

void readMpu(float &roll, float &pitch, float &motion) {
  Wire.beginTransmission(0x68);
  Wire.write(0x3B);
  Wire.endTransmission(false);
  Wire.requestFrom(0x68, 14, true);

  int16_t ax = (Wire.read() << 8) | Wire.read();
  int16_t ay = (Wire.read() << 8) | Wire.read();
  int16_t az = (Wire.read() << 8) | Wire.read();
  Wire.read();
  Wire.read();
  int16_t gx = (Wire.read() << 8) | Wire.read();
  int16_t gy = (Wire.read() << 8) | Wire.read();
  int16_t gz = (Wire.read() << 8) | Wire.read();

  float axG = ax / 16384.0f;
  float ayG = ay / 16384.0f;
  float azG = az / 16384.0f;
  roll = atan2(ayG, azG) * 180.0f / PI;
  pitch = atan2(-axG, azG) * 180.0f / PI;
  motion = sqrt(static_cast<float>(gx) * gx + static_cast<float>(gy) * gy + static_cast<float>(gz) * gz);
}

bool allStraight(const int values[]) {
  for (int finger = 0; finger < 5; ++finger) {
    if (values[finger] >= flexStraight[finger]) return false;
  }
  return true;
}

bool thumbsUp(const int values[]) {
  return values[0] < thumbUpThumbBent &&
         values[1] > flexBent[1] && values[2] > flexBent[2] &&
         values[3] > flexBent[3] && values[4] > flexBent[4];
}

int detectGesture(const int values[], float roll, float pitch, float motion) {
  if (thumbsUp(values) && pitch >= thumbUpPitchMin && pitch <= thumbUpPitchMax &&
      roll >= thumbUpRollMin && roll <= thumbUpRollMax) return 3;
  if (!allStraight(values)) return 0;

  if (motion > shakeThreshold) {
    shakeActive = true;
    shakeStart = millis();
  }
  if (shakeActive) {
    if (millis() - shakeStart < shakeWindowMs) return 1;
    shakeActive = false;
  }
  if (pitch >= chestPitchMin && pitch <= chestPitchMax && motion < chestMotionMax) return 2;
  return 0;
}

void drawCentered(const char *text, int y, const uint8_t *font) {
  u8g2.setFont(font);
  int width = u8g2.getStrWidth(text);
  u8g2.drawStr(max(0, (128 - width) / 2), y, text);
}

void showIdle() {
  u8g2.clearBuffer();
  u8g2.setFont(u8g2_font_ncenB08_tr);
  u8g2.drawStr(0, 12, "ISL Smart Glove");
  u8g2.drawHLine(0, 15, 128);
  u8g2.setFont(u8g2_font_6x10_tr);
  u8g2.drawStr(0, 30, "Waiting...");
  u8g2.drawStr(0, 44, "1: Wave hand");
  u8g2.drawStr(0, 54, "2: Hand on chest");
  u8g2.drawStr(0, 63, "3: Thumbs up");
  u8g2.sendBuffer();
}

void showHolding(int gesture, uint32_t start) {
  u8g2.clearBuffer();
  drawCentered("Hold position...", 12, u8g2_font_ncenB08_tr);
  u8g2.drawHLine(0, 15, 128);
  int progress = constrain(static_cast<int>((millis() - start) * 124.0f / holdMs), 0, 124);
  u8g2.drawFrame(2, 20, 124, 10);
  u8g2.drawBox(2, 20, progress, 10);
  drawCentered(phraseLine1[gesture], 44, u8g2_font_6x10_tr);
  if (strlen(phraseLine2[gesture]) > 0) drawCentered(phraseLine2[gesture], 56, u8g2_font_6x10_tr);
  u8g2.sendBuffer();
}

void showPhrase(int gesture) {
  u8g2.clearBuffer();
  char header[16];
  snprintf(header, sizeof(header), "Phrase %d", gesture);
  u8g2.setFont(u8g2_font_ncenB08_tr);
  u8g2.drawStr(0, 12, header);
  u8g2.drawHLine(0, 15, 128);
  drawCentered(phraseLine1[gesture], 36, strlen(phraseLine1[gesture]) <= 13 ? u8g2_font_ncenB12_tr : u8g2_font_ncenB08_tr);
  if (strlen(phraseLine2[gesture]) > 0) drawCentered(phraseLine2[gesture], 48, u8g2_font_6x10_tr);
  drawCentered(phraseLine3[gesture], 63, u8g2_font_6x10_tr);
  u8g2.sendBuffer();
}

void setup() {
  Serial.begin(115200);
  Wire.begin(21, 22);
  analogReadResolution(12);
  analogSetAttenuation(ADC_11db);

  for (int warmup = 0; warmup < 50; ++warmup) {
    for (uint8_t pin : flexPins) analogRead(pin);
    delay(10);
  }

  Wire.beginTransmission(0x68);
  Wire.write(0x6B);
  Wire.write(0x00);
  Wire.endTransmission(true);

  u8g2.begin();
  u8g2.clearBuffer();
  drawCentered("ISL Smart Glove", 25, u8g2_font_ncenB08_tr);
  drawCentered("Initializing...", 42, u8g2_font_6x10_tr);
  u8g2.sendBuffer();

  dfSerial.begin(9600, SERIAL_8N1, 16, 17);
  delay(500);
  if (!df.begin(dfSerial)) {
    Serial.println("[WARN] DFPlayer not found");
  } else {
    df.volume(28);
    df.EQ(DFPLAYER_EQ_NORMAL);
    df.outputDevice(DFPLAYER_DEVICE_SD);
    Serial.println("[PASS] DFPlayer ready");
  }
  showIdle();
  Serial.println("[READY] ISL Glove ready");
}

void loop() {
  int flexValues[5];
  for (int finger = 0; finger < 5; ++finger) {
    filteredFlex[finger] = 0.85f * filteredFlex[finger] + 0.15f * averageAdc(flexPins[finger]);
    flexValues[finger] = static_cast<int>(filteredFlex[finger]);
  }

  float roll, pitch, motion;
  readMpu(roll, pitch, motion);
  uint32_t now = millis();
  int detected = now - lastTrigger < cooldownMs ? 0 : detectGesture(flexValues, roll, pitch, motion);

  if (detected != 0) {
    if (detected != candidateGesture) {
      candidateGesture = detected;
      gestureStart = now;
    } else if (detected == 1 || now - gestureStart >= holdMs) {
      confirmedGesture = detected;
      lastTrigger = now;
      candidateGesture = 0;
      df.play(confirmedGesture);
      showPhrase(confirmedGesture);
      Serial.printf("[CONFIRMED] Phrase %d: %s\n", confirmedGesture, phraseLine1[confirmedGesture]);
    } else {
      showHolding(detected, gestureStart);
    }
  } else {
    candidateGesture = 0;
    if (confirmedGesture != 0 && now - lastTrigger > cooldownMs) {
      confirmedGesture = 0;
      showIdle();
    }
  }

  Serial.printf("T:%4d I:%4d M:%4d R:%4d P:%4d | R:%6.1f P:%6.1f G:%6.0f | Det:%d\n",
                flexValues[0], flexValues[1], flexValues[2], flexValues[3], flexValues[4], roll, pitch, motion, detected);
  delay(80);
}
