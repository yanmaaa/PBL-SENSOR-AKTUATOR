#include <Servo.h>

const int flameSensorPin = A0;
const int servoPin = 2;
const int relayPin = 4;

Servo myServo;

// Variables
int flameValue;
bool fireDetected = false;
bool pumpActive = false;
int fireAngle = 0;
const int FLAME_THRESHOLD = 400;

// Sweep variables
int angle = 0;
int sweepDir = 1; // 1 = 0→180, -1 = 180→0
const int SWEEP_MIN = 0;
const int SWEEP_MAX = 180;
unsigned long lastMoveTime = 0;
const unsigned long SWEEP_INTERVAL = 20;

void setup() {
  Serial.begin(9600);
  myServo.attach(servoPin);
  pinMode(relayPin, OUTPUT);
  digitalWrite(relayPin, LOW); // Pompa MATI
  
  Serial.println("180° Continuous Sweep System");
  Serial.println("=============================");
}

void loop() {
  unsigned long now = millis();
  
  if (!fireDetected) {
    // Mode sweep 180°
    if (now - lastMoveTime >= SWEEP_INTERVAL) {
      lastMoveTime = now;
      
      // Update angle berdasarkan direction
      angle += sweepDir;
      
      // Balik arah di batas
      if (angle >= SWEEP_MAX) {
        sweepDir = -1;
        angle = SWEEP_MAX;
        Serial.println(">>> Reached 180°, sweeping back");
      } else if (angle <= SWEEP_MIN) {
        sweepDir = 1;
        angle = SWEEP_MIN;
        Serial.println(">>> Reached 0°, sweeping forward");
      }
      
      myServo.write(angle);
    }
    
    // Baca sensor
    flameValue = analogRead(flameSensorPin);
    
    // Deteksi api
    if (flameValue < FLAME_THRESHOLD) {
      fireDetected = true;
      fireAngle = angle;
      
      Serial.print("FIRE! Angle: ");
      Serial.print(fireAngle);
      Serial.print("° Value: ");
      Serial.println(flameValue);
      
      // Pompa ON
      digitalWrite(relayPin, HIGH);
      pumpActive = true;
      Serial.println("PUMP: ON");
    }
    
  } else {
    // Mode fire fighting
    // Servo lock di posisi api
    myServo.write(fireAngle);
    
    // Baca sensor
    flameValue = analogRead(flameSensorPin);
    
    // Kontrol pompa: MATI jika api hilang
    if (flameValue >= FLAME_THRESHOLD) {
      if (pumpActive) {
        digitalWrite(relayPin, LOW); // Pompa OFF
        pumpActive = false;
        Serial.println("PUMP: OFF (flame gone)");
      }
      
      // Tunggu 2 detik, jika masih tidak ada api, kembali sweep
      static unsigned long flameLostTime = 0;
      static bool waiting = false;
      
      if (!waiting) {
        flameLostTime = millis();
        waiting = true;
      }
      
      if (millis() - flameLostTime > 2000) {
        fireDetected = false;
        waiting = false;
        Serial.println("Returning to sweep mode...");
        
        // Lanjut sweep dari posisi terakhir
        angle = fireAngle;
        sweepDir = (angle > 90) ? -1 : 1; // Tentukan arah
      }
      
    } else {
      // Api masih ada, pompa tetap ON
      if (!pumpActive) {
        digitalWrite(relayPin, HIGH); // Pompa ON
        pumpActive = true;
      }
    }
    
    // Status update
    static unsigned long lastStatus = 0;
    if (now - lastStatus >= 500) {
      lastStatus = now;
      Serial.print("Locked: ");
      Serial.print(fireAngle);
      Serial.print("° | Flame: ");
      Serial.print(flameValue);
      Serial.print(" | Pump: ");
      Serial.println(pumpActive ? "ON" : "OFF");
    }
  }
}
