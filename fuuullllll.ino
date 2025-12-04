#include <Servo.h>

const int flameSensorPin = A0;
const int servoPin = 10;
const int relayPin = 12;

// ----------- L298N MOTOR DRIVER ---------------
const int IN1 = 9;    // Motor kiri
const int IN2 = 8;
const int ENA = 6;    // PWM kiri

const int IN3 = 7;    // Motor kanan
const int IN4 = 4;
const int ENB = 5;    // PWM kanan
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
int sweepDir = 1;
const int SWEEP_MIN = 0;
const int SWEEP_MAX = 180;
unsigned long lastMoveTime = 0;
const unsigned long SWEEP_INTERVAL = 20;

void setup() {
  Serial.begin(9600);
  myServo.attach(servoPin);
  myServo.write(90); // Posisi awal servo di tengah

  pinMode(relayPin, OUTPUT);
  digitalWrite(relayPin, LOW);

  pinMode(IN1, OUTPUT);
  pinMode(IN2, OUTPUT);
  pinMode(ENA, OUTPUT);

  pinMode(IN3, OUTPUT);
  pinMode(IN4, OUTPUT);
  pinMode(ENB, OUTPUT);

  Serial.println("System Ready");
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

// Belok kiri (Roda kiri mundur, Roda kanan maju) - Robot berputar ke KIRI
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

// Belok kanan (Roda kiri maju, Roda kanan mundur) - Robot berputar ke KANAN
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


// ---------------- MAIN LOOP ------------------

void loop() {
  unsigned long now = millis();
  
  // Membaca nilai sensor api (di luar mode sweep/fire detected)
  flameValue = analogRead(flameSensorPin); 

  // ---------------------------------------------------------
  // MODE SWEEP - MENCARI API
  // ---------------------------------------------------------
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
      fireAngle = angle;      // LOCK ANGLE !! (Sudut Deteksi Awal)
      stopMotor();            // Pastikan motor berhenti saat deteksi
      Serial.print("FIRE detected at angle: ");
      Serial.println(fireAngle);

      digitalWrite(relayPin, HIGH);
      pumpActive = true;
    }
  }

  // ---------------------------------------------------------
  // MODE MENGHADAP API (ALIGNMENT)
  // ---------------------------------------------------------
  else {

    // Servo menunjuk ke sudut api terakhir yang terdeteksi untuk pelacakan
    myServo.write(fireAngle);
    
    // --------- MOTOR ALIGNMENT (Proportional Control) ----------
    // Tujuan: Membuat fireAngle berada di 90 derajat
    int error = fireAngle - 90; // Error = 0 saat api di tengah (90 derajat)
    int absErr = abs(error);

    if (absErr > 6) { // DEAD BAND 6° (Jika error lebih dari 6 derajat, lakukan pelurusan)

      // Speed makin besar kalau error makin jauh (Kontrol Proporsional)
      int speed = absErr * 3; // Faktor proporsional (P) = 3
      speed = constrain(speed, 90, 200); // Batasi kecepatan (min 90, max 200)

      if (error > 0) { 
        // fireAngle > 90 (Api di sisi KANAN) -> Robot harus Belok KIRI
        Serial.println("Turning LEFT to align...");
        turnLeft(speed);
      } else { 
        // fireAngle < 90 (Api di sisi KIRI) -> Robot harus Belok KANAN
        Serial.println("Turning RIGHT to align...");
        turnRight(speed);
      }

      // --- PENTING: Update fireAngle setelah bergerak ---
      
      // 1. Gerakkan motor sebentar
      delay(50); 
      stopMotor();
      
      // 2. Lakukan mini-sweep untuk menemukan sudut api yang BARU
      int bestAngle = 90;
      int lowestFlameValue = 1024;
      
      // Sweep kecil di sekitar sudut 90 (misalnya 60 sampai 120 derajat)
      for (int a = 60; a <= 120; a += 5) { 
          myServo.write(a);
          delay(10); // Tunggu servo bergerak
          int currentFlameValue = analogRead(flameSensorPin);
          if (currentFlameValue < lowestFlameValue) {
              lowestFlameValue = currentFlameValue;
              bestAngle = a;
          }
      }
      
      // 3. Update fireAngle dan kembalikan servo ke posisi api terbaru
      fireAngle = bestAngle; 
      myServo.write(fireAngle);
      
      delay(50); // Jeda singkat
      // 4. Loop akan berulang, motor akan bergerak lagi berdasarkan fireAngle yang baru

    } else {
      stopMotor();
      myServo.write(90); // Kunci servo di tengah saat sudah lurus (dalam deadband)
      Serial.println("Aligned! (inside deadband)");
    }

    // ---------- PUMP & LOSS CONTROL -------------
    
    // Gunakan posisi 90 derajat untuk pembacaan kekuatan api terbaik
    myServo.write(90); 
    flameValue = analogRead(flameSensorPin);

    if (flameValue >= FLAME_THRESHOLD) { // Api HILANG
      digitalWrite(relayPin, LOW);
      pumpActive = false;

      // Logika kehilangan api (tunggu 2 detik sebelum sweep ulang)
      static unsigned long lostTime = millis();
      if (millis() - lostTime > 2000) {
        fireDetected = false;
        angle = fireAngle; // Mulai sweep dari sudut terakhir
        sweepDir = (angle > 90) ? -1 : 1;
      }
    } else { // Api ADA
      static unsigned long lostTime = millis(); // Reset waktu kehilangan
      if (!pumpActive) {
        digitalWrite(relayPin, HIGH);
        pumpActive = true;
      }
    }
  }
}
