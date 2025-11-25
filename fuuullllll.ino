#include <Servo.h>

// === PIN SETUP ===
int flameDepan = 3;       // KY-026 (HIGH = api)
int flameBelakang = 2;    // IR flame sensor (LOW = api)

int ena = 6;  
int enb = 5;
int in1 = 9;
int in2 = 8;
int in3 = 7;
int in4 = 4;

int pump = 12;

Servo servoDepan;
Servo servoBelakang;

int posDepan = 90;
int posBelakang = 90;

// === MOTOR CONTROL ===
void motorStop() {
  analogWrite(ena, 0);
  analogWrite(enb, 0);
}

void motorKananForward() {  // chasis belok kiri
  digitalWrite(in3, HIGH);
  digitalWrite(in4, LOW);
  analogWrite(enb, 120);
}

void motorKiriForward() {   // chasis belok kanan
  digitalWrite(in1, HIGH);
  digitalWrite(in2, LOW);
  analogWrite(ena, 120);
}

void motorKananBackward() {
  digitalWrite(in3, LOW);
  digitalWrite(in4, HIGH);
  analogWrite(enb, 120);
}

void motorKiriBackward() {
  digitalWrite(in1, LOW);
  digitalWrite(in2, HIGH);
  analogWrite(ena, 120);
}

void kecilBelokKanan() {   // step pendek → mencegah overshoot
  motorKiriForward();
  delay(80);
  motorStop();
}

void kecilBelokKiri() {
  motorKananForward();
  delay(80);
  motorStop();
}

// === SERVO SCAN ===
void scanServo() {
  for (int p = 10; p <= 170; p++) {
    servoDepan.write(p);
    servoBelakang.write(p);
    delay(12);   // servo diperlambat
  }
  for (int p = 170; p >= 10; p--) {
    servoDepan.write(p);
    servoBelakang.write(p);
    delay(12);
  }
}

void setup() {
  servoDepan.attach(11);
  servoBelakang.attach(10);

  pinMode(flameDepan, INPUT);
  pinMode(flameBelakang, INPUT);

  pinMode(ena, OUTPUT);
  pinMode(enb, OUTPUT);
  pinMode(in1, OUTPUT);
  pinMode(in2, OUTPUT);
  pinMode(in3, OUTPUT);
  pinMode(in4, OUTPUT);

  pinMode(pump, OUTPUT);

  motorStop();
  digitalWrite(pump, LOW);
}

void loop() {

  // terus lakukan scanning jika tidak ada api
  scanServo();

  // ===== SENSOR DEPAN =====
  if (digitalRead(flameDepan) == HIGH) {
    int sudut = servoDepan.read();

    while (digitalRead(flameDepan) == HIGH) {
      sudut = servoDepan.read();

      if (sudut < 80) {
        kecilBelokKiri();   // api di kiri → gerakkan motor kanan sedikit
      } 
      else if (sudut > 100) {
        kecilBelokKanan();  // api di kanan → gerakkan motor kiri sedikit
      } 
      else {
        motorStop();
        digitalWrite(pump, HIGH); // pas depan api
        delay(1500);
        digitalWrite(pump, LOW);
        break;
      }
    }
  }

  // ===== SENSOR BELAKANG =====
  if (digitalRead(flameBelakang) == LOW) {  // IR flame LOW = api
    int sudut = servoBelakang.read();

    while (digitalRead(flameBelakang) == LOW) {
      sudut = servoBelakang.read();

      if (sudut < 80) {
        kecilBelokKiri();
      } 
      else if (sudut > 100) {
        kecilBelokKanan();
      } 
      else {
        motorStop();
        digitalWrite(pump, HIGH);
        delay(1500);
        digitalWrite(pump, LOW);
        break;
      }
    }
  }

  motorStop();
}
