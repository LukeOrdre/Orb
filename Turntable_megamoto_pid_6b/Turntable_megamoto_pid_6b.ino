#include <PID_v1.h>

bool debugMode = true;

int spinMode = 0;  // 0 = stop, 1 = startup, 5 = constant, 9 = slowdown,

double pidSetpoint = 3; // 7ms for 2048 clicks = 14.5 seconds, 80 frames at 5.5fps

double pidInput, pidOutput;

int pidSampleInterval = 1; //ms between samples

int pwmDivisor = 1; // 1 for 31.2khz (inaudible) or 8 (audible) for 3.9khz Timer 2 PWM frequency

PID myPid(&pidInput, &pidOutput, &pidSetpoint, 0.1, 0.3, 0.001, REVERSE); //4, 1, 0 jerky start but stays constant

//PID myPid(&pidInput, &pidOutput, &pidSetpoint, 1.5, 15, 0, REVERSE); //1.5, 15, 0 jerky start but stays constant power 40 100

//PID myPid(&pidInput, &pidOutput, &pidSetpoint, 0.3, 0.1, 0, REVERSE); //4, 1, 0 jerky start but stays constant

int MegaMotoPWMpin = 11;
double powerMin = 40; // dependent on voltage & PWM frequency. At 24V with divisor 1:
                      // 27 turns in 11 seconds with the lid off
                      // 30 is enough to overcome the lowest friction with the lid on, and crawl around.

double powerMax = 140; // turntable can get stuck if this is too low

double power = 0; //current power
double oldpower=0;
double Tf=0.001,Ts=0.014;//derivative 
double weightPower; // for soft starting

double powerWindowMin, powerWindowMax, powerWindowAvg; //used for debug
double clickTimeMin, clickTimeMax, clickTimeAvg; //used for debug

char incomingCharacter;

unsigned long targetClickTime = pidSetpoint; // number of ms between encoder clicks, so higher = slower. 200ms for 80 clicks = 16 seconds.
unsigned long clickTimes[5] = {0,0,0,0,0};
unsigned long startTime, now, powerOverrideTime = 0; 

int clickCount = 0;
int clickTarget = 0;

double avgClickTime = 0;

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
   pinMode(MegaMotoPWMpin, OUTPUT);
   pinMode(subjectLightpin, OUTPUT);
   
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
  if(spinMode == 0)
  {
    incomingCharacter = Serial.read();
    //incomingCharacter = '1';
  
    switch (incomingCharacter) {
       case '1':

         delay(2000);
         
         power = powerMin; 
         weightPower = powerMin;
         powerOverrideTime = 0;
         
         startTime = millis();
         clickTimes[0] = startTime;
         clickTimes[1] = startTime - targetClickTime;
         clickTimes[2] = startTime - (2*targetClickTime);
         clickTimes[3] = startTime - (3*targetClickTime);
         clickTimes[4] = startTime - (4*targetClickTime);
         
         clickCount = 1;
         clickTarget = 6144;
         
         aPinLastChangeTime = millis();
         bPinLastChangeTime = millis();

         digitalWrite(subjectLightpin, HIGH);
         
         spinMode = 1;
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
  
   if(bPinLong != bPinShort)
   {           
      //recordClick();
      bPinLong = bPinShort;
   }
  
   if(spinMode == 1) // startup
   {
      if(clickCount < 50) //soft start by increasing power by 1 unit every 50ms
      {
        //if we're overdue a click, let's update the average straight away
        if(millis() - clickTimes[0] > targetClickTime) 
        {
          avgClickTime += 0.01;
        }
        
        pidInput = avgClickTime;
        
        myPid.Compute();
        
        power = (int)pidOutput;        
        power=Tf*(power-oldpower)/Ts+oldpower;
        oldpower = power;
        
        analogWrite(MegaMotoPWMpin, power);    
      }
      else
      {
          myPid.SetTunings(1.5, 15, 0);
          myPid.SetOutputLimits(40,150);
          spinMode = 5;
      }     
    }
    
    if(spinMode == 5 )//&& (powerOverrideTime == 0 || millis() > powerOverrideTime))
    {
      //if we're overdue a click, let's update the average straight away
      if(millis() - clickTimes[0] > targetClickTime) 
      {
        avgClickTime += 0.01;
        //analogWrite(MegaMotoPWMpin, (int)power + 10);
        //powerOverrideTime = millis();
      }
      
      pidInput = avgClickTime;
      
      myPid.Compute();
      power = (int)pidOutput;
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
      
      if(millis() % 25 == 0)
      {

            //Serial.print(clickCount);
            //Serial.print("\t");
            Serial.print(clickTimeMin);
            Serial.print("\t");
            //Serial.print(avgClickTime);
            //Serial.print("\t");
            Serial.print(clickTimeMax);
            Serial.print("\t");
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
    spinMode = 0;
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
  now = millis();

  // take a weighed average to sooth out time gradient of avgClickTime, reduce power jumps in pid
  
//  avgClickTime =  
//                  (100 * (now - clickTimes[0])) + 
//                  (100 * (clickTimes[0] - clickTimes[1])) + 
//                  (100 * (clickTimes[1] - clickTimes[2])) + 
//                  (100 * (clickTimes[2] - clickTimes[3])) +
//                  (100 * (clickTimes[3] - clickTimes[4]));
//
//  avgClickTime /= 500; 

  //average this click with where we should be. Odd results.

  //avgClickTime = (100 * (now - clickTimes[0])) + (20 * (now - startTime));
  //avgClickTime /= 120;

  // no averages, just keep it simple

  avgClickTime = now - clickTimes[0];

  if(targetClickTime > (now - clickTimes[0])) //if we're going too fast, let's just cut the power for this loop
  {
    //analogWrite(MegaMotoPWMpin, powerMin);
    //powerOverrideTime = millis();
  }

  //if(avgClickTime < 1) avgClickTime = 1;
  //if(avgClickTime > 14) avgClickTime = 14;

  clickTimes[4] = clickTimes[3];
  clickTimes[3] = clickTimes[2];
  clickTimes[2] = clickTimes[1];
  clickTimes[1] = clickTimes[0];
  clickTimes[0] = now;
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


