#include <Servo.h>

Servo servoDepan;   // Servo depan di pin D11
Servo servoBelakang; // Servo belakang di pin D10
int angle = 0;

void setup() {
  // Hubungkan servo ke pin
  servoDepan.attach(11);
  servoBelakang.attach(10);

  // Mulai posisi awal di tengah
  servoDepan.write(angle);
  servoBelakang.write(angle);
}

void loop() {
  // Gerakkan servo dari 0 ke 180 derajat
  for (angle = 0; angle <= 200; angle += 1) {
    servoDepan.write(angle);
    servoBelakang.write(angle);
    delay(15); // Delay menentukan kecepatan gerak
  }

  // Gerakkan servo dari 180 kembali ke 0 derajat
  for (angle = 200; angle >= 0; angle -= 1) {
    servoDepan.write(angle);
    servoBelakang.write(angle);
    delay(15);
  }
}
