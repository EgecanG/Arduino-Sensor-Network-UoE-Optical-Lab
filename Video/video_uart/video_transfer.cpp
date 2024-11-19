// video_transfer.cpp
#include "video_transfer.h"

// Static buffers
static uint8_t compressionBuffer[BUFFER_SIZE];
static uint8_t decompressionBuffer[BUFFER_SIZE];

VideoTransfer::VideoTransfer() {
    heatshrink_encoder_reset(&hse);
    heatshrink_decoder_reset(&hsd);
}

bool VideoTransfer::begin() {
    // Initialize Serial for debugging
    SerialUSB.begin(0);
    while (!SerialUSB);
    
    // Initialize SD card
    if (!SD.begin(SD_CS_PIN)) {
        SerialUSB.println("SD card initialization failed!");
        return false;
    }
    
    setupUART();
    SerialUSB.println("Video Transfer System Ready");
    SerialUSB.println("Press 'S' to send or 'R' to receive");
    return true;
}

void VideoTransfer::setupUART() {
    // Enable UART clock
    pmc_enable_periph_clk(ID_UART);
    
    // Reset and configure UART
    UART->UART_CR = UART_CR_RSTRX | UART_CR_RSTTX | UART_CR_RSTSTA;
    
    // Calculate and set baud rate
    uint32_t divisor = SystemCoreClock / (16 * BAUD_RATE);
    UART->UART_BRGR = divisor;
    
    // Configure mode
    UART->UART_MR = UART_MR_PAR_NO | UART_MR_CHMODE_NORMAL;
    
    // Enable UART
    UART->UART_CR = UART_CR_RXEN | UART_CR_TXEN;
    
    // Configure pins
    pinMode(PIN_UART_RX, INPUT);
    pinMode(PIN_UART_TX, OUTPUT);
}

bool VideoTransfer::sendPacket(TransferPacket& packet) {
    const uint8_t* data = (const uint8_t*)&packet;
    size_t remaining = sizeof(TransferPacket);
    
    while (remaining > 0) {
        if (UART->UART_SR & UART_SR_TXRDY) {
            UART->UART_THR = data[sizeof(TransferPacket) - remaining];
            remaining--;
        }
    }
    return true;
}

bool VideoTransfer::receivePacket(TransferPacket& packet) {
    uint8_t* data = (uint8_t*)&packet;
    size_t remaining = sizeof(TransferPacket);
    unsigned long timeout = millis() + 1000;  // 1 second timeout
    
    while (remaining > 0) {
        if (millis() > timeout) {
            return false;
        }
        
        if (UART->UART_SR & UART_SR_RXRDY) {
            data[sizeof(TransferPacket) - remaining] = UART->UART_RHR;
            remaining--;
        }
    }
    return true;
}

uint16_t VideoTransfer::calculateChecksum(const uint8_t* data, size_t length) {
    uint16_t checksum = 0;
    for (size_t i = 0; i < length; i++) {
        checksum += data[i];
    }
    return checksum;
}

size_t VideoTransfer::compressData(const uint8_t* input, size_t inputSize, 
                                 uint8_t* output, size_t outputSize) {
    size_t totalOutput = 0;
    size_t sunk = 0;
    size_t polled = 0;
    
    heatshrink_encoder_reset(&hse);
    
    // Sink input
    HSE_sink_res sinkRes = heatshrink_encoder_sink(&hse, input, inputSize, &sunk);
    if (sinkRes != HSER_SINK_OK || sunk != inputSize) {
        return 0;
    }
    
    heatshrink_encoder_finish(&hse);
    
    // Poll for output
    HSE_poll_res pollRes;
    do {
        pollRes = heatshrink_encoder_poll(&hse, output + totalOutput,
                                        outputSize - totalOutput, &polled);
        totalOutput += polled;
    } while (pollRes == HSER_POLL_MORE);
    
    return totalOutput;
}

