#include <Servo.h>

const int flameSensorPin = A0;
const int servoPin = 2;
const int relayPin = 4;       // Pin relay

Servo myServo;

// Variables
int flameValue;
bool isLocked = false;
bool pumpOn = false;
int lockedAngle = 0;
const int FLAME_THRESHOLD = 400;

// Millis timing
unsigned long previousMoveTime = 0;
unsigned long previousSensorTime = 0;
unsigned long lockStartTime = 0;
unsigned long pumpStartTime = 0;

// Speed control
const unsigned long MOVE_INTERVAL = 30;
const unsigned long SENSOR_INTERVAL = 100;
const unsigned long LOCK_TIME = 10000;
const unsigned long MIN_PUMP_TIME = 3000;

// Scan variables
int currentAngle = 200;
int scanDirection = -1;
int scanStep = 1;

// Variabel BARU untuk track status api
bool flameDetectedNow = false;
bool flameWasDetected = false;

void setup() {
  Serial.begin(9600);
  
  myServo.attach(servoPin);
  pinMode(relayPin, OUTPUT);
  
  // Matikan pompa di awal
  digitalWrite(relayPin, LOW); // LOW = POMPA MATI (sesuaikan dengan relay Anda)
  
  myServo.write(currentAngle);
  
  Serial.println("Fire Detection & Water Pump System");
  Serial.println("===================================");
}

void loop() {
  unsigned long currentMillis = millis();
  
  if (!isLocked) {
    scanMode(currentMillis);
  } else {
    fireFightingMode(currentMillis);
  }
}

void scanMode(unsigned long currentMillis) {
  // Gerakkan servo
  if (currentMillis - previousMoveTime >= MOVE_INTERVAL) {
    previousMoveTime = currentMillis;
    
    if (scanDirection == -1) {
      if (currentAngle > 0) {
        currentAngle -= scanStep;
      } else {
        scanDirection = 1;
      }
    } else {
      if (currentAngle < 200) {
        currentAngle += scanStep;
      } else {
        scanDirection = -1;
      }
    }
    
    myServo.write(currentAngle);
  }
  
  // Baca sensor
  if (currentMillis - previousSensorTime >= SENSOR_INTERVAL) {
    previousSensorTime = currentMillis;
    
    flameValue = analogRead(flameSensorPin);
    
    // Print status
    static unsigned long lastPrint = 0;
    if (currentMillis - lastPrint >= 1000) {
      lastPrint = currentMillis;
      Serial.print("Scanning: ");
      Serial.print(currentAngle);
      Serial.print("° | Flame: ");
      Serial.println(flameValue);
    }
    
    // Deteksi api - PERBAIKAN DI SINI
    if (flameValue < FLAME_THRESHOLD) {
      lockedAngle = currentAngle;
      isLocked = true;
      lockStartTime = currentMillis;
      flameDetectedNow = true;  // API TERDETEKSI
      
      Serial.print("!!! FIRE DETECTED !!! Angle: ");
      Serial.print(lockedAngle);
      Serial.print("° Value: ");
      Serial.println(flameValue);
      
      myServo.write(lockedAngle);
      
      // NYALAKAN POMPA - PERBAIKAN DI SINI
      turnPumpOn();  // Ubah dari turnPumpOff() menjadi turnPumpOn()
    }
  }
}

void fireFightingMode(unsigned long currentMillis) {
  // 1. Lock servo di posisi api
  myServo.write(lockedAngle);
  
  // 2. Baca sensor dan update status api
  if (currentMillis - previousSensorTime >= SENSOR_INTERVAL) {
    previousSensorTime = currentMillis;
    flameValue = analogRead(flameSensorPin);
    
    // PERBAIKAN PENTING: Cek status api saat ini
    flameWasDetected = flameDetectedNow;  // Simpan status sebelumnya
    flameDetectedNow = (flameValue < FLAME_THRESHOLD);  // Update status sekarang
    
    // LOGIKA BARU: Matikan relay saat api hilang
    if (flameWasDetected && !flameDetectedNow) {
      // Api baru saja hilang
      Serial.println("Flame disappeared! Turning pump OFF immediately.");
      turnPumpOff();  // Langsung matikan pompa
    }
    
    // Jika api muncul kembali
    if (!flameWasDetected && flameDetectedNow) {
      Serial.println("Flame reappeared! Turning pump ON.");
      turnPumpOn();  // Nyalakan kembali pompa
    }
  }
  
  // 3. Kontrol pompa - DIUPDATE
  if (pumpOn && flameDetectedNow) {
    // Pompa ON hanya jika ada api
    digitalWrite(relayPin, HIGH); // Pompa ON (sesuaikan dengan relay)
  } else {
    // Pompa OFF jika tidak ada api
    digitalWrite(relayPin, LOW); // Pompa OFF (sesuaikan dengan relay)
  }
  
  // 4. Cek status - MODIFIKASI SEDIKIT
  static unsigned long lastStatus = 0;
  if (currentMillis - lastStatus >= 500) {
    lastStatus = currentMillis;
    
    unsigned long lockDuration = currentMillis - lockStartTime;
    
    Serial.print("FIGHTING FIRE: ");
    Serial.print(lockedAngle);
    Serial.print("° | Flame: ");
    Serial.print(flameValue);
    Serial.print(" | Status: ");
    Serial.print(flameDetectedNow ? "FIRE" : "NO FIRE");
    Serial.print(" | Pump: ");
    Serial.print(pumpOn ? "ON" : "OFF");
    Serial.print(" | Time: ");
    Serial.print(lockDuration / 1000);
    Serial.println("s");
    
    // LOGIKA BARU: Kembali scan jika api hilang cukup lama
    bool fireOut = checkIfFireOut();
    
    if (fireOut || lockDuration >= LOCK_TIME) {
      if (fireOut) {
        Serial.println("Fire extinguished!");
      } else {
        Serial.println("Maximum fight time reached");
      }
      
      // Pastikan pompa mati
      turnPumpOff();
      
      // Kembali ke scan mode
      isLocked = false;
      currentAngle = lockedAngle;
      //scanDirection = (currentAngle > 90) ? -1 : 1;
      flameDetectedNow = false; // Reset status api
      
      Serial.println("Returning to scan mode...");
      delay(1000);
    }
  }
}

void turnPumpOn() {
  if (!pumpOn) {
    pumpOn = true;
    pumpStartTime = millis();
    digitalWrite(relayPin, HIGH); // Pompa ON (sesuaikan dengan relay)
    Serial.println("PUMP: ON - Water spraying!");
  }
}

void turnPumpOff() {
  if (pumpOn) {
    pumpOn = false;
    digitalWrite(relayPin, LOW); // Pompa OFF (sesuaikan dengan relay)
    Serial.println("PUMP: OFF immediately (flame gone)");
  }
}

bool checkIfFireOut() {
  // Api dianggap padam jika nilai sensor tinggi untuk beberapa pembacaan
  static int safeCount = 0;
  const int SAFE_LIMIT = FLAME_THRESHOLD + 100;
  const int REQUIRED_SAFE = 5;
  
  if (flameValue > SAFE_LIMIT && !flameDetectedNow) {
    safeCount++;
    if (safeCount >= REQUIRED_SAFE) {
      safeCount = 0;
      return true;
    }
  } else {
    safeCount = 0;
  }
  
  return false;
}
