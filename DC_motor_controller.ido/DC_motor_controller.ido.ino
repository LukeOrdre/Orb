
#include <Wire.h>
#include <Adafruit_MotorShield.h>
Adafruit_MotorShield AFMS = Adafruit_MotorShield(); 

Adafruit_DCMotor *myMotor = AFMS.getMotor(4);


void setup() {
  AFMS.begin();  // create with the default frequency 1.6KHz
}

void loop() {
  
//  delay(2000);
  myMotor->setSpeed(255);
  myMotor->run(FORWARD);
//  delay(5000);
//  myMotor->run(RELEASE);
  
}

