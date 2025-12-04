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
int currentAngle = 180;
int scanDirection = -1;
int scanStep = 1;

void setup() {
  Serial.begin(9600);
  
  myServo.attach(servoPin);
  pinMode(relayPin, OUTPUT);
  
  // Matikan pompa di awal
  digitalWrite(relayPin, LOW); // HIGH = POMPA MATI
  
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
      if (currentAngle < 180) {
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
    
    // Deteksi api
    if (flameValue < FLAME_THRESHOLD) {
      lockedAngle = currentAngle;
      isLocked = true;
      lockStartTime = currentMillis;
      
      Serial.print("!!! FIRE DETECTED !!! Angle: ");
      Serial.print(lockedAngle);
      Serial.print("° Value: ");
      Serial.println(flameValue);
      
      myServo.write(lockedAngle);
      
      // NYALAKAN POMPA
      turnPumpOff();
    }
  }
}

void fireFightingMode(unsigned long currentMillis) {
  // 1. Lock servo di posisi api
  myServo.write(lockedAngle);
  
  // 2. Baca sensor
  if (currentMillis - previousSensorTime >= SENSOR_INTERVAL) {
    previousSensorTime = currentMillis;
    flameValue = analogRead(flameSensorPin);
  }
  
  // 3. Kontrol pompa
  controlPump(currentMillis);
  
  // 4. Cek status
  static unsigned long lastStatus = 0;
  if (currentMillis - lastStatus >= 500) {
    lastStatus = currentMillis;
    
    unsigned long lockDuration = currentMillis - lockStartTime;
    
    Serial.print("FIGHTING FIRE: ");
    Serial.print(lockedAngle);
    Serial.print("° | Flame: ");
    Serial.print(flameValue);
    Serial.print(" | Pump: ");
    Serial.print(pumpOn ? "ON" : "OFF");
    Serial.print(" | Time: ");
    Serial.print(lockDuration / 1000);
    Serial.println("s");
    
    // Cek apakah api sudah padam
    bool fireOut = checkIfFireOut();
    
    if (fireOut || lockDuration >= LOCK_TIME) {
      if (fireOut) {
        Serial.println("Fire extinguished!");
      } else {
        Serial.println("Maximum fight time reached");
      }
      
      // Matikan pompa
      turnPumpOn();
      
      // Kembali ke scan mode
      isLocked = false;
      currentAngle = lockedAngle;
      scanDirection = (currentAngle > 90) ? -1 : 1;
      
      Serial.println("Returning to scan mode...");
      delay(2000);
    }
  }
}

void controlPump(unsigned long currentMillis) {
  if (pumpOn) {
    unsigned long pumpDuration = currentMillis - pumpStartTime;
    
    // Jika api masih ada, pertahankan pompa
    if (flameValue < FLAME_THRESHOLD) {
      digitalWrite(relayPin, LOW); // Pompa ON
    } 
    // Jika api padam tapi pompa belum cukup lama menyala
    else if (pumpDuration < MIN_PUMP_TIME) {
      digitalWrite(relayPin, LOW); // Pompa tetap ON
      Serial.print("Pump minimum time: ");
      Serial.print((MIN_PUMP_TIME - pumpDuration) / 1000);
      Serial.println("s remaining");
    }
    // Jika api padam dan pompa sudah cukup lama
    else {
      turnPumpOff();
    }
  }
}

void turnPumpOn() {
  if (!pumpOn) {
    pumpOn = true;
    pumpStartTime = millis();
    digitalWrite(relayPin, LOW); // Pompa ON
    Serial.println("PUMP: ON - Water spraying!");
  }
}

void turnPumpOff() {
  if (pumpOn) {
    pumpOn = false;
    digitalWrite(relayPin, HIGH); // Pompa OFF
    Serial.println("PUMP: OFF");
  }
}

bool checkIfFireOut() {
  // Api dianggap padam jika nilai sensor tinggi untuk beberapa pembacaan
  static int safeCount = 0;
  const int SAFE_LIMIT = FLAME_THRESHOLD + 100;
  const int REQUIRED_SAFE = 5;
  
  if (flameValue > SAFE_LIMIT) {
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
