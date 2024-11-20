// Arduino Due Board 1 (Sender) - Debug Version
const int analogPin = A0;
const int DATA_SIZE = 32;
const uint32_t SYNC_PATTERN = 0xAA55AA55;  // Changed to more robust pattern
const uint8_t FRAME_END = 0x55;

struct __attribute__((packed)) DataPacket {
  uint32_t syncPattern;   // 4 bytes
  uint8_t sequence;       // 1 byte
  uint8_t length;         // 1 byte
  uint8_t data[DATA_SIZE];// 32 bytes
  uint8_t frameEnd;       // 1 byte
};

DataPacket txPacket;
uint8_t sequenceNumber = 0;
unsigned long packetsCount = 0;
unsigned long lastStatsTime = 0;

void setup() {
  Serial.begin(115200);     // Debug serial
  Serial1.begin(1000000);   // Reduced to 1Mbaud for stability
  analogReadResolution(12);
  
  // Initialize packet constants
  txPacket.syncPattern = SYNC_PATTERN;
  txPacket.frameEnd = FRAME_END;
  txPacket.length = DATA_SIZE;
  
  Serial.println("=== Sender Debug Mode ===");
  Serial.print("Packet size: ");
  Serial.println(sizeof(DataPacket));
  Serial.print("Sync pattern: 0x");
  Serial.println(SYNC_PATTERN, HEX);
}

void loop() {
  // Fill packet with analog readings
  for(int i = 0; i < DATA_SIZE; i += 2) {
    int analogValue = analogRead(analogPin);
    txPacket.data[i] = analogValue & 0xFF;
    txPacket.data[i + 1] = (analogValue >> 8);
  }
  
  txPacket.sequence = sequenceNumber++;
  
  // Send the packet with small gaps between fields
  Serial1.write((uint8_t*)&txPacket.syncPattern, 4);
  delayMicroseconds(100);
  Serial1.write(&txPacket.sequence, 1);
  delayMicroseconds(100);
  Serial1.write(&txPacket.length, 1);
  delayMicroseconds(100);
  Serial1.write(txPacket.data, DATA_SIZE);
  delayMicroseconds(100);
  Serial1.write(&txPacket.frameEnd, 1);
  Serial1.flush();
  
  // Debug output for every 100th packet
  packetsCount++;
  if (packetsCount % 100 == 0) {
    Serial.println("\n=== Sending Packet ===");
    Serial.print("Sequence: ");
    Serial.println(txPacket.sequence);
    Serial.print("First analog value: ");
    int analogValue = (txPacket.data[1] << 8) | txPacket.data[0];
    Serial.println(analogValue);
  }
  
  // Print statistics every second
  if (millis() - lastStatsTime >= 1000) {
    float dataRate = (float)(packetsCount * sizeof(DataPacket)) / 1024;
    Serial.print("\nPackets sent: ");
    Serial.println(packetsCount);
    Serial.print("Data rate: ");
    Serial.print(dataRate);
    Serial.println(" KB/s");
    packetsCount = 0;
    lastStatsTime = millis();
  }
  
  delay(1); // Small delay between packets
}