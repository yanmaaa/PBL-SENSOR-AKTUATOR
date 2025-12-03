#include <Servo.h>

// Pin Configuration
const int flameSensorPin = A0;
const int servoPin = 2;
const int ledPin = 13;      // LED indicator

// Servo Object
Servo myServo;

// Variables
int flameValue;
int bestAngle = 90;
int bestValue = 1023;
bool isSearching = true;
bool isLocked = false;
unsigned long lockStartTime = 0;
const unsigned long LOCK_DURATION = 10000; // Lock selama 10 detik

// Parameters
const int FLAME_THRESHOLD = 400;
const int SCAN_SPEED = 20;  // ms per degree

void setup() {
  Serial.begin(9600);
  
  // Initialize pins
  myServo.attach(servoPin);
  pinMode(ledPin, OUTPUT);
  
  // Start at center
  myServo.write(90);
  
  Serial.println("=== SCAN & LOCK SYSTEM ===");
  Serial.println("Mode: Scan -> Find -> Lock -> Hold");
  Serial.println("==============================");
  delay(1000);
}

void loop() {
  if (isSearching) {
    scanForFlame();
  } else if (isLocked) {
    holdLockPosition();
  }
}

// Fungsi untuk scanning mencari api
void scanForFlame() {
  Serial.println("Scanning for flame...");
  digitalWrite(ledPin, LOW);
  
  bestValue = 1023;  // Reset best value
  bestAngle = 90;
  
  // Scan dari kiri ke kanan
  for (int angle = 0; angle <= 180; angle += 2) {
    myServo.write(angle);
    delay(30); // Tunggu servo stabil
    
    // Baca sensor
    flameValue = analogRead(flameSensorPin);
    
    Serial.print("Angle: ");
    Serial.print(angle);
    Serial.print("° - Value: ");
    Serial.println(flameValue);
    
    // Cek jika ada api
    if (flameValue < FLAME_THRESHOLD && flameValue < bestValue) {
      bestValue = flameValue;
      bestAngle = angle;
    }
    
    delay(SCAN_SPEED);
  }
  
  // Cek apakah api ditemukan
  if (bestValue < FLAME_THRESHOLD) {
    Serial.print("Flame found at ");
    Serial.print(bestAngle);
    Serial.print("° with value ");
    Serial.println(bestValue);
    
    // Lock ke posisi tersebut
    lockToPosition(bestAngle);
  } else {
    Serial.println("No flame detected during scan.");
    delay(1000);
  }
}

// Fungsi untuk lock ke posisi tertentu
void lockToPosition(int angle) {
  Serial.print("LOCKING to ");
  Serial.print(angle);
  Serial.println("°");
  
  // Pindah ke posisi api
  myServo.write(angle);
  delay(500);
  
  // Set status lock
  isSearching = false;
  isLocked = true;
  lockStartTime = millis();
  digitalWrite(ledPin, HIGH);
  
  // Konfirmasi lock
  Serial.println("=== POSITION LOCKED ===");
}

// Fungsi untuk mempertahankan posisi lock
void holdLockPosition() {
  // Baca sensor di posisi terkunci
  flameValue = analogRead(flameSensorPin);
  
  Serial.print("LOCKED at ");
  Serial.print(bestAngle);
  Serial.print("° | Flame: ");
  Serial.print(flameValue);
  
  // Cek kondisi lock
  unsigned long currentTime = millis();
  unsigned long lockTime = currentTime - lockStartTime;
  
  // Cek apakah masih ada api
  if (flameValue < FLAME_THRESHOLD) {
    Serial.print(" | Flame OK");
    
    // Tetap di posisi lock
    myServo.write(bestAngle);
    
    // Cek apakah lock time sudah habis
    if (lockTime >= LOCK_DURATION) {
      Serial.println(" | Lock time expired, rescanning...");
      isLocked = false;
      isSearching = true;
      digitalWrite(ledPin, LOW);
    } else {
      Serial.print(" | Lock time: ");
      Serial.print(LOCK_DURATION - lockTime);
      Serial.println("ms remaining");
    }
    
  } else {
    // Api hilang saat lock
    Serial.println(" | Flame LOST! Rescanning...");
    isLocked = false;
    isSearching = true;
    digitalWrite(ledPin, LOW);
  }
  
  delay(500);
}
