#include <PID_v1.h>

bool debugMode = true;

// 4096 encoder clicks, both A & B = 8192.... 14500ms spin, 80 frames at 5.5fps.
// 14.5 seconds is 483 lots of 30ms
// 8192 / 483 = 17 clicks per window

double pidSetpoint = 7.5; 

int clickWindow = 30; //window measured in ms for counting ticks

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

double powerMax = 150; // turntable can get stuck if this is too low

double power = 0; //current power
//double oldpower=0;
//double Tf=0.001,Ts=0.014;//derivative factor, sampling time

double powerWindowMin, powerWindowMax, powerWindowAvg; //used for debug

char incomingCharacter;

unsigned long startTime, lastUpdateTime, now;

int clickCount = 0;
int prevClickCount = 0;
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
   
   Serial.begin (115200);
 } 


void loop()
{
  now = millis();
  
  if(clickTarget == 0)
  {
    incomingCharacter = Serial.read();
    //incomingCharacter = '1';
  
    switch (incomingCharacter) {
       case '1':
         power = powerMin; 

         startTime = now;
         lastUpdateTime = now; 
         aPinLastChangeTime = now;
         bPinLastChangeTime = now;

         clickCount = 0;
         prevClickCount = 0;
         clickTarget = 16384;
        
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
     aPinLastChangeTime = now;
     aPinShort = 0;
  }

  if (aPinReading == 1 && aPinLong == 0)
  {
     aPinLastChangeTime = now;
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
      bPinLastChangeTime = now;
      bPinShort = 0;
  }

   if (bPinReading == 1 && bPinLong == 0)
   {
      bPinLastChangeTime = now;
      bPinShort = 1;
   }
    
   if(bPinLong != bPinShort)
   {           
      recordClick();
      bPinLong = bPinShort;
   }

   myPid.Compute();
   power = (int)pidOutput;        
   //power=Tf*(power-oldpower)/Ts+oldpower;
   //oldpower = power;
   analogWrite(MegaMotoPWMpin, power);    
  
   if(now % clickWindow == 0 && now != lastUpdateTime && clickTarget != 0)
   {
     lastUpdateTime = now;
     
     turntableSpeed = (100 * ((double)clickCount - (double)prevClickCount)) / (double)clickWindow;
     turntableSpeed /= 10;

     prevClickCount = clickCount;
    
     pidInput = turntableSpeed;

    if(debugMode)
    {

      Serial.print(lastUpdateTime - startTime);
      Serial.print("\t");
      Serial.print(clickCount);
      Serial.print("\t");
      Serial.print(turntableSpeed);
      Serial.print("\t");
      Serial.print(power);
      Serial.println();
          
      //powerWindowMin = (int)power;
      //powerWindowMax = (int)power;
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


