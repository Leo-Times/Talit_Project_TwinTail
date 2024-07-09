volatile unsigned int temp, counter = 20000; 
    
void setup() {
  Serial.begin (9600);
  pinMode(2, INPUT_PULLUP); 
  pinMode(3, INPUT_PULLUP); 
  //A rising pulse from encodenren activated ai0(). AttachInterrupt 0 is DigitalPin nr 2 on moust Arduino.
  attachInterrupt(0, ai0, RISING);
  //B rising pulse from encodenren activated ai1(). AttachInterrupt 1 is DigitalPin nr 3 on moust Arduino.
  attachInterrupt(1, ai1, RISING);
  }
   
  void loop() {
  if( counter != temp ){
  Serial.println (map(counter, 19400, 20600, 0, 180));
  temp = counter;
  }
  }
  void ai0() {
  if(digitalRead(3)==LOW) {
  counter++;
  }else{
  counter--;
  }
  }

  void ai1() {
  if(digitalRead(2)==LOW) {
  counter--;
  }else{
  counter++;
  }
  }