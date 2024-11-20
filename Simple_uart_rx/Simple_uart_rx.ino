// Arduino Due Board 2 (Sender and Receiver)
const int ledPin = 13;

void setup() {
  Serial.begin(9600);     // Start Serial for debugging (optional)
  Serial1.begin(2000000);  // Initialize Serial1 (pins 18/19) at 115200 baud
  pinMode(ledPin, OUTPUT);
}

void loop() {
  // Send message
  Serial1.println("Hello from Board 2");
  Serial.println("Sent message to Board 1"); // Debug message (optional)
  
  // Check for incoming messages
  if (Serial1.available() > 0) {
    String message = Serial1.readStringUntil('\n');
    Serial.println("Received: " + message); // Debug message (optional)
    
    if (message == "Hello from Board 1") {
      digitalWrite(ledPin, HIGH);
      delay(500);
      digitalWrite(ledPin, LOW);
    }
  }
  
  delay(1000);  // Wait for 1 second before sending next message
}