#include <Servo.h>

#define ENA 6      // Motor Kiri PWM
#define ENB 5      // Motor Kanan PWM
#define IN1 9      // Motor Kiri IN1
#define IN2 8      // Motor Kiri IN2  
#define IN3 7      // Motor Kanan IN3
#define IN4 4      // Motor Kanan IN4

// Servo
#define SERVO_DEPAN_PIN 11
#define SERVO_BELAKANG_PIN 10

// Sensor Api
#define FLAME_DEPAN_PIN A0    // KY-026 Analog
#define FLAME_BELAKANG_PIN 2  // IR Digital

// Pompa
#define POMPA_PIN 12

// =================== VARIABEL GLOBAL ===================
Servo servoDepan;
Servo servoBelakang;

// Posisi servo
int posDepan = 0;
int posBelakang = 0;
int directionDepan = 1;
int directionBelakang = 1;

// Parameter sistem
#define SCAN_SPEED 15         // ms per derajat
#define MOTOR_SPEED 180       // 0-255
#define PUMP_TIME 3000        // ms
#define FLAME_THRESHOLD 500   // Untuk sensor analog

// State sistem
bool fireDetected = false;
int firePosition = -1;        // -1 = tidak ada, 0-180 = posisi
bool isFrontFire = true;      // true = depan, false = belakang

// =================== SETUP ===================
void setup() {
  // Initialize motor pins
  pinMode(ENA, OUTPUT);
  pinMode(ENB, OUTPUT);
  pinMode(IN1, OUTPUT);
  pinMode(IN2, OUTPUT);
  pinMode(IN3, OUTPUT);
  pinMode(IN4, OUTPUT);
  
  // Initialize pompa
  pinMode(POMPA_PIN, OUTPUT);
  digitalWrite(POMPA_PIN, LOW);
  
  // Initialize sensor belakang (digital)
  pinMode(FLAME_BELAKANG_PIN, INPUT);
  
  // Attach servos
  servoDepan.attach(SERVO_DEPAN_PIN);
  servoBelakang.attach(SERVO_BELAKANG_PIN);
  
  // LED onboard
  pinMode(LED_BUILTIN, OUTPUT);
  
  // Start position
  servoDepan.write(90);
  servoBelakang.write(90);
  stopMotor();
  
  // Startup sequence
  startupBeep();
  delay(1000);
}

// =================== MAIN LOOP ===================
void loop() {
  if (!fireDetected) {
    // Mode SCAN: Cari api
    scanningMode();
  } else {
    // Mode EXTINGUISH: Padamkan api
    extinguishingMode();
  }
}

// =================== MODE SCANNING ===================
void scanningMode() {
  digitalWrite(LED_BUILTIN, LOW); // LED mati saat scan
  
  // Update servo positions
  posDepan += directionDepan;
  posBelakang += directionBelakang;
  
  // Batasi range 0-180
  posDepan = constrain(posDepan, 0, 180);
  posBelakang = constrain(posBelakang, 0, 180);
  
  // Gerakkan servo
  servoDepan.write(posDepan);
  servoBelakang.write(posBelakang);
  
  // Baca sensor
  checkFireSensors();
  
  // Delay untuk kecepatan scanning
  delay(SCAN_SPEED);
  
  // Balik arah jika mencapai batas
  if (posDepan >= 180 || posDepan <= 0) directionDepan *= -1;
  if (posBelakang >= 180 || posBelakang <= 0) directionBelakang *= -1;
}

// =================== DETEKSI API ===================
void checkFireSensors() {
  // Baca sensor depan (analog)
  int flameDepanValue = analogRead(FLAME_DEPAN_PIN);
  bool apiDepan = (flameDepanValue < FLAME_THRESHOLD);
  
  // Baca sensor belakang (digital)
  int flameBelakangValue = digitalRead(FLAME_BELAKANG_PIN);
  bool apiBelakang = (flameBelakangValue == LOW);
  
  // Prioritas: DEPAN dulu
  if (apiDepan) {
    fireDetected = true;
    firePosition = posDepan;
    isFrontFire = true;
    Serial.println("API TERDETEKSI di DEPAN!");
  } 
  else if (apiBelakang) {
    fireDetected = true;
    firePosition = posBelakang;
    isFrontFire = false;
    Serial.println("API TERDETEKSI di BELAKANG!");
  }
}

