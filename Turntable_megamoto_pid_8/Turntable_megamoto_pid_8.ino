#include <PID_v1.h>

bool debugMode = true;

double pidSetpoint = 40; // 10 factor ... 4 ticks per 15ms for 4096 clicks = 14.5 seconds, 80 frames at 5.5fps
int clickWindow = 15; //window measured in ms for counting ticks
double turntableSpeed = 0;

double pidInput, pidOutput;

int pidSampleInterval = 1; //ms between samples

int pwmDivisor = 1; // 1 for 31.2khz (inaudible) or 8 (audible) for 3.9khz Timer 2 PWM frequency

PID myPid(&pidInput, &pidOutput, &pidSetpoint, 1.5, 15, 0, DIRECT); //4, 1, 0 jerky start but stays constant

int MegaMotoPWMpin = 11;
double powerMin = 40; // dependent on voltage & PWM frequency. At 24V with divisor 1:
                      // 27 turns in 11 seconds with the lid off
                      // 30 is enough to overcome the lowest friction with the lid on, and crawl around.
                      // at 12v with divisor 1:  40 to 90 seems like a good range

double powerMax = 90; // turntable can get stuck if this is too low

double power = 0; //current power
double oldpower=0;
double Tf=0.001,Ts=0.014;//derivative factor, sampling time

double powerWindowMin, powerWindowMax, powerWindowAvg; //used for debug
double avgClickTime, clickTimeMin, clickTimeMax, clickTimeAvg; //used for debug

char incomingCharacter;

unsigned long startTime, now, prevClickTime;

int clickCount = 0;
int clicksThisWindow = 0;
int clickTarget = 0;

int encoderPinA = 12;  // Connected to CLK on KY-040
int encoderPinB = 13;  // Connected to DT on KY-040
int subjectLightpin = 7;

unsigned long aPinLastChangeTime;
unsigned long bPinLastChangeTime;

int aPinReading = 0;
int aPinLong = 0;
int aPinShort = 0;

int bPinReading = 0;
int bPinLong = 0;
int bPinShort = 0;

void setup()
{ 
   myPid.SetMode(MANUAL);
   myPid.SetOutputLimits(powerMin, powerMax);
   myPid.SetSampleTime(pidSampleInterval);
  
   pinMode (encoderPinA,INPUT);
   pinMode (encoderPinB,INPUT);
   pinMode (MegaMotoPWMpin, OUTPUT);
   pinMode (subjectLightpin, OUTPUT);
   
   setPwmFrequency(MegaMotoPWMpin, pwmDivisor);
   
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

         delay(2000);
         
         power = powerMin; 
         
         startTime = millis();
         
         clickCount = 0;
         clickTarget = 3048;
         clicksThisWindow = 0;
         
         aPinLastChangeTime = millis();
         bPinLastChangeTime = millis();

         digitalWrite(subjectLightpin, HIGH);
         
         myPid.SetOutputLimits(powerMin, powerMax);
         myPid.SetMode(AUTOMATIC);

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

  if (aPinReading == 0 && aPinLong == 1)
  {
     aPinLastChangeTime = millis();
     aPinShort = 0;
  }

  if (aPinReading == 1 && aPinLong == 0)
  {
     aPinLastChangeTime = millis();
     aPinShort = 1;
  }

  if(aPinLong != aPinShort)
  {           
     recordClick();
     aPinLong = aPinShort;
  }
 
  bPinReading = digitalRead(encoderPinB);   

  if (bPinReading == 0 && bPinLong == 1)
  {
      bPinLastChangeTime = millis();
      bPinShort = 0;
  }

   if (bPinReading == 1 && bPinLong == 0)
   {
      bPinLastChangeTime = millis();
      bPinShort = 1;
   }
//    
   if(bPinLong != bPinShort)
   {           
      recordClick();
      bPinLong = bPinShort;
   }
  
   if(millis() % clickWindow == 0)
   {
     turntableSpeed = (10 * clicksThisWindow) / (double)clickWindow;
     clicksThisWindow = 0;
    
     pidInput = turntableSpeed;
     myPid.Compute();
      
     power = (int)pidOutput;        
     //power=Tf*(power-oldpower)/Ts+oldpower;
     //oldpower = power;
      
     analogWrite(MegaMotoPWMpin, power);    
   }
    
    if(debugMode)
    {
      //min, max, rolling average
      
      if(avgClickTime < clickTimeMin) clickTimeMin = avgClickTime;
      if(avgClickTime > clickTimeMax) clickTimeMax = avgClickTime;
      clickTimeAvg = (clickTimeAvg + avgClickTime) / 2; //not exactly accurate but a good guide

      if(clickTimeMax > 30) clickTimeMax = 30;
      if(clickTimeMin > 30) clickTimeMin = 30;
      if(clickTimeAvg > 30) clickTimeAvg = 30;

      if(power < powerWindowMin) powerWindowMin = power;
      if(power > powerWindowMax) powerWindowMax = power;
      powerWindowAvg = (powerWindowAvg + (int)power) / 2; //not exactly accurate but a good guide
      
      if(millis() % 15 == 0)
      {

            Serial.print(turntableSpeed);
            Serial.print("\t");
            //Serial.print(clickTimeMin);
            //Serial.print("\t");
            //Serial.print(avgClickTime);
            //Serial.print("\t");
            //Serial.print(clickTimeMax);
            //Serial.print("\t");
            Serial.print(powerWindowMin);
            Serial.print("\t");
            //Serial.print(powerWindowAvg);
            //Serial.print("\t");
            Serial.print(powerWindowMax);
            //Serial.print("\t");
            //Serial.print(weightPower);
            //Serial.print("\t");
            Serial.println();
          
            powerWindowMin = (int)power;
            powerWindowMax = (int)power;
            powerWindowAvg = (int)power;
      
            clickTimeMin = avgClickTime;
            clickTimeMax = avgClickTime;
            clickTimeAvg = avgClickTime;
      }
    }
  
  if(clickCount >= clickTarget)
  {
    clickCount = 0;
    clickTarget = 0;
    digitalWrite(MegaMotoPWMpin, LOW);
    digitalWrite(subjectLightpin, LOW);
    myPid.SetMode(MANUAL);
    delay(2000);
  }
}   

void recordClick()
{
  clickCount++;
  clicksThisWindow++;
  now = millis();
  avgClickTime = now - prevClickTime;
  prevClickTime = now;
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


