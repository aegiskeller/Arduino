// Pin assignments
const int highLevelPin = 2;   // High water level switch (digital input)
const int lowLevelPin = 3;    // Low water level switch (digital input)
const int relayPin = 8;       // Relay control pin (digital output)

void setup() {
  pinMode(highLevelPin, INPUT_PULLUP); // Use internal pull-up resistor
  pinMode(lowLevelPin, INPUT_PULLUP);  // Use internal pull-up resistor
  pinMode(relayPin, OUTPUT);
  digitalWrite(relayPin, LOW);         // Ensure relay is off at start
  Serial.begin(9600);                  // Initialize serial communication
}

void loop() {
  bool highLevel = digitalRead(highLevelPin) == LOW; // Switch closed = LOW
  bool lowLevel = digitalRead(lowLevelPin) == LOW;   // Switch closed = LOW

  Serial.print("High Level: ");
  Serial.print(highLevel ? "ON" : "OFF");
  Serial.print(" | Low Level: ");
  Serial.print(lowLevel ? "ON" : "OFF");

  // here we determine the state of the water tank
  static int lastTankState = -1; // -1 means undefined
  
  if (highLevel == 0 && lowLevel == 0 ) {
    lastTankState = 0; // empty
    Serial.print(" | Tank State: EMPTY");
  }
  else if (highLevel == 1 && lowLevel == 1) {
    lastTankState = 1; //full
    Serial.print(" | Tank State: FULL");
  }
  if (lastTankState == 0 && highLevel == 1 && lowLevel == 1){
    digitalWrite(relayPin, HIGH);
    Serial.println(" | Relay: ON");
  }
  else if (lastTankState == 1 && lowLevel == 1){
    digitalWrite(relayPin, HIGH);
    Serial.println(" | Relay: ON");
  }
  else {
    digitalWrite(relayPin, LOW);
    Serial.println(" | Relay: OFF");
  } 
  delay(100); // Debounce delay
}