// =================== MODE PEMADAMAN ===================
void extinguishingMode() {
  digitalWrite(LED_BUILTIN, HIGH); // LED nyala saat padamkan
  
  // 1. STOP scanning servo
  // Servo tetap di posisi terakhir
  
  // 2. Putar mobil ke arah api
  rotateToFire();
  
  // 3. Beri jeda sebelum pompa
  delay(500);
  
  // 4. Aktifkan pompa
  activatePump();
  
  // 5. Tunggu selesai
  delay(PUMP_TIME);
  
  // 6. Matikan pompa
  deactivatePump();
  
  // 7. Reset untuk scanning ulang
  delay(2000); // Tunggu asap/kondisi stabil
  fireDetected = false;
  firePosition = -1;
  
  // Kembali ke posisi netral
  servoDepan.write(90);
  servoBelakang.write(90);
  posDepan = 90;
  posBelakang = 90;
}

// =================== PUTAR KE ARAH API ===================
void rotateToFire() {
  Serial.print("Memutar ke api di posisi: ");
  Serial.println(firePosition);
  
  if (isFrontFire) {
    // Api di SENSOR DEPAN
    if (firePosition < 60) {
      // Api di KANAN depan -> putar KANAN sedikit
      Serial.println("Api KANAN depan -> Putar KANAN");
      rotateRight();
      delay(400); // Sesuaikan jika perlu
      stopMotor();
    } 
    else if (firePosition > 120) {
      // Api di KIRI depan -> putar KIRI sedikit
      Serial.println("Api KIRI depan -> Putar KIRI");
      rotateLeft();
      delay(400);
      stopMotor();
    } 
    else {
      // Api di TENGAH (60-120) -> sudah menghadap
      Serial.println("Api TENGAH depan -> STOP");
      stopMotor();
    }
  } 
  else {
    // Api di SENSOR BELAKANG
    if (firePosition < 60) {
      // Api di KIRI belakang -> putar KANAN banyak
      Serial.println("Api KIRI belakang -> Putar KANAN 90°");
      rotateRight();
      delay(800); // Putar 90 derajat
      stopMotor();
    } 
    else if (firePosition > 120) {
      // Api di KANAN belakang -> putar KIRI banyak
      Serial.println("Api KANAN belakang -> Putar KIRI 90°");
      rotateLeft();
      delay(800);
      stopMotor();
    } 
    else {
      // Api di TENGAH belakang -> putar 180°
      Serial.println("Api TENGAH belakang -> Putar 180°");
      rotateRight();
      delay(1600); // Putar setengah lingkaran
      stopMotor();
    }
  }
}

// =================== FUNGSI MOTOR ===================
void rotateRight() {
  // Mobil berputar KANAN di tempat
  digitalWrite(IN1, LOW);    // Kiri: MUNDUR
  digitalWrite(IN2, HIGH);
  digitalWrite(IN3, HIGH);   // Kanan: MAJU
  digitalWrite(IN4, LOW);
  analogWrite(ENA, MOTOR_SPEED);
  analogWrite(ENB, MOTOR_SPEED);
}

void rotateLeft() {
  // Mobil berputar KIRI di tempat
  digitalWrite(IN1, HIGH);   // Kiri: MAJU
  digitalWrite(IN2, LOW);
  digitalWrite(IN3, LOW);    // Kanan: MUNDUR
  digitalWrite(IN4, HIGH);
  analogWrite(ENA, MOTOR_SPEED);
  analogWrite(ENB, MOTOR_SPEED);
}

void stopMotor() {
  digitalWrite(IN1, LOW);
  digitalWrite(IN2, LOW);
  digitalWrite(IN3, LOW);
  digitalWrite(IN4, LOW);
  analogWrite(ENA, 0);
  analogWrite(ENB, 0);
}

// =================== FUNGSI POMPA ===================
void activatePump() {
  Serial.println("MENGAAKTIFKAN POMPA!");
  digitalWrite(POMPA_PIN, HIGH);
}

void deactivatePump() {
  Serial.println("Mematikan pompa");
  digitalWrite(POMPA_PIN, LOW);
}

// =================== FUNGSI BANTUAN ===================
void startupBeep() {
  // Indikator startup dengan LED
  for(int i = 0; i < 3; i++) {
    digitalWrite(LED_BUILTIN, HIGH);
    delay(100);
    digitalWrite(LED_BUILTIN, LOW);
    delay(100);
  }
}
