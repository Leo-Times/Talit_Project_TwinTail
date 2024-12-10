#include <Servo.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>

LiquidCrystal_I2C lcd(0x27,  16, 2);

#define accelPin A2
#define breakPin A3
#define brbalPin A6
#define strimPin A0
#define stratPin A7
//#define interAPin 6
//#define interBPin 7
#define neutralPin 4
#define pitPin 9
#define autoPin 5
#define manualPin 8
#define downshiftPin 1
#define upshiftPin 0

struct Data_Pack{
  int speed;
  byte steering;
};

Data_Pack data;

float accelPer; 
float breakPer; 
bool pit = true;
bool neutral = true;
bool transmittionMan = false;
volatile unsigned int temp, counter = 20000; 
int angle;

int strat;
int brbal;
int strim;
int prestrat;
int prebrbal;
int prestrim;

int gear = 0;
int speed = 1402;
int engineBraking = 2;
float brakeing = 20;

int settings[8][3] = {
  {1405, 1405, 0},
  {1410, 1450, 10},
  {1440, 1500, 9},
  {1490, 1584, 8},
  {1585, 1600, 7},
  {1590, 1650, 6},
  {1640, 1700, 5},
  {1690, 1750, 4}
};

void setup() {
//  pinMode(interAPin, INPUT_PULLUP);
//  pinMode(interBPin, INPUT_PULLUP);
//  pinMode(upshiftPin, INPUT_PULLUP);
//  pinMode(downshiftPin, INPUT_PULLUP);

  pinMode(neutralPin, INPUT_PULLUP);
  pinMode(pitPin, INPUT_PULLUP);
  pinMode(autoPin, INPUT_PULLUP);
  pinMode(manualPin, INPUT_PULLUP);

  lcd.init();
  lcd.backlight();

//  attachInterrupt(digitalPinToInterrupt(interAPin), ai0, RISING);
 // attachInterrupt(digitalPinToInterrupt(interBPin), ai1, RISING);

  Serial.begin(9600);
}

void loop() {
// Steering check ------------------------------------------------------------------------
 //* if( counter != temp ){
 //       angle = map(counter, 19400, 20600, 40, 140);
 //       angle += map(analogRead(strimPin), 0, 1023, -10, 10);
 //       data.steering = angle;
 //       temp = counter;
 //     } 
  updateGear();
  findSpeed();
  if (digitalRead(neutralPin) == 0){
    neutral = !neutral;
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
  
  prestrat = strat;
  prestrim = strim;
  prebrbal = brbal;

  strat = map(analogRead(stratPin), 0, 1023, 1, 10);
  strim = map(analogRead(strimPin), 0, 1023, 0, 11) - 5;
  brbal = map(analogRead(brbalPin), 0, 1023, 0, 11) - 5;

  if (prestrat != strat){
    lcd.setCursor(0, 0);
    lcd.print("Strat: ");
    lcd.print(strat);
  }

    if (prestrim != strim){
    lcd.setCursor(0, 0);
    lcd.print("Strim: ");
    lcd.print(strim);
  }

    if (prebrbal != brbal){
    lcd.setCursor(0, 0);
    lcd.print("Brbal: ");
    lcd.print(brbal);
  }

  delay(200);

  lcd.clear();
  lcd.setCursor(0, 1);
  lcd.print(speed);
  if (transmittionMan == true && pit == false){
    lcd.print(" M");
  }
  else if (transmittionMan == false && pit == false) {
    lcd.print(" A");
  }
  else{
    lcd.print(" P");
  }
  lcd.setCursor(15, 1);
  lcd.print(gear);

  //Serial.print("neutral: ");
  //Serial.print(neutral);
  //Serial.print(" | ");
  //Serial.print("pit: ");
  //Serial.print(pit);
  //Serial.print(" | ");
  //Serial.print("manual: ");
  //Serial.println(transmittionMan);

  //Serial.print("brbak: ");
  //Serial.print(brbal);
  //Serial.print(" | ");
  //Serial.print("strat: ");
  //Serial.print(strat);
  //Serial.print(" | ");
  //Serial.print("strim: ");
  //Serial.println(strim);
  
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

//------------------------------------------------------
void updateGear() {
  // Check the state of the upshift and downshift switches
  if (digitalRead(upshiftPin) == 0 && gear < 7) { // Assuming LOW means pressed
    gear++;
    delay(200); // Debounce delay
  } else if (digitalRead(downshiftPin) == 0 && gear > 1) {
    gear--;
    delay(200); // Debounce delay
  }
}


void findSpeed(){
  if (neutral){
    if (analogRead(breakPin) > 10){ 
      speed -= (map(analogRead(breakPin), 0, 1023, 0, 100))/100 * brakeing;
    }
    speed -= engineBraking;
  }
  else if (transmittionMan){
    //Breaking
    if (analogRead(breakPin) > 10){ 
      speed -= (map(analogRead(breakPin), 0, 1023, 0, 100))/100 * brakeing;
    }
    //accell
    float change = (map(analogRead(accelPin),0, 1023, 0, 100));
    change = change/100;

    speed += (settings[gear][2] * change);
    if (speed >= settings[gear][0] && speed <= settings[gear][1]){
      speed -= engineBraking;
    }
    else{
      if ((speed + 20) < settings[gear][0]){
        gear = 0;
        //stall
      }
      else if ((speed + 20) >= settings[gear][0] && speed >= settings[gear][0] && speed <= settings[gear][1]){
        speed = settings[gear][0];
        //push up
      }
      else if (speed > settings[gear][1]){
        speed -= floor((speed-settings[gear][1])/2) + 1;
        //pull down
      }
    }
  }
  else if (!transmittionMan){
    //Breaking
    if (analogRead(breakPin) > 10){ 
      speed -= (map(analogRead(breakPin), 0, 1023, 0, 100))/100 * brakeing;
    }
    //accell
    float change = (map(analogRead(accelPin),0, 1023, 0, 100));
    change = change/100;

    speed += (settings[gear][2] * change);
    if (speed >= settings[gear][0] && speed <= settings[gear][1]){
      speed -= engineBraking;
    }
    else{
      if ((speed + 20) < settings[gear][0]){
        gear--;
      }
      else if ((speed + 20) >= settings[gear][0] && speed >= settings[gear][0] && speed <= settings[gear][1]){
        speed = settings[gear][0];
      }
      else if (speed > settings[gear][1]){
        speed -= floor((speed-settings[gear][1])/2) + 1;
        gear++;
      }
    }
  }
}