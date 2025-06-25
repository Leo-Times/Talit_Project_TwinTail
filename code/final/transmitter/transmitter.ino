#include <SPI.h>
#include <nRF24L01.h>
#include <RF24.h>
#include <LiquidCrystal_I2C.h>


LiquidCrystal_I2C lcd(0x27, 16, 2);

#define accelPin A2
#define breakPin A3
#define brbalPin A6
#define strimPin A0
#define stratPin A7
#define neutralPin 4
#define pitPin 9
#define autoPin 5
#define manualPin 8
#define downshiftPin 7
#define upshiftPin 6

RF24 radio(3, 2); // CE, CSN pins
const byte address[6] = "00001";

struct Data_Pack {
  int speed;         
  int steering;     
};

Data_Pack data; 

unsigned long lastDebounceTime = 0; // Timestamp for debounce
const unsigned long debounceDelay = 10; // Debounce delay in milliseconds

float accelPer; 
float breakPer; 
bool pit = true;
bool neutral = true;
bool transmittionMan = false;
bool rev = false;
bool bios = false;

bool upshiftState = HIGH;   // Track the current state of the upshift button
bool downshiftState = HIGH; // Track the current state of the downshift button
bool hardBrake = false;

int strat = 6;
int brbal = 0;
int strim = 0;
int prestrat;
int prebrbal;
int prestrim;
int preangle = 90;

int current;
String tac; 

int angle = 90;
int gear = 0;
int agear = 0;
int speed = 1402;
int engineBraking = 1;
float brakeing = 10;

int settings[8][3] = {
  {1450, 1450, 0},
  {1460, 1500, 7},
  {1490, 1550, 7},
  {1540, 1584, 6},
  {1585, 1600, 6},
  {1590, 1650, 5},
  {1640, 1700, 5},
  {1690, 1750, 4}
};

int stratList[10][2] = {
  {0, 0},
  {0, 0},
  {0, 0},
  {0, 0},
  {0, 0},
  {0, 0},
  {200, 4},
  {0, 0},
  {0, 0},
  {0, 0}
};

String message = ""; 

void setup() {
  radio.begin();
  radio.openWritingPipe(address);
  radio.setPALevel(RF24_PA_LOW); // Power level
  radio.setDataRate(RF24_250KBPS);
  radio.stopListening();         // Set module as transmitter

  pinMode(upshiftPin, INPUT_PULLUP);
  pinMode(downshiftPin, INPUT_PULLUP);

  Serial.begin(115200); 
  lcd.init();
  lcd.backlight();
  lcd.setCursor(0, 0);

  pinMode(neutralPin, INPUT_PULLUP);
  pinMode(pitPin, INPUT_PULLUP);
  pinMode(autoPin, INPUT_PULLUP);
  pinMode(manualPin, INPUT_PULLUP);

  // Initialize test data
  data.speed = 1410;     // Start with speed 0
  data.steering = 90; // Steering centered at 90 degrees
}

