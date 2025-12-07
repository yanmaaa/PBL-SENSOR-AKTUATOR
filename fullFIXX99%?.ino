#include <Servo.h>

const int flameSensorPin = A0;
const int servoPin = 10;
const int relayPin = 12;

// ----------- L298N MOTOR DRIVER PINS ---------------
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
const unsigned long SWEEP_INTERVAL = 30;

void setup() {
  Serial.begin(9600);
  myServo.attach(servoPin);
  myServo.write(90); // Posisi awal servo di tengah

  pinMode(relayPin, OUTPUT);
  digitalWrite(relayPin, LOW); // Pastikan aktuator mati saat startup

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
      fireAngle = angle;      // Sudut Deteksi Awal
      stopMotor();            // Pastikan motor berhenti saat deteksi
      Serial.print("FIRE detected at angle: ");
      Serial.println(fireAngle);

      // *** MODIFIKASI: Relai/Pompa DIHILANGKAN dari sini.
      // PumpActive tetap false sampai alignment selesai.
    }
  }

  // ---------------------------------------------------------
  // MODE MENGHADAP API (ALIGNMENT)
  // ---------------------------------------------------------
  else {

    // Servo menunjuk ke sudut api terakhir yang terdeteksi untuk pelacakan
    myServo.write(fireAngle);
    
    // --------- MOTOR ALIGNMENT (Proportional Control) ----------
    int error = fireAngle - 90; // Error = 0 saat api di tengah (90 derajat)
    int absErr = abs(error);

    // Dead Band diperkecil menjadi 4 derajat untuk akurasi lebih tinggi
    if (absErr > 3) { // Robot BELUM LURUS (di luar deadband)

      // Pastikan pompa/aktuator mati saat robot sedang meluruskan diri
      if (pumpActive) {
          digitalWrite(relayPin, LOW);
          pumpActive = false;
      }
      
      // Speed makin besar kalau error makin jauh (Kontrol Proporsional)
      int speed = absErr * 3; // Faktor proporsional (P) = 3
      
      // MODIFIKASI: Batas minimal PWM dinaikkan dari 90 ke 100
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

      // --- PENTING: Update fireAngle setelah bergerak ---
      
      // 1. Gerakkan motor sebentar
      delay(50); 
      stopMotor();
      
      // 2. Lakukan mini-sweep yang lebih FOKUS dan cerdas
      int bestAngle = fireAngle; // Mulai dari sudut terakhir
      int lowestFlameValue = analogRead(flameSensorPin);
      
      // Tentukan range sweep baru: 15 derajat di setiap sisi fireAngle
      int startAngle = constrain(fireAngle - 15, SWEEP_MIN, SWEEP_MAX);
      int endAngle = constrain(fireAngle + 15, SWEEP_MIN, SWEEP_MAX);

      // Iterasi dengan resolusi 2 derajat
      for (int a = startAngle; a <= endAngle; a += 2) { 
          myServo.write(a);
          delay(10); // Tunggu servo bergerak
          int currentFlameValue = analogRead(flameSensorPin);
          
          // Cek jika sinyal lebih kuat (nilai lebih rendah) DAN masih di bawah Threshold
          if (currentFlameValue < lowestFlameValue && currentFlameValue < FLAME_THRESHOLD) {
              lowestFlameValue = currentFlameValue;
              bestAngle = a;
          }
      }
      
      // 3. Update fireAngle dan kembalikan servo ke posisi api terbaru
      fireAngle = bestAngle; 
      myServo.write(fireAngle);
      
      delay(50); // Jeda singkat

    } else {
      // Robot SUDAH LURUS (di dalam deadband)

      stopMotor();
      myServo.write(90); // Kunci servo di tengah saat sudah lurus (deadband)
      Serial.println("Aligned! (inside deadband) - PUMP ACTIVATION");

      // *** MODIFIKASI: AKTIFKAN RELAY HANYA DI SINI ***
      if (!pumpActive) {
          digitalWrite(relayPin, HIGH);
          pumpActive = true;
          Serial.println("Aktuator (Pompa) ON.");
      }
    }

    // ---------- PUMP & LOSS CONTROL -------------
    
    // Gunakan posisi 90 derajat untuk pembacaan kekuatan api terbaik
    myServo.write(90); 
    flameValue = analogRead(flameSensorPin);

    if (flameValue >= FLAME_THRESHOLD) { // Api HILANG
      digitalWrite(relayPin, LOW); // Matikan pompa
      pumpActive = false;

      // Logika kehilangan api (tunggu 2 detik sebelum sweep ulang)
      static unsigned long lostTime = millis();
      if (millis() - lostTime > 2000) {
        fireDetected = false;
        angle = fireAngle; // Mulai sweep dari sudut terakhir
        sweepDir = (angle > 90) ? -1 : 1;
        Serial.println("Fire LOST. Returning to Sweep Mode.");
      }
    } else { // Api ADA (Reset Timer Kehilangan)
      static unsigned long lostTime = millis();
      lostTime = millis(); 
    }
  }
}
