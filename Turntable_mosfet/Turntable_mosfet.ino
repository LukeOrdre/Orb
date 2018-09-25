int speedtarget = 110; // works inversely, 110 corresponds to ~8 second spin. Experimentally, motor power of 80 gave this unloaded.
int powerDefault = 100; //sensible place to start with no load
int powerMin = 80;
int power = powerDefault;
int encoderpinA = 3;  // Connected to CLK on KY-040
int encoderpinB = 4;  // Connected to DT on KY-040
int encoderPosition = 0; 
int encoderpinAPrev;  
int encoderpinACurrent;
boolean rotateClockwise;
int loopcount = 0;
bool loopTickTock = false;

void setup() {
  Serial.begin(9600);           // set up Serial library at 9600 bps
  
  AFMS.begin(1600);  // pwm frequency 1.6KHz

  pinMode (encoderpinA,INPUT);
  pinMode (encoderpinB,INPUT);

  encoderpinAPrev = digitalRead(encoderpinA);
}

void loop() {
  
  loopcount++;
  
  char incomingCharacter = Serial.read();

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

  encoderpinACurrent = digitalRead(encoderpinA);
  //encoderpinBCurrent = digitalRead(encoderpinB);
  
  if (encoderpinACurrent != encoderpinAPrev){ // encoder has rotated since last loop
     // if the encoder has rotated, pin B will indicate direction, as it lags behind.
     if (digitalRead(encoderpinB) != encoderpinACurrent) {  // Means pin A Changed first - We're Rotating Clockwise
       encoderPosition ++;
       rotateClockwise = true;
     } else {// Otherwise B changed first and we're moving CCW
       rotateClockwise = false;
       encoderPosition--;
     }
     if(loopTickTock)
          loopTickTock = false;
     else
     {
          loopTickTock = true;     
          //Serial.println(power);
          if(loopcount < speedtarget)
            power -= 5;
          if(power < powerMin)
            power = powerMin;
          loopcount = 0;
     }
     Serial.println(encoderPosition);
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