size_t VideoTransfer::decompressData(const uint8_t* input, size_t inputSize,
                                   uint8_t* output, size_t outputSize) {
    size_t totalOutput = 0;
    size_t sunk = 0;
    size_t polled = 0;
    
    heatshrink_decoder_reset(&hsd);
    
    // Sink compressed data
    HSD_sink_res sinkRes = heatshrink_decoder_sink(&hsd, input, inputSize, &sunk);
    if (sinkRes != HSDR_SINK_OK || sunk != inputSize) {
        return 0;
    }
    
    heatshrink_decoder_finish(&hsd);
    
    // Poll for output
    HSD_poll_res pollRes;
    do {
        pollRes = heatshrink_decoder_poll(&hsd, output + totalOutput,
                                        outputSize - totalOutput, &polled);
        totalOutput += polled;
    } while (pollRes == HSDR_POLL_MORE);
    
    return totalOutput;
}

void VideoTransfer::sendFile(const char* filename) {
    File videoFile = SD.open(filename, FILE_READ);
    if (!videoFile) {
        SerialUSB.println("Error opening file!");
        return;
    }
    
    // Initialize statistics
    memset(&stats, 0, sizeof(stats));
    stats.totalBytes = videoFile.size();
    stats.startTime = millis();
    
    // Send start packet
    TransferPacket packet;
    packet.type = START_TRANSFER;
    packet.packetId = 0;
    packet.originalSize = strlen(filename);
    memcpy(packet.data, filename, packet.originalSize);
    packet.checksum = calculateChecksum(packet.data, packet.originalSize);
    
    if (!sendPacket(packet)) {
        SerialUSB.println("Failed to send start packet!");
        videoFile.close();
        return;
    }
    
    // Wait for acknowledgment
    if (!receivePacket(packet) || packet.type != ACK_PACKET) {
        SerialUSB.println("Start transfer not acknowledged!");
        videoFile.close();
        return;
    }
    
    // Send file data
    uint32_t packetId = 1;
    uint8_t readBuffer[PACKET_SIZE];
    
    while (videoFile.available()) {
        size_t bytesRead = videoFile.read(readBuffer, PACKET_SIZE);
        
        // Compress the data
        size_t compressedSize = compressData(readBuffer, bytesRead,
                                           packet.data, PACKET_SIZE);
        
        if (compressedSize > 0) {
            packet.type = DATA_PACKET;
            packet.packetId = packetId++;
            packet.originalSize = bytesRead;
            packet.compressedSize = compressedSize;
            packet.checksum = calculateChecksum(packet.data, compressedSize);
            
            if (!sendPacket(packet)) {
                SerialUSB.println("Failed to send data packet!");
                break;
            }
            
            // Wait for acknowledgment
            if (!receivePacket(packet) || packet.type != ACK_PACKET) {
                stats.packetsLost++;
                videoFile.seek(videoFile.position() - bytesRead);
                packetId--;
                continue;
            }
            
            stats.bytesTransferred += compressedSize;
            stats.compressedBytes += compressedSize;
            updateProgress();
        }
    }
    
    // Send end transfer packet
    packet.type = END_TRANSFER;
    packet.packetId = packetId;
    packet.compressedSize = 0;
    sendPacket(packet);
    
    videoFile.close();
    printFinalStats();
}

