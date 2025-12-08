#include <Servo.h>

Servo mySservo;
int angle = 0;

void setup() {
  myServo.attach(11);
  myServo.write(angle);
}

void loop() {
  for (angle = 0; angle <= 200; angle += 1) {
    myServo.write(angle);
    delay(15);
  }
  for (angle = 200; angle >= 0; angle -= 1) {
    myServo.write(angle);
    delay(15);
  }
}
