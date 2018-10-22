#include <PID_v1.h>

bool debugMode = true;

double pidSetpoint = 7; // 7ms for 2048 clicks = 14.5 seconds, 80 frames at 5.5fps
double pidInput, pidOutput;
//PID(&Input, &Output, &Setpoint, Kp, Ki, Kd, Direction)
PID myPID(&pidInput, &pidOutput, &pidSetpoint, 1, 0, 0.01, REVERSE); //0.002, 0.02, 0.002 best so far
//PID myPID(&pidInput, &pidOutput, &pidSetpoint, 0.02, 0.04, 0.01, REVERSE); //0.002, 0.02, 0.002 best so far

int MegaMotoPWMpin = 11;
double powerMin = 27; // dependent on voltage. 28 gives steady rotation, less stutters
double powerMax = 50;
double power = powerMin; //current power
double prevPower;
char incomingCharacter;

unsigned long targetClickTime = pidSetpoint; // number of ms between encoder clicks, so higher = slower. 200ms for 80 clicks = 16 seconds.
unsigned long startTime, lastPowerUpdate;
unsigned long powerUpdateDelay = 5;

int clickCount = 0;
int clickTarget = 0;

double avgClickTime = 0;

int encoderPinA = 12;  // Connected to CLK on KY-040
int encoderPinB = 13;  // Connected to DT on KY-040

unsigned long aPinLastChangeTime;
unsigned long bPinLastChangeTime;

unsigned long deBounceTime = 1;

int aPinReading = 0;
int aPinLong = 0;
int aPinShort = 0;

int bPinReading = 0;
int bPinLong = 0;
int bPinShort = 0;

void setup()
{ 
   myPID.SetMode(MANUAL);
   myPID.SetOutputLimits(powerMin, powerMax);
   myPID.SetSampleTime(1);
  
   pinMode (encoderPinA,INPUT);
   pinMode (encoderPinB,INPUT);
   pinMode(MegaMotoPWMpin, OUTPUT);
   setPwmFrequency(MegaMotoPWMpin, 1);  // change Timer2 divisor to 8 gives 3.9kHz PWM freq
   
   aPinReading = digitalRead(encoderPinA);   
   aPinShort = aPinReading;
   aPinLong = aPinReading;

   bPinReading = digitalRead(encoderPinB);
   bPinShort = bPinReading;
   bPinLong = bPinReading;
   
   Serial.begin (9600);
 } 


void loop()
{
  if(clickTarget == 0)
  {
    //incomingCharacter = Serial.read();
    incomingCharacter = '1';
  
    switch (incomingCharacter) {
       case '1':
         startTime = millis();
         clickCount = 1;
         clickTarget = 2048;
         power = powerMin;
         aPinLastChangeTime = millis();
         bPinLastChangeTime = millis();
         analogWrite(MegaMotoPWMpin, (int)power);
         myPID.SetMode(AUTOMATIC);
        break;
       case '2':
         clickTarget = 80;
        break;
       case '3':
         clickTarget = 120;
        break;
      }
  }
   
  aPinReading = digitalRead(encoderPinA);   

  if (aPinReading == 0 && aPinLong == 1){
     aPinLastChangeTime = millis();
     aPinShort = 0;
  }

  if (aPinReading == 1 && aPinLong == 0)
  {
     aPinLastChangeTime = millis();
     aPinShort = 1;
  }

  if((millis() - aPinLastChangeTime) > deBounceTime)
  {
     if(aPinLong != aPinShort)
     {           
       recordClick();
       aPinLong = aPinShort;
     }
  }
  
  bPinReading = digitalRead(encoderPinB);   

  if (bPinReading == 0 && bPinLong == 1){
      bPinLastChangeTime = millis();
      bPinShort = 0;
   }

   if (bPinReading == 1 && bPinLong == 0)
   {
      bPinLastChangeTime = millis();
      bPinShort = 1;
   }

   if((millis() - bPinLastChangeTime) > deBounceTime)
   {
      if(bPinLong != bPinShort)
      {           
        recordClick();
        bPinLong = bPinShort;
      }
   }

  if(clickTarget != 0)
  {
    updateTiming();
    if(clickCount < 200)
    {
        myPID.SetTunings(clickCount/2, 1, 0);
    }
    else
    {
      myPID.SetTunings(100, 1, 0);
    }
      
    pidInput = (double)avgClickTime;
    myPID.Compute();
    analogWrite(MegaMotoPWMpin, (int)pidOutput);    
    
    if(millis() % 50 == 0) debugPrint(' ');
  }
  
  if(clickCount >= clickTarget) 
  {
    clickCount = 0;
    clickTarget = 0;
    //power = powerDefault;
    digitalWrite(MegaMotoPWMpin, LOW);
    myPID.SetMode(MANUAL);
    delay(2000);
  }
}   

void recordClick()
{
  clickCount++;
  updateTiming();
}

void updateTiming()
{
  avgClickTime = 100 * (millis() - startTime);
  avgClickTime /= clickCount;
  avgClickTime /= 100;
  if(avgClickTime < 1) avgClickTime = 1;
  if(avgClickTime > 2 * targetClickTime) avgClickTime = 2 * targetClickTime;
}


void setPwmFrequency(int pin, int divisor) {
  byte mode;
  if(pin == 5 || pin == 6 || pin == 9 || pin == 10) { // Timer0 or Timer1
    switch(divisor) {
      case 1: mode = 0x01; break;
      case 8: mode = 0x02; break;
      case 64: mode = 0x03; break;
      case 256: mode = 0x04; break;
      case 1024: mode = 0x05; break;
      default: return;
    }
    if(pin == 5 || pin == 6) { 
      TCCR0B = TCCR0B & 0b11111000 | mode; // Timer0
    } else {
      TCCR1B = TCCR1B & 0b11111000 | mode; // Timer1
    }
  } else if(pin == 3 || pin == 11) {
    switch(divisor) {
      case 1: mode = 0x01; break;
      case 8: mode = 0x02; break;
      case 32: mode = 0x03; break;
      case 64: mode = 0x04; break;
      case 128: mode = 0x05; break;
      case 256: mode = 0x06; break;
      case 1024: mode = 0x7; break;
      default: return;
    }
    TCCR2B = TCCR2B & 0b11111000 | mode; // Timer2
  }
}

void debugPrint(char originMessage)
{
  if(debugMode)
  {
          Serial.print(pidInput);
          Serial.print("     ");
          Serial.print(pidOutput);
          Serial.println();
  }          
}