void VideoTransfer::receiveFile() {
    TransferPacket packet;
    File videoFile;
    bool transferActive = false;
    
    // Initialize statistics
    memset(&stats, 0, sizeof(stats));
    stats.startTime = millis();
    
    while (true) {
        if (!receivePacket(packet)) {
            continue;
        }
        
        switch (packet.type) {
            case START_TRANSFER: {
                // Extract filename
                char filename[100];
                memcpy(filename, packet.data, packet.originalSize);
                filename[packet.originalSize] = '\0';
                
                // Create file
                if (SD.exists(filename)) {
                    SD.remove(filename);
                }
                videoFile = SD.open(filename, FILE_WRITE);
                if (!videoFile) {
                    packet.type = ERROR_PACKET;
                    sendPacket(packet);
                    return;
                }
                
                transferActive = true;
                packet.type = ACK_PACKET;
                sendPacket(packet);
                break;
            }
            
            case DATA_PACKET: {
                if (!transferActive) break;
                
                // Verify checksum
                if (calculateChecksum(packet.data, packet.compressedSize) != packet.checksum) {
                    packet.type = ERROR_PACKET;
                    sendPacket(packet);
                    continue;
                }
                
                // Decompress data
                size_t decompressedSize = decompressData(
                    packet.data, packet.compressedSize,
                    decompressionBuffer, BUFFER_SIZE
                );
                
                if (decompressedSize != packet.originalSize) {
                    packet.type = ERROR_PACKET;
                    sendPacket(packet);
                    continue;
                }
                
                // Write decompressed data
                videoFile.write(decompressionBuffer, decompressedSize);
                
                stats.bytesTransferred += packet.compressedSize;
                stats.compressedBytes += packet.compressedSize;
                stats.totalBytes += decompressedSize;
                
                // Send acknowledgment
                packet.type = ACK_PACKET;
                sendPacket(packet);
                
                updateProgress();
                break;
            }
            
            case END_TRANSFER: {
                if (transferActive) {
                    videoFile.close();
                    transferActive = false;
                    printFinalStats();
                }
                return;
            }
            
            case ERROR_PACKET: {
                SerialUSB.println("Error in transfer!");
                if (transferActive) {
                    videoFile.close();
                    transferActive = false;
                }
                return;
            }
        }
    }
}

void VideoTransfer::updateProgress() {
    unsigned long elapsed = (millis() - stats.startTime) / 1000;
    if (elapsed == 0) elapsed = 1;
    
    stats.speed = stats.bytesTransferred / 1024.0f / elapsed;
    stats.compressionRatio = stats.totalBytes / (float)stats.compressedBytes;
    stats.effectiveSpeed = stats.speed * stats.compressionRatio;
    
    SerialUSB.print("\rProgress: ");
    SerialUSB.print((stats.bytesTransferred * 100) / stats.compressedBytes);
    SerialUSB.print("% Speed: ");
    SerialUSB.print(stats.speed);
    SerialUSB.print(" KB/s Compression: ");
    SerialUSB.print(stats.compressionRatio);
    SerialUSB.print(":1 Effective: ");
    SerialUSB.print(stats.effectiveSpeed);
    SerialUSB.print(" KB/s    ");
}

void VideoTransfer::printFinalStats() {
    SerialUSB.println("\nTransfer Complete!");
    SerialUSB.print("Original size: "); 
    SerialUSB.print(stats.totalBytes / 1024); SerialUSB.println(" KB");
    SerialUSB.print("Compressed size: "); 
    SerialUSB.print(stats.compressedBytes / 1024); SerialUSB.println(" KB");
    SerialUSB.print("Compression ratio: "); 
    SerialUSB.print(stats.compressionRatio); SerialUSB.println(":1");
    SerialUSB.print("Average speed: "); 
    SerialUSB.print(stats.speed); SerialUSB.println(" KB/s");
    SerialUSB.print("Effective speed: "); 
    SerialUSB.print(stats.effectiveSpeed); SerialUSB.println(" KB/s");
    SerialUSB.print("Packets lost: ");
    SerialUSB.println(stats.packetsLost);
    SerialUSB.print("Transfer time: ");
    SerialUSB.print((millis() - stats.startTime) / 1000.0f);
    SerialUSB.println(" seconds");
}
void VideoTransfer::handleCommand(char cmd) {
    switch (cmd) {
        case 'S':
        case 's': {
            SerialUSB.println("Enter video filename:");
            while (!SerialUSB.available());
            
            String filename = SerialUSB.readStringUntil('\n');
            filename.trim();
            
            SerialUSB.print("Sending file: ");
            SerialUSB.println(filename);
            sendFile(filename.c_str());
            break;
        }
        
        case 'R':
        case 'r': {
            SerialUSB.println("Waiting for file...");
            receiveFile();
            break;
        }
        
        default:
            SerialUSB.println("Invalid command. Use 'S' to send or 'R' to receive.");
            break;
    }
}