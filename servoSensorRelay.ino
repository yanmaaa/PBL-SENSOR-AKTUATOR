#include <Servo.h>

const int flameSensorPin = A0;
const int servoPin = 2;
const int relayPin = 4;

Servo myServo;

int flameValue;
bool fireDetected = false;
bool pumpActive = false;
int fireAngle = 0;
const int FLAME_THRESHOLD = 400;

int angle = 0;
int sweepDir = 1;
const int SWEEP_MIN = 0;
const int SWEEP_MAX = 180;
unsigned long lastMoveTime = 0;
const unsigned long SWEEP_INTERVAL = 20;

void setup() {
  Serial.begin(9600);
  myServo.attach(servoPin);
  pinMode(relayPin, OUTPUT);
  digitalWrite(relayPin, LOW);
}

void loop() {
  unsigned long now = millis();
  
  if (!fireDetected) {
    if (now - lastMoveTime >= SWEEP_INTERVAL) {
      lastMoveTime = now;
      angle += sweepDir;
    
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
  
    flameValue = analogRead(flameSensorPin);

    if (flameValue < FLAME_THRESHOLD) {
      fireDetected = true;
      fireAngle = angle;
      
      Serial.print("FIRE! Angle: ");
      Serial.print(fireAngle);
      Serial.print("° Value: ");
      Serial.println(flameValue);
      
      digitalWrite(relayPin, HIGH);
      pumpActive = true;
      Serial.println("PUMP: ON");
    }
    
  } else {
    myServo.write(fireAngle);
    flameValue = analogRead(flameSensorPin);

    if (flameValue >= FLAME_THRESHOLD) {
      if (pumpActive) {
        digitalWrite(relayPin, LOW);
        pumpActive = false;
        Serial.println("PUMP: OFF (flame gone)");
      }
    
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
    
        angle = fireAngle;
        sweepDir = (angle > 90) ? -1 : 1;
      }
      
    } else {
      if (!pumpActive) {
        digitalWrite(relayPin, HIGH);
        pumpActive = true;
      }
    }
    
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
