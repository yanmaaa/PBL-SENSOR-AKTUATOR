#include <Servo.h>

const int flameSensorPin = A0;
const int servoPin = 10;
const int relayPin = 12;

const int IN1 = 9;
const int IN2 = 8;
const int ENA = 6;
const int IN3 = 7;
const int IN4 = 4;
const int ENB = 5;

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
const unsigned long SWEEP_INTERVAL = 30;

void setup() {
  Serial.begin(9600);
  myServo.attach(servoPin);
  myServo.write(90);

  pinMode(relayPin, OUTPUT);
  digitalWrite(relayPin, LOW);

  pinMode(IN1, OUTPUT);
  pinMode(IN2, OUTPUT);
  pinMode(ENA, OUTPUT);

  pinMode(IN3, OUTPUT);
  pinMode(IN4, OUTPUT);
  pinMode(ENB, OUTPUT);
}

void stopMotor() {
  analogWrite(ENA, 0);
  analogWrite(ENB, 0);
  digitalWrite(IN1, LOW);
  digitalWrite(IN2, LOW);
  digitalWrite(IN3, LOW);
  digitalWrite(IN4, LOW);
}

void turnLeft(int speed) {
  // Motor kiri (IN1, IN2)
  digitalWrite(IN1, LOW); 
  digitalWrite(IN2, HIGH);

  // Motor kanan (IN3, IN4)
  digitalWrite(IN3, HIGH);
  digitalWrite(IN4, LOW);

  analogWrite(ENA, speed);
  analogWrite(ENB, speed);
}

void turnRight(int speed) {
  // Motor kiri (IN1, IN2)
  digitalWrite(IN1, HIGH);
  digitalWrite(IN2, LOW);

  // Motor kanan (IN3, IN4)
  digitalWrite(IN3, LOW);
  digitalWrite(IN4, HIGH);

  analogWrite(ENA, speed);
  analogWrite(ENB, speed);
}

void loop() {
  unsigned long now = millis();
  flameValue = analogRead(flameSensorPin); 

  if (!fireDetected) {
    if (now - lastMoveTime >= SWEEP_INTERVAL) {
      lastMoveTime = now;
      angle += sweepDir;
      if (angle >= SWEEP_MAX) { sweepDir = -1; angle = SWEEP_MAX; }
      else if (angle <= SWEEP_MIN) { sweepDir = 1; angle = SWEEP_MIN; }
      myServo.write(angle);
    }
    if (flameValue < FLAME_THRESHOLD) {
      fireDetected = true;
      fireAngle = angle;
      stopMotor();
      digitalWrite(relayPin, HIGH);
      pumpActive = true;
    }
  }
  else {
    myServo.write(fireAngle);
  
    int error = fireAngle - 90;
    int absErr = abs(error);

    if (absErr > 3) {
      int speed = absErr * 3;
      speed = constrain(speed, 120, 200); 

      if (error > 0) { 
        // fireAngle > 90 (Api di sisi KANAN) -> Robot harus Belok KIRI
        Serial.println("Turning LEFT to align...");
        turnLeft(speed);
      } else { 
        // fireAngle < 90 (Api di sisi KIRI) -> Robot harus Belok KANAN
        Serial.println("Turning RIGHT to align...");
        turnRight(speed);
      }
      delay(50); 
      stopMotor();
    
      int bestAngle = fireAngle;
      int lowestFlameValue = analogRead(flameSensorPin);
      int startAngle = constrain(fireAngle - 15, SWEEP_MIN, SWEEP_MAX);
      int endAngle = constrain(fireAngle + 15, SWEEP_MIN, SWEEP_MAX);

      for (int a = startAngle; a <= endAngle; a += 2) { 
          myServo.write(a);
          delay(10);
          int currentFlameValue = analogRead(flameSensorPin);
          
          if (currentFlameValue < lowestFlameValue && currentFlameValue < FLAME_THRESHOLD) {
              lowestFlameValue = currentFlameValue;
              bestAngle = a;
          }
      }
      
      fireAngle = bestAngle; 
      myServo.write(fireAngle);
      
      delay(50);

    } else {
      stopMotor();
      myServo.write(90);
    }

    myServo.write(90); 
    flameValue = analogRead(flameSensorPin);

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
      static unsigned long lostTime = millis();
      lostTime = millis(); 

      if (!pumpActive) {
        digitalWrite(relayPin, HIGH);
        pumpActive = true;
      }
    }
  }
}
