// Arduino Due Board 2 (Receiver & Debug Display)
unsigned long validPackets = 0;
unsigned long invalidPackets = 0;
unsigned long lastStatTime = 0;
unsigned long totalBytesReceived = 0;
const unsigned long STAT_INTERVAL = 5000; // 5 seconds

void setup() {
  Serial.begin(115200);     // Start Serial for debugging
  Serial1.begin(2000000);   // Initialize Serial1 (pins 18/19) at 2Mbaud
  lastStatTime = millis();
  
  Serial.println("Receiver started");
  Serial.println("Statistics will be shown every 5 seconds");
}

void loop() {
  // Check for incoming data
  if (Serial1.available() > 0) {
    // Read the incoming value
    String receivedData = Serial1.readStringUntil('\n');
    totalBytesReceived += receivedData.length() + 1; // +1 for newline
    
    // Split the string into value and checksum
    int commaIndex = receivedData.indexOf(',');
    if (commaIndex != -1) {
      String valueStr = receivedData.substring(0, commaIndex);
      String checksumStr = receivedData.substring(commaIndex + 1);
      
      int analogValue = valueStr.toInt();
      int receivedChecksum = checksumStr.toInt();
      
      // Verify checksum
      if (analogValue == receivedChecksum) {
        validPackets++;
        // Send to Serial Plotter
        //Serial.println(analogValue);
      } else {
        invalidPackets++;
      }
    }
  }
  
  // Calculate and display statistics every 5 seconds
  if (millis() - lastStatTime >= STAT_INTERVAL) {
    float elapsedSeconds = (millis() - lastStatTime) / 1000.0;
    
    // Calculate rates
    float sampleRate = validPackets / elapsedSeconds;
    float errorRate = (invalidPackets > 0) ? 
                     (invalidPackets * 100.0) / (validPackets + invalidPackets) : 0;
    float bitRate = (totalBytesReceived * 8) / elapsedSeconds; // bits per second
    
    Serial.println("\n--- Receiver Statistics ---");
    Serial.print("Sample rate: ");
    Serial.print(sampleRate, 1);
    Serial.println(" Hz");
    
    Serial.print("Error rate: ");
    Serial.print(errorRate, 1);
    Serial.println("%");
    
    Serial.print("Bit rate: ");
    Serial.print(bitRate / 1000.0, 1);
    Serial.println(" kbps");
    
    Serial.print("Valid packets: ");
    Serial.println(validPackets);
    Serial.print("Invalid packets: ");
    Serial.println(invalidPackets);
    Serial.print("Total bytes received: ");
    Serial.println(totalBytesReceived);
    Serial.println("----------------------------\n");
    
    // Reset counters
    validPackets = 0;
    invalidPackets = 0;
    totalBytesReceived = 0;
    lastStatTime = millis();
  }
}