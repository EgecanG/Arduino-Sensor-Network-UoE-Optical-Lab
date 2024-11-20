// Arduino Due Board 1 (Analog Reader & Sender)
const int analogPin = A0;  // Analog input pin
unsigned long packetsSent = 0;
unsigned long lastStatTime = 0;
const unsigned long STAT_INTERVAL = 5000; // 5 seconds

void setup() {
  Serial.begin(115200);     // Start Serial for debugging
  Serial1.begin(2000000);   // Initialize Serial1 (pins 18/19) at 2Mbaud
  analogReadResolution(12); // Set analog resolution to 12 bits (Due specific)
  lastStatTime = millis();
}

void loop() {
  // Read analog value
  int analogValue = analogRead(analogPin);
  
  // Calculate simple checksum (just the value itself for verification)
  int checksum = analogValue;
  
  // Send the analog value and checksum as a string with a separator and newline
  Serial1.print(analogValue);
  Serial1.print(",");
  Serial1.println(checksum);
  
  packetsSent++;
  
  // Calculate and display transmission rate every 5 seconds
  if (millis() - lastStatTime >= STAT_INTERVAL) {
    float elapsedSeconds = (millis() - lastStatTime) / 1000.0;
    float dataRate = packetsSent / elapsedSeconds; // packets per second
    
    Serial.println("\n--- Transmitter Statistics ---");
    Serial.print("Transmission rate: ");
    Serial.print(dataRate, 1);
    Serial.println(" samples/second");
    
    // Reset counters
    packetsSent = 0;
    lastStatTime = millis();
  }
  
  delayMicroseconds(1000);  // 1ms delay (1000 microseconds)
}