#define PIT_Pin 4

bool inPit = false;
int toggle_cooldown = 200;

void setup() {
  Serial.begin(9600);
  pinMode(PIT_Pin, INPUT_PULLUP);
}

void loop() {
  if (digitalRead(PIT_Pin) == LOW && toggle_cooldown >= 400) {
    inPit = !inPit;
    toggle_cooldown = 0;
  }
  
  // Print the state of inPit
  Serial.println(inPit ? "In Pit" : "Not in Pit");
  
  // Delay and update cooldown
  delay(100);
  toggle_cooldown += 100;
}
