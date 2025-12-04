#include <Servo.h>

const int flameSensorPin = A0;
const int servoPin = 2;
Servo myServo;

// Variables
int flameValue;
bool isLocked = false;
int lockedAngle = 0;
const int FLAME_THRESHOLD = 400;

// Millis timing
unsigned long previousMoveTime = 0;
unsigned long previousSensorTime = 0;
unsigned long lockStartTime = 0;

// Speed control
const unsigned long MOVE_INTERVAL = 50;    // Gerak setiap 50ms (LAMBAT - bisa diatur 30-100)
const unsigned long SENSOR_INTERVAL = 100; // Baca sensor setiap 100ms
const unsigned long LOCK_TIME = 10000;     // Lock 10 detik

// Scan variables
int currentAngle = 180;
int scanDirection = -1; // -1 = turun, 1 = naik
int scanStep = 1;       // Derajat per step (lebih kecil = lebih halus)

void setup() {
  Serial.begin(9600);
  myServo.attach(servoPin);
  myServo.write(currentAngle);
  
  Serial.println("=== SLOW SCAN INSTANT LOCK ===");
  Serial.println("Speed: SLOW with millis()");
  Serial.println("=============================");
}

void loop() {
  unsigned long currentMillis = millis();
  
  if (!isLocked) {
    // Mode scanning
    scanMode(currentMillis);
  } else {
    // Mode locked
    lockMode(currentMillis);
  }
}

void scanMode(unsigned long currentMillis) {
  // 1. Gerakkan servo secara periodic (LAMBAT)
  if (currentMillis - previousMoveTime >= MOVE_INTERVAL) {
    previousMoveTime = currentMillis;
    
    // Update posisi berdasarkan direction
    if (scanDirection == -1) {
      if (currentAngle > 0) {
        currentAngle -= scanStep;
      } else {
        scanDirection = 1; // Ganti arah
      }
    } else {
      if (currentAngle < 180) {
        currentAngle += scanStep;
      } else {
        scanDirection = -1; // Ganti arah
      }
    }
    
    myServo.write(currentAngle);
  }
  
  // 2. Baca sensor secara periodic
  if (currentMillis - previousSensorTime >= SENSOR_INTERVAL) {
    previousSensorTime = currentMillis;
    
    flameValue = analogRead(flameSensorPin);
    
    // Print status (tidak terlalu sering)
    static unsigned long lastPrint = 0;
    if (currentMillis - lastPrint >= 1000) {
      lastPrint = currentMillis;
      Serial.print("Scanning: ");
      Serial.print(currentAngle);
      Serial.print("° | Flame: ");
      Serial.println(flameValue);
    }
    
    // Cek api untuk instant lock
    if (flameValue < FLAME_THRESHOLD) {
      lockedAngle = currentAngle;
      isLocked = true;
      lockStartTime = currentMillis;
      
      Serial.print("!!! INSTANT LOCK !!! ");
      Serial.print("Angle: ");
      Serial.print(lockedAngle);
      Serial.print("° | Value: ");
      Serial.println(flameValue);
      
      // Hentikan scanning, langsung lock
      myServo.write(lockedAngle);
    }
  }
}

void lockMode(unsigned long currentMillis) {
  // Pertahankan posisi lock
  myServo.write(lockedAngle);
  
  // Cek sensor secara periodic
  static unsigned long lastLockCheck = 0;
  if (currentMillis - lastLockCheck >= 500) {
    lastLockCheck = currentMillis;
    
    flameValue = analogRead(flameSensorPin);
    
    // Hitung waktu lock
    unsigned long lockDuration = currentMillis - lockStartTime;
    
    Serial.print("Locked: ");
    Serial.print(lockedAngle);
    Serial.print("° | Flame: ");
    Serial.print(flameValue);
    Serial.print(" | Time: ");
    Serial.print(lockDuration / 1000);
    Serial.println("s");
    
    // Cek apakah api masih ada atau waktu lock habis
    if (flameValue >= FLAME_THRESHOLD || lockDuration >= LOCK_TIME) {
      if (flameValue >= FLAME_THRESHOLD) {
        Serial.println("Flame lost, unlocking...");
      } else {
        Serial.println("Lock time expired, unlocking...");
      }
      
      isLocked = false;
      
      // Lanjut scanning dari posisi terakhir
      currentAngle = lockedAngle;
      scanDirection = (currentAngle > 90) ? -1 : 1;
    }
  }
}
