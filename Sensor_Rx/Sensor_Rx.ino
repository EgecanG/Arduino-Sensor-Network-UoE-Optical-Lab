// Arduino Due Board 2 (Receiver & Debug Display)
void setup() {
  Serial.begin(115200);     // Start Serial for debugging
  Serial1.begin(2000000);  // Initialize Serial1 (pins 18/19) at 115200 baud
}

void loop() {
  // Check for incoming data
  if (Serial1.available() > 0) {
    // Read the incoming value
    String receivedValue = Serial1.readStringUntil('\n');
    int analogValue = receivedValue.toInt();

    // Send just the number for Serial Plotter
    Serial.println(analogValue);

    /*
    // Print received value with formatting
    Serial.print("Received Analog Value: ");
    Serial.print(analogValue);
    Serial.print(" (");
    
    // Calculate percentage (Due uses 12-bit resolution: 0-4095)
    float percentage = (analogValue / 4095.0) * 100.0;
    Serial.print(percentage, 1);
    Serial.println("%)");
    
    // Optional: Add visual representation
    int barLength = map(analogValue, 0, 4095, 0, 50);  // Map to 50 characters max
    Serial.print("[");
    for(int i = 0; i < 50; i++) {
      if(i < barLength) {
        Serial.print("=");
      } else {
        Serial.print(" ");
      }
    }
    Serial.println("]");
    */
  }
}