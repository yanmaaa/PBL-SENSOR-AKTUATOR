#include <Servo.h>

#define SERVO_DEPAN_PIN 11
#define SERVO_BELAKANG_PIN 10
#define FLAME_DEPAN_PIN A0
#define FLAME_BELAKANG_PIN 2

Servo servoDepan;
Servo servoBelakang;
int posDepan = 0;
int posBelakang = 0;
int directionDepan = 1;
int directionBelakang = 1;
int scanSpeed = 15;

int flameThreshold = 500;
int apiDetectPosDepan = -1;
int apiDetectPosBelakang = -1;

void setup() {
  Serial.begin(9600);
  servoDepan.attach(SERVO_DEPAN_PIN);
  servoBelakang.attach(SERVO_BELAKANG_PIN);
  
  pinMode(FLAME_BELAKANG_PIN, INPUT);
  
  servoDepan.write(0);
  servoBelakang.write(0);
  delay(1000);
}

void loop() {
  posDepan += directionDepan;
  posBelakang += directionBelakang;
  
  servoDepan.write(posDepan);
  servoBelakang.write(posBelakang);
  
  int flameDepanValue = analogRead(FLAME_DEPAN_PIN);
  int flameBelakangValue = digitalRead(FLAME_BELAKANG_PIN);
  
  bool apiDepan = (flameDepanValue < flameThreshold);
  bool apiBelakang = (flameBelakangValue == LOW);
  
  if (apiDepan && apiDetectPosDepan == -1) {
    apiDetectPosDepan = posDepan;
    Serial.print(">>> API DETECTED DEPAN di posisi: ");
    Serial.println(posDepan);
  }
  
  if (apiBelakang && apiDetectPosBelakang == -1) {
    apiDetectPosBelakang = posBelakang;
    Serial.print(">>> API DETECTED BELAKANG di posisi: ");
    Serial.println(posBelakang);
  }
  
  if (!apiDepan) apiDetectPosDepan = -1;
  if (!apiBelakang) apiDetectPosBelakang = -1;

  delay(scanSpeed);

  if (posDepan >= 180 || posDepan <= 0) directionDepan *= -1;
  if (posBelakang >= 180 || posBelakang <= 0) directionBelakang *= -1;
}
