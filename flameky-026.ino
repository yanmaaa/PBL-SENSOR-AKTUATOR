// Pin sensor KY-026
const int analogPin = A0;   // Pin AO sensor (output analog)
const int digitalPin = 2;   // Pin DO sensor (output digital) - opsional

// Variabel pembacaan
int analogValue = 0;
int digitalValue = 0;

void setup() {
  Serial.begin(9600);          // Inisialisasi Serial Monitor
  pinMode(analogPin, INPUT);   // Set pin analog sebagai input
  pinMode(digitalPin, INPUT);  // Set pin digital sebagai input
  
  Serial.println("Flame Sensor KY-026 Test");
  Serial.println("=========================");
  delay(1000);
}

void loop() {
  // Baca nilai dari sensor
  analogValue = analogRead(analogPin);   // Baca nilai analog (0-1023)
  digitalValue = digitalRead(digitalPin); // Baca nilai digital (0/1)
  
  // Tampilkan di Serial Monitor
  Serial.print("Analog Value: ");
  Serial.print(analogValue);
  Serial.print(" | Digital Value: ");
  Serial.print(digitalValue);
  
  // Interpretasi nilai analog
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
  
  delay(500); // Delay untuk stabilitas pembacaan
}
