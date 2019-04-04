#include <PID_v1.h>
#include <PID_AutoTune_v0.h>

#include <TimerOne.h>
int MegaMotoPWMpin = 9;
int encoderPinA = 12;  // Connected to CLK on KY-040
int encoderPinB = 13;  // Connected to DT on KY-040
int aPinReading = 0;
int aPinLong = 0;
int aPinShort = 0;
int bPinReading = 0;
int bPinLong = 0;
int bPinShort = 0;
unsigned long startTime, lastUpdateTime, aPinLastChangeTime, bPinLastChangeTime;
unsigned long thisCaptureStartTime, lastFocusTime, prevCaptureTime;
int clickWindow = 7; //window measured in ms for counting ticks
int clickCount = 0;
int prevClickCount = 0;
int clickTarget = 0;
double powerMin = 60; // dependent on voltage & PWM frequency
double powerMax = 500; // turntable can get stuck if this is too low
double power = 0; //current power
int pidSampleInterval = 1; //ms between samples
double turntableSpeed = 0, instantTurntableSpeed = 0;

double input=10, output=80, setpoint=10;
double kp=0.1,ki=30,kd=0;

double kpmodel=1.5, taup=100, theta[50];
double outputStart=50;
double aTuneStep=1, aTuneNoise=5, aTuneStartValue=80;
unsigned int aTuneLookBack=3;

unsigned long now;

boolean tuning = true;
unsigned long  modelTime, serialTime;

PID myPID(&input, &output, &setpoint,kp,ki,kd, DIRECT);
PID_ATune aTune(&input, &output);

void setup()
{

  //Setup the pid 
  myPID.SetMode(AUTOMATIC);

  aTune.SetNoiseBand(aTuneNoise);
  aTune.SetOutputStep(aTuneStep);
  aTune.SetLookbackSec((int)aTuneLookBack);
      
  serialTime = 0;
  Serial.begin(230400);

  Timer1.initialize(100);  //100us = 10khz

  //myPid.SetMode(MANUAL);
  myPID.SetOutputLimits(powerMin, powerMax);
  myPID.SetSampleTime(pidSampleInterval);

  pinMode (encoderPinA, INPUT);
  pinMode (encoderPinB, INPUT);
  pinMode (MegaMotoPWMpin, OUTPUT);
  
  aPinReading = digitalRead(encoderPinA);
  aPinShort = aPinReading;
  aPinLong = aPinReading;

  bPinReading = digitalRead(encoderPinB);
  bPinShort = bPinReading;
  bPinLong = bPinReading;

}

void loop()
{

  now = millis();

  CheckForClicks();

  if (now % clickWindow == 0 && now != lastUpdateTime)// && clickTarget != 0)
  {
    lastUpdateTime = now;
    instantTurntableSpeed = 10 * ((double)clickCount - (double)prevClickCount);
    turntableSpeed = (0.95 * turntableSpeed) + (0.05 * instantTurntableSpeed);

    prevClickCount = clickCount;
    //pidInput = turntableSpeed;
    //Serial.print(100+(20*turntableSpeed));
    //Serial.println(100+(20*turntableSpeed));
    //Serial.print("\t");
    //Serial.println(pidOutput);

    if(aTune.GetKp() > 0) kp = aTune.GetKp();
    if(aTune.GetKi() > 0) ki = aTune.GetKi();
    if(aTune.GetKd() > 0) kd = aTune.GetKd();

    myPID.SetTunings(kp,ki,kd);
  }
  
 input = turntableSpeed;
  
  myPID.Compute();
  
  Timer1.pwm(MegaMotoPWMpin, output);

  if(now>serialTime)
  {
    SerialSend();
    serialTime+=500;
  }
}

void SerialSend()
{
  Serial.print("setpoint: ");Serial.print(setpoint); Serial.print(" ");
  Serial.print("input: ");Serial.print(input); Serial.print(" ");
  Serial.print("output: ");Serial.print(output); Serial.print(" ");
  Serial.print("kp: ");Serial.print(myPID.GetKp());Serial.print(" ");
  Serial.print("ki: ");Serial.print(myPID.GetKi());Serial.print(" ");
  Serial.print("kd: ");Serial.print(myPID.GetKd());Serial.println();

}



void CheckForClicks()
{
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
  
    if (aPinLong != aPinShort)
    {
      clickCount++;
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
  
    if (bPinLong != bPinShort)
    {
      clickCount++;
      bPinLong = bPinShort;
    }
}

