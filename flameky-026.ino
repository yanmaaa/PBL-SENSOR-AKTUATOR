const int analogPin = A0;
const int digitalPin = 2;

int analogValue = 0;
int digitalValue = 0;

void setup() {
  Serial.begin(9600);
  pinMode(analogPin, INPUT);
  pinMode(digitalPin, INPUT);
}

void loop() {
  analogValue = analogRead(analogPin);
  digitalValue = digitalRead(digitalPin);
  
  Serial.print("Analog Value: ");
  Serial.print(analogValue);
  Serial.print(" | Digital Value: ");
  Serial.print(digitalValue);
  
  Serial.print(" | Status: ");
  if (analogValue < 100) {
    Serial.println("API DEKAT/SANGAT KUAT");
  } else if (analogValue < 300) {
    Serial.println("API SEDANG");
  } else if (analogValue < 600) {
    Serial.println("API JAUH/LEMAH");
  } else {
    Serial.println("TIDAK ADA API");
  }
  
  delay(500);
}
