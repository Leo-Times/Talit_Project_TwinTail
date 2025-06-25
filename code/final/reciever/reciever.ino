#include <SPI.h>
#include <nRF24L01.h>
#include <RF24.h>
#include <Servo.h>

RF24 radio(3, 2); // CE, CSN pins
const byte address[6] = "00001";

unsigned long lastReceivedTime = 0;
unsigned long currentTime = 0;
unsigned long lastUpdateTime = 0;  // Timer for 50Hz updates

Servo esc;
Servo turn;

unsigned long lastTurnTime = 0;

struct Data_Pack {
  int speed;         
  int steering;     
};

int reality;

Data_Pack data;      
void setup() {
  Serial.begin(115200);
  radio.begin();
  radio.openReadingPipe(0, address);
  radio.setPALevel(RF24_PA_LOW); // Power level
  radio.setDataRate(RF24_250KBPS);
  radio.startListening();        
  resetData();
  esc.attach(10);
  turn.attach(5);
}

void loop() {

  currentTime = millis();
  if (currentTime - lastReceivedTime > 5000){
    resetData();
  }
  if (radio.available()) {
    radio.read(&data, sizeof(data));
    lastReceivedTime = millis();
  }
  
if (currentTime - lastUpdateTime >= 20) {
    lastUpdateTime = currentTime;
    esc.writeMicroseconds(data.speed);
    reality = map(data.steering, 0, 180, 2000, 1000);
    turn.writeMicroseconds(reality);
    //Serial.println(data.steering);
}
  
  
  // Serial.println(data.speed);
}

void resetData() {
  
  data.speed = 1450;  
}