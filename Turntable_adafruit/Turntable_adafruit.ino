#include <Wire.h>
#include <Adafruit_MotorShield.h>

// Create the motor shield object with the default I2C address
Adafruit_MotorShield AFMS = Adafruit_MotorShield(); 
// Or, create it with a different I2C address (say for stacking)
// Adafruit_MotorShield AFMS = Adafruit_MotorShield(0x61); 

// Select which 'port' M1, M2, M3 or M4. In this case, M1 
Adafruit_DCMotor *myMotor = AFMS.getMotor(3);
// You can also make another motor on port M2
//Adafruit_DCMotor *myOtherMotor = AFMS.getMotor(2);

int speedtarget = 1000; // number of loops between encoder clicks, so higher = slower 
int powerDefault = 100; // 100 sensible place to start with no load
int powerMin = 90; // 90 is still turning
int power = powerDefault;
int encoderpinA = 12;  // Connected to CLK on KY-040
int encoderpinB = 13;  // Connected to DT on KY-040
int encoderPosition = 0; 
int encoderpinAPrev;  
int encoderpinACurrent;
//int encoderpinBPrev;
//int encoderpinBCurrent;
boolean rotateClockwise;
int loopcount = 0;
bool loopTickTock = false;
char incomingCharacter;
int ticktocktime = 0;

void setup() {
  Serial.begin(9600);           // set up Serial library at 9600 bps
  
  AFMS.begin(1600);  // pwm frequency 1.6KHz

  pinMode (encoderpinA,INPUT);
  pinMode (encoderpinB,INPUT);

  encoderpinAPrev = digitalRead(encoderpinA);
}

void loop() {
  
  loopcount++;
  
  if(encoderPosition == 0)
  {
    
    incomingCharacter = Serial.read();
  
    switch (incomingCharacter) {
       case '1':
         encoderPosition = 40;
         power = powerDefault;
         loopcount = 0;
        break;
       case '2':
         encoderPosition = 80;
        break;
       case '3':
         encoderPosition = 120;
        break;
  
      }
  }

  encoderpinACurrent = digitalRead(encoderpinA);
  //encoderpinBCurrent = digitalRead(encoderpinB);
  //Serial.println(power);
  if (encoderpinACurrent != encoderpinAPrev){ // encoder has rotated since last loop
     // if the encoder has rotated, pin B will indicate direction, as it lags behind.
     if (digitalRead(encoderpinB) != encoderpinACurrent) {  // Means pin A Changed first - We're Rotating Clockwise
       encoderPosition ++;
       rotateClockwise = true;
     } else {// Otherwise B changed first and we're moving CCW
       rotateClockwise = false;
       encoderPosition--;
     }
     

     
     if(loopTickTock){
          loopTickTock = false;
          ticktocktime += loopcount;
          loopcount = 0;
     }
     else
     {
          ticktocktime += loopcount;
          loopTickTock = true;     
          //Serial.println(power);
          if(ticktocktime < speedtarget)
            power -= 5;
          if(power < powerMin)
            power = powerMin;
          //Serial.println(encoderPosition);
          ticktocktime = 0;
          loopcount = 0;
     }
     //Serial.println(encoderPosition);
     
   }
    
   encoderpinAPrev = encoderpinACurrent;
   //encoderpinBPrev = encoderpinBCurrent;

  if(encoderPosition > 0)
  {
     if(loopcount >= speedtarget) power += 1;

     if(power > 255) power = 255;
     
     myMotor->run(FORWARD);
     myMotor->setSpeed(power);
      
  }
    
  if(encoderPosition <= 0) {
    encoderPosition = 0;
    power = powerDefault;
    myMotor->run(FORWARD);
    myMotor->setSpeed(0);  
  }
  
  myMotor->run(RELEASE); 
  //delay(100);
}

