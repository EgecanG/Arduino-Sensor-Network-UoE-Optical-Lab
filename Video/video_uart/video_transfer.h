// video_transfer.h
#ifndef VIDEO_TRANSFER_H
#define VIDEO_TRANSFER_H

#include <Arduino.h>
#include <SD.h>
#include "heatshrink_encoder.h"
#include "heatshrink_decoder.h"

// Configuration constants
const uint32_t BAUD_RATE = 2000000;    // 2 Mbps
const size_t BUFFER_SIZE = 8192;        // 8KB buffer
const size_t PACKET_SIZE = 1024;        // 1KB per packet
const uint8_t SD_CS_PIN = 4;           // SD card chip select pin
const size_t WINDOW_SIZE = 8;          // Compression window size
const size_t LOOKAHEAD_SIZE = 4;       // Compression lookahead size

// Packet types
enum PacketType {
    START_TRANSFER = 0x01,
    DATA_PACKET = 0x02,
    ACK_PACKET = 0x03,
    ERROR_PACKET = 0x04,
    END_TRANSFER = 0x05
};

// Packet structure
struct TransferPacket {
    PacketType type;
    uint32_t packetId;
    uint16_t originalSize;
    uint16_t compressedSize;
    uint16_t checksum;
    uint8_t data[PACKET_SIZE];
} __attribute__((packed));

// Statistics structure
struct TransferStats {
    uint32_t totalBytes;
    uint32_t compressedBytes;
    uint32_t bytesTransferred;
    uint32_t packetsLost;
    uint32_t startTime;
    float compressionRatio;
    float speed;
    float effectiveSpeed;
};

// Function declarations
class VideoTransfer {
public:
    VideoTransfer();
    bool begin();
    void sendFile(const char* filename);
    void receiveFile();
    void handleCommand(char cmd);
    
private:
    TransferStats stats;
    heatshrink_encoder hse;
    heatshrink_decoder hsd;
    
    bool sendPacket(TransferPacket& packet);
    bool receivePacket(TransferPacket& packet);
    uint16_t calculateChecksum(const uint8_t* data, size_t length);
    size_t compressData(const uint8_t* input, size_t inputSize, 
                       uint8_t* output, size_t outputSize);
    size_t decompressData(const uint8_t* input, size_t inputSize,
                         uint8_t* output, size_t outputSize);
    void updateProgress();
    void printFinalStats();
    void setupUART();
};

#endif // VIDEO_TRANSFER_H