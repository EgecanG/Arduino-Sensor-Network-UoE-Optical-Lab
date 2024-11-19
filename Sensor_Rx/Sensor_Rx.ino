// Arduino Due UART Bidirectional Test using Serial1
#include <Arduino.h>

// Constants
const uint32_t BAUD_RATE = 2000000;  // 2 Mbps
const size_t BUFFER_SIZE = 1024;
const uint32_t TEST_INTERVAL = 1000;  // Send test packet every 1 second

// Test packet structure
struct TestPacket {
    uint32_t packetId;
    uint32_t timestamp;
    uint32_t data[8];  // 32 bytes of test data
    uint16_t checksum;
} __attribute__((packed));

// Statistics
struct Statistics {
    uint32_t packetsSent;
    uint32_t packetsReceived;
    uint32_t errors;
    float latency;
} stats;

void setupUART() {
    // Initialize Serial for debugging
    SerialUSB.begin(0);
    while(!SerialUSB);
    
    // Initialize Serial1 for UART communication
    Serial1.begin(BAUD_RATE);
    
    // Configure pins (optional as Serial1.begin() does this)
    pinMode(0, INPUT);   // RX0
    pinMode(1, OUTPUT);  // TX0
}

// Send data through UART
void sendData(const uint8_t* data, size_t length) {
    Serial1.write(data, length);
}

// Send test packet
void sendTestPacket() {
    TestPacket packet;
    packet.packetId = stats.packetsSent + 1;
    packet.timestamp = micros();
    
    // Fill test data
    for (int i = 0; i < 8; i++) {
        packet.data[i] = random(0, UINT32_MAX);
    }
    
    // Calculate checksum (excluding checksum field itself)
    packet.checksum = calculateChecksum((uint8_t*)&packet, sizeof(TestPacket) - sizeof(uint16_t));
    
    // Send packet
    Serial1.write((uint8_t*)&packet, sizeof(TestPacket));
    stats.packetsSent++;
}

// Process received data
void processReceivedData() {
    static uint8_t packetBuffer[sizeof(TestPacket)];
    static size_t packetPos = 0;
    
    while (Serial1.available()) {
        packetBuffer[packetPos++] = Serial1.read();
        
        if (packetPos == sizeof(TestPacket)) {
            TestPacket* packet = (TestPacket*)packetBuffer;
            
            // Verify checksum
            uint16_t calculatedChecksum = calculateChecksum(packetBuffer, sizeof(TestPacket) - sizeof(uint16_t));
            
            if (calculatedChecksum == packet->checksum) {
                // Calculate latency
                float latency = (micros() - packet->timestamp) / 1000.0f; // in milliseconds
                stats.latency = (stats.latency * stats.packetsReceived + latency) / (stats.packetsReceived + 1);
                stats.packetsReceived++;
                
                // Send response packet
                packet->timestamp = micros();
                packet->checksum = calculateChecksum((uint8_t*)packet, sizeof(TestPacket) - sizeof(uint16_t));
                Serial1.write((uint8_t*)packet, sizeof(TestPacket));
            } else {
                stats.errors++;
            }
            
            packetPos = 0;
        }
    }
}

// Calculate checksum
uint16_t calculateChecksum(const uint8_t* data, size_t length) {
    uint16_t sum = 0;
    for (size_t i = 0; i < length; i++) {
        sum += data[i];
    }
    return sum;
}

void printStatistics() {
    SerialUSB.println("\n--- UART Test Statistics ---");
    SerialUSB.print("Packets Sent: "); SerialUSB.println(stats.packetsSent);
    SerialUSB.print("Packets Received: "); SerialUSB.println(stats.packetsReceived);
    SerialUSB.print("Errors: "); SerialUSB.println(stats.errors);
    SerialUSB.print("Average Latency: "); SerialUSB.print(stats.latency); SerialUSB.println(" ms");
    SerialUSB.print("Transfer Rate: "); 
    SerialUSB.print((stats.packetsReceived * sizeof(TestPacket)) / 1024.0f); 
    SerialUSB.println(" KB/s");
}

void setup() {
    setupUART();
    randomSeed(analogRead(0));
    memset(&stats, 0, sizeof(stats));
    
    SerialUSB.println("UART Bidirectional Test");
    SerialUSB.print("Baud Rate: "); SerialUSB.println(BAUD_RATE);
}

void loop() {
    static uint32_t lastSendTime = 0;
    static uint32_t lastPrintTime = 0;
    
    // Send test packet every TEST_INTERVAL
    if (millis() - lastSendTime >= TEST_INTERVAL) {
        sendTestPacket();
        lastSendTime = millis();
    }
    
    // Process received data
    processReceivedData();
    
    // Print statistics every 5 seconds
    if (millis() - lastPrintTime >= 5000) {
        printStatistics();
        lastPrintTime = millis();
    }
}