void loop() {
  unsigned long currentTime = millis();
  if (Serial.available() > 0) { // Check if data is available to read
    String receivedMessage = Serial.readStringUntil('\n'); // Read the incoming message
    preangle = angle;
    angle = (receivedMessage.toInt());
    delay(10);
  }
  
  if (digitalRead(neutralPin) == 0){
    neutral = true;
    rev = false;
  }
  if (digitalRead(pitPin) == 0){
    pit = !pit;
  }
  if (digitalRead(manualPin) == 0){
    transmittionMan = true;
  }
  if (digitalRead(autoPin) == 0){
    transmittionMan = false;
  }

  if (digitalRead(upshiftPin) == 0 && digitalRead(downshiftPin) == 0){
    bios = !bios;
    neutral = true;
    gear = 0;
  }
  

  if (neutral){
    gear = 0;
    agear = 0;
  }
  
  prestrat = strat;
  prestrim = strim;
  prebrbal = brbal;


  strat = map(analogRead(stratPin), 0, 1023, 1, 10);
  strim = map(analogRead(strimPin), 0, 1023, 0, 11) - 5;
  brbal = map(analogRead(brbalPin), 0, 1023, 0, 11) - 5;
  

  accelPer = map(analogRead(accelPin), 0, 240, 0, 100);
  breakPer = map(analogRead(breakPin), 0, 260, 0, 100);

  accelPer = accelPer/100;
  breakPer = breakPer/100;

  if (abs(angle - preangle) > 40) {
    angle = preangle + (angle > preangle ? 5 : -5);
  }

  if (angle > 130){
    angle = 130;
  }
  else if (angle < 50){
    angle = 50;
  }

  preangle = angle; 

  if(neutral){
    if (analogRead(breakPin) > 5){ 
      speed -= (breakPer) * (brakeing + brbal);
    }
    
    if (digitalRead(upshiftPin) == 0) { 
        neutral = false;
        agear = 1;
    }
    
    if (digitalRead(downshiftPin) == 0){
      rev = true;
    }

    speed -= engineBraking;

    if (rev){
      if (accelPer > 0.5){
        speed = 1350;
      }
      else{
        speed = 1450;
      }
    }
    else if (speed < settings[0][0]){
      speed = settings[0][0];
    }
  }

  else if (!neutral) {
    if (!transmittionMan){
      speed += ((settings[agear][2] + stratList[strat][1])* accelPer);

      if (speed > (settings[7][1] + stratList[strat][0])){ // limit
        speed = (settings[7][1] + stratList[strat][0]);
      }

      if (speed >= ((settings[agear][0] + ((agear - 1) * (stratList[strat][0]/7)))) && speed <= (settings[agear][1] + ((agear) * (stratList[strat][0]/7)))+10){ // If speed is in gear range do nothing
        speed -= engineBraking;
      }
    
      if (speed < (settings[agear][0] + ((agear - 1) * (stratList[strat][0]/7)))){ // If speed too low, reduce gear
        agear--;
        gear = agear;
      }
      else if(speed > (settings[agear][1] + ((agear) * (stratList[strat][0]/7))) && agear < 7){ //If speed is higher, increase gear
        agear++;
        gear = agear;
      }

      if (analogRead(breakPin) > 5){ 
        speed -= (breakPer) * (brakeing + brbal);
      }

      if(breakPer > 0.8){
        hardBrake = true;
      }

      if(breakPer < 0.7){
        hardBrake = false;
      }

    

      if (accelPer > 0.05 && agear == 0){
        agear = 1;
      }

      if (pit){
        if (speed > 1590){
          speed = 1590;
        }
      }
    }


    else if(transmittionMan){
      bool currentUpshift = digitalRead(upshiftPin);
      bool currentDownshift = digitalRead(downshiftPin);

      if (currentUpshift == 0 && upshiftState == 1) {
        if (gear < 7) {
          gear++;
        }
      }
      upshiftState = currentUpshift;

      if (currentDownshift == 0 && downshiftState == 1) {
        if (gear > 1) {
          gear--;
        }
      }
      downshiftState = currentDownshift; 
       
      speed += ((settings[gear][2] + stratList[strat][1])* accelPer);

      if (speed > (settings[7][1] + stratList[strat][0])){ // limit
        speed = (settings[7][1] + stratList[strat][0]);
      }

      if (speed >= (settings[0][0])){ // If speed is in gear range do nothing
        speed -= engineBraking;
      }
    
      if (speed < (settings[gear][0] + ((gear - 1) * (stratList[strat][0]/7))-100)){ // If speed too low, reduce gear (STALL)
        neutral = true;
        agear = gear;
      }
      else if(speed > (settings[gear][1] + ((gear) * (stratList[strat][0]/7)))){ //If speed is higher, increase gear
        speed = settings[gear][1] + ((gear) * (stratList[strat][0]/7));
        agear = gear;
      }

      if (analogRead(breakPin) > 5){ 
        speed -= (breakPer) * (brakeing + brbal);
      }

      if(breakPer > 0.8){
        hardBrake = true;
      }

      if(breakPer < 0.7){
        hardBrake = false;
      }


      if (pit){
        if (speed > 1590){
          speed = 1590;
        }
      }
    }
  }

 if (speed < 1425 && !rev){
    speed = 1450;
    agear = 0;
    gear = 1;
  }
  if (speed > 2000){
    speed = 2000;
  }

  if (hardBrake){
    speed = 1350;
  }

  if (currentTime - lastDebounceTime > debounceDelay && bios == false){
    data.speed = speed; 
    data.steering = angle + strim;

    bool success = radio.write(&data, sizeof(data));
    lastDebounceTime = currentTime;
  }

  if(!bios){
lcd.clear(); 
  lcd.setCursor(0, 0);
  if (transmittionMan){
int min = (settings[gear][0]+(gear - 1) * (stratList[strat][0]/7));
  int max = (settings[gear][1] + ((gear) * (stratList[strat][0]/7)));
  current = map(speed, min, max, 0, 15);
  tac = "";
  for (int op = 0; op <= current; op++){
    tac += (char)255;
  }
  }
  else {
  int min = (settings[agear][0]+(agear - 1) * (stratList[strat][0]/7));
  int max = (settings[agear][1] + ((agear) * (stratList[strat][0]/7)));
  current = map(speed, min, max, 0, 15);
  tac = "";
  for (int op = 0; op <= current; op++){
    tac += (char)255;
  }
  }
  
  if (gear > 0){
    lcd.print(tac);
  }
  lcd.setCursor(0, 1);
  lcd.print(speed); 
  lcd.setCursor(4, 1);
  if (transmittionMan){
    lcd.print("M");
  }
  else{
    lcd.print("A");
  }

  lcd.setCursor(5, 1);
  if (pit){
    lcd.print("P");
  }

  lcd.setCursor(15, 1);
  if (rev){
    lcd.print("R");
  }
  else if (neutral){
    lcd.print("N");
  }
  else if (transmittionMan){
    lcd.print(gear); 
  }
  else{
    lcd.print(agear); 
  }

  if (prestrat != strat){
    lcd.setCursor(0, 0);
    lcd.print("strat: ");
    lcd.print(strat);
  }

  if (prestrim != strim){
    lcd.setCursor(0, 0);
    lcd.print("strim: ");
    lcd.print(strim);
  }

  if (prebrbal != brbal){
    lcd.setCursor(0, 0);
    lcd.print("brbal: ");
    lcd.print(brbal);
  }
  }
  else if(bios){
    bool currentUpshift = digitalRead(upshiftPin);
    bool currentDownshift = digitalRead(downshiftPin);

      if (currentUpshift == 0 && upshiftState == 1) {
        if (gear < 2) {
          gear++;
        }
      }
      upshiftState = currentUpshift;

      if (currentDownshift == 0 && downshiftState == 1) {
        if (gear >= 1) {
          gear--;
        }
      }
      downshiftState = currentDownshift; 

    
    if (gear == 0){
      lcd.clear(); 
      lcd.setCursor(0, 0);
      lcd.print(digitalRead(neutralPin));
      lcd.setCursor(0, 1);
      lcd.print(digitalRead(autoPin));
      lcd.setCursor(15, 0);
      lcd.print(digitalRead(pitPin));
      lcd.setCursor(15, 1);
      lcd.print(digitalRead(manualPin));
    }
    else if (gear == 1){
      lcd.clear(); 
      lcd.setCursor(0, 0);
      lcd.print(analogRead(breakPin));
      lcd.setCursor(0, 1);
      lcd.print(analogRead(accelPin));
    }
    else if (gear == 2){
      lcd.clear(); 
      lcd.setCursor(0, 0);
      lcd.print(angle);
      lcd.setCursor(0, 1);
      lcd.print(analogRead(brbalPin));
    }
    else if (gear == 3){
      lcd.clear(); 
      lcd.setCursor(0, 0);
      lcd.print(analogRead(stratPin));
      lcd.setCursor(0, 1);
      lcd.print(analogRead(strimPin));
    }
  }
  

  
  /*
  lcd.clear(); 
  lcd.setCursor(0, 0);
  lcd.print(data.steering);*/
}
