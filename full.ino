#include <Servo.h>

// =================== PIN DEFINITIONS ===================
#define ENA 6
#define ENB 5
#define IN1 9
#define IN2 8
#define IN3 7
#define IN4 4

// Servo untuk scanning sensor
#define SERVO_DEPAN_PIN 11
#define SERVO_BELAKANG_PIN 10

// Sensor Api
#define FLAME_DEPAN_PIN A0
#define FLAME_BELAKANG_PIN 2

// Pompa
#define POMPA_PIN 12

// =================== VARIABEL GLOBAL ===================
Servo servoDepan;
Servo servoBelakang;

int posDepan = 90;
int posBelakang = 90;
int directionDepan = 1;
int directionBelakang = 1;

// Parameter
#define SCAN_SPEED 30  // Lebih lambat untuk stabil
#define MOTOR_SPEED 150 // Lebih lambat untuk kontrol
#define PUMP_TIME 3000
#define FLAME_THRESHOLD 500

// State
bool fireDetected = false;
int fireSensorAngle = -1;
bool isFrontFire = true;

// Untuk debouncing deteksi api
unsigned long lastDetectionTime = 0;
#define DEBOUNCE_TIME 500 // ms

// =================== SETUP ===================
void setup() {
  Serial.begin(9600);
  
  // Motor pins
  pinMode(ENA, OUTPUT);
  pinMode(ENB, OUTPUT);
  pinMode(IN1, OUTPUT);
  pinMode(IN2, OUTPUT);
  pinMode(IN3, OUTPUT);
  pinMode(IN4, OUTPUT);
  
  // Pompa
  pinMode(POMPA_PIN, OUTPUT);
  digitalWrite(POMPA_PIN, LOW);
  
  // Sensor
  pinMode(FLAME_BELAKANG_PIN, INPUT);
  
  // Servo
  servoDepan.attach(SERVO_DEPAN_PIN);
  servoBelakang.attach(SERVO_BELAKANG_PIN);
  
  // LED
  pinMode(LED_BUILTIN, OUTPUT);
  
  // Posisi awal
  servoDepan.write(90);
  servoBelakang.write(90);
  stopMotor();
  
  // Startup
  startupBeep();
  Serial.println("System Ready - Simple Mode");
  delay(1000);
}

// =================== MAIN LOOP ===================
void loop() {
  if (!fireDetected) {
    scanningMode();
  } else {
    simpleExtinguishingMode(); // Mode sederhana
  }
}

// =================== SCANNING MODE ===================
void scanningMode() {
  digitalWrite(LED_BUILTIN, LOW);
  
  // Gerakkan servo scanning
  posDepan += directionDepan;
  posBelakang += directionBelakang;
  
  posDepan = constrain(posDepan, 0, 180);
  posBelakang = constrain(posBelakang, 0, 180);
  
  servoDepan.write(posDepan);
  servoBelakang.write(posBelakang);
  
  // Cek api dengan debounce
  if (millis() - lastDetectionTime > DEBOUNCE_TIME) {
    checkFireSensors();
  }
  
  delay(SCAN_SPEED);
  
  // Balik arah
  if (posDepan >= 180 || posDepan <= 0) directionDepan *= -1;
  if (posBelakang >= 180 || posBelakang <= 0) directionBelakang *= -1;
}

// =================== DETEKSI API ===================
void checkFireSensors() {
  int flameDepanValue = analogRead(FLAME_DEPAN_PIN);
  bool apiDepan = (flameDepanValue < FLAME_THRESHOLD);
  
  int flameBelakangValue = digitalRead(FLAME_BELAKANG_PIN);
  bool apiBelakang = (flameBelakangValue == LOW);
  
  if (apiDepan) {
    fireDetected = true;
    fireSensorAngle = posDepan;
    isFrontFire = true;
    lastDetectionTime = millis();
    Serial.print("Api DEPAN! Angle: ");
    Serial.print(fireSensorAngle);
    Serial.print(", Sensor value: ");
    Serial.println(flameDepanValue);
  } 
  else if (apiBelakang) {
    fireDetected = true;
    fireSensorAngle = posBelakang;
    isFrontFire = false;
    lastDetectionTime = millis();
    Serial.print("Api BELAKANG! Angle: ");
    Serial.println(fireSensorAngle);
  }
}

