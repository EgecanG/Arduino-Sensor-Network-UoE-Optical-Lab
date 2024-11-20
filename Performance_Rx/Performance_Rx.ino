// Arduino Due Board 2 (Receiver) - Clear Debugging
const int DATA_SIZE = 32;
const uint32_t SYNC_PATTERN = 0xAA55AA55;
const uint8_t FRAME_END = 0x55;

struct __attribute__((packed)) DataPacket {
  uint32_t syncPattern;
  uint8_t sequence;
  uint8_t length;
  uint8_t data[DATA_SIZE];
  uint8_t frameEnd;
};

// Circular buffer to show last 16 bytes received
const int HISTORY_SIZE = 16;
byte byteHistory[HISTORY_SIZE];
int historyIndex = 0;

// Statistics
unsigned long bytesReceived = 0;
unsigned long packetsReceived = 0;
unsigned long syncErrors = 0;
unsigned long lastStatsTime = 0;

void setup() {
  Serial.begin(115200);
  Serial1.begin(1000000);
  
  Serial.println("\n=== Receiver Starting ===");
  Serial.println("Packet Size: " + String(sizeof(DataPacket)) + " bytes");
  Serial.println("Sync Pattern: 0x" + String(SYNC_PATTERN, HEX));
  Serial.println("Frame End: 0x" + String(FRAME_END, HEX));
  Serial.println("Waiting for data...\n");
}

void printByteHistory() {
  Serial.print("Last 16 bytes received: ");
  for(int i = 0; i < HISTORY_SIZE; i++) {
    byte b = byteHistory[(historyIndex + i) % HISTORY_SIZE];
    if(b < 0x10) Serial.print("0");
    Serial.print(b, HEX);
    Serial.print(" ");
  }
  Serial.println();
}

void printStateTransition(String from, String to, byte receivedByte) {
  Serial.println("\n--- State Change ---");
  Serial.println("From: " + from);
  Serial.println("To: " + to);
  Serial.print("Triggered by byte: 0x");
  if(receivedByte < 0x10) Serial.print("0");
  Serial.println(receivedByte, HEX);
  printByteHistory();
}

void loop() {
  static enum {
    FIND_SYNC,
    CHECK_LENGTH,
    READ_DATA,
    CHECK_END
  } state = FIND_SYNC;
  
  static uint32_t syncBuffer = 0;
  static byte dataBuffer[DATA_SIZE];
  static int dataCount = 0;
  static unsigned long stateStartTime = 0;
  static String currentState = "FIND_SYNC";
  
  while (Serial1.available()) {
    byte inByte = Serial1.read();
    
    // Store in circular history buffer
    byteHistory[historyIndex] = inByte;
    historyIndex = (historyIndex + 1) % HISTORY_SIZE;
    bytesReceived++;

    switch(state) {
      case FIND_SYNC:
        syncBuffer = (syncBuffer << 8) | inByte;
        if (syncBuffer == SYNC_PATTERN) {
          printStateTransition("FIND_SYNC", "CHECK_LENGTH", inByte);
          state = CHECK_LENGTH;
          stateStartTime = millis();
        }
        break;

      case CHECK_LENGTH:
        if (inByte == DATA_SIZE) {
          printStateTransition("CHECK_LENGTH", "READ_DATA", inByte);
          state = READ_DATA;
          dataCount = 0;
        } else {
          printStateTransition("CHECK_LENGTH", "FIND_SYNC", inByte);
          Serial.println("Invalid length: " + String(inByte) + " (expected " + String(DATA_SIZE) + ")");
          state = FIND_SYNC;
        }
        break;

      case READ_DATA:
        dataBuffer[dataCount++] = inByte;
        if (dataCount % 8 == 0) {  // Print progress every 8 bytes
          Serial.println("Reading data: " + String(dataCount) + "/" + String(DATA_SIZE) + " bytes");
        }
        if (dataCount >= DATA_SIZE) {
          printStateTransition("READ_DATA", "CHECK_END", inByte);
          state = CHECK_END;
        }
        break;

      case CHECK_END:
        if (inByte == FRAME_END) {
          Serial.println("\n=== Packet Successfully Received ===");
          Serial.println("Time in states: " + String(millis() - stateStartTime) + "ms");
          Serial.println("First analog value: " + 
            String((dataBuffer[1] << 8) | dataBuffer[0]));
          packetsReceived++;
        } else {
          Serial.println("\n=== Frame End Error ===");
          Serial.print("Expected: 0x");
          Serial.print(FRAME_END, HEX);
          Serial.print(" Got: 0x");
          Serial.println(inByte, HEX);
          syncErrors++;
        }
        printStateTransition("CHECK_END", "FIND_SYNC", inByte);
        state = FIND_SYNC;
        break;
    }
  }

  // Print statistics every second
  if (millis() - lastStatsTime >= 1000) {
    Serial.println("\n=== Performance Statistics ===");
    Serial.println("Bytes Received: " + String(bytesReceived));
    Serial.println("Complete Packets: " + String(packetsReceived));
    Serial.println("Sync Errors: " + String(syncErrors));
    Serial.println("Current State: " + currentState);
    Serial.println("Time in current state: " + String(millis() - stateStartTime) + "ms");
    printByteHistory();
    Serial.println();
    
    lastStatsTime = millis();
    bytesReceived = 0;
  }
}