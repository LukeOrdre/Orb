#include <PID_v1.h>

bool debugMode = true;

int spinMode = 0;  // 0 = stop, 1 = startup, 5 = constant, 9 = slowdown,

double pidSetpoint = 5; // 7ms for 2048 clicks = 14.5 seconds, 80 frames at 5.5fps
double pidInput, pidOutput;
int pidSampleInterval = 1; //ms between samples
int pwmDivisor = 8; // 1 for 31.2khz (inaudible) or 8 (audible) for 3.9khz Timer 2 PWM frequency
double softPowerFactor = 1.5;  //set up imaginary previous clicks for a smooth over powered start

//PID myPID(&pidInput, &pidOutput, &pidSetpoint, 0.01, 3, 0.0, REVERSE); //0.01, 2, 0 always starts smoothly, but oscilates in speed

PID myPID(&pidInput, &pidOutput, &pidSetpoint, 4, 1, 0.0, REVERSE); //4, 1, 0 jerky start but stays constant


//PID myPID(&pidInput, &pidOutput, &pidSetpoint, 0.15, 0.1 , 0.001, REVERSE); //0.002, 0.02, 0.002 best so far

//PID myPID(&pidInput, &pidOutput, &pidSetpoint, 10, 100, 0, REVERSE); //10, 0.1, 0.001 gives acceptable results

//PID myPID(&pidInput, &pidOutput, &pidSetpoint, 10, 0.1, 0.001, REVERSE); //10, 0.1, 0.001 gives acceptable results

//PID(&Input, &Output, &Setpoint, Kp, Ki, Kd, Direction)
//PID myPID(&pidInput, &pidOutput, &pidSetpoint, 10, 1, 0.01, REVERSE); //0.002, 0.02, 0.002 best so far
//PID myPID(&pidInput, &pidOutput, &pidSetpoint, 5, 3, 1, REVERSE); //0.002, 0.02, 0.002 best so far
//PID myPID(&pidInput, &pidOutput, &pidSetpoint, 0.02, 0.04, 0.01, REVERSE); //0.002, 0.02, 0.002 best so far

int MegaMotoPWMpin = 11;
double powerMin = 1; // dependent on voltage & PWM frequency. at 1 24V 
                      // 28 gives steady rotation, no stutters without lid
                      // 33 gives an ok 4ms per tick spin with lid friction

double powerMax = 100; // turntable can get stuck if this is too low
double power = powerMin; //current power
double powerWindowMin, powerWindowMax, powerWindowAvg; //used for debug
unsigned long clickTimeMin, clickTimeMax, clickTimeAvg; //used for debug

char incomingCharacter;

unsigned long targetClickTime = pidSetpoint; // number of ms between encoder clicks, so higher = slower. 200ms for 80 clicks = 16 seconds.
unsigned long clickTimes[5] = {0,0,0,0,0};
unsigned long startTime, now; 

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
   myPID.SetMode(MANUAL);
   myPID.SetOutputLimits(powerMin, powerMax);
   myPID.SetSampleTime(pidSampleInterval);
  
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
  if(clickTarget == 0)
  {
    //incomingCharacter = Serial.read();
    incomingCharacter = '1';
  
    switch (incomingCharacter) {
       case '1':

         power = powerMin + 5; 

         startTime = millis();

         clickTimes[0] = millis();
         clickTimes[1] = clickTimes[0] + (targetClickTime * softPowerFactor);
         clickTimes[2] = clickTimes[1] + (2 * targetClickTime * softPowerFactor);
         clickTimes[3] = clickTimes[2] + (3 * targetClickTime * softPowerFactor);
         clickTimes[4] = clickTimes[3] + (4 * targetClickTime *  softPowerFactor);

         avgClickTime = targetClickTime * softPowerFactor;
         
         clickCount = 1;
         clickTarget = 2048;
         
         aPinLastChangeTime = millis();
         bPinLastChangeTime = millis();
         
         analogWrite(MegaMotoPWMpin, (int)power);
         digitalWrite(subjectLightpin, HIGH);
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

  //if((millis() - aPinLastChangeTime) > deBounceTime)
  //{
     if(aPinLong != aPinShort)
     {           
       recordClick();
       aPinLong = aPinShort;
     }
  //}
  
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

   //if((millis() - bPinLastChangeTime) > deBounceTime)
   //{
      if(bPinLong != bPinShort)
      {           
        recordClick();
        bPinLong = bPinShort;
      }
   //}

  if(clickTarget > 0 && clickCount > 0)
  {

    //if we're overdue a click, let the pid know straight away. Shouldn't give smoother results, but does.
    if(millis() - clickTimes[0] > targetClickTime) 
    {
      avgClickTime = millis() - clickTimes[0];
    }

    pidInput = avgClickTime;
    myPID.Compute();
    analogWrite(MegaMotoPWMpin, (int)pidOutput);    

    if(debugMode)
    {
      //min, max, rolling average
      
      if(avgClickTime < clickTimeMin) clickTimeMin = avgClickTime;
      if(avgClickTime > clickTimeMax) clickTimeMax = avgClickTime;
      clickTimeAvg = (clickTimeAvg + avgClickTime) / 2;
      
      if(pidOutput < powerWindowMin) powerWindowMin = (int)pidOutput;
      if(pidOutput > powerWindowMax) powerWindowMax = (int)pidOutput;
      powerWindowAvg = (powerWindowAvg + (int)pidOutput) / 2; //not exactly accurate but a good guide
    
      if(millis() % 100 == 0)
      {
  
            Serial.print(clickTimeMin);
            Serial.print("     ");
            Serial.print(avgClickTime);
            Serial.print("     ");
            Serial.print(clickTimeMax);
            Serial.print("     ");
            Serial.print(powerWindowMin);
            Serial.print("     ");
            Serial.print(powerWindowAvg);
            Serial.print("     ");
            Serial.print(powerWindowMax);
            Serial.print("     ");
            Serial.println();
          
            powerWindowMin = (int)pidOutput;
            powerWindowMax = (int)pidOutput;
            powerWindowAvg = (int)pidOutput;
      
            clickTimeMin = avgClickTime;
            clickTimeMax = avgClickTime;
            clickTimeAvg = avgClickTime;
      }
    }
  }
  
  if(clickCount >= clickTarget)// || avgClickTime > 2000) 
  {
    clickCount = 0;
    clickTarget = 0;
    digitalWrite(MegaMotoPWMpin, LOW);
    digitalWrite(subjectLightpin, LOW);
    myPID.SetMode(MANUAL);
    delay(2000);
  }
}   

void recordClick()
{
  clickCount++;
  now = millis();

  // take a weighed average to sooth out time gradient of avgClickTime, reduce power jumps in pid
  
//  avgClickTime =  (
//                  (200 * (now - clickTimes[0])) + 
//                  (100 * (clickTimes[0] - clickTimes[1])) + 
//                  (75 * (clickTimes[1] - clickTimes[2])) + 
//                  (50 * (clickTimes[2] - clickTimes[3])) +
//                  (25 * (clickTimes[3] - clickTimes[4]))
//                  );
//  avgClickTime /= 450; 

  //average this click with where we should be. Odd results.

  //avgClickTime = (100 * (now - clickTimes[0])) + (20 * (now - startTime));
  //avgClickTime /= 120;

  // no averages, just keep it simple

  avgClickTime = now - clickTimes[0];

  //if(avgClickTime < 4) avgClickTime = 4;
  //if(avgClickTime > 6) avgClickTime = 6;

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


