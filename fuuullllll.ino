#include <Servo.h>

const int flameSensorPin = A0;
const int servoPin = 10;
const int relayPin = 12;

// ----------- L298N MOTOR DRIVER (BENAR) ---------------
const int IN1 = 9;   // Motor kiri
const int IN2 = 8;
const int ENA = 6;   // PWM kiri

const int IN3 = 7;   // Motor kanan
const int IN4 = 4;
const int ENB = 5;   // PWM kanan
// -------------------------------------------------------

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
  digitalWrite(relayPin, LOW);

  // Motor pins
  pinMode(IN1, OUTPUT);
  pinMode(IN2, OUTPUT);
  pinMode(ENA, OUTPUT);

  pinMode(IN3, OUTPUT);
  pinMode(IN4, OUTPUT);
  pinMode(ENB, OUTPUT);

  Serial.println("180° Sweep + Motor Control Ready");
}

// ---------------- MOTOR CONTROL ------------------

void stopMotor() {
  analogWrite(ENA, 0);
  analogWrite(ENB, 0);
  digitalWrite(IN1, LOW);
  digitalWrite(IN2, LOW);
  digitalWrite(IN3, LOW);
  digitalWrite(IN4, LOW);
}

// Belok kiri (motor kanan maju, kiri mundur)
void turnLeft(int speed) {
  // Motor kiri mundur
  digitalWrite(IN1, LOW);
  digitalWrite(IN2, HIGH);

  // Motor kanan maju
  digitalWrite(IN3, HIGH);
  digitalWrite(IN4, LOW);

  analogWrite(ENA, speed);
  analogWrite(ENB, speed);
}

// Belok kanan (motor kiri maju, kanan mundur)
void turnRight(int speed) {
  // Motor kiri maju
  digitalWrite(IN1, HIGH);
  digitalWrite(IN2, LOW);

  // Motor kanan mundur
  digitalWrite(IN3, LOW);
  digitalWrite(IN4, HIGH);

  analogWrite(ENA, speed);
  analogWrite(ENB, speed);
}

// ---------------- MAIN LOOP ------------------

void loop() {
  unsigned long now = millis();

  // ---------------------------------------------------------
  // MODE SWEEP - MENCARI API
  // ---------------------------------------------------------
  if (!fireDetected) {

    if (now - lastMoveTime >= SWEEP_INTERVAL) {
      lastMoveTime = now;

      angle += sweepDir;

      if (angle >= SWEEP_MAX) {
        sweepDir = -1;
        angle = SWEEP_MAX;
      } else if (angle <= SWEEP_MIN) {
        sweepDir = 1;
        angle = SWEEP_MIN;
      }

      myServo.write(angle);
    }

    flameValue = analogRead(flameSensorPin);

    if (flameValue < FLAME_THRESHOLD) {
      fireDetected = true;
      fireAngle = angle;

      Serial.print("FIRE DETECTED at angle ");
      Serial.println(fireAngle);

      digitalWrite(relayPin, HIGH);
      pumpActive = true;
    }

  }

  // ---------------------------------------------------------
  // MODE MENGHADAP API
  // ---------------------------------------------------------
  else {

    myServo.write(fireAngle);
    flameValue = analogRead(flameSensorPin);

    // ---------- PROPORTIONAL CONTROL TURNING -------------
    int error = fireAngle - 90;     // 90 = arah depan mobil
    int speed = abs(error) * 2;
    speed = constrain(speed, 80, 200);

    if (abs(error) > 6) {

      if (error > 0) {
        Serial.println("Turning RIGHT...");
        turnLeft(speed);
      } else {
        Serial.println("Turning LEFT...");
        turnRight(speed);
      }

      delay(150);
      stopMotor();
      delay(120);

    } else {
      stopMotor();
      Serial.println("Aligned with fire!");
    }

    // ---------- PUMP CONTROL -------------
    if (flameValue >= FLAME_THRESHOLD) {

      digitalWrite(relayPin, LOW);
      pumpActive = false;

      static unsigned long lostTime = millis();
      if (millis() - lostTime > 2000) {
        fireDetected = false;
        angle = fireAngle;
        sweepDir = (angle > 90) ? -1 : 1;
      }

    } else {
      if (!pumpActive) {
        digitalWrite(relayPin, HIGH);
        pumpActive = true;
      }
    }
  }
}
