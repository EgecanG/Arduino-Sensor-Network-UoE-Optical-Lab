// Arduino Due Board 1 (Analog Reader & Sender)
const int analogPin = A0;  // Analog input pin

void setup() {
  Serial.begin(115200);     // Start Serial for debugging
  Serial1.begin(2000000);  // Initialize Serial1 (pins 18/19) at 115200 baud
  analogReadResolution(12);  // Set analog resolution to 12 bits (Due specific)
}

void loop() {
  // Read analog value
  int analogValue = analogRead(analogPin);
  
  // Send the analog value as a string with a newline
  Serial1.println(analogValue);
  
  // Optional: Debug print to see what we're sending
  //Serial.print("Sending value: ");
  Serial.println(analogValue);
  
  delay(100);  // Small delay to prevent flooding the serial buffer
}