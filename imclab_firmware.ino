/*
  iMCLab Internet-Based Motor Control Lab
  Firmware FIXED: LED + Motor OP + (Optional) RPM
  December 2025
*/

#include <Arduino.h>

// ===== constants =====
const String vers = "1.1";
const int baud = 115200;
const char sp = ' ';
const char nl = '\n';

// ===== pin numbers on iMCLab Shield (sesuai kode kamu) =====
const int motor1Pin1 = 27;   // IN1
const int motor1Pin2 = 26;   // IN2
const int enable1Pin = 12;   // EN (PWM motor)
const int pinLED     = 2;    // LED

// Encoder pin (RPM)
const int pin_rpm = 13;      // input encoder

// ===== PWM properties =====
const int freq = 30000;
const int pwmChannel = 1;
const int pwmresolution = 8;   // 0..255

const int ledChannel = 0;
const int ledresolution = 8;   // 0..255

// ===== RPM variables =====
volatile unsigned long pulseCount = 0;
unsigned long last_pulse_count = 0;
unsigned long last_rpm_time = 0;

float rpm = 0;
float rpm_filtered = 0;

// Ubah ini kalau disk encoder kamu beda
// Contoh dosen: holes = 2 (2 lubang/slot per putaran)
const int PULSES_PER_REV = 2;

// ===== serial parsing variables =====
char Buffer[64];
String cmd;
double pv = 0;

// ===== ISR encoder =====
void IRAM_ATTR onPulse() {
  pulseCount++;
}

void parseSerial() {
  // baca sampai newline
  int ByteCount = Serial.readBytesUntil(nl, Buffer, sizeof(Buffer));
  (void)ByteCount;

  String read_ = String(Buffer);
  memset(Buffer, 0, sizeof(Buffer));

  // pisahkan command dan data
  int idx = read_.indexOf(sp);
  if (idx < 0) {
    cmd = read_;
    pv = 0;
  } else {
    cmd = read_.substring(0, idx);
    String data = read_.substring(idx + 1);
    data.trim();
    pv = data.toFloat();
  }

  cmd.trim();
  cmd.toUpperCase();
}

void calculateRPM() {
  unsigned long now = millis();
  unsigned long dt = now - last_rpm_time;

  if (dt >= 200) {  // update tiap 200ms biar responsif
    noInterrupts();
    unsigned long p = pulseCount;
    interrupts();

    unsigned long dp = p - last_pulse_count;

    float rev = (float)dp / (float)PULSES_PER_REV;
    float rps = rev / (dt / 1000.0f);
    rpm = rps * 60.0f;

    // low-pass filter sederhana
    rpm_filtered = 0.7f * rpm_filtered + 0.3f * rpm;

    last_pulse_count = p;
    last_rpm_time = now;
  }
}

void setMotorPercent(float percent) {
  percent = constrain(percent, 0.0f, 100.0f);

  // arah default forward
  digitalWrite(motor1Pin1, HIGH);
  digitalWrite(motor1Pin2, LOW);

  int duty = (int)(percent * 255.0 / 100.0);
  duty = constrain(duty, 0, 255);

  ledcWrite(pwmChannel, duty);
}

void setLEDPercent(float percent) {
  percent = constrain(percent, 0.0f, 100.0f);

  int duty = (int)(percent * 255.0 / 100.0);
  duty = constrain(duty, 0, 255);

  ledcWrite(ledChannel, duty);
}

void dispatchCommand() {
  if (cmd == "OP") {
    // motor PWM 0..100%
    setMotorPercent((float)pv);
    Serial.println((float)pv);
  }
  else if (cmd == "RPM") {
    calculateRPM();
    Serial.println(rpm_filtered);
  }
  else if (cmd == "V" || cmd == "VER") {
    Serial.println("iMCLab Firmware Version " + vers);
  }
  else if (cmd == "LED") {
    setLEDPercent((float)pv);
    Serial.println((float)pv);
  }
  else if (cmd == "X") {
    ledcWrite(pwmChannel, 0);
    Serial.println("Stop");
  }
  else {
    Serial.println("ERR");
  }
}

// ===== arduino startup =====
void setup() {
  Serial.begin(baud);
  delay(1500);
  Serial.println("READY");

  // motor direction pins
  pinMode(motor1Pin1, OUTPUT);
  pinMode(motor1Pin2, OUTPUT);
  digitalWrite(motor1Pin1, LOW);
  digitalWrite(motor1Pin2, LOW);

  // motor PWM setup (PENTING: attach ke enable1Pin, bukan pin_rpm)
  ledcSetup(pwmChannel, freq, pwmresolution);
  ledcAttachPin(enable1Pin, pwmChannel);
  ledcWrite(pwmChannel, 0);

  // LED PWM setup
  ledcSetup(ledChannel, freq, ledresolution);
  ledcAttachPin(pinLED, ledChannel);
  ledcWrite(ledChannel, 0);

  // encoder interrupt
  pinMode(pin_rpm, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(pin_rpm), onPulse, RISING);

  last_rpm_time = millis();
}

// ===== main loop =====
void loop() {
  // update RPM in background
  calculateRPM();

  // handle serial command (blocking readBytesUntil will wait timeout if no data,
  // jadi lebih aman cek dulu tersedia)
  if (Serial.available()) {
    parseSerial();
    dispatchCommand();
  }
}
