#include <Servo.h>

// Pin Definitions (sesuai rencana Anda)
#define SERVO_DEPAN_PIN 11
#define SERVO_BELAKANG_PIN 10
#define FLAME_DEPAN_PIN A0     // KY-026 (Analog)
#define FLAME_BELAKANG_PIN 2   // IR Flame Sensor (Digital)

// Variabel servo
Servo servoDepan;
Servo servoBelakang;

// Posisi servo
int posDepan = 0;
int posBelakang = 0;
int directionDepan = 1;
int directionBelakang = 1;
int scanSpeed = 15; // ms per degree

// Threshold untuk deteksi api
int flameThreshold = 500; // Untuk sensor analog (sesuaikan!)
// Sensor digital: LOW = ada api, HIGH = tidak ada api

// Untuk menyimpan posisi saat detect api
int apiDetectPosDepan = -1;
int apiDetectPosBelakang = -1;

void setup() {
  Serial.begin(9600);
  Serial.println("Fire Detection Test - Servo Scanning Active");
  
  // Initialize servos
  servoDepan.attach(SERVO_DEPAN_PIN);
  servoBelakang.attach(SERVO_BELAKANG_PIN);
  
  // Initialize sensor pins
  pinMode(FLAME_BELAKANG_PIN, INPUT); // Digital input
  
  // Set servos to start position
  servoDepan.write(0);
  servoBelakang.write(0);
  delay(1000);
}

void loop() {
  // Update servo positions
  posDepan += directionDepan;
  posBelakang += directionBelakang;
  
  servoDepan.write(posDepan);
  servoBelakang.write(posBelakang);
  
  // Read sensors
  int flameDepanValue = analogRead(FLAME_DEPAN_PIN);
  int flameBelakangValue = digitalRead(FLAME_BELAKANG_PIN);
  
  // Check for fire detection
  bool apiDepan = (flameDepanValue < flameThreshold); // Analog: nilai kecil = ada api
  bool apiBelakang = (flameBelakangValue == LOW);     // Digital: LOW = ada api
  
  // Display readings
  Serial.print("Depan: Pos=");
  Serial.print(posDepan);
  Serial.print("° Val=");
  Serial.print(flameDepanValue);
  Serial.print(" Api=");
  Serial.print(apiDepan ? "YA" : "TIDAK");
  
  Serial.print(" | Belakang: Pos=");
  Serial.print(posBelakang);
  Serial.print("° Val=");
  Serial.print(flameBelakangValue);
  Serial.print(" Api=");
  Serial.println(apiBelakang ? "YA" : "TIDAK");
  
  // Record detection positions
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
  
  // Reset detection if no fire
  if (!apiDepan) apiDetectPosDepan = -1;
  if (!apiBelakang) apiDetectPosBelakang = -1;
  
  // Delay for scanning speed
  delay(scanSpeed);
  
  // Reverse direction at limits
  if (posDepan >= 180 || posDepan <= 0) directionDepan *= -1;
  if (posBelakang >= 180 || posBelakang <= 0) directionBelakang *= -1;
}