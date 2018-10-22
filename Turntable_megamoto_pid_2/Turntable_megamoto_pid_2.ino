#include <PID_v1.h>

double pidSetpoint = 7; // 7ms for 2048 clicks = 14.5 seconds, 80 frames at 5.5fps
double pidInput, pidOutput;
//PID(&Input, &Output, &Setpoint, Kp, Ki, Kd, Direction)
//PID myPID(&pidInput, &pidOutput, &pidSetpoint, 0.002, 0.03, 0.002, REVERSE); //0.002, 0.02, 0.002 best so far
PID myPID(&pidInput, &pidOutput, &pidSetpoint, 0.002, 0.02, 0.004, REVERSE); //0.002, 0.02, 0.002 best so far


bool debugMode = true;
int MegaMotoPWMpin = 11;
double powerMin = 30; // dependent on voltage. 28 gives steady rotation, less stutters
double powerMax = 60;
double power = powerMin; //current power

char incomingCharacter;

unsigned long targetClickTime = 200; // number of ms between encoder clicks, so higher = slower. 200ms for 80 clicks = 16 seconds.
unsigned long targetBuffer = targetClickTime / 10;
unsigned long clickWindow = 1500; // size of sliding window to check speed

int windowCount = 0;

unsigned long lastPowerChange;

unsigned long clickTimes[100] = {0,0,0,0,0,0,0,0,0,0,    0,0,0,0,0,0,0,0,0,0,    0,0,0,0,0,0,0,0,0,0,    
                                 0,0,0,0,0,0,0,0,0,0,    0,0,0,0,0,0,0,0,0,0,    0,0,0,0,0,0,0,0,0,0,    
                                 0,0,0,0,0,0,0,0,0,0,    0,0,0,0,0,0,0,0,0,0,    0,0,0,0,0,0,0,0,0,0,
                                 0,0,0,0,0,0,0,0,0,0};

int clickCount = 0;
int clickTarget = 0;

unsigned long avgClickTime = 0;
unsigned long prevAvgClickTime = 0;

int encoderPinA = 12;  // Connected to CLK on KY-040
int encoderPinB = 13;  // Connected to DT on KY-040

unsigned long aPinLastChangeTime;
unsigned long bPinLastChangeTime;

int aPinReading = 0;
int aPinLong = 0;
int aPinShort = 0;

int bPinReading = 0;
int bPinLong = 0;
int bPinShort = 0;

unsigned long deBounceTime = 1;

void setup()
{ 
   myPID.SetMode(MANUAL);
   myPID.SetOutputLimits(powerMin, powerMax);
   myPID.SetSampleTime(50);
  
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
    incomingCharacter = Serial.read();
    //incomingCharacter = '1';
  
    switch (incomingCharacter) {
       case '1':
         wipeClicks();
         clickTimes[0] = millis();
         //startTime = millis();
         clickCount = 1;
         clickTarget = 2048;
         power = 10;
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
    pidInput = (double)avgClickTime;
    myPID.Compute();
    analogWrite(MegaMotoPWMpin, (int)pidOutput);
    if(millis() % 100 == 0) debugPrint(' ');
  }
  
  if(clickCount >= clickTarget) 
  {
    clickCount = 0;
    clickTarget = 0;
    //power = powerDefault;
    digitalWrite(MegaMotoPWMpin, LOW);
    delay(2000);
  }
}   

void wipeClicks()
{
  for(int i=0; i<100; i++)
  {
    clickTimes[i]=0;
  }
}

void recordClick()
{
  clickTimes[clickCount] = millis();
  clickCount++;
  updateTiming();
}

void updateTiming()
{
   windowCount = 0;

   if(clickCount * targetClickTime > clickWindow)
   {   
     for(int i=clickCount; i> -1; i--)
     {
        if(clickTimes[i] > millis() - clickWindow)
        {
          windowCount++;
        }
     }
     avgClickTime = (millis() - clickTimes[clickCount - windowCount]) / windowCount;
   }
   else
   {
      avgClickTime = (millis() - clickTimes[0]) / clickCount;
   }
  
  if(avgClickTime < 0) avgClickTime = 0.1 * targetClickTime;
  if(avgClickTime > 15 * targetClickTime) avgClickTime = 15 * targetClickTime;
}

void updatePower()
{
    //if(millis() - lastPowerChange > 3*targetClickTime)
    //{
      if(avgClickTime < targetClickTime + targetBuffer)
      {
          lastPowerChange = millis();
          power -= (avgClickTime / targetClickTime);
          if(power < powerMin) power = powerMin;
          if(power > 254) power = 254;
          analogWrite(MegaMotoPWMpin, (int)power);
      }
      if(avgClickTime > targetClickTime - targetBuffer)
      {
        lastPowerChange = millis();
        power += (avgClickTime / targetClickTime);
        if(power < powerMin) power = powerMin;
        if(power > 254) power = 254;
        analogWrite(MegaMotoPWMpin, (int)power);
      }
    //}
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
//          Serial.print(millis());
//          Serial.print("     ");
//          Serial.print(clickTimes[0]);
//          Serial.print("     ");
//          Serial.print(clickTimes[1]);
//          Serial.print("     ");
//          Serial.print(clickTimes[2]);
//          Serial.print("     ");
//          Serial.print(clickTimes[3]);
//          Serial.print("     ");
//          Serial.print(pidInput);
//          Serial.print("     ");
          Serial.print(pidOutput);
          Serial.print("     ");
//          Serial.print(avgClickTime);
//          Serial.print("     ");
//          Serial.print((power - 20) * 10);
//          Serial.print("     ");
          Serial.print(clickCount);
//          Serial.print("     ");
//          Serial.print(targetClickTime + targetBuffer);
//          Serial.print("     ");
//          Serial.print(targetClickTime - targetBuffer);
//          Serial.print("     ");
//          Serial.print(targetClickTime);
//          Serial.print("     ");
//          Serial.print(windowCount);
//          Serial.print("     ");
          //Serial.println(originMessage);
          Serial.println();
  }          
}

