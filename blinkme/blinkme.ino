// Arduino Due LED Blink Program
// Uses the built-in LED connected to pin 13

void setup() {
  // Initialize digital pin LED_BUILT_IN (pin 13) as an output
  pinMode(LED_BUILTIN, OUTPUT);
}

void loop() {
  digitalWrite(LED_BUILTIN, HIGH);   // Turn the LED on
  delay(1000);                       // Wait for 1 second
  digitalWrite(LED_BUILTIN, LOW);    // Turn the LED off
  delay(1000);                       // Wait for 1 second
}