// =================== MODE PEMADAMAN SEDERHANA ===================
void simpleExtinguishingMode() {
  digitalWrite(LED_BUILTIN, HIGH);
  Serial.println("=== MULAI PEMADAMAN SEDERHANA ===");
  
  // 1. STOP semua gerakan
  stopMotor();
  delay(500);
  
  // 2. Tentukan arah putaran SEDERHANA
  if (isFrontFire) {
    // API di DEPAN
    if (fireSensorAngle < 70) {
      // Api di KANAN (0-70°)
      Serial.println("Api KANAN -> Putar KANAN 400ms");
      rotateRight();
      delay(400);
      stopMotor();
    } 
    else if (fireSensorAngle > 110) {
      // Api di KIRI (110-180°)
      Serial.println("Api KIRI -> Putar KIRI 400ms");
      rotateLeft();
      delay(400);
      stopMotor();
    } 
    else {
      // Api di TENGAH (70-110°)
      Serial.println("Api TENGAH -> Tidak perlu putar");
    }
  } 
  else {
    // API di BELAKANG
    Serial.println("Api BELAKANG -> Putar 180°");
    rotateRight();
    delay(1600); // Putar 180°
    stopMotor();
  }
  
  // 3. Tunggu sebentar
  delay(1000);
  
  // 4. NYALAKAN POMPA
  Serial.println("MENYALAKAN POMPA");
  digitalWrite(POMPA_PIN, HIGH);
  delay(PUMP_TIME);
  
  // 5. MATIKAN POMPA
  Serial.println("MEMATIKAN POMPA");
  digitalWrite(POMPA_PIN, LOW);
  
  // 6. RESET untuk scanning ulang
  delay(2000);
  fireDetected = false;
  fireSensorAngle = -1;
  
  // Kembali ke posisi tengah
  servoDepan.write(90);
  servoBelakang.write(90);
  posDepan = 90;
  posBelakang = 90;
  
  Serial.println("=== SELESAI ===");
  digitalWrite(LED_BUILTIN, LOW);
  delay(1000); // Jeda sebelum scan ulang
}

// =================== FUNGSI MOTOR SEDERHANA ===================
void rotateRight() {
  Serial.println("Motor: PUTAR KANAN");
  digitalWrite(IN1, LOW);
  digitalWrite(IN2, HIGH);
  digitalWrite(IN3, HIGH);
  digitalWrite(IN4, LOW);
  analogWrite(ENA, MOTOR_SPEED);
  analogWrite(ENB, MOTOR_SPEED);
}

void rotateLeft() {
  Serial.println("Motor: PUTAR KIRI");
  digitalWrite(IN1, HIGH);
  digitalWrite(IN2, LOW);
  digitalWrite(IN3, LOW);
  digitalWrite(IN4, HIGH);
  analogWrite(ENA, MOTOR_SPEED);
  analogWrite(ENB, MOTOR_SPEED);
}

void stopMotor() {
  Serial.println("Motor: STOP");
  digitalWrite(IN1, LOW);
  digitalWrite(IN2, LOW);
  digitalWrite(IN3, LOW);
  digitalWrite(IN4, LOW);
  analogWrite(ENA, 0);
  analogWrite(ENB, 0);
}

// =================== FUNGSI BANTUAN ===================
void startupBeep() {
  for(int i = 0; i < 3; i++) {
    digitalWrite(LED_BUILTIN, HIGH);
    delay(100);
    digitalWrite(LED_BUILTIN, LOW);
    delay(100);
  }
}
