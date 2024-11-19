#include "video_transfer.h"

VideoTransfer videoTransfer;

void setup() {
    if (!videoTransfer.begin()) {
        while (1); // Halt if initialization fails
    }
}

void loop() {
    if (SerialUSB.available()) {
        char cmd = SerialUSB.read();
        videoTransfer.handleCommand(cmd);
    